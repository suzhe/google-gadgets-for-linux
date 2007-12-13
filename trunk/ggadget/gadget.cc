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
#include "file_manager_interface.h"
#include "gadget_consts.h"
#include "gadget_host_interface.h"
#include "menu_interface.h"
#include "script_context_interface.h"
#include "scriptable_array.h"
#include "scriptable_framework.h"
#include "scriptable_helper.h"
#include "scriptable_menu.h"
#include "scriptable_options.h"
#include "view_host_interface.h"
#include "view.h"
#include "xml_utils.h"

namespace ggadget {

class Gadget::Impl : public ScriptableHelper<ScriptableInterface> {
 public:
  DEFINE_CLASS_ID(0x6a3c396b3a544148, ScriptableInterface);

  class Debug : public ScriptableHelper<ScriptableInterface> {
   public:
    DEFINE_CLASS_ID(0xa9b59e70c74649da, ScriptableInterface);
    Debug(Gadget::Impl *owner) {
      RegisterMethod("error", NewSlot(owner, &Impl::DebugError));
      RegisterMethod("trace", NewSlot(owner, &Impl::DebugTrace));
      RegisterMethod("warning", NewSlot(owner, &Impl::DebugWarning));
    }
    virtual OwnershipPolicy Attach() { return NATIVE_PERMANENT; }
  };

  class Storage : public ScriptableHelper<ScriptableInterface> {
   public:
    DEFINE_CLASS_ID(0xd48715e0098f43d1, ScriptableInterface);
    Storage(Gadget::Impl *owner) {
      RegisterMethod("extract", NewSlot(owner, &Impl::ExtractFile));
      RegisterMethod("openText", NewSlot(owner, &Impl::OpenTextFile));
    }
    virtual OwnershipPolicy Attach() { return NATIVE_PERMANENT; }
  };

  static void RegisterStrings(
      GadgetStringMap *strings,
      ScriptableHelper<ScriptableInterface> *scriptable) {
    for (GadgetStringMap::const_iterator it = strings->begin();
         it != strings->end(); ++it) {
      scriptable->RegisterConstant(it->first.c_str(), it->second);
    }
  }

  class Strings : public ScriptableHelper<ScriptableInterface> {
   public:
    DEFINE_CLASS_ID(0x13679b3ef9a5490e, ScriptableInterface);
    virtual OwnershipPolicy Attach() { return NATIVE_PERMANENT; }
  };

  class Plugin : public ScriptableHelper<ScriptableInterface> {
   public:
    DEFINE_CLASS_ID(0x05c3f291057c4c9c, ScriptableInterface);
    Plugin(Impl *gadget_impl) :
        gadget_host_(gadget_impl->host_),
        main_view_(down_cast<View *>(gadget_impl->main_view_host_->GetView())) {
      RegisterProperty("plugin_flags", NULL, // No getter.
                       NewSlot(gadget_host_,
                               &GadgetHostInterface::SetPluginFlags));
      RegisterMethod("RemoveMe",
                     NewSlot(gadget_host_, &GadgetHostInterface::RemoveMe));
      RegisterMethod("ShowDetailsView",
                     NewSlot(gadget_impl, &Impl::ScriptShowDetailsView));
      RegisterMethod("CloseDetailsView",
                     NewSlot(gadget_impl, &Impl::CloseDetailsView));
      RegisterMethod("ShowOptionsDialog",
                     NewSlot(gadget_impl, &Impl::ShowOptionsDialog));
      RegisterSignal("onShowOptionsDlg",
                     &gadget_impl->onshowoptionsdlg_signal_);
      RegisterSignal("onAddCustomMenuItems", &onaddcustommenuitems_signal_);
      RegisterSignal("onCommand", &oncommand_signal_);
      RegisterSignal("onDisplayStateChange", &ondisplaystatechange_signal_);
      RegisterSignal("onDisplayTargetChange", &ondisplaytargetchange_signal_);

      // Deprecated or unofficial properties and methods.
      RegisterProperty("title", NULL, // No getter.
                       NewSlot(main_view_, &View::SetCaption));
      RegisterProperty("about_text", NULL, // No getter.
                       NewSlot(gadget_impl, &Impl::SetAboutText));
      RegisterProperty("window_width",
                       NewSlot(main_view_, &View::GetWidth), NULL);
      RegisterProperty("window_height",
                       NewSlot(main_view_, &View::GetHeight), NULL);
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

    virtual OwnershipPolicy Attach() { return NATIVE_PERMANENT; }

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
      LOG("pluginHelper.content_items is no longer supported. "
          "Please use the methods of the contentarea element instead.");
      return NULL;
    }
    void SetContentItems(ScriptableArray *array) { GetContentItems(); }

    ScriptableArray *GetPinImages() {
      LOG("pluginHelper.pin_images is no longer supported. "
          "Please use the methods of contentarea element instead.");
      return NULL;
    }
    void SetPinImages(ScriptableArray *array) { GetPinImages(); }

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

  class GadgetGlobalPrototype : public ScriptableHelper<ScriptableInterface> {
   public:
    DEFINE_CLASS_ID(0x2c8d4292025f4397, ScriptableInterface);
    GadgetGlobalPrototype(Gadget::Impl *owner)
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
      SetPrototype(&framework_);
    }
    virtual OwnershipPolicy Attach() { return NATIVE_PERMANENT; }

