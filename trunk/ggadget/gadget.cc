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
#include "file_manager.h"
#include "gadget_consts.h"
#include "string_utils.h"
#include "scriptable_helper.h"
#include "script_context_interface.h"
#include "script_runtime_interface.h"
#include "view.h"
#include "xml_utils.h"

namespace ggadget {

class Gadget::Impl : public ScriptableInterface {
 public:
  DEFINE_CLASS_ID(0x6a3c396b3a544148, ScriptableInterface);

  class Debug : public ScriptableInterface {
   public:
    DEFINE_CLASS_ID(0xa9b59e70c74649da, ScriptableInterface);
    Debug(Gadget::Impl *owner) {
      scriptable_helper_.RegisterMethod("error",
                                        NewSlot(owner, &Impl::DebugError));
      scriptable_helper_.RegisterMethod("trace",
                                        NewSlot(owner, &Impl::DebugTrace));
      scriptable_helper_.RegisterMethod("warning",
                                        NewSlot(owner, &Impl::DebugWarning));
    }
    DEFAULT_OWNERSHIP_POLICY
    DELEGATE_SCRIPTABLE_INTERFACE(scriptable_helper_)
    virtual bool IsStrict() const { return true; }
    ScriptableHelper scriptable_helper_;
  };

  class Storage : public ScriptableInterface {
   public:
    DEFINE_CLASS_ID(0xd48715e0098f43d1, ScriptableInterface);
    Storage(Gadget::Impl *owner) {
      scriptable_helper_.RegisterMethod("extract",
                                        NewSlot(owner, &Impl::ExtractFile));
      scriptable_helper_.RegisterMethod("openText",
                                        NewSlot(owner, &Impl::OpenTextFile));
    }
    DEFAULT_OWNERSHIP_POLICY
    DELEGATE_SCRIPTABLE_INTERFACE(scriptable_helper_)
    virtual bool IsStrict() const { return true; }
    ScriptableHelper scriptable_helper_;
  };

  class GadgetGlobalPrototype : public ScriptableInterface {
   public:
    DEFINE_CLASS_ID(0x2c8d4292025f4397, ScriptableInterface);
    GadgetGlobalPrototype(Gadget::Impl *owner) {
      scriptable_helper_.RegisterConstant("gadget", owner);
      // TODO: options
      // TODO: scriptable_helper_.SetPrototype(The System global prototype).
      // The System global prototype provides global constants and framework.
    }
    DEFAULT_OWNERSHIP_POLICY
    DELEGATE_SCRIPTABLE_INTERFACE(scriptable_helper_)
    virtual bool IsStrict() const { return true; }
    ScriptableHelper scriptable_helper_;
  };

  Impl(ScriptRuntimeInterface *script_runtime,
       ElementFactoryInterface *element_factory,
       Gadget *owner)
      : debug_(this),
        storage_(this),
        gadget_global_prototype_(this),
        script_runtime_(script_runtime),
        file_manager_(new FileManager()),
        main_context_(script_runtime->CreateContext()),
        main_(new View(main_context_, owner, &gadget_global_prototype_,
                       element_factory)),
        options_context_(script_runtime->CreateContext()),
        options_(new View(options_context_, owner, &gadget_global_prototype_,
                          element_factory)) {
    script_runtime_->ConnectErrorReporter(NewSlot(this,
                                                  &Impl::ReportScriptError));
    scriptable_helper_.RegisterConstant("debug", &debug_);
    scriptable_helper_.RegisterConstant("storage", &storage_);
  }

  ~Impl() {
    delete main_;
    main_ = NULL;
    delete options_;
    options_ = NULL;
    delete file_manager_;
    file_manager_ = NULL;
    main_context_->Destroy();
    main_context_ = NULL;
    options_context_->Destroy();
    options_context_ = NULL;
  }

  void ReportScriptError(const char *message) {
    LOG("Script ERROR: %s", message);
  }

  void DebugError(const char *message) {
    LOG("ERROR: %s", message);
  }

  void DebugTrace(const char *message) {
    LOG("TRACE: %s", message);
  }

  void DebugWarning(const char *warning) {
    LOG("WARNING: %s", warning);
  }

  std::string ExtractFile(const char *filename) {
    std::string extracted_file;
    return file_manager_->ExtractFile(filename, &extracted_file) ?
           extracted_file : "";
  }

  std::string OpenTextFile(const char *filename) {
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

  bool InitFromPath(const char *base_path) {
    if (!file_manager_->Init(base_path))
      return false;

    std::string manifest_contents;
    std::string manifest_path;
    if (!file_manager_->GetXMLFileContents(kGadgetGManifest,
                                           &manifest_contents,
                                           &manifest_path))
      return false;
    if (!ParseXMLIntoXPathMap(manifest_contents.c_str(),
                              manifest_path.c_str(),
                              kGadgetTag,
                              &manifest_info_map_))
      return false;
    // TODO: Is it necessary to check the required fields in manifest?

    if (!main_->InitFromFile(kMainXML)) {
      DLOG("Failed to setup the main view");
      return false;
    }

    // TODO: SetupView(options_, kOptionsXML); // Ignore any error.

    // Start running the main view.
    return true;
  }

  DEFAULT_OWNERSHIP_POLICY
  DELEGATE_SCRIPTABLE_INTERFACE(scriptable_helper_)
  virtual bool IsStrict() const { return true; }
  ScriptableHelper scriptable_helper_;
  Debug debug_;
  Storage storage_;
  GadgetGlobalPrototype gadget_global_prototype_;
  ScriptRuntimeInterface *script_runtime_;
  FileManagerInterface *file_manager_;
  ScriptContextInterface *main_context_;
  ViewInterface *main_;
  ScriptContextInterface *options_context_;
  ViewInterface *options_;
  GadgetStringMap manifest_info_map_;
};

Gadget::Gadget(ScriptRuntimeInterface *script_runtime,
               ElementFactoryInterface *element_factory)
    : impl_(new Impl(script_runtime, element_factory, this)) {
}

Gadget::~Gadget() {
  delete impl_;
  impl_ = NULL;
}

ViewInterface* Gadget::GetMainView() {
  return impl_->main_;
}

ViewInterface* Gadget::GetOptionsView() {
  return impl_->options_;
}

FileManagerInterface *Gadget::GetFileManager() {
  return impl_->file_manager_;
}

bool Gadget::InitFromPath(const char *base_path) {
  return impl_->InitFromPath(base_path);
}

const char *Gadget::GetManifestInfo(const char *key) {
  return impl_->GetManifestInfo(key);
}

void Gadget::DebugError(const char *message) {
  return impl_->DebugError(message);
}

void Gadget::DebugTrace(const char *message) {
  return impl_->DebugTrace(message);
}

void Gadget::DebugWarning(const char *message) {
  return impl_->DebugWarning(message);
}

} // namespace ggadget
