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
#include <X11/Xlib.h>

#include <vector>
#include <cmath>
#include <fstream>
#include <sstream>

#include <ggadget/main_loop_interface.h>
#include <ggadget/xml_http_request_interface.h>
#include <ggadget/gadget.h>
#include <ggadget/view.h>
#include <ggadget/scoped_ptr.h>
#include <ggadget/math_utils.h>

#include "npapi_impl.h"
#include "npapi_plugin_script.h"

namespace ggadget {
namespace npapi {

#define NOT_IMPLEMENTED() \
  LOGW("Unimplemented function %s at line %d\n", __func__, __LINE__)

#define FF3_USERAGENT_ID \
  "Mozilla/5.0 (X11; U; Linux i686 (x86_64); en-US; rv:1.9.0.1) " \
  "Gecko/2008072401 Minefield/3.0.1"

static const char *kHttpURLPrefix = "http://";
static const char *kHttpsURLPrefix = "https://";
static const char *kLocalURLPrefix = "file://";

static const int kStreamCallbackTimeout = 20;

class NPPlugin::Impl {
 public:
  class StreamHost : public WatchCallbackInterface {
   public:
    StreamHost(Impl *owner, NPStream *stream,
               XMLHttpRequestInterface *http_request)
        : instance_(owner->instance_), plugin_funcs_(owner->plugin_funcs_),
          mime_type_(owner->mime_type_.c_str()),
          stream_(stream), invalid_stream_(true),
          http_request_(http_request),
          file_opened_(false), offset_(0) {
      // If it's a local stream, http_request must be NULL.
      if (!http_request_) {
        if (!InitStream())
          return;
        ASSERT(strncmp(stream->url, kLocalURLPrefix,
                       strlen(kLocalURLPrefix)) == 0);
        path_ = stream->url + strlen(kLocalURLPrefix);
        if (stype_ == NP_NORMAL) {
          file_.open(path_.c_str());
          if (!file_)
            return;
          file_opened_ = true;
        }
      } else {
        if (http_request_->GetReadyState() != XMLHttpRequestInterface::DONE)
          return;
        // The stream has been downloaded.
        if ((http_request_->GetResponseBody(&data_) !=
             XMLHttpRequestInterface::NO_ERR) || data_.empty()) {
          LOGE("Failed to download stream %s", stream_->url);
          return;
        }
        if (!InitStream())
          return;
        if (stype_ == NP_ASFILEONLY || stype_ == NP_ASFILE) {
          // If the mode is AsFileOnly or AsFile, we save the data into a
          // local file, and will use pass the file path to plugin directly
          // without any reading.
          static size_t suffix = 0;
          std::stringstream str;
          str << "/tmp/tmp_flash_" << suffix << ".swf";
          path_ = str.str();
          suffix++;
        } else {
          // Will read data from the data string directly.
          ASSERT(stype_ == NP_NORMAL);
        }
      }
      invalid_stream_ = false;
    }

    ~StreamHost() {
      if (file_opened_)
        file_.close();
      if (plugin_funcs_->destroystream)
        plugin_funcs_->destroystream(instance_, stream_, NPRES_DONE);
      delete stream_;
    }

    virtual bool Call(MainLoopInterface *main_loop, int watch_id) {
      if (invalid_stream_ || !plugin_funcs_)
        return false;

      // There are four ways to pass data to the plugin:
      // 1. Local file && NP_NORMAL mode:
      //    Read from the local file, and use NPP_Write to pass the data.
      // 3. Http stream && NP_NORMAL mode:
      //    Using the data string directly and NPP_Write to pass the data.
      // 2. Local file && (NP_ASFILEONLY or NP_ASFILE) mode:
      //    Pass the local file path to the plugin using NPP_StreamAsFile.
      // 4. Http stream && (NP_ASFILEONLY or NP_ASFILE) mode:
      //    Save the data to a local tmp file, and pass the file path to plugin
      //    using NPP_StreamAsFile.
      if (stype_ == NP_NORMAL || stype_ == 0) {
        if (!plugin_funcs_->writeready || !plugin_funcs_->write)
          return false;
        int32 len = plugin_funcs_->writeready(instance_, stream_);
        if (len <= 0) {
          // Plugin doesn't need data this time, but it doesn't mean the stream
          // is not needed any more.
          return true;
        }

        char *buf;
        scoped_array<char> proxy_buf;
        if (http_request_) {
          ASSERT(!data_.empty());
          if (offset_ >= data_.size())
            return false;
          buf = const_cast<char *>(data_.data()) + offset_;
          if (data_.size() < offset_ + len)
            len = data_.size() - offset_;
        } else if (!file_.eof()) {
          proxy_buf.reset(new char[len]);
          len = static_cast<int32>(file_.read(proxy_buf.get(), len).gcount());
          if (file_.bad())
            return false;
          buf = proxy_buf.get();
        } else {
          return false;
        }

        int32 consumed = plugin_funcs_->write(instance_, stream_,
                                              offset_, len, buf);
        // Error occurs.
        if (consumed < 0) {
          return false;
        }
        if (consumed != len && !http_request_) {
          file_.seekg(consumed - len, std::ios::cur);
        }

        offset_ += consumed;
        return true;
      } else if (stype_ == NP_ASFILEONLY || stype_ == NP_ASFILE) {
        if (!plugin_funcs_->asfile || path_.empty())
          return false;
        plugin_funcs_->asfile(instance_, stream_, path_.c_str());
        // The whole file has been pass over to the plugin, no need to
        // keep this timeout watch anymore.
      }
      return false;
    }

    virtual void OnRemove(MainLoopInterface *main_loop, int watch_id) {
      delete this;
    }

   private:
    bool InitStream() {
      if (stream_ && plugin_funcs_ &&
          plugin_funcs_->newstream &&
          plugin_funcs_->newstream(instance_, const_cast<char *>(mime_type_),
                                   stream_, false, &stype_) == NPERR_NO_ERROR) {
        if (stype_ != NP_SEEK) {
          return true;
        }
        LOGE("Plugin needs NP_SEEK stream mode which is not supported.");
      }
      return false;
    }

    NPP instance_;
    NPPluginFuncs *plugin_funcs_;
    const char *mime_type_;
    NPStream *stream_;
    bool invalid_stream_;
    XMLHttpRequestInterface *http_request_;
    uint16 stype_;
    std::string data_;
    std::string path_;
    bool file_opened_;
    std::ifstream file_;
    size_t offset_;
  };