    ScriptableFramework framework_; 
  };

  Impl(GadgetHostInterface *host, Gadget *owner)
      : host_(host),
        debug_(this),
        storage_(this),
        scriptable_options_(host->GetOptions()),
        gadget_global_prototype_(this),
        main_view_host_(host->NewViewHost(GadgetHostInterface::VIEW_MAIN,
                                          &gadget_global_prototype_)),
        plugin_(this),
        details_view_host_(NULL),
        has_options_xml_(false) {
    RegisterConstant("debug", &debug_);
    RegisterConstant("storage", &storage_);
  }

  ~Impl() {
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
    main_view_host_ = NULL;
  }

  virtual OwnershipPolicy Attach() { return NATIVE_PERMANENT; }

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
    if (has_options_xml_) {
      options_view_host = host_->NewViewHost(
          GadgetHostInterface::VIEW_OPTIONS, &gadget_global_prototype_);
      if (!options_view_host->GetView()->InitFromFile(kOptionsXML)) {
        LOG("Failed to setup the options view");
        delete options_view_host;
        return false;
      }
    } else if (onshowoptionsdlg_signal_.HasActiveConnections()) {
      options_view_host = host_->NewViewHost(
          GadgetHostInterface::VIEW_OLD_OPTIONS, NULL);
      window = new DisplayWindow(options_view_host->GetView());
      onshowoptionsdlg_signal_(window);
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
    CloseDetailsView();
    if (!details_view->ContentIsView()) {
      LOG("HTML/Text view is not supported for now");
      return false;
    }

    details_view_host_ = host_->NewViewHost(GadgetHostInterface::VIEW_DETAILS,
                                            &gadget_global_prototype_);
    // Set up the detailsViewData variable in the opened details view.
    details_view_host_->GetScriptContext()->AssignFromContext(
        NULL, "", "detailsViewData",
        main_view_host_->GetScriptContext(), details_view, "detailsViewData");
    std::string xml_file = details_view->GetText();
    if (!details_view_host_->GetView()->InitFromFile(xml_file.c_str())) {
      LOG("Failed to load details view from %s", xml_file.c_str());
      delete details_view_host_;
      details_view_host_ = NULL;
      return false;
    }
    details_view_host_->ShowInDetailsView(title, flags, feedback_handler);
    return true;
  }

  void ScriptShowDetailsView(DetailsView *details_view,
                             const char *title, int flags,
                             Slot *callback) {
    ShowDetailsView(details_view, title, flags,
                    callback ? new SlotProxy1<void, int>(callback) : NULL);
  }

  void CloseDetailsView() {
    if (details_view_host_) {
      details_view_host_->CloseDetailsView();
      delete details_view_host_;
      details_view_host_ = NULL;
    }
  }

  bool Init() {
    FileManagerInterface *file_manager = host_->GetFileManager();
    ASSERT(file_manager);

    GadgetStringMap *strings = file_manager->GetStringTable();
    RegisterStrings(strings, &gadget_global_prototype_);
    RegisterStrings(strings, &strings_);

    std::string manifest_contents;
    std::string manifest_path;
    if (!file_manager->GetXMLFileContents(kGadgetGManifest,
                                          &manifest_contents,
                                          &manifest_path))
      return false;
    if (!ParseXMLIntoXPathMap(manifest_contents.c_str(),
                              manifest_path.c_str(),
                              kGadgetTag, NULL,
                              &manifest_info_map_))
      return false;

    // TODO: Is it necessary to check the required fields in manifest?
    DLOG("Gadget min version: %s",
         GetManifestInfo(kManifestMinVersion).c_str());
    DLOG("Gadget id: %s", GetManifestInfo(kManifestId).c_str());
    DLOG("Gadget name: %s", GetManifestInfo(kManifestName).c_str());
    DLOG("Gadget description: %s",
         GetManifestInfo(kManifestDescription).c_str());

    // load fonts
    for (GadgetStringMap::const_iterator i = manifest_info_map_.begin();
         i != manifest_info_map_.end(); ++i) {
      const std::string &key = i->first;
      // Keys are of the form "install/font@src" or "install/font[k]@src"
      if (0 == key.find(kManifestInstallFont) &&
          (key.length() - strlen(kSrcAttr)) == key.rfind(kSrcAttr)) {
        // ignore return, error not fatal
        host_->LoadFont(i->second.c_str());
      }
    }

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

  Signal1<void, DisplayWindow *> onshowoptionsdlg_signal_;
  GadgetHostInterface *host_;
  Debug debug_;
  Storage storage_;
  Strings strings_;
  ScriptableOptions scriptable_options_;
  GadgetGlobalPrototype gadget_global_prototype_;
  ViewHostInterface *main_view_host_;
  Plugin plugin_;
  ViewHostInterface *details_view_host_;
  GadgetStringMap manifest_info_map_;
  bool has_options_xml_;
};

Gadget::Gadget(GadgetHostInterface *host)
    : impl_(new Impl(host, this)) {
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
