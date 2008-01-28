/*
  Copyright 2007 Google Inc.

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

#include "gadget.h"
#include "contentarea_element.h"
#include "content_item.h"
#include "details_view.h"
#include "display_window.h"
#include "element_factory.h"
#include "file_manager_interface.h"
#include "gadget_consts.h"
#include "gadget_host_interface.h"
#include "logger.h"
#include "main_loop_interface.h"
#include "menu_interface.h"
#include "options_interface.h"
#include "script_context_interface.h"
#include "scriptable_array.h"
#include "scriptable_framework.h"
#include "scriptable_helper.h"
#include "scriptable_menu.h"
#include "scriptable_options.h"
#include "view_host_interface.h"
#include "view.h"
#include "xml_parser_interface.h"
#include "extension_manager.h"

namespace ggadget {

class Gadget::Impl : public ScriptableHelperNativeOwnedDefault {
 public:
  DEFINE_CLASS_ID(0x6a3c396b3a544148, ScriptableInterface);

  class Debug : public ScriptableHelperNativeOwnedDefault {
   public:
    DEFINE_CLASS_ID(0xa9b59e70c74649da, ScriptableInterface);
    Debug(Gadget::Impl *owner) {
      RegisterMethod("error", NewSlot(owner, &Impl::DebugError));
      RegisterMethod("trace", NewSlot(owner, &Impl::DebugTrace));
      RegisterMethod("warning", NewSlot(owner, &Impl::DebugWarning));
    }
  };

  class Storage : public ScriptableHelperNativeOwnedDefault {
   public:
    DEFINE_CLASS_ID(0xd48715e0098f43d1, ScriptableInterface);
    Storage(Gadget::Impl *owner) {
      RegisterMethod("extract", NewSlot(owner, &Impl::ExtractFile));
      RegisterMethod("openText", NewSlot(owner, &Impl::OpenTextFile));
    }
  };

  static void RegisterStrings(
      const GadgetStringMap *strings,
      ScriptableHelperNativeOwnedDefault *scriptable) {
    for (GadgetStringMap::const_iterator it = strings->begin();
         it != strings->end(); ++it) {
      scriptable->RegisterConstant(it->first.c_str(), it->second);
    }
  }

  class Strings : public ScriptableHelperNativeOwnedDefault {
   public:
    DEFINE_CLASS_ID(0x13679b3ef9a5490e, ScriptableInterface);
  };

  class Plugin : public ScriptableHelperNativeOwnedDefault {
   public:
    DEFINE_CLASS_ID(0x05c3f291057c4c9c, ScriptableInterface);
    Plugin(Impl *gadget_impl) :
        gadget_host_(gadget_impl->host_),
        main_view_(NULL) {
      RegisterProperty("plugin_flags", NULL, // No getter.
                       NewSlot(gadget_host_,
                               &GadgetHostInterface::SetPluginFlags));
      RegisterMethod("RemoveMe",
                     NewSlot(gadget_host_, &GadgetHostInterface::RemoveMe));
      RegisterMethod("ShowDetailsView",
                     NewSlot(gadget_impl, &Impl::DelayedShowDetailsView));
      RegisterMethod("CloseDetailsView",
                     NewSlot(gadget_impl, &Impl::DelayedCloseDetailsView));
      RegisterMethod("ShowOptionsDialog",
                     NewSlot(gadget_impl, &Impl::ShowOptionsDialog));
      RegisterSignal("onShowOptionsDlg",
                     &gadget_impl->onshowoptionsdlg_signal_);
      RegisterSignal("onAddCustomMenuItems", &onaddcustommenuitems_signal_);
      RegisterSignal("onCommand", &oncommand_signal_);
      RegisterSignal("onDisplayStateChange", &ondisplaystatechange_signal_);
      RegisterSignal("onDisplayTargetChange", &ondisplaytargetchange_signal_);

      // Deprecated or unofficial properties and methods.
      RegisterProperty("about_text", NULL, // No getter.
                       NewSlot(gadget_impl, &Impl::SetAboutText));
      RegisterMethod("SetFlags", NewSlot(this, &Plugin::SetFlags));
      RegisterMethod("SetIcons", NewSlot(this, &Plugin::SetIcons));

      // Register properties and methods for content area.
      RegisterProperty("contant_flags", NULL, // Write only.
                       NewSlot(this, &Plugin::SetContentFlags));
      RegisterProperty("max_content_items",
                       NewSlot(this, &Plugin::GetMaxContentItems),
                       NewSlot(this, &Plugin::SetMaxContentItems));
      RegisterProperty("content_items",
                       NewSlot(this, &Plugin::GetContentItems),
                       NewSlot(this, &Plugin::SetContentItems));
      RegisterProperty("pin_images",
                       NewSlot(this, &Plugin::GetPinImages),
                       NewSlot(this, &Plugin::SetPinImages));
      RegisterMethod("AddContentItem",
                     NewSlot(this, &Plugin::AddContentItem));
      RegisterMethod("RemoveContentItem",
                     NewSlot(this, &Plugin::RemoveContentItem));
      RegisterMethod("RemoveAllContentItems",
                     NewSlot(this, &Plugin::RemoveAllContentItems));
    }

    void SetMainView(View *main_view) {
      ASSERT(main_view_ == NULL && main_view != NULL);
      main_view_ = main_view;
      // Deprecated or unofficial properties and methods.
      RegisterProperty("title", NULL, // No getter.
                       NewSlot(main_view_, &View::SetCaption));
      RegisterProperty("window_width",
                       NewSlot(main_view_, &View::GetWidth), NULL);
      RegisterProperty("window_height",
                       NewSlot(main_view_, &View::GetHeight), NULL);
    }

    void OnAddCustomMenuItems(MenuInterface *menu) {
      ScriptableMenu scriptable_menu(menu);
      onaddcustommenuitems_signal_(&scriptable_menu);
    }

    void SetFlags(int plugin_flags, int content_flags) {
      gadget_host_->SetPluginFlags(plugin_flags);
      SetContentFlags(content_flags);
    }

    void SetIcons(const Variant &param1, const Variant &param2) {
      LOG("pluginHelper.SetIcons is no longer supported. "
          "Please specify icons in the manifest file.");
    }

    void SetContentFlags(int flags) {
      ContentAreaElement *content_area = main_view_->GetContentAreaElement();
      if (content_area) content_area->SetContentFlags(flags);
    }

    size_t GetMaxContentItems() {
      ContentAreaElement *content_area = main_view_->GetContentAreaElement();
      return content_area ? content_area->GetMaxContentItems() : 0;
    }

    void SetMaxContentItems(size_t max_content_items) {
      ContentAreaElement *content_area = main_view_->GetContentAreaElement();
      if (content_area) content_area->SetMaxContentItems(max_content_items);
    }

    ScriptableArray *GetContentItems() {
      ContentAreaElement *content_area = main_view_->GetContentAreaElement();
      return content_area ? content_area->ScriptGetContentItems() : NULL;
    }

    void SetContentItems(ScriptableInterface *array) {
      ContentAreaElement *content_area = main_view_->GetContentAreaElement();
      if (content_area) content_area->ScriptSetContentItems(array);
    }

    ScriptableArray *GetPinImages() {
      ContentAreaElement *content_area = main_view_->GetContentAreaElement();
      return content_area ? content_area->ScriptGetPinImages() : NULL;
    }

    void SetPinImages(ScriptableInterface *array) {
      ContentAreaElement *content_area = main_view_->GetContentAreaElement();
      if (content_area) content_area->ScriptSetPinImages(array);
    }

    void AddContentItem(ContentItem *item,
                        ContentAreaElement::DisplayOptions options) {
      ContentAreaElement *content_area = main_view_->GetContentAreaElement();
      if (content_area) content_area->AddContentItem(item, options);
    }

    void RemoveContentItem(ContentItem *item) {
      ContentAreaElement *content_area = main_view_->GetContentAreaElement();
      if (content_area) content_area->RemoveContentItem(item);
    }

    void RemoveAllContentItems() {
      ContentAreaElement *content_area = main_view_->GetContentAreaElement();
      if (content_area) content_area->RemoveAllContentItems();
    }

    GadgetHostInterface *gadget_host_;
    View *main_view_;
    Signal1<void, ScriptableMenu *> onaddcustommenuitems_signal_;
    Signal1<void, int> oncommand_signal_;
    Signal1<void, int> ondisplaystatechange_signal_;
    Signal1<void, int> ondisplaytargetchange_signal_;
  };

  class GadgetGlobal : public ScriptableHelperNativeOwnedDefault {
   public:
    DEFINE_CLASS_ID(0x2c8d4292025f4397, ScriptableInterface);
    GadgetGlobal(Gadget::Impl *owner)
        : framework_(owner->host_) {
      RegisterConstant("gadget", owner);
      RegisterConstant("options", &owner->scriptable_options_);
      RegisterConstant("strings", &owner->strings_);
      RegisterConstant("plugin", &owner->plugin_);
      RegisterConstant("pluginHelper", &owner->plugin_);

      // As an unofficial feature, "gadget.debug" and "gadget.storage" can also
      // be accessed as "debug" and "storage" global objects.
      RegisterConstant("debug", &owner->debug_);
      RegisterConstant("storage", &owner->storage_);

      // Properties and methods of framework can also be accessed directly as
      // globals.
      RegisterConstant("framework", &framework_);
      SetInheritsFrom(&framework_);
    }

    ScriptableFramework framework_;
  };

  Impl(GadgetHostInterface *host, Gadget *owner, int debug_mode)
      : host_(host),
        debug_(this),
        storage_(this),
        scriptable_options_(host->GetOptions(), false),
        plugin_(this),
        gadget_global_(this),
        element_factory_(new ElementFactory()),
        extension_manager_(
            ExtensionManager::CreateExtensionManager(host->GetMainLoop())),
        main_view_(new View(&gadget_global_, element_factory_, debug_mode)),
        main_view_host_(host->NewViewHost(GadgetHostInterface::VIEW_MAIN,
                                          main_view_)),
        details_view_(NULL), details_view_host_(NULL),
        has_options_xml_(false),
        close_details_view_timer_(0), show_details_view_timer_(0),
        debug_mode_(debug_mode) {
    // Main view must be set here, to break circular dependency.
    plugin_.SetMainView(main_view_);
    RegisterConstant("debug", &debug_);
    RegisterConstant("storage", &storage_);
  }

  ~Impl() {
    if (close_details_view_timer_ != 0) {
      host_->GetMainLoop()->RemoveWatch(close_details_view_timer_);
      close_details_view_timer_ = 0;
    }
    if (show_details_view_timer_ != 0) {
      host_->GetMainLoop()->RemoveWatch(show_details_view_timer_);
      show_details_view_timer_ = 0;
    }

    // unload any fonts
    for (GadgetStringMap::const_iterator i = manifest_info_map_.begin();
         i != manifest_info_map_.end(); ++i) {
      const std::string &key = i->first;
      // Keys are of the form "install/font@src" or "install/font[k]@src"
      if (0 == key.find(kManifestInstallFont) &&
          (key.length() - strlen(kSrcAttr)) == key.rfind(kSrcAttr)) {
        // ignore return, error not fatal
        host_->UnloadFont(i->second.c_str());
      }
    }

    CloseDetailsView();
    delete main_view_host_;
    delete element_factory_;
    if (extension_manager_)
      extension_manager_->Destroy();
  }

  void DebugError(const char *message) {
    host_->DebugOutput(GadgetHostInterface::DEBUG_ERROR, message);
  }

  void DebugTrace(const char *message) {
    host_->DebugOutput(GadgetHostInterface::DEBUG_TRACE, message);
  }

  void DebugWarning(const char *message) {
    host_->DebugOutput(GadgetHostInterface::DEBUG_WARNING, message);
  }

  std::string ExtractFile(const char *filename) {
    FileManagerInterface *file_manager = host_->GetFileManager();
    ASSERT(file_manager);
    std::string extracted_file;
    return file_manager->ExtractFile(filename, &extracted_file) ?
           extracted_file : "";
  }

  std::string OpenTextFile(const char *filename) {
    FileManagerInterface *file_manager = host_->GetFileManager();
    ASSERT(file_manager);
    std::string data;
    std::string real_path;
    return file_manager->GetFileContents(filename, &data, &real_path) ?
           data : "";
  }

  std::string GetManifestInfo(const char *key) {
    GadgetStringMap::const_iterator it = manifest_info_map_.find(key);
    if (it == manifest_info_map_.end())
      return NULL;
    return it->second;
  }

  bool HasOptionsDialog() {
    return has_options_xml_ || onshowoptionsdlg_signal_.HasActiveConnections();
  }

  bool ShowOptionsDialog() {
    ViewHostInterface *options_view_host = NULL;
    DisplayWindow *window = NULL;
    View *view = new View(&gadget_global_, element_factory_, debug_mode_);
    if (onshowoptionsdlg_signal_.HasActiveConnections()) {
      options_view_host = host_->NewViewHost(
          GadgetHostInterface::VIEW_OLD_OPTIONS, view);
      window = new DisplayWindow(view);
      Variant result = onshowoptionsdlg_signal_(window);
      if (result.type() == Variant::TYPE_BOOL && !VariantValue<bool>()(result))
        return false;
      if (!window->AdjustSize())
        return false;
    } else if (has_options_xml_) {
      options_view_host = host_->NewViewHost(GadgetHostInterface::VIEW_OPTIONS,
                                             view);
      if (!view->InitFromFile(kOptionsXML)) {
        LOG("Failed to setup the options view");
        delete options_view_host;
        return false;
      }
    } else {
      LOG("Failed to show options dialog because there is neither options.xml"
          "nor OnShowOptionsDlg handler");
      delete options_view_host;
      return false;
    }

    options_view_host->RunDialog();
    delete window;
    delete options_view_host;
    return true;
  }

  bool ShowDetailsView(DetailsView *details_view, const char *title, int flags,
                       Slot1<void, int> *feedback_handler) {
    details_view->Ref();
    CloseDetailsView();
    details_view_ = details_view;
    View *view = new View(&gadget_global_, element_factory_, debug_mode_);
    details_view_host_ = host_->NewViewHost(GadgetHostInterface::VIEW_DETAILS,
                                            view);
    ScriptContextInterface *script_context =
        details_view_host_->GetScriptContext();
    ScriptableOptions *scriptable_data = details_view->GetDetailsViewData();
    OptionsInterface *data = scriptable_data->GetOptions();
    // Set up the detailsViewData variable in the opened details view.
    script_context->AssignFromNative(NULL, "", "detailsViewData",
                                     Variant(scriptable_data));

    std::string xml_file;
    if (details_view->ContentIsHTML() || !details_view->ContentIsView()) {
      if (details_view->ContentIsHTML()) {
        xml_file = kHTMLDetailsView;
        details_view_host_->GetScriptContext()->AssignFromNative(
            NULL, "", "external", Variant(details_view->GetExternalObject()));
        data->PutValue("contentType", Variant("text/html"));
      } else {
        xml_file = kTextDetailsView;
        data->PutValue("contentType", Variant("text/plain"));
      }
      data->PutValue("content", Variant(details_view->GetText()));
    } else {
      xml_file = details_view->GetText();
    }

    if (!view->InitFromFile(xml_file.c_str())) {
      LOG("Failed to load details view from %s", xml_file.c_str());
      delete details_view_host_;
      details_view_host_ = NULL;
      details_view->Unref();
      return false;
    }
    details_view_host_->ShowInDetailsView(title, flags, feedback_handler);
    return true;
  }

  class ShowDetailsViewCallback : public WatchCallbackInterface {
   public:
    ShowDetailsViewCallback(Impl *impl, DetailsView *details_view,
                            const char *title, int flags,
                            Slot1<void, int> *callback)
        : impl_(impl), details_view_(details_view),
          title_(title), flags_(flags), callback_(callback) {
      details_view_->Ref();
    }
    virtual bool Call(MainLoopInterface *main_loop, int watch_id) {
      impl_->ShowDetailsView(details_view_, title_.c_str(), flags_, callback_);
      impl_->show_details_view_timer_ = 0;
      callback_ = NULL;
      return false;
    }
    virtual void OnRemove(MainLoopInterface *main_loop, int watch_id) {
      details_view_->Unref();
      delete callback_;
    }
   private:
    Impl *impl_;
    DetailsView *details_view_;
    std::string title_;
    int flags_;
    Slot1<void, int> *callback_;
  };

  // Show the details view in the next event loop.
  void DelayedShowDetailsView(DetailsView *details_view,
                              const char *title, int flags,
                              Slot *callback) {
    if (show_details_view_timer_ == 0) {
      show_details_view_timer_ = host_->GetMainLoop()->AddTimeoutWatch(0,
          new ShowDetailsViewCallback(
              this, details_view, title, flags,
              callback ? new SlotProxy1<void, int>(callback) : NULL));
    }
  }

  void CloseDetailsView() {
    if (details_view_host_) {
      details_view_host_->CloseDetailsView();
      delete details_view_host_;
      details_view_host_ = NULL;
      details_view_->Unref();
    }
  }

  bool CloseDetailsViewCallback(int id) {
    ASSERT(id == close_details_view_timer_);
    CloseDetailsView();
    close_details_view_timer_ = 0;
    return false;
  };

  // Close the details view in the next event loop.
  void DelayedCloseDetailsView() {
    if (close_details_view_timer_ == 0) {
      close_details_view_timer_ = host_->GetMainLoop()->AddTimeoutWatch(0,
        new WatchCallbackSlot(NewSlot(this, &Impl::CloseDetailsViewCallback)));
    }
  }

  bool Init() {
    FileManagerInterface *file_manager = host_->GetFileManager();
    ASSERT(file_manager);

    const GadgetStringMap *strings = file_manager->GetStringTable();
    RegisterStrings(strings, &gadget_global_);
    RegisterStrings(strings, &strings_);

    std::string manifest_contents;
    std::string manifest_path;
    if (!file_manager->GetXMLFileContents(kGadgetGManifest,
                                          &manifest_contents,
                                          &manifest_path))
      return false;
    if (!host_->GetXMLParser()->ParseXMLIntoXPathMap(manifest_contents,
                                                     manifest_path.c_str(),
                                                     kGadgetTag, NULL,
                                                     &manifest_info_map_)) {
      // For compatibility with some Windows gadget files that use ISO8859-1
      // encoding without declaration.
      if (!host_->GetXMLParser()->ParseXMLIntoXPathMap(manifest_contents,
                                                       manifest_path.c_str(),
                                                       kGadgetTag, "ISO8859-1",
                                                       &manifest_info_map_))
        return false;
    }

    // TODO: Is it necessary to check the required fields in manifest?
    DLOG("Gadget min version: %s",
         GetManifestInfo(kManifestMinVersion).c_str());
    DLOG("Gadget id: %s", GetManifestInfo(kManifestId).c_str());
    DLOG("Gadget name: %s", GetManifestInfo(kManifestName).c_str());
    DLOG("Gadget description: %s",
         GetManifestInfo(kManifestDescription).c_str());

    // load fonts and objects
    for (GadgetStringMap::const_iterator i = manifest_info_map_.begin();
         i != manifest_info_map_.end(); ++i) {
      const std::string &key = i->first;
      // Keys are of the form "install/font@src" or "install/font[k]@src"
      if (0 == key.find(kManifestInstallFont) &&
          (key.length() - strlen(kSrcAttr)) == key.rfind(kSrcAttr)) {
        // ignore return, error not fatal
        host_->LoadFont(i->second.c_str());
      } else if (0 == key.find(kManifestInstallObject) &&
          (key.length() - strlen(kSrcAttr)) == key.rfind(kSrcAttr) &&
          extension_manager_) {
        std::string temp_file;
        file_manager->ExtractFile(i->second.c_str(), &temp_file);
        extension_manager_->LoadExtension(temp_file.c_str(), false);
      }
    }

    // Register extensions
    ScriptContextInterface *context = main_view_host_->GetScriptContext();
    const ExtensionManager *global_manager =
        ExtensionManager::GetGlobalExtensionManager();
    if (global_manager)
      global_manager->RegisterLoadedExtensions(element_factory_, context);
    if (extension_manager_)
      extension_manager_->RegisterLoadedExtensions(element_factory_, context);

    main_view_host_->GetView()->SetCaption(
        GetManifestInfo(kManifestName).c_str());

    if (!main_view_host_->GetView()->InitFromFile(kMainXML)) {
      LOG("Failed to setup the main view");
      return false;
    }

    has_options_xml_ = file_manager->FileExists(kOptionsXML);
    return true;
  }

  void SetAboutText(const char *about_text) {
    manifest_info_map_[kManifestAboutText] = about_text;
  }

  Signal1<Variant, DisplayWindow *> onshowoptionsdlg_signal_;
  GadgetHostInterface *host_;
  Debug debug_;
  Storage storage_;
  Strings strings_;
  ScriptableOptions scriptable_options_;
  Plugin plugin_;
  GadgetGlobal gadget_global_;
  ElementFactory *element_factory_;
  ExtensionManager *extension_manager_;
  View *main_view_;
  ViewHostInterface *main_view_host_;
  DetailsView *details_view_;
  ViewHostInterface *details_view_host_;
  GadgetStringMap manifest_info_map_;
  bool has_options_xml_;
  int close_details_view_timer_, show_details_view_timer_;
  int debug_mode_;
};

Gadget::Gadget(GadgetHostInterface *host, int debug_mode)
    : impl_(new Impl(host, this, debug_mode)) {
}

Gadget::~Gadget() {
  delete impl_;
  impl_ = NULL;
}

bool Gadget::Init() {
  return impl_->Init();
}

ViewHostInterface *Gadget::GetMainViewHost() {
  return impl_->main_view_host_;
}

std::string Gadget::GetManifestInfo(const char *key) const {
  return impl_->GetManifestInfo(key);
}

bool Gadget::HasOptionsDialog() const {
  return impl_->HasOptionsDialog();
}

bool Gadget::ShowOptionsDialog() {
  return impl_->ShowOptionsDialog();
}

void Gadget::OnAddCustomMenuItems(MenuInterface *menu) {
  impl_->plugin_.OnAddCustomMenuItems(menu);
}

void Gadget::OnCommand(Command command) {
  impl_->plugin_.oncommand_signal_(command);
}

void Gadget::OnDisplayStateChange(DisplayState display_state) {
  impl_->plugin_.ondisplaystatechange_signal_(display_state);
}

void Gadget::OnDisplayTargetChange(DisplayTarget display_target) {
  impl_->plugin_.ondisplaytargetchange_signal_(display_target);
}

bool Gadget::ShowDetailsView(DetailsView *details_view,
                             const char *title, int flags,
                             Slot1<void, int> *feedback_handler) {
  return impl_->ShowDetailsView(details_view, title, flags, feedback_handler);
}

void Gadget::CloseDetailsView() {
  return impl_->CloseDetailsView();
}

} // namespace ggadget