  class XMLHttpRequestSlot : public Slot0<void> {
   public:
    XMLHttpRequestSlot(Impl *owner, NPStream *stream,
                       XMLHttpRequestInterface *http_request,
                       bool notify, void *notify_data)
        : owner_(owner), stream_(stream),
          http_request_(http_request),
          notify_(notify), notify_data_(notify_data) {
    }

    virtual ResultVariant Call(ScriptableInterface *object,
                               int argc, const Variant argv[]) const {
      XMLHttpRequestInterface::State state = http_request_->GetReadyState();
      if (state == XMLHttpRequestInterface::HEADERS_RECEIVED)
        http_request_->GetAllResponseHeaders(&stream_->headers);
      else if (state == XMLHttpRequestInterface::DONE)
        owner_->OnStreamReady(stream_, http_request_, notify_, notify_data_);
      return ResultVariant(Variant());
    }

   private:
    virtual bool operator==(const Slot &another) const {
      return false;
    }

    Impl *owner_;
    NPStream *stream_;
    XMLHttpRequestInterface *http_request_;
    bool notify_;
    void *notify_data_;
  };

  class GetURLCallback : public WatchCallbackInterface {
   public:
    GetURLCallback(Impl *owner, const char *url, bool use_browser,
                   bool notify, void *notify_data)
        : owner_(owner), url_(url), use_browser_(use_browser),
          notify_(notify), notify_data_(notify_data) {
    }

    virtual bool Call(MainLoopInterface *main_loop, int watch_id) {
      if (notify_ && !owner_->plugin_funcs_->urlnotify)
        return false;
      bool ret = NPRES_NETWORK_ERR;
      if (owner_->in_user_interaction_) {
        View *view= owner_->element_->GetView();
        bool old = view->GetGadget()->SetInUserInteraction(true);
        if (use_browser_) {
          // Load the URL in a new blank unnamed browser window.
          if (owner_->element_) {
            if (view->OpenURL(url_.c_str()))
              ret = NPERR_NO_ERROR;
            if (ret == NPERR_NO_ERROR && notify_)
              owner_->plugin_funcs_->urlnotify(owner_->instance_, url_.c_str(),
                                               NPRES_DONE, notify_data_);
          }
        } else {
          ret = owner_->SetURL(url_.c_str(), notify_, notify_data_);
        }
        view->GetGadget()->SetInUserInteraction(old);
        owner_->in_user_interaction_ = false;
      } else {
        // Just notify the plugin that the user breaks the stream.
        // Don't return ERROR because we don't want that the plugin may fail to
        // continue just because we forbid its unsafe GetURL request. Returning
        // DONE may cause the plugin to do extra work for the stream which we
        // forbid. Both are not expected.
        ret = NPERR_NO_ERROR;
        if (notify_)
          owner_->plugin_funcs_->urlnotify(owner_->instance_, url_.c_str(),
                                           NPRES_USER_BREAK, notify_data_);
        LOGW("Warning: the plugin sends GetURL request without user's claim.");
        // TODO:
        // For windowed mode, user's actions in the window, such as button click
        // and key press will be pass to the window(or socket window) directly,
        // but not through our view. So in_user_interaction will always be
        // false even user clicks on the window. This means, for window mode,
        // GetURL always fails. This should be fixed in future.
        if (owner_->plugin_window_type_ == WindowTypeWindowed)
          LOGW("GetURL request is not supported under window mode currently.");
      }

      if (ret != NPERR_NO_ERROR && notify_) {
        // Stream fails due to problems with network, disk I/O, lack of memory,
        // or other problems.
        if (owner_->plugin_funcs_ && owner_->plugin_funcs_->urlnotify)
          owner_->plugin_funcs_->urlnotify(owner_->instance_, url_.c_str(),
                                           NPRES_NETWORK_ERR, notify_data_);
      }
      return false;
    }

    virtual void OnRemove(MainLoopInterface *main_loop, int watch_id) {
      delete this;
    }

   private:
    Impl *owner_;
    std::string url_;
    bool use_browser_;
    bool notify_;
    void *notify_data_;
  };

  Impl(const std::string &mime_type, BasicElement *element,
       const std::string &name, const std::string &description,
       NPP instance, NPPluginFuncs *plugin_funcs,
       bool xembed, ToolkitType toolkit)
    : mime_type_(mime_type), element_(element),
      name_(name), description_(description),
      instance_(instance), plugin_funcs_(plugin_funcs), plugin_root_(NULL),
      window_(NULL), display_(NULL),
      host_xembed_(xembed), host_toolkit_(toolkit),
      plugin_window_type_(WindowTypeWindowed),
      transparent_(false),
      in_user_interaction_(false) {
    ASSERT(instance_ && plugin_funcs_);
  }

  ~Impl() {
    if (plugin_root_)
      delete plugin_root_;
    for (size_t i = 0; i < watch_ids_.size(); i++)
      GetGlobalMainLoop()->RemoveWatch(watch_ids_[i]);
  }

  bool SetWindow(Window *window) {
    if (!window || window->type_ != plugin_window_type_) {
      if (window) {
        LOGE("Window types don't match (type passed in: %d, "
             "while plugin's type: %d", window->type_, plugin_window_type_);
      }
      return false;
    }
    // Host must have set the window info struct.
    if (!window->ws_info_)
      return false;
    window->ws_info_->type_ = NP_SETWINDOW;
    if (window->type_ == WindowTypeWindowed) {
      if (plugin_funcs_->getvalue == NULL)
        return false;
      PRBool needs_xembed;
      NPError error = plugin_funcs_->getvalue(instance_,
                                              NPPVpluginNeedsXEmbed,
                                              (void *)&needs_xembed);
      // Currently we only support xembed when windowed mode is used.
      if (error != NPERR_NO_ERROR || host_xembed_ != needs_xembed)
        return false;
    }
    if (plugin_funcs_->setwindow != NULL &&
        plugin_funcs_->setwindow(instance_,
                                 reinterpret_cast<NPWindow *>(window)) ==
        NPERR_NO_ERROR) {
      window_ = window;
      dirty_rect_.left_ = dirty_rect_.top_ = 0;
      dirty_rect_.right_ = window_->width_;
      dirty_rect_.bottom_ = window_->height_;
      return true;
    }

    return false;
  }

