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
#include "file_manager_interface.h"
#include "gadget_consts.h"
#include "gadget_host_interface.h"
#include "options_interface.h"
#include "scriptable_helper.h"
#include "scriptable_options.h"
#include "view_host_interface.h"
#include "view_interface.h"
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

  class GadgetGlobalPrototype : public ScriptableHelper<ScriptableInterface> {
   public:
    DEFINE_CLASS_ID(0x2c8d4292025f4397, ScriptableInterface);
    GadgetGlobalPrototype(Gadget::Impl *owner) {
      RegisterConstant("gadget", owner);
      RegisterConstant("options", &owner->scriptable_options_);
      RegisterConstant("strings", &owner->strings_);
      // TODO: SetPrototype(The System global prototype).
      // The System global prototype provides global constants and framework.
    }
    virtual OwnershipPolicy Attach() { return NATIVE_PERMANENT; }
  };

  Impl(GadgetHostInterface *host, OptionsInterface *options, Gadget *owner)
      : host_(host),
        debug_(this),
        storage_(this),
        gadget_global_prototype_(this),
        options_(options),
        scriptable_options_(options),
        file_manager_(NULL),
        main_view_host_(host->NewViewHost(GadgetHostInterface::VIEW_MAIN,
                                          &gadget_global_prototype_,
                                          options)) {
    RegisterConstant("debug", &debug_);
    RegisterConstant("storage", &storage_);
  }

  ~Impl() {
    delete main_view_host_;
    main_view_host_ = NULL;
    delete file_manager_;
    file_manager_ = NULL;
    delete options_;
    options_ = NULL;
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
    ASSERT(file_manager_);
    std::string extracted_file;
    return file_manager_->ExtractFile(filename, &extracted_file) ?
           extracted_file : "";
  }

  std::string OpenTextFile(const char *filename) {
    ASSERT(file_manager_);
    std::string data;
    std::string real_path;
    return file_manager_->GetFileContents(filename, &data, &real_path) ?
           data : "";
  }

  const char *GetManifestInfo(const char *key) {
    GadgetStringMap::const_iterator it = manifest_info_map_.find(key);
    if (it == manifest_info_map_.end())
      return NULL;
    return it->second.c_str();
  }

  bool Init(FileManagerInterface *file_manager) {
    ASSERT(file_manager);
    file_manager_ = file_manager;

    GadgetStringMap *strings = file_manager_->GetStringTable();
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
                              kGadgetTag,
                              &manifest_info_map_))
      return false;
    // TODO: Is it necessary to check the required fields in manifest?

    if (!main_view_host_->GetView()->InitFromFile(file_manager, kMainXML)) {
      DLOG("Failed to setup the main view");
      return false;
    }

    // TODO: SetupView(options_, kOptionsXML); // Ignore any error.

    // Start running the main view.
    return true;
  }

  GadgetHostInterface *host_;
  Debug debug_;
  Storage storage_;
  Strings strings_;
  GadgetGlobalPrototype gadget_global_prototype_;
  OptionsInterface *options_;
  ScriptableOptions scriptable_options_;
  FileManagerInterface *file_manager_;
  ViewHostInterface *main_view_host_;
  GadgetStringMap manifest_info_map_;
};

Gadget::Gadget(GadgetHostInterface *host, OptionsInterface *options)
    : impl_(new Impl(host, options, this)) {
}

Gadget::~Gadget() {
  delete impl_;
  impl_ = NULL;
}

bool Gadget::Init(FileManagerInterface *file_manager) {
  return impl_->Init(file_manager);
}

ViewHostInterface *Gadget::GetMainViewHost() {
  return impl_->main_view_host_;
}

const char *Gadget::GetManifestInfo(const char *key) {
  return impl_->GetManifestInfo(key);
}

} // namespace ggadget
