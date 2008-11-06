/*
  Copyright 2008 Google Inc.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

#include "npapi_plugin.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>
#include <cmath>

#include <ggadget/basic_element.h>
#include <ggadget/gadget.h>
#include <ggadget/gadget_consts.h>
#include <ggadget/main_loop_interface.h>
#include <ggadget/math_utils.h>
#include <ggadget/permissions.h>
#include <ggadget/signals.h>
#include <ggadget/string_utils.h>
#include <ggadget/system_utils.h>
#include <ggadget/view.h>
#include <ggadget/xml_http_request_interface.h>

#include "npapi_plugin_script.h"
#include "npapi_utils.h"

#ifdef MOZ_X11
#include <X11/Xlib.h>
#include <X11/Intrinsic.h>
#endif

namespace ggadget {
namespace npapi {

const char *kFirefox3UserAgent = "Mozilla/5.0 (Linux; U; en-US; rv:1.9.1.0) "
                                 "Gecko/20080724 Firefox/3.0.1";

class Plugin::Impl {
 public:
  static const int kStreamCallbackInterval = 50;
  static const int kCheckQueueDrawInterval = 100;

  // Writes a block of data to the plugin stream asynchronously if the plugin
  // can't receive the whole data at once.
  class StreamWriter : public WatchCallbackInterface {
   public:
    StreamWriter(Impl *owner, const std::string &url,
                 const std::string &mime_type, const std::string &headers,
                 const std::string &data, bool notify, void *notify_data)
        : owner_(owner), plugin_funcs_(&owner->library_info_->plugin_funcs),
          stream_(NULL), stream_type_(NP_NORMAL),
          // These fields hold string data to live during the stream's life.
          url_(url), mime_type_(mime_type), headers_(headers),
          data_(data), offset_(0),
          notify_(notify),
          watch_id_(0), idle_count_(0),
          on_owner_destroy_connection_(owner_->on_destroy_.Connect(
              NewSlot(this, &StreamWriter::OnOwnerDestroy))) {
      if (!plugin_funcs_->writeready || !plugin_funcs_->write ||
          !plugin_funcs_->newstream)
        return;

      // NPAPI uses int32/uint32 for data size.
      // Limit data size to prevent overflow.
      if (data.size() >= (1U << 31))
        return;

      stream_ = new NPStream;
      memset(stream_, 0, sizeof(NPStream));
      stream_->ndata = owner;
      stream_->url = url_.c_str();
      stream_->headers = headers_.c_str();
      stream_->end = static_cast<uint32>(data.size());
      stream_->notifyData = notify_data;
      char *mime_type_copy = strdup(mime_type_.c_str());
      NPError err = plugin_funcs_->newstream(&owner->instance_, mime_type_copy,
                                             stream_, false, &stream_type_);
      free(mime_type_copy);
      if (err == NPERR_NO_ERROR) {
        switch (stream_type_) {
          case NP_NORMAL:
            break;
          case NP_ASFILEONLY:
          case NP_ASFILE: {
            if (plugin_funcs_->asfile) {
              std::string temp_name = owner->GetTempFileName();
              if (!temp_name.empty() &&
                  WriteFileContents(temp_name.c_str(), data)) {
                plugin_funcs_->asfile(&owner->instance_, stream_,
                                      temp_name.c_str());
              }
            }
            DestroyStream(NPRES_DONE);
            break;
          }
          case NP_SEEK:
            LOG("Plugin needs NP_SEEK stream type which is not supported.");
            DestroyStream(NPRES_USER_BREAK);
            break;
        }
      } else {
        DestroyStream(NPRES_DONE);
      }

      // First call NPP_Write immediately, and schedule timer only if some
      // data is left.
      if (stream_ && Call(NULL, 0)) {
        watch_id_ = GetGlobalMainLoop()->AddTimeoutWatch(
            kStreamCallbackInterval, this);
      }
    }

    ~StreamWriter() {
      on_owner_destroy_connection_->Disconnect();
      DestroyStream(NPRES_DONE);
    }

    void OnOwnerDestroy() {
      GetGlobalMainLoop()->RemoveWatch(watch_id_);
    }

    void DestroyStream(NPReason reason) {
      if (stream_) {
        if (plugin_funcs_->destroystream) {
          plugin_funcs_->destroystream(&owner_->instance_, stream_, reason);
        }
        if (notify_ && plugin_funcs_->urlnotify) {
          plugin_funcs_->urlnotify(&owner_->instance_, url_.c_str(),
                                   reason, stream_->notifyData);
        }
        delete stream_;
        stream_ = NULL;
      }
    }

    virtual bool Call(MainLoopInterface *main_loop, int watch_id) {
      if (!stream_)
        return false;

      while (offset_ < data_.size()) {
        int32 len = plugin_funcs_->writeready(&owner_->instance_, stream_);
        if (len <= 0) {
          if (++idle_count_ > 5000 / kStreamCallbackInterval) {
            LOG("Timeout to write to stream");
            DestroyStream(NPRES_USER_BREAK);
            return false;
          }
          // Plugin doesn't need data this time. Try again later.
          return true;
        }

        idle_count_ = 0;
        if (data_.size() < offset_ + len)
          len = static_cast<int32>(data_.size() - offset_);
        int32 consumed = plugin_funcs_->write(&owner_->instance_, stream_,
                                              static_cast<int32>(offset_), len,
                                              // This gets a non-const pointer.
                                              &data_[offset_]);
        // Error occurs.
        if (consumed < 0)
          return false;
        offset_ += consumed;
      }
      return false;
    }

    virtual void OnRemove(MainLoopInterface *main_loop, int watch_id) {
      delete this;
    }

    Impl *owner_;
    NPPluginFuncs *plugin_funcs_;
    NPStream *stream_;
    uint16 stream_type_;
    std::string url_, mime_type_, headers_, data_;
    size_t offset_;
    bool notify_;
    int watch_id_;
    int idle_count_;
    Connection *on_owner_destroy_connection_;
  };

  class XMLHttpRequestCallback {
   public:
    XMLHttpRequestCallback(Impl *owner, const std::string &url,
                           XMLHttpRequestInterface *http_request,
                           bool notify, void *notify_data)
        : owner_(owner), url_(url),
          http_request_(http_request),
          notify_(notify), notify_data_(notify_data),
          on_owner_destroy_connection_(owner->on_destroy_.Connect(
              NewSlot(this, &XMLHttpRequestCallback::OnOwnerDestroy))),
          on_state_change_connection_(http_request->ConnectOnReadyStateChange(
              NewSlot(this, &XMLHttpRequestCallback::OnStateChange))) {
    }

    ~XMLHttpRequestCallback() {
      on_owner_destroy_connection_->Disconnect();
      on_state_change_connection_->Disconnect();
    }

    void OnOwnerDestroy() {
      http_request_->Abort();
      // Then OnStateChange() will be called and this object will be deleted.
    }

    std::string GetHeaders() {
      const char *headers_ptr = NULL;
      http_request_->GetAllResponseHeaders(&headers_ptr);
      std::string result;
      if (headers_ptr) {
        // Remove all '\r's according to NPAPI's requirement.
        while (*headers_ptr) {
          if (*headers_ptr != '\r')
            result += *headers_ptr;
        }
      }
      return result;
    }

    void OnStateChange() {
      std::string headers;
      if (http_request_->GetReadyState() == XMLHttpRequestInterface::DONE) {
        unsigned short status = 0;
        if (http_request_->IsSuccessful() &&
            http_request_->GetStatus(&status) ==
                XMLHttpRequestInterface::NO_ERR &&
            status == 200) {
          std::string mime_type = http_request_->GetResponseContentType();
          if (mime_type.empty())
            mime_type = owner_->mime_type_;
          const char *response = NULL;
          size_t response_size = 0;
          http_request_->GetResponseBody(&response, &response_size);
          std::string response_str;
          if (response)
            response_str.assign(response, response_size);
          owner_->OnStreamReady(url_, http_request_->GetEffectiveUrl(),
                                mime_type, headers, response_str,
                                notify_, notify_data_);
        } else {
          owner_->OnStreamError(url_, notify_, notify_data_, NPRES_NETWORK_ERR);
        }
        delete this;
      }
    }

    Impl *owner_;
    std::string url_;
    XMLHttpRequestInterface *http_request_;
    bool notify_;
    void *notify_data_;
    Connection *on_owner_destroy_connection_;
    Connection *on_state_change_connection_;
  };

  Impl(const char *mime_type, BasicElement *element,
       PluginLibraryInfo *library_info, const NPWindow &window,
       const StringMap &parameters)
      : mime_type_(mime_type),
        element_(element),
        library_info_(library_info),
        plugin_root_(NULL),
        window_(window),
        windowless_(false), transparent_(false),
        init_error_(NPERR_GENERIC_ERROR),
        temp_file_seq_(0),
        browser_window_npobject_(this, &kBrowserWindowClass),
        location_npobject_(this, &kLocationClass) {
    ASSERT(library_info);
    memset(&instance_, 0, sizeof(instance_));
    instance_.ndata = this;
    if (library_info->plugin_funcs.newp) {
      // NPP_NewUPP require non-const parameters.
      char **argn = NULL;
      char **argv = NULL;
      size_t argc = parameters.size();
      if (argc) {
        argn = new char *[argc];
        argv = new char *[argc];
        StringMap::const_iterator it = parameters.begin();
        for (size_t i = 0; i < argc; ++it, ++i) {
          argn[i] = strdup(it->first.c_str());
          argv[i] = strdup(it->second.c_str());
        }
      }
      init_error_ = library_info->plugin_funcs.newp(
          const_cast<NPMIMEType>(mime_type), &instance_, NP_EMBED,
          static_cast<int16>(argc), argn, argv, NULL);
      for (size_t i = 0; i < argc; i++) {
        free(argn[i]);
        free(argv[i]);
      }
      delete [] argn;
      delete [] argv;
    }

    if (!windowless_)
      SetWindow(window);
    // Otherwise the caller should handle the change of windowless state.
  }

  ~Impl() {
    if (!temp_dir_.empty())
      RemoveDirectory(temp_dir_.c_str(), true);
    if (plugin_root_)
      plugin_root_->Unref();
    on_destroy_();

    if (library_info_->plugin_funcs.destroy) {
      NPError ret = library_info_->plugin_funcs.destroy(&instance_, NULL);
      if (ret != NPERR_NO_ERROR)
        LOG("Failed to destroy plugin instance - nperror code %d.", ret);
    }
    ReleasePluginLibrary(library_info_);
  }

  bool SetWindow(const NPWindow &window) {
    // Host must have set the window info struct.
    if (!window.ws_info)
      return false;
    if (windowless_ != (window.type == NPWindowTypeDrawable)) {
      LOG("Window types don't match (passed in: %s, "
          "while plugin's type: %s)",
          window.type == NPWindowTypeDrawable ? "windowless" : "windowed",
          windowless_ ? "windowless" : "windowed");
      return false;
    }

    NPWindow window_tmp = window;
    if (library_info_->plugin_funcs.setwindow &&
        library_info_->plugin_funcs.setwindow(&instance_, &window_tmp) ==
            NPERR_NO_ERROR) {
      window_ = window_tmp;
      return true;
    }
    return false;
  }

  bool HandleEvent(void *event) {
    if (!library_info_->plugin_funcs.event)
      return false;
    return library_info_->plugin_funcs.event(&instance_, event);
  }

  ScriptableInterface *GetScriptablePlugin() {
    if (!plugin_root_) {
      NPObject *plugin_root;
      if (library_info_->plugin_funcs.getvalue != NULL &&
          library_info_->plugin_funcs.getvalue(
              &instance_, NPPVpluginScriptableNPObject, &plugin_root) ==
              NPERR_NO_ERROR &&
          plugin_root) {
        plugin_root_ = new ScriptableNPObject(plugin_root);
        plugin_root_->Ref();
        return plugin_root_;
      }
      return NULL;
    }
    return plugin_root_;
  }

  void OnStreamReady(const std::string &url, const std::string &effective_url,
                     const std::string &mime_type,
                     const std::string &headers, const std::string &data,
                     bool notify, void *notify_data) {
    if (url == location_) {
      // Change location to the effective URL only if the request is of
      // top level.
      location_ = effective_url;
    }

    StreamWriter *writer = new StreamWriter(this, effective_url, mime_type,
                                            headers, data, notify, notify_data);
    if (writer->watch_id_ == 0) {
      // The writer encountered errors or has finished its task.
      delete writer;
    }
  }

  void OnStreamError(const std::string &url, bool notify, void *notify_data,
                     NPReason reason) {
    if (notify && library_info_->plugin_funcs.urlnotify) {
      library_info_->plugin_funcs.urlnotify(&instance_, url.c_str(),
                                            reason, notify_data);
    }
  }

  NPError HandleURL(const char *method, const char *url, const char *target,
                    const std::string &post_data,
                    bool notify, void *notify_data) {
    if (!url || !*url)
      return NPERR_INVALID_PARAM;
    if (notify && !library_info_->plugin_funcs.urlnotify)
      return NPERR_INVALID_PARAM;

    Gadget *gadget = element_->GetView()->GetGadget();
    if (!gadget)
      return NPERR_GENERIC_ERROR;

    if (target) {
      // Let the gadget allow this OpenURL gracefully.
      bool old_interaction = gadget->SetInUserInteraction(true);
      gadget->OpenURL(url);
      gadget->SetInUserInteraction(old_interaction);
      // Mozilla also doesn't send notification if target is not NULL.
      return NPERR_NO_ERROR;
    }

    std::string file_path = IsAbsolutePath(url) ? url : GetFileNameFromURL(url);
    if (!file_path.empty()) {
      if (!gadget->GetPermissions()->IsRequiredAndGranted(
          Permissions::FILE_READ)) {
        OnStreamError(url, notify, notify_data, NPRES_USER_BREAK);
        LOG("The gadget doesn't permit the plugin to read local files.");
        return NPERR_GENERIC_ERROR;
      }

      std::string content;
      if (!ReadFileContents(file_path.c_str(), &content)) {
        OnStreamError(url, notify, notify_data, NPRES_NETWORK_ERR);
        return NPERR_FILE_NOT_FOUND;
      }
      OnStreamReady(url, url, mime_type_, "", content, notify, notify_data);
    } else {
      XMLHttpRequestInterface *request = gadget->CreateXMLHttpRequest();
      if (!request) {
        OnStreamError(url, notify, notify_data, NPRES_USER_BREAK);
        return NPERR_GENERIC_ERROR;
      }
      request->Ref();
      // The following object will add itself as the listener of the request.
      XMLHttpRequestCallback *callback = new XMLHttpRequestCallback(
          this, url, request, notify, notify_data);
      if (request->Open(method, url, true, NULL, NULL) !=
              XMLHttpRequestInterface::NO_ERR ||
          request->Send(NULL) != XMLHttpRequestInterface::NO_ERR) {
        delete callback;
        OnStreamError(url, notify, notify_data, NPRES_NETWORK_ERR);
      }
      request->Unref();
    }
    return NPERR_NO_ERROR;
  }

  void InvalidateRect(NPRect *invalid_rect) {
    if (!invalid_rect)
      return;
    // The current plugins seems always invalid the whole rect.
    // Otherwise we need to consider the zoom factor of the view.
    element_->QueueDraw();
  }

  void ForceRedraw() {
    element_->MarkRedraw();
  }

  std::string GetTempFileName() {
    if (temp_dir_.empty())
      CreateTempDirectory("ggl-np-", &temp_dir_);
    if (temp_dir_.empty())
      return std::string();
    std::string file_name = StringPrintf("t%d", ++temp_file_seq_);
    return BuildFilePath(temp_dir_.c_str(), file_name.c_str());
  }

  static NPError HandleURL(NPP instance, const char *method, const char *url,
                           const char *target, const std::string &post_data,
                           bool notify, void *notify_data) {
    ENSURE_MAIN_THREAD(NPERR_INVALID_PARAM);
    if (instance && instance->ndata) {
      Impl *impl = static_cast<Impl *>(instance->ndata);
      return impl->HandleURL(method, url, target, post_data,
                             notify, notify_data);
    }
    return NPERR_INVALID_PARAM;
  }

  static NPError NPN_GetURL(NPP instance, const char *url, const char *target) {
    return HandleURL(instance, "GET", url, target, "", false, NULL);
  }

  static NPError NPN_GetURLNotify(NPP instance, const char *url,
                                  const char *target, void *notify_data) {
    return HandleURL(instance, "GET", url, target, "", true, notify_data);
  }

  static NPError HandlePostURL(NPP instance, const char *url,
                               const char *target, uint32 len, const char *buf,
                               NPBool file, bool notify, void *notify_data) {
    if (!buf)
      return NPERR_INVALID_PARAM;

    std::string post_data;
    if (file) {
      std::string file_name(buf, len);
      if (!ReadFileContents(file_name.c_str(), &post_data)) {
        LOG("Failed to read file: %s", file_name.c_str());
        return NPERR_GENERIC_ERROR;
      }
    } else {
      post_data.assign(buf, len);
    }
    return HandleURL(instance, "POST", url, target, post_data,
                     notify, notify_data);
  }

  static NPError NPN_PostURL(NPP instance, const char *url, const char *target,
                             uint32 len, const char *buf, NPBool file) {
    return HandlePostURL(instance, url, target, len, buf, file, false, NULL);
  }

  static NPError NPN_PostURLNotify(NPP instance, const char *url,
                                   const char *target,
                                   uint32 len, const char *buf,
                                   NPBool file, void *notify_data) {
    return HandlePostURL(instance, url, target, len, buf, file,
                         true, notify_data);
  }

  static NPError NPN_RequestRead(NPStream *stream, NPByteRange *rangeList) {
    NOT_IMPLEMENTED();
    return NPERR_GENERIC_ERROR;
  }

  static NPError NPN_NewStream(NPP instance, NPMIMEType type,
                               const char *target, NPStream **stream) {
    // Plugin-produced stream is not supported.
    NOT_IMPLEMENTED();
    return NPERR_GENERIC_ERROR;
  }

  static int32 NPN_Write(NPP instance, NPStream *stream,
                         int32 len, void *buffer) {
    NOT_IMPLEMENTED();
    return -1;
  }

  static NPError NPN_DestroyStream(NPP instance, NPStream *stream,
                                   NPReason reason) {
    NOT_IMPLEMENTED();
    return NPERR_GENERIC_ERROR;
  }

  static void NPN_Status(NPP instance, const char *message) {
    ENSURE_MAIN_THREAD_VOID();
    if (instance && instance->ndata) {
      Impl *impl = static_cast<Impl *>(instance->ndata);
      impl->on_new_message_handler_(message);
    }
  }

  static const char *NPN_UserAgent(NPP instance) {
    CHECK_MAIN_THREAD();
    // Returns the same UserAgent string with firefox-3.0.1.
    // When wmode transparent/opaque is used, flash player 10 beta 2 plugin for
    // Linux first try to detect browser's user agent string, and if the string
    // is not one of those it expects, it will turn to window mode, no matter
    // whether browser-side support windowless mode or not.
    return kFirefox3UserAgent;
  }

  static uint32 NPN_MemFlush(uint32 size) {
    CHECK_MAIN_THREAD();
    return 0;
  }

  static void NPN_ReloadPlugins(NPBool reloadPages) {
    // We don't provide any plugin-in with the authority that can reload
    // all plug-ins in the plugins directory.
    NOT_IMPLEMENTED();
  }

  static JRIEnv *NPN_GetJavaEnv(void) {
    NOT_IMPLEMENTED();
    return NULL;
  }

  static jref NPN_GetJavaPeer(NPP instance) {
    NOT_IMPLEMENTED();
    return NULL;
  }

  static NPError NPN_GetValue(NPP instance, NPNVariable variable, void *value) {
    // This function may be called before any instance is constructed.
    ENSURE_MAIN_THREAD(NPERR_INVALID_PARAM);
    switch (variable) {
      case NPNVjavascriptEnabledBool:
        *static_cast<NPBool *>(value) = true;
        break;
      case NPNVSupportsXEmbedBool:
        *static_cast<NPBool *>(value) = true;
        break;
      case NPNVToolkit:
        // This value is only applicable for GTK.
        *static_cast<NPNToolkitType *>(value) = NPNVGtk2;
        break;
      case NPNVisOfflineBool:
      case NPNVasdEnabledBool:
        *static_cast<NPBool *>(value) = false;
        break;
      case NPNVSupportsWindowless:
        *static_cast<NPBool *>(value) = true;
        break;
#ifdef MOZ_X11
      case NPNVxDisplay:
        *static_cast<Display **>(value) = display_;
        DLOG("NPN_GetValue NPNVxDisplay: %p", display_);
        break;
      case NPNVxtAppContext:
        if (!display_)
          return NPERR_GENERIC_ERROR;
        *static_cast<XtAppContext *>(value) =
            XtDisplayToApplicationContext(display_);
        break;
#endif
      case NPNVWindowNPObject:
        if (instance && instance->ndata) {
          Impl *impl = static_cast<Impl *>(instance->ndata);
          RetainNPObject(&impl->browser_window_npobject_);
          *static_cast<NPObject **>(value) = &impl->browser_window_npobject_;
          break;
        }
        // Fall through!
      case NPNVserviceManager:
      case NPNVDOMElement:
      case NPNVPluginElementNPObject:
        // TODO: This variable is for popup window/menu purpose when
        // windowless mode is used. We must provide a parent window for
        // the plugin to show popups if we want to support.
      case NPNVnetscapeWindow:
      default:
        LOG("NPNVariable %d is not supported.", variable);
        return NPERR_GENERIC_ERROR;
    }
    return NPERR_NO_ERROR;
  }

  static NPError NPN_SetValue(NPP instance, NPPVariable variable, void *value) {
    ENSURE_MAIN_THREAD(NPERR_INVALID_PARAM);
    if (instance && instance->ndata) {
      Impl *impl = static_cast<Impl *>(instance->ndata);
      switch (variable) {
        case NPPVpluginWindowBool:
          impl->windowless_ = !value;
          break;
        case NPPVpluginTransparentBool:
          impl->transparent_ = value ? true : false;
          break;
        case NPPVjavascriptPushCallerBool:
        case NPPVpluginKeepLibraryInMemory:
        default:
          LOG("NPNVariable %d is not supported.", variable);
          return NPERR_GENERIC_ERROR;
      }
      return NPERR_NO_ERROR;
    }
    return NPERR_INVALID_PARAM;
  }

  static void NPN_InvalidateRect(NPP instance, NPRect *invalid_rect) {
    ENSURE_MAIN_THREAD_VOID();
    if (instance && instance->ndata) {
      Impl *impl = static_cast<Impl *>(instance->ndata);
      impl->InvalidateRect(invalid_rect);
    }
  }

  static void NPN_InvalidateRegion(NPP instance, NPRegion invalidRegion) {
    NOT_IMPLEMENTED();
  }

  static void NPN_ForceRedraw(NPP instance) {
    ENSURE_MAIN_THREAD_VOID();
    if (instance && instance->ndata) {
      Impl *impl = static_cast<Impl *>(instance->ndata);
      impl->ForceRedraw();
    }
  }

  static void NPN_GetStringIdentifiers(const NPUTF8 **names,
                                       int32_t name_count,
                                       NPIdentifier *identifiers) {
    ENSURE_MAIN_THREAD_VOID();
    for (int32_t i = 0; i < name_count; i++)
      identifiers[i] = GetStringIdentifier(names[i]);
  }

  static bool NPN_Invoke(NPP npp, NPObject *npobj, NPIdentifier method_name,
                         const NPVariant *args, uint32_t arg_count,
                         NPVariant *result) {
    ENSURE_MAIN_THREAD(false);
    return npobj && npobj->_class && npobj->_class->invoke &&
           npobj->_class->invoke(npobj, method_name, args, arg_count, result);
  }

  static bool NPN_InvokeDefault(NPP npp, NPObject *npobj,
                                const NPVariant *args, uint32_t arg_count,
                                NPVariant *result) {
    ENSURE_MAIN_THREAD(false);
    return npobj && npobj->_class && npobj->_class->invokeDefault &&
           npobj->_class->invokeDefault(npobj, args, arg_count, result);
  }

  static bool NPN_Evaluate(NPP npp, NPObject *npobj, NPString *script,
                           NPVariant *result) {
    NOT_IMPLEMENTED();
    return false;
  }

  static bool NPN_GetProperty(NPP npp, NPObject *npobj,
                              NPIdentifier property_name, NPVariant *result) {
    ENSURE_MAIN_THREAD(false);
    return npobj && npobj->_class && npobj->_class->getProperty &&
           npobj->_class->getProperty(npobj, property_name, result);
  }

  static bool NPN_SetProperty(NPP npp, NPObject *npobj,
                              NPIdentifier property_name,
                              const NPVariant *value) {
    ENSURE_MAIN_THREAD(false);
    return npobj && npobj->_class && npobj->_class->setProperty &&
           npobj->_class->setProperty(npobj, property_name, value);
  }

  static bool NPN_RemoveProperty(NPP npp, NPObject *npobj,
                                 NPIdentifier property_name) {
    ENSURE_MAIN_THREAD(false);
    return npobj && npobj->_class && npobj->_class->removeProperty &&
           npobj->_class->removeProperty(npobj, property_name);
  }

  static bool NPN_HasProperty(NPP npp, NPObject *npobj,
                              NPIdentifier property_name) {
    ENSURE_MAIN_THREAD(false);
    return npobj && npobj->_class && npobj->_class->hasProperty &&
           npobj->_class->hasProperty(npobj, property_name);
  }

  static bool NPN_HasMethod(NPP npp, NPObject *npobj,
                            NPIdentifier method_name) {
    ENSURE_MAIN_THREAD(false);
    return npobj && npobj->_class && npobj->_class->hasMethod &&
           npobj->_class->hasMethod(npobj, method_name);
  }

  static void NPN_SetException(NPObject *npobj, const NPUTF8 *message) {
    NOT_IMPLEMENTED();
  }

  static bool NPN_PushPopupsEnabledState(NPP instance, NPBool enabled) {
    NOT_IMPLEMENTED();
    return false;
  }

  static bool NPN_PopPopupsEnabledState(NPP instance) {
    NOT_IMPLEMENTED();
    return false;
  }

  static bool NPN_Enumerate(NPP npp, NPObject *npobj,
                            NPIdentifier **identifiers, uint32_t *count) {
    ENSURE_MAIN_THREAD(false);
    return npobj && npobj->_class && npobj->_class->enumerate &&
           npobj->_class->enumerate(npobj, identifiers, count);
  }

  class AsyncCall : public WatchCallbackInterface {
   public:
    AsyncCall(void (*func)(void *), void *user_data)
        : func_(func), user_data_(user_data) { }
    virtual bool Call(MainLoopInterface *mainloop, int watch_id) {
      (*func_)(user_data_);
      return false;
    }
    virtual void OnRemove(MainLoopInterface *mainloop, int watch_id) {
      delete this;
    }
   private:
    void (*func_)(void *);
    void *user_data_;
  };

  struct OwnedNPObject : public NPObject {
    OwnedNPObject(Impl *a_owner, NPClass *a_class) {
      memset(this, 0, sizeof(OwnedNPObject));
      owner = a_owner;
      _class = a_class;
      referenceCount = 1;
    }
    Impl *owner;
  };

  // According to NPAPI specification, plugins should perform appropriate
  // synchronization with the code in their NPP_Destroy routine to avoid
  // incorrect execution and memory leaks caused by the race conditions
  // between calling this function and termination of the plugin instance.
  static void NPN_PluginThreadAsyncCall(NPP instance, void (*func)(void *),
                                        void *user_data) {
    if (GetGlobalMainLoop()->IsMainThread())
      DLOG("NPN_PluginThreadAsyncCall called from the non-main thread.");
    else
      DLOG("NPN_PluginThreadAsyncCall called from the main thread.");
    GetGlobalMainLoop()->AddTimeoutWatch(0, new AsyncCall(func, user_data));
  }

  static bool NPN_Construct(NPP npp, NPObject *npobj,
                            const NPVariant *args, uint32_t argc,
                            NPVariant *result) {
    ENSURE_MAIN_THREAD(false);
    return npp && npobj && npobj->_class && npobj->_class->construct &&
           npobj->_class->construct(npobj, args, argc, result);
  }

  // Only support window.top and window.location because the flash plugin
  // requires them.
  static bool BrowserWindowHasProperty(NPObject *npobj, NPIdentifier name) {
    ENSURE_MAIN_THREAD(false);
    std::string name_str = GetIdentifierName(name);
    return name_str == "location" || name_str == "top";
  }
  static bool BrowserWindowGetProperty(NPObject *npobj, NPIdentifier name,
                                       NPVariant *result) {
    ENSURE_MAIN_THREAD(false);
    std::string name_str = GetIdentifierName(name);
    Impl *owner = reinterpret_cast<OwnedNPObject *>(npobj)->owner;
    if (name_str == "location") {
      RetainNPObject(&owner->location_npobject_);
      OBJECT_TO_NPVARIANT(&owner->location_npobject_, *result);
      return true;
    } else if (name_str == "top") {
      RetainNPObject(&owner->browser_window_npobject_);
      OBJECT_TO_NPVARIANT(&owner->browser_window_npobject_, *result);
      return true;
    }
    return false;
  }

  static bool LocationHasMethod(NPObject *npobj, NPIdentifier name) {
    ENSURE_MAIN_THREAD(false);
    return GetIdentifierName(name) == "toString";
  }

  static bool LocationInvoke(NPObject *npobj, NPIdentifier name,
                             const NPVariant *args, uint32_t argCount,
                             NPVariant *result) {
    ENSURE_MAIN_THREAD(false);
    if (GetIdentifierName(name) == "toString") {
      Impl *owner = reinterpret_cast<OwnedNPObject *>(npobj)->owner;
      NewNPVariantString(owner->location_, result);
      return true;
    }
    return false;
  }

  static bool LocationHasProperty(NPObject *npobj, NPIdentifier name) {
    ENSURE_MAIN_THREAD(false);
    return GetIdentifierName(name) == "href";
  }

  static bool LocationGetProperty(NPObject *npobj, NPIdentifier name,
                                       NPVariant *result) {
    ENSURE_MAIN_THREAD(false);
    if (GetIdentifierName(name) == "href") {
      Impl *owner = reinterpret_cast<OwnedNPObject *>(npobj)->owner;
      NewNPVariantString(owner->location_, result);
      return true;
    }
    return false;
  }

  std::string mime_type_;
  BasicElement *element_;
  PluginLibraryInfo *library_info_;
  NPP_t instance_;
  ScriptableNPObject *plugin_root_;
  NPWindow window_;
  bool windowless_;
  bool transparent_;
  NPError init_error_;
  std::string temp_dir_;
  int temp_file_seq_;

  Signal1<void, const char *> on_new_message_handler_;
  Signal0<void> on_destroy_;
  static const NPNetscapeFuncs kContainerFuncs;
#ifdef MOZ_X11
  // Stores the XDisplay pointer to make it available during plugin library
  // initialization. nspluginwrapper requires this.
  static Display *display_;
#endif

  std::string location_;
  OwnedNPObject browser_window_npobject_;
  OwnedNPObject location_npobject_;
  static NPClass kBrowserWindowClass;
  static NPClass kLocationClass;
};

NPClass Plugin::Impl::kBrowserWindowClass = {
  NP_CLASS_STRUCT_VERSION,
  NULL, NULL, NULL, NULL, NULL, NULL,
  BrowserWindowHasProperty,
  BrowserWindowGetProperty,
  NULL, NULL, NULL, NULL
};

NPClass Plugin::Impl::kLocationClass = {
  NP_CLASS_STRUCT_VERSION,
  NULL, NULL, NULL,
  LocationHasMethod,
  LocationInvoke,
  NULL,
  LocationHasProperty,
  LocationGetProperty,
  NULL, NULL, NULL, NULL
};

const NPNetscapeFuncs Plugin::Impl::kContainerFuncs = {
  static_cast<uint16>(sizeof(NPNetscapeFuncs)),
  static_cast<uint16>((NP_VERSION_MAJOR << 8) + NP_VERSION_MINOR),
  NPN_GetURL,
  NPN_PostURL,
  NPN_RequestRead,
  NPN_NewStream,
  NPN_Write,
  NPN_DestroyStream,
  NPN_Status,
  NPN_UserAgent,
  MemAlloc,
  MemFree,
  NPN_MemFlush,
  NPN_ReloadPlugins,
  NPN_GetJavaEnv,
  NPN_GetJavaPeer,
  NPN_GetURLNotify,
  NPN_PostURLNotify,
  NPN_GetValue,
  NPN_SetValue,
  NPN_InvalidateRect,
  NPN_InvalidateRegion,
  NPN_ForceRedraw,
  GetStringIdentifier,
  NPN_GetStringIdentifiers,
  GetIntIdentifier,
  IdentifierIsString,
  UTF8FromIdentifier,
  IntFromIdentifier,
  CreateNPObject,
  RetainNPObject,
  ReleaseNPObject,
  NPN_Invoke,
  NPN_InvokeDefault,
  NPN_Evaluate,
  NPN_GetProperty,
  NPN_SetProperty,
  NPN_RemoveProperty,
  NPN_HasProperty,
  NPN_HasMethod,
  ReleaseNPVariant,
  NPN_SetException,
  NPN_PushPopupsEnabledState,
  NPN_PopPopupsEnabledState,
  NPN_Enumerate,
  NPN_PluginThreadAsyncCall,
  NPN_Construct,
};

#ifdef MOZ_X11
Display *Plugin::Impl::display_ = NULL;
#endif

Plugin::Plugin() : impl_(NULL) {
  // impl_ should be set in Plugin::Create().
}

Plugin::~Plugin() {
  delete impl_;
}

void Plugin::Destroy() {
  delete this;
}

std::string Plugin::GetName() const {
  return impl_->library_info_->name;
}

std::string Plugin::GetDescription() const {
  return impl_->library_info_->description;
}

bool Plugin::IsWindowless() const {
  return impl_->windowless_;
}

bool Plugin::SetWindow(const NPWindow &window) {
  return impl_->SetWindow(window);
}

bool Plugin::IsTransparent() const {
  return impl_->transparent_;
}

bool Plugin::HandleEvent(void *event) {
  return impl_->HandleEvent(event);
}

void Plugin::SetSrc(const char *src) {
  // Start the initial data stream.
  impl_->location_ = src;
  impl_->HandleURL("GET", src, NULL, "", false, NULL);
}

Connection *Plugin::ConnectOnNewMessage(Slot1<void, const char *> *handler) {
  return impl_->on_new_message_handler_.Connect(handler);
}

ScriptableInterface *Plugin::GetScriptablePlugin() {
  return impl_->GetScriptablePlugin();
}

Plugin *Plugin::Create(const char *mime_type, BasicElement *element,
                       const NPWindow &window, const StringMap &parameters) {
#ifdef MOZ_X11
  // Set it early because it may be used during library initialization.
  if (!Impl::display_ && window.ws_info)
    Impl::display_ = static_cast<NPSetWindowCallbackStruct *>(
        window.ws_info)->display;
#endif
  PluginLibraryInfo *library_info = GetPluginLibrary(mime_type,
                                                     &Impl::kContainerFuncs);
  if (!library_info)
    return NULL;

  Plugin *plugin = new Plugin();
  plugin->impl_ = new Plugin::Impl(mime_type, element, library_info, window,
                                   parameters);

  if (plugin->impl_->init_error_ != NPERR_NO_ERROR) {
    plugin->Destroy();
    return NULL;
  }

  return plugin;
}

} // namespace npapi
} // namespace ggadget