  bool SetURL(const char *url) {
    return SetURL(url, false, NULL);
  }

  bool SetURL(const char *url, bool notify, void *notify_data) {
    if (!url || !*url)
      return false;

    NPStream *stream = new NPStream;
    memset(stream, 0, sizeof(NPStream));
    stream->ndata = this;
    stream->url = url;

    // Currently, we only support local files and http/https streams.
    // For http/https streams, use xmlhttprequest to download it.
    if (strncasecmp(url, kHttpURLPrefix, strlen(kHttpURLPrefix)) == 0 ||
        strncasecmp(url, kHttpsURLPrefix, strlen(kHttpsURLPrefix)) == 0) {
      XMLHttpRequestInterface *http_request =
          GetXMLHttpRequestFactory()->CreateXMLHttpRequest(0, NULL);
      http_request->ConnectOnReadyStateChange(new XMLHttpRequestSlot(
          this, stream, http_request, notify, notify_data));
      // Don't free http_request ourselves as we are using async mode and the
      // internal thread will free the object itself.
      if ((http_request->Open("Get", url, true, NULL, NULL) !=
           XMLHttpRequestInterface::NO_ERR) ||
          (http_request->Send(NULL, 0) != XMLHttpRequestInterface::NO_ERR)) {
        LOGE("Failed to download the http stream: %s", url);
        return false;
      }
      return true;
    } else if (strncasecmp(url, kLocalURLPrefix, strlen(kLocalURLPrefix)) == 0) {
      const char *path = url + strlen(kLocalURLPrefix);
      struct stat fstat;
      if (stat(path, &fstat) != 0) {
        LOGE("Local file %s doesn't exist.", path);
        return false;
      }
      stream->end = fstat.st_size;
      stream->lastmodified = fstat.st_mtime;
      OnStreamReady(stream, NULL, notify, notify_data);
      return true;
    } else {
      LOGE("The protocal of URL %s is not supported.", url);
      return false;
    }
  }

  EventResult HandleEvent(XEvent &event) {
    // Set coordinate fields for GraphicsExpose event.
    if (event.type == GraphicsExpose) {
      XGraphicsExposeEvent &expose = event.xgraphicsexpose;
      // Area to redraw, note to add the offset of x, y.
      expose.x = window_->x_ + dirty_rect_.left_;
      expose.y = window_->y_ + dirty_rect_.top_;
      expose.width = dirty_rect_.right_ - dirty_rect_.left_;
      expose.height = dirty_rect_.bottom_ - dirty_rect_.top_;
      // For transparent mode, the invalid area of drawable must be cleared
      // before the plugin draws on it.
      if (transparent_) {
        // If display or drawable changes, create a new graphics context.
        if (!(drawable_ == expose.drawable && display_ == expose.display)) {
          drawable_ = expose.drawable;
          display_ = expose.display;
          XGCValues value = { GXset, };
          gc_ = XCreateGC(display_, drawable_, GCFunction, &value);
        }
        // Clear the invalid area. It's host's responsibility to clear the
        // background of the drawable.
        XFillRectangle(display_, drawable_, gc_, expose.x, expose.y,
                       expose.width, expose.height);
      }
      // Information not set:
      expose.count = 0;
      expose.serial = 0;
      expose.send_event = false;
      expose.major_code = 0;
      expose.minor_code = 0;
    } else if (event.type == ButtonPress || event.type == KeyPress) {
      in_user_interaction_ = true;
    }
    bool handled = false;
    if (plugin_funcs_ && plugin_funcs_->event) {
      handled = plugin_funcs_->event(instance_, &event);
    }
    return handled ? EVENT_RESULT_HANDLED : EVENT_RESULT_UNHANDLED;
  }

  ScriptableInterface *GetScriptablePlugin() {
    if (!plugin_root_) {
      NPObject *plugin_root;
      if (plugin_funcs_->getvalue != NULL &&
          plugin_funcs_->getvalue(instance_, NPPVpluginScriptableNPObject,
                                  &plugin_root) == NPERR_NO_ERROR) {
        if (plugin_root) {
          plugin_root_ = new NPPluginObject(instance_, plugin_root);
          return plugin_root_;
        }
      }
      return NULL;
    }
    return plugin_root_;
  }

  void OnStreamReady(NPStream *stream, XMLHttpRequestInterface *http_request,
                     bool notify, void *notify_data) {
    if (notify && plugin_funcs_ && plugin_funcs_->urlnotify) {
      plugin_funcs_->urlnotify(instance_, stream->url, NPRES_DONE, notify_data);
    }
    StreamHost *stream_host = new StreamHost(this, stream, http_request);
    watch_ids_.push_back(GetGlobalMainLoop()->
                         AddTimeoutWatch(kStreamCallbackTimeout, stream_host));
  }

  // TODO:
  // URL passed in by plugin may be a javascript request, such as "javascript:
  // object.subobject", to get browse-side object. But currently, we don't need
  // to support this. Only normal url request is effective.
  NPError GetURLHelper(const char *url, const char *target,
                       bool notify, void *notify_data) {
    bool use_browser;
    if (!url || !*url)
      return NPERR_GENERIC_ERROR;
    // Target is not specified, deliver the new stream into the plugin instance.
    if (!target) {
      use_browser = false;
    } else if (strncmp(target, "_blank", 6) == 0 ||
               strncmp(target, "_new", 4) == 0) {
      use_browser = true;
    } else {
      // It's not allowed to loading the URL into the same area the plugin
      // instance occupies, which will destroy the current plugin instance.
      return NPERR_GENERIC_ERROR;
    }

    // When user is not in interaction, GetURL will fail. But we don't check
    // the condition here, because NPN_GetURL and NPN_GetURLNotify should always
    // return NO_ERROR state immediately unless the parameters passed in are
    // incorrect. Expecially, if GetURLNotify is used, plugin may wait for the
    // result asynchronously, returning directly may cause unexpected effect
    // on the current stream.

    GetURLCallback *callback = new GetURLCallback(this, url, use_browser,
                                                  notify, notify_data);
    watch_ids_.push_back(GetGlobalMainLoop()->AddTimeoutWatch(0, callback));
    return NPERR_NO_ERROR;
  }

  /*==============================================================
   *       Native Browser-side NPAPIs -- called by plugin
   *============================================================*/

  static NPError NPN_RequestRead(NPStream* stream, NPByteRange* rangeList) {
    if (stream && stream->ndata) {
      Impl *impl = static_cast<Impl *>(stream->ndata);
      return impl->_NPN_RequestRead(stream, rangeList);
    }
    return NPERR_INVALID_PARAM;
  }

  NPError NPN_GetURL(const char* url, const char* target) {
    return GetURLHelper(url, target, false, NULL);
  }

  NPError NPN_GetURLNotify(const char* url, const char* target,
                           void *notify_data) {
    return GetURLHelper(url, target, true, notify_data);
  }

  NPError NPN_PostURL(const char* url, const char* target,
                      uint32 len, const char* buf, NPBool file) {
    NOT_IMPLEMENTED();
    return NPERR_GENERIC_ERROR;
  }

  NPError NPN_PostURLNotify(const char* url, const char* target,
                            uint32 len, const char* buf, NPBool file,
                            void* notifyData) {
    NOT_IMPLEMENTED();
    return NPERR_GENERIC_ERROR;
  }

  NPError _NPN_RequestRead(NPStream* stream, NPByteRange* rangeList) {
    NOT_IMPLEMENTED();
    return NPERR_GENERIC_ERROR;
  }

  NPError NPN_NewStream(NPMIMEType type, const char* target,
                        NPStream** stream) {
    // Plugin-produced stream is not supported.
    NOT_IMPLEMENTED();
    return NPERR_GENERIC_ERROR;
  }

  int32  NPN_Write(NPStream* stream, int32 len, void* buffer) {
    NOT_IMPLEMENTED();
    return NPERR_GENERIC_ERROR;
  }

  NPError NPN_DestroyStream(NPStream* stream, NPReason reason) {
    NOT_IMPLEMENTED();
    return NPERR_GENERIC_ERROR;
  }

  void NPN_Status(const char* message) {
    on_new_message_handler_(message);
  }

  NPError NPN_GetValue(NPNVariable variable, void *value) {
    switch (variable) {
      case NPNVjavascriptEnabledBool:
        *(bool*)value = true;
      case NPNVSupportsXEmbedBool:
        *(bool*)value = host_xembed_;
        break;
      case NPNVToolkit:
        *(NPNToolkitType *)value = (NPNToolkitType)host_toolkit_;
        break;
      case NPNVisOfflineBool:
      case NPNVasdEnabledBool:
        *(bool*)value = false;
        break;
#if NP_VERSION_MAJOR > 0 || NP_VERSION_MINOR >= 19
      case NPNVSupportsWindowless:
        *(bool*)value = true;
        break;
#endif
      case NPNVxDisplay:
      case NPNVxtAppContext:
      case NPNVserviceManager:
      case NPNVDOMElement:
      case NPNVPluginElementNPObject:
      // TODO:
      // This variable is for popup window/menu purpose when windowless mode is
      // used. We must provide a parent window for the plugin to show popups if
      // we want to support.
      case NPNVnetscapeWindow:
      case NPNVWindowNPObject:
      default:
        LOGW("NPNVariable %d is not supported.", variable);
        return NPERR_GENERIC_ERROR;
    }
    return NPERR_NO_ERROR;
  }

  NPError NPN_SetValue(NPPVariable variable, void *value) {
    switch (variable) {
      case NPPVpluginWindowBool:
        plugin_window_type_ = value ? WindowTypeWindowed : WindowTypeWindowless;
        return NPERR_NO_ERROR;
      case NPPVpluginTransparentBool:
        transparent_ = value ? true : false;
        return NPERR_NO_ERROR;
      case NPPVjavascriptPushCallerBool:
      case NPPVpluginKeepLibraryInMemory:
      default:
          break;
    }
    return NPERR_GENERIC_ERROR;
  }

  void NPN_InvalidateRect(NPRect *invalidRect) {
    if (!invalidRect)
      return;
    dirty_rect_ = *reinterpret_cast<ClipRect *>(invalidRect);
    // If right or bottom is zero or out of range, refresh the whole area.
    if (dirty_rect_.right_ == 0 || dirty_rect_.right_ > window_->width_ ||
        dirty_rect_.bottom_ == 0 || dirty_rect_.bottom_ > window_->height_) {
      dirty_rect_.left_ = 0;
      dirty_rect_.top_ = 0;
      dirty_rect_.right_ = window_->width_;
      dirty_rect_.bottom_ = window_->height_;
    }
    if (element_) {
      // Note to add the offset of x, y.
      element_->QueueDrawRect(Rectangle(window_->x_ + dirty_rect_.left_,
                                        window_->y_ + dirty_rect_.top_,
                                        dirty_rect_.right_ - dirty_rect_.left_,
                                        dirty_rect_.bottom_ -dirty_rect_.top_));
    }
  }

  void NPN_ForceRedraw() {
    XEvent event;
    event.type = GraphicsExpose;
    event.xgraphicsexpose.drawable = drawable_;
    event.xgraphicsexpose.display = display_;
    HandleEvent(event);
  }

  std::string mime_type_;
  BasicElement *element_;
  std::string name_;
  std::string description_;
  NPP instance_;
  NPPluginFuncs *plugin_funcs_;
  NPPluginObject *plugin_root_;

  Window *window_;
  Display *display_;
  Drawable drawable_;
  GC gc_;
  bool host_xembed_;
  ToolkitType host_toolkit_;
  WindowType plugin_window_type_;
  bool transparent_;
  bool in_user_interaction_;
  ClipRect dirty_rect_;

  std::vector<int> watch_ids_;
  Signal1<void, const char *> on_new_message_handler_;
};

NPPlugin::NPPlugin(const std::string &mime_type, BasicElement *element,
                   const std::string &name, const std::string &description,
                   void *instance, void *plugin_funcs,
                   bool xembed, ToolkitType toolkit)
    : impl_(new Impl(mime_type, element,
                     name, description,
                     reinterpret_cast<NPP>(instance),
                     reinterpret_cast<NPPluginFuncs *>(plugin_funcs),
                     xembed, toolkit)) {
  // Cannot call plugin functions here as we have not created the new plugin
  // instance. See how NPContainer create NPPlugin object.
}

NPPlugin::~NPPlugin() {
  delete impl_;
}

std::string NPPlugin::GetDescription() {
  return impl_->description_;
}

WindowType NPPlugin::GetWindowType() {
  return impl_->plugin_window_type_;
}

bool NPPlugin::SetWindow(Window *window) {
  return impl_->SetWindow(window);
}

bool NPPlugin::SetURL(const char *url) {
  return impl_->SetURL(url);
}

bool NPPlugin::IsTransparent() {
  return impl_->transparent_;
}

EventResult NPPlugin::HandleEvent(XEvent event) {
  return impl_->HandleEvent(event);
}

Connection *NPPlugin::ConnectOnNewMessage(Slot1<void, const char *> *handler) {
  return impl_->on_new_message_handler_.Connect(handler);
}

ScriptableInterface *NPPlugin::GetScriptablePlugin() {
  return impl_->GetScriptablePlugin();
}

/*============================================================
 *             Implementation of class NPAPIImpl.
 *==========================================================*/

static const int kPluginCallbackTimeout = 100; // ms
class PluginCallback : public WatchCallbackInterface {
 public:
  typedef void (*PluginCallbackType)(void*);
  PluginCallback(PluginCallbackType func, void *user_data)
      : func_(func), user_data_(user_data) { }
  virtual bool Call(MainLoopInterface *mainloop, int watch_id) {
    (*func_)(user_data_);
    return false;
  }
  virtual void OnRemove(MainLoopInterface *mainloop, int watch_id) {
    delete this;
  }
 private:
  PluginCallbackType func_;
  void *user_data_;
};

static bool IsMainThread() {
  return GetGlobalMainLoop()->IsMainThread();
}

void NPAPIImpl::InitContainerFuncs(NPNetscapeFuncs *container_funcs) {
  if (!container_funcs)
    return;

  memset(container_funcs, 0, sizeof(*container_funcs));
  container_funcs->size = sizeof(*container_funcs);
  container_funcs->version = (NP_VERSION_MAJOR << 8) + NP_VERSION_MINOR;

  // Browser-side NPAPIs.
  container_funcs->geturl = NewNPN_GetURLProc(NPN_GetURL);
  container_funcs->posturl = NewNPN_PostURLProc(NPN_PostURL);
  container_funcs->requestread = NewNPN_RequestReadProc(NPN_RequestRead);
  container_funcs->newstream = NewNPN_NewStreamProc(NPN_NewStream);
  container_funcs->write = NewNPN_WriteProc(NPN_Write);
  container_funcs->destroystream = NewNPN_DestroyStreamProc(NPN_DestroyStream);
  container_funcs->status = NewNPN_StatusProc(NPN_Status);
  container_funcs->uagent = NewNPN_UserAgentProc(NPN_UserAgent);
  container_funcs->memalloc = NewNPN_MemAllocProc(NPN_MemAlloc);
  container_funcs->memfree = NewNPN_MemFreeProc(NPN_MemFree);
  container_funcs->memflush = NewNPN_MemFlushProc(NPN_MemFlush);
  container_funcs->reloadplugins = NewNPN_ReloadPluginsProc(NPN_ReloadPlugins);
  container_funcs->getJavaEnv = NewNPN_GetJavaEnvProc(NPN_GetJavaEnv);
  container_funcs->getJavaPeer = NewNPN_GetJavaPeerProc(NPN_GetJavaPeer);
  container_funcs->geturlnotify = NewNPN_GetURLNotifyProc(NPN_GetURLNotify);
  container_funcs->posturlnotify = NewNPN_PostURLNotifyProc(NPN_PostURLNotify);
  container_funcs->getvalue = NewNPN_GetValueProc(NPN_GetValue);
  container_funcs->setvalue = NewNPN_SetValueProc(NPN_SetValue);
  container_funcs->invalidaterect = NewNPN_InvalidateRectProc(NPN_InvalidateRect);
  container_funcs->invalidateregion = NewNPN_InvalidateRegionProc(NPN_InvalidateRegion);
  container_funcs->forceredraw = NewNPN_ForceRedrawProc(NPN_ForceRedraw);
  container_funcs->pushpopupsenabledstate = NewNPN_PushPopupsEnabledStateProc(NPN_PushPopupsEnabledState);
  container_funcs->poppopupsenabledstate = NewNPN_PopPopupsEnabledStateProc(NPN_PopPopupsEnabledState);
#ifdef NPVERS_HAS_PLUGIN_THREAD_ASYNC_CALL
  container_funcs->pluginthreadasynccall = NewNPN_PluginThreadAsyncCallProc(NPN_PluginThreadAsyncCall);
#endif

  // npruntime APIs.
  container_funcs->getstringidentifier = NewNPN_GetStringIdentifierProc(NPN_GetStringIdentifier);
  container_funcs->getstringidentifiers = NewNPN_GetStringIdentifiersProc(NPN_GetStringIdentifiers);
  container_funcs->getintidentifier = NewNPN_GetIntIdentifierProc(NPN_GetIntIdentifier);
  container_funcs->identifierisstring = NewNPN_IdentifierIsStringProc(NPN_IdentifierIsString);
  container_funcs->utf8fromidentifier = NewNPN_UTF8FromIdentifierProc(NPN_UTF8FromIdentifier);
  container_funcs->intfromidentifier = NewNPN_IntFromIdentifierProc(NPN_IntFromIdentifier);
  container_funcs->createobject = NewNPN_CreateObjectProc(NPN_CreateObject);
  container_funcs->retainobject = NewNPN_RetainObjectProc(NPN_RetainObject);
  container_funcs->releaseobject = NewNPN_ReleaseObjectProc(NPN_ReleaseObject);
  container_funcs->invoke = NewNPN_InvokeProc(NPN_Invoke);
  container_funcs->invokeDefault = NewNPN_InvokeDefaultProc(NPN_InvokeDefault);
  container_funcs->evaluate = NewNPN_EvaluateProc(NPN_Evaluate);
  container_funcs->getproperty = NewNPN_GetPropertyProc(NPN_GetProperty);
  container_funcs->setproperty = NewNPN_SetPropertyProc(NPN_SetProperty);
  container_funcs->removeproperty = NewNPN_RemovePropertyProc(NPN_RemoveProperty);
  container_funcs->hasproperty = NewNPN_HasPropertyProc(NPN_HasProperty);
  container_funcs->hasmethod = NewNPN_HasMethodProc(NPN_HasMethod);
  container_funcs->releasevariantvalue = NewNPN_ReleaseVariantValueProc(NPN_ReleaseVariantValue);
  container_funcs->setexception = NewNPN_SetExceptionProc(NPN_SetException);
#ifdef NPVERS_HAS_NPOBJECT_ENUM
  container_funcs->enumerate = NewNPN_EnumerateProc(NPN_Enumerate);
  container_funcs->construct = NewNPN_ConstructProc(NPN_Construct);
#endif
}

NPError NPAPIImpl::NPN_GetURLNotify(NPP instance,
                                    const char* url, const char* target,
                                    void* notifyData) {
  if (!IsMainThread()) {
    LOGE("NPN_GetURLNotify called from the wrong thread.\n");
    return NPERR_INVALID_PARAM;
  }
  if (instance && instance->ndata) {
    NPPlugin *plugin = static_cast<NPPlugin *>(instance->ndata);
    return plugin->impl_->NPN_GetURLNotify(url, target, notifyData);
  }
  return NPERR_INVALID_PARAM;
}

NPError NPAPIImpl::NPN_GetURL(NPP instance,
                              const char* url, const char* target) {
  if (!IsMainThread()) {
    LOGE("NPN_GetURL called from the wrong thread.\n");
    return NPERR_INVALID_PARAM;
  }
  if (instance && instance->ndata) {
    NPPlugin *plugin = static_cast<NPPlugin *>(instance->ndata);
    return plugin->impl_->NPN_GetURL(url, target);
  }
  return NPERR_INVALID_PARAM;
}

NPError NPAPIImpl::NPN_PostURL(NPP instance,
                               const char* url, const char* target,
                               uint32 len, const char* buf, NPBool file) {
  if (!IsMainThread()) {
    LOGE("NPN_PostURL called from the wrong thread.\n");
    return NPERR_INVALID_PARAM;
  }
  if (instance && instance->ndata) {
    NPPlugin *plugin = static_cast<NPPlugin *>(instance->ndata);
    return plugin->impl_->NPN_PostURL(url, target, len, buf, file);
  }
  return NPERR_INVALID_PARAM;
}

NPError NPAPIImpl::NPN_PostURLNotify(NPP instance,
                                     const char* url, const char* target,
                                     uint32 len, const char* buf, NPBool file,
                                     void* notifyData) {
  if (!IsMainThread()) {
    LOGE("NPN_PostURLNotify called from the wrong thread.\n");
    return NPERR_INVALID_PARAM;
  }
  if (instance && instance->ndata) {
    NPPlugin *plugin = static_cast<NPPlugin *>(instance->ndata);
    return plugin->impl_->NPN_PostURLNotify(url, target, len, buf, file,
                                            notifyData);
  }
  return NPERR_INVALID_PARAM;
}

NPError NPAPIImpl::NPN_RequestRead(NPStream* stream, NPByteRange* rangeList) {
  if (!IsMainThread()) {
    LOGE("NPN_RequestRead called from the wrong thread.\n");
    return NPERR_INVALID_PARAM;
  }
  return NPPlugin::Impl::NPN_RequestRead(stream, rangeList);
}

NPError NPAPIImpl::NPN_NewStream(NPP instance, NPMIMEType type,
                                 const char* target, NPStream** stream) {
  if (!IsMainThread()) {
    LOGE("NPN_NewStream called from the wrong thread.\n");
    return NPERR_INVALID_PARAM;
  }
  if (instance && instance->ndata) {
    NPPlugin *plugin = static_cast<NPPlugin *>(instance->ndata);
    return plugin->impl_->NPN_NewStream(type, target, stream);
  }
  return NPERR_INVALID_PARAM;
}

int32 NPAPIImpl::NPN_Write(NPP instance, NPStream* stream,
                           int32 len, void* buffer) {
  if (!IsMainThread()) {
    LOGE("NPN_Write called from the wrong thread.\n");
    return -1;
  }
  if (instance && instance->ndata) {
    NPPlugin *plugin = static_cast<NPPlugin *>(instance->ndata);
    return plugin->impl_->NPN_Write(stream, len, buffer);
  }
  return -1;
}

NPError NPAPIImpl::NPN_DestroyStream(NPP instance, NPStream* stream,
                                     NPReason reason) {
  if (!IsMainThread()) {
    LOGE("NPN_DestroyStream called from the wrong thread.\n");
    return NPERR_INVALID_PARAM;
  }
  if (instance && instance->ndata) {
    NPPlugin *plugin = static_cast<NPPlugin *>(instance->ndata);
    return plugin->impl_->NPN_DestroyStream(stream, reason);
  }
  return NPERR_INVALID_PARAM;
}

void NPAPIImpl::NPN_Status(NPP instance, const char* message) {
  if (!IsMainThread()) {
    LOGE("NPN_Status called from the wrong thread.\n");
    return;
  }
  if (instance && instance->ndata) {
    NPPlugin *plugin = static_cast<NPPlugin *>(instance->ndata);
    plugin->impl_->NPN_Status(message);
  }
}

const char *NPAPIImpl::NPN_UserAgent(NPP instance) {
  if (!IsMainThread()) {
    LOGE("NPN_UserAgent called from the wrong thread.\n");
    return NULL;
  }
  // Returns the same UserAgent string with firefox-3.0.1.
  // When wmode transparent/opaque is used, flash player 10 beta 2 plugin for
  // Linux first try to detect browser's user agent string, and if the string
  // is not one of those it expects, it will turn to window mode, no matter
  // whether browser-side support windowless mode or not.
  return FF3_USERAGENT_ID;
}

void *NPAPIImpl::NPN_MemAlloc(uint32 size) {
  if (!IsMainThread()) {
    LOGW("NPN_MemAlloc called from the wrong thread.\n");
  }
  return new char[size];
}

void NPAPIImpl::NPN_MemFree(void* ptr) {
  if (!IsMainThread()) {
    LOGW("NPN_MemFree called from the wrong thread.\n");
  }
  delete[] reinterpret_cast<char*>(ptr);
}

uint32 NPAPIImpl::NPN_MemFlush(uint32 size) {
  if (!IsMainThread()) {
    LOGW("NPN_MemFlush called from the wrong thread.\n");
  }
  return 0;
}

void NPAPIImpl::NPN_ReloadPlugins(NPBool reloadPages) {
  // We don't provide any plugin-in with the authority that can reload
  // all plug-ins in the plugins directory.
  NOT_IMPLEMENTED();
}

JRIEnv* NPAPIImpl::NPN_GetJavaEnv(void) {
  NOT_IMPLEMENTED();
  return NULL;
}

jref NPAPIImpl::NPN_GetJavaPeer(NPP instance) {
  NOT_IMPLEMENTED();
  return NULL;
}

NPError NPAPIImpl::NPN_GetValue(NPP instance,
                                NPNVariable variable, void *value) {
  if (!IsMainThread()) {
    LOGE("NPN_GetValue called from the wrong thread.\n");
    return NPERR_INVALID_PARAM;
  }
  if(instance && instance->ndata) {
    NPPlugin *plugin = static_cast<NPPlugin *>(instance->ndata);
    return plugin->impl_->NPN_GetValue(variable, value);
  }
  return NPERR_INVALID_PARAM;
}

NPError NPAPIImpl::NPN_SetValue(NPP instance,
                                NPPVariable variable, void *value) {
  if (!IsMainThread()) {
    LOGE("NPN_SetValue called from the wrong thread.\n");
    return NPERR_INVALID_PARAM;
  }
  if(instance && instance->ndata) {
    NPPlugin *plugin = static_cast<NPPlugin *>(instance->ndata);
    return plugin->impl_->NPN_SetValue(variable, value);
  }
  return NPERR_INVALID_PARAM;
}

void NPAPIImpl::NPN_InvalidateRect(NPP instance, NPRect *invalidRect) {
  if (!IsMainThread()) {
    LOGE("NPN_InvalidateRect called from the wrong thread.\n");
    return;
  }
  if(instance && instance->ndata) {
    NPPlugin *plugin = static_cast<NPPlugin *>(instance->ndata);
    plugin->impl_->NPN_InvalidateRect(invalidRect);
  }
}

void NPAPIImpl::NPN_InvalidateRegion(NPP instance, NPRegion invalidRegion) {
  NOT_IMPLEMENTED();
}

void NPAPIImpl::NPN_ForceRedraw(NPP instance) {
  if (!IsMainThread()) {
    LOGE("NPN_ForceRedraw called from the wrong thread.\n");
    return;
  }
  if (instance && instance->ndata) {
    NPPlugin *plugin = static_cast<NPPlugin *>(instance->ndata);
    plugin->impl_->NPN_ForceRedraw();
  }
}

void NPAPIImpl::NPN_PushPopupsEnabledState(NPP instance, NPBool enabled) {
  NOT_IMPLEMENTED();
}

void NPAPIImpl::NPN_PopPopupsEnabledState(NPP instance) {
  NOT_IMPLEMENTED();
}

// According to npapi specification, plugins should perform appropriate
// synchronization with the code in their NPP_Destroy routine to avoid
// incorrect execution and memory leaks caused by the race conditions
// between calling this function and termination of the plugin instance.
void NPAPIImpl::NPN_PluginThreadAsyncCall(NPP instance, void (*func) (void *),
                                          void *userData) {
  if (!IsMainThread())
    LOGI("NPN_PluginThreadAsyncCall called from the main thread.\n");
  else
    LOGI("NPN_PluginThreadAsyncCall called from the non-main thread.\n");
  GetGlobalMainLoop()->AddTimeoutWatch(kPluginCallbackTimeout,
                                       new PluginCallback(func, userData));
}

void NPAPIImpl::NPN_ReleaseVariantValue(NPVariant *variant) {
  if (!IsMainThread())
    LOGW("NPN_ReleaseVariantValue called from the wrong thread.\n");
  switch (variant->type) {
    case NPVariantType_String:
      {
        NPString *s = &NPVARIANT_TO_STRING(*variant);
        if (s->utf8characters)
          delete s->utf8characters;
        break;
      }
    case NPVariantType_Object:
      {
        NPObject *npobj = NPVARIANT_TO_OBJECT(*variant);
        if (npobj)
          NPN_ReleaseObject(npobj);
        break;
      }
    default:
      break;
  }
}

NPIdentifier NPAPIImpl::NPN_GetStringIdentifier(const NPUTF8 *name) {
  if (!IsMainThread())
    LOGW("NPN_GetStringIdentifier called from the wrong thread.\n");
  if (!name)
    return NULL;
  // Use the same allocation function (i.e. new) with NPN_MemAlloc,
  // as the plugin may use NPN_MemFree to free NPIdentifier.
  return (new _NPIdentifier(_NPIdentifier::TYPE_STRING, name));
}

void NPAPIImpl::NPN_GetStringIdentifiers(const NPUTF8 **names,
                                         int32_t nameCount,
                                         NPIdentifier *identifiers) {
  if (!IsMainThread())
    LOGW("NPN_GetStringIdentifiers called from the wrong thread.\n");
  for (int32_t i = 0; i < nameCount; i++) {
    identifiers[i] = NPN_GetStringIdentifier(names[i]);
  }
}

NPIdentifier NPAPIImpl::NPN_GetIntIdentifier(int32_t intid) {
  if (!IsMainThread())
    LOGW("NPN_GetIntIdentifier called from the wrong thread.\n");
  return (new _NPIdentifier(_NPIdentifier::TYPE_INT, intid));
}

bool NPAPIImpl::NPN_IdentifierIsString(NPIdentifier identifier) {
  if (!IsMainThread())
    LOGW("NPN_IdentifierIsString called from the wrong thread.\n");
  if (!identifier)
    return false;
  return identifier->type_ == _NPIdentifier::TYPE_STRING;
}

NPUTF8 *NPAPIImpl::NPN_UTF8FromIdentifier(NPIdentifier identifier) {
  if (!IsMainThread())
    LOGW("NPN_UTF8FromIdentifier called from the wrong thread.\n");
  if (!identifier || identifier->type_ != _NPIdentifier::TYPE_STRING)
    return NULL;
  size_t size = identifier->name_.size();
  NPUTF8 *buf = reinterpret_cast<NPUTF8 *>(NPN_MemAlloc(size + 1));
  buf[size] = '\0';
  identifier->name_.copy(buf, size);
  return buf;
}

int32_t NPAPIImpl::NPN_IntFromIdentifier(NPIdentifier identifier) {
  if (!IsMainThread())
    LOGW("NPN_IntFromIdentifier called from the wrong thread.\n");
  if (!identifier || identifier->type_ != _NPIdentifier::TYPE_INT)
    return -1; // The behaviour is undefined by NPAPI.
  return identifier->intid_;
}

NPObject *NPAPIImpl::NPN_CreateObject(NPP npp, NPClass *aClass) {
  if (!IsMainThread())
    LOGW("NPN_CreateObject called from the wrong thread.\n");
  if (!aClass)
    return NULL;
  NPObject *obj;
  if (aClass->allocate) {
    obj = aClass->allocate(npp, aClass);
  } else {
    obj = new NPObject;
    obj->_class = aClass;
  }
  obj->referenceCount = 1;
  return obj;
}

NPObject *NPAPIImpl::NPN_RetainObject(NPObject *npobj) {
  if (!IsMainThread())
    LOGW("NPN_RetainObject called from the wrong thread.\n");
  if (npobj)
    npobj->referenceCount++;
  return npobj;
}

void NPAPIImpl::NPN_ReleaseObject(NPObject *npobj) {
  if (!IsMainThread())
    LOGW("NPN_ReleaseObject called from the wrong thread.\n");
  if (npobj) {
    if (--npobj->referenceCount == 0) {
      if (npobj->_class->deallocate) {
        npobj->_class->deallocate(npobj);
      } else {
        if (npobj->_class->invalidate)
          npobj->_class->invalidate(npobj);
        delete npobj;
      }
    }
  }
}

bool NPAPIImpl::NPN_Invoke(NPP npp, NPObject *npobj, NPIdentifier methodName,
                           const NPVariant *args, uint32_t argCount,
                           NPVariant *result) {
  if (!IsMainThread()) {
    LOGE("NPN_Invoke called from the wrong thread.\n");
    return false;
  }
  if (npp && npobj && npobj->_class && npobj->_class->invoke) {
    return npobj->_class->invoke(npobj, methodName, args, argCount, result);
  }
  return false;
}

bool NPAPIImpl::NPN_InvokeDefault(NPP npp, NPObject *npobj,
                                  const NPVariant *args, uint32_t argCount,
                                  NPVariant *result) {
  if (!IsMainThread()) {
    LOGE("NPN_InvokeDefault called from the wrong thread.\n");
    return false;
  }
  if (npp && npobj && npobj->_class && npobj->_class->invokeDefault) {
    return npobj->_class->invokeDefault(npobj, args, argCount, result);
  }
  return false;
}

bool NPAPIImpl::NPN_Evaluate(NPP npp, NPObject *npobj, NPString *script,
                             NPVariant *result) {
  NOT_IMPLEMENTED();
  return false;
}

bool NPAPIImpl::NPN_GetProperty(NPP npp,
                                NPObject *npobj, NPIdentifier propertyName,
                                NPVariant *result) {
  if (!IsMainThread()) {
    LOGE("NPN_GetProperty called from the wrong thread.\n");
    return false;
  }
  if (npp && npobj && npobj->_class && npobj->_class->getProperty) {
    return npobj->_class->getProperty(npobj, propertyName, result);
  }
  return false;
}

bool NPAPIImpl::NPN_SetProperty(NPP npp, NPObject *npobj,
                                NPIdentifier propertyName,
                                const NPVariant *value) {
  if (!IsMainThread()) {
    LOGE("NPN_SetProperty called from the wrong thread.\n");
    return false;
  }
  if (npp && npobj && npobj->_class && npobj->_class->setProperty) {
    return npobj->_class->setProperty(npobj, propertyName, value);
  }
  return false;
}

bool NPAPIImpl::NPN_RemoveProperty(NPP npp, NPObject *npobj,
                                   NPIdentifier propertyName) {
  if (!IsMainThread()) {
    LOGE("NPN_RemoveProperty called from the wrong thread.\n");
    return false;
  }
  if (npp && npobj && npobj->_class && npobj->_class->removeProperty) {
    return npobj->_class->removeProperty(npobj, propertyName);
  }
  return false;
}

bool NPAPIImpl::NPN_HasProperty(NPP npp,
                            NPObject *npobj, NPIdentifier propertyName) {
  if (!IsMainThread()) {
    LOGE("NPN_HasProperty called from the wrong thread.\n");
    return false;
  }
  if (npp && npobj && npobj->_class && npobj->_class->hasProperty) {
    return npobj->_class->hasProperty(npobj, propertyName);
  }
  return false;
}

bool NPAPIImpl::NPN_HasMethod(NPP npp,
                              NPObject *npobj, NPIdentifier methodName) {
  if (!IsMainThread()) {
    LOGE("NPN_HasMethod called from the wrong thread.\n");
    return false;
  }
  if (npp && npobj && npobj->_class && npobj->_class->hasMethod) {
    return npobj->_class->hasMethod(npobj, methodName);
  }
  return false;
}

void NPAPIImpl::NPN_SetException(NPObject *npobj, const NPUTF8 *message) {
  NOT_IMPLEMENTED();
}

#ifdef NPVERS_HAS_NPOBJECT_ENUM
bool NPAPIImpl::NPN_Enumerate(NPP npp, NPObject *npobj,
                          NPIdentifier **identifier, uint32_t *count) {
  if (!IsMainThread()) {
    LOGE("NPN_Enumerate called from the wrong thread.\n");
    return false;
  }
  if (npp && npobj && npobj->_class && npobj->_class->enumerate) {
    return npobj->_class->enumerate(npobj,
                                    reinterpret_cast<void ***>(identifier),
                                    count);
  }
  return false;
}

bool NPAPIImpl::NPN_Construct(NPP npp, NPObject *npobj,
                              const NPVariant *args, uint32_t argCount,
                              NPVariant *result) {
  if (!IsMainThread()) {
    LOGE("NPN_Construct called from the wrong thread.\n");
    return false;
  }
  if (npp && npobj && npobj->_class && npobj->_class->construct) {
    return npobj->_class->construct(npobj, args, argCount, result);
  }
  return false;
}
#endif

} // namespace npapi
} // namespace ggadget
