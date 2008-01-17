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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <algorithm>
#include <map>
#include "logger.h"
#include "module.h"
#include "common.h"
#include "extension_manager.h"
#include "main_loop_interface.h"

namespace ggadget {

static const char* kRegisterExtensionFuncName = "RegisterExtension";

class ExtensionManager::Impl {
  typedef bool (*RegisterExtensionFunc)(ElementFactory *factory,
                                        ScriptContextInterface *context);

  class Extension : public Module {
   public:
    Extension(MainLoopInterface *main_loop, const char *name, bool resident)
        : Module(main_loop, name) {
      if (IsValid()) {
        register_func_ = reinterpret_cast<RegisterExtensionFunc>(
            GetSymbol(kRegisterExtensionFuncName));
        if (!register_func_) {
          DLOG("Failed to load extension %s, symbol %s() can't be resolved.",
               name, kRegisterExtensionFuncName);
          Unload();
        } else if (resident) {
          if (!MakeResident())
            DLOG("Failed to make extension %s resident.", name);
        }
      }
    }

    bool RegisterExtension(ElementFactory *factory,
                           ScriptContextInterface *context) {
      if (register_func_)
        return register_func_(factory, context);
      return false;
    }

   private:
    RegisterExtensionFunc register_func_;

    DISALLOW_EVIL_CONSTRUCTORS(Extension);
  };

 public:
  Impl() : main_loop_(NULL), readonly_(false) {
  }

  ~Impl() {
    for (ExtensionMap::iterator it= extensions_.begin();
         it != extensions_.end(); ++it) {
      delete it->second;
    }
  }

  void SetMainLoop(MainLoopInterface *main_loop) {
    main_loop_ = main_loop;
  }

  Extension *LoadExtension(const char *name, bool resident) {
    ASSERT(name && *name);
    if (readonly_) {
      LOG("Can't load extension %s, into a readonly ExtensionManager.",
          name);
      return NULL;
    }

    if (name && *name) {
      std::string name_str(name);

      // If the module has already been loaded, then just return true.
      ExtensionMap::iterator it = extensions_.find(name_str);
      if (it != extensions_.end()) {
        if (!it->second->IsResident() && resident)
          it->second->MakeResident();
        return it->second;
      }

      Extension *extension = new Extension(main_loop_, name, resident);
      if (!extension->IsValid()) {
        delete extension;
        return NULL;
      }

      extensions_[name_str] = extension;
      return extension;
    }
    return NULL;
  }

  bool UnloadExtension(const char *name) {
    ASSERT(name && *name);
    if (readonly_) {
      LOG("Can't unload extension %s, from a readonly ExtensionManager.", name);
      return false;
    }

    if (name && *name) {
      std::string name_str(name);

      ExtensionMap::iterator it = extensions_.find(name_str);
      if (it != extensions_.end()) {
        if (it->second->IsResident()) {
          LOG("Can't unload extension %s, it's resident.", name);
          return false;
        }
        delete it->second;
        extensions_.erase(it);
        return true;
      }
    }
    return false;
  }

  bool EnumerateLoadedExtensions(
      Slot2<bool, const char *, const char *> *callback) {
    ASSERT(callback);

    bool result = false;
    for (ExtensionMap::const_iterator it = extensions_.begin();
         it != extensions_.end(); ++it) {
      result = (*callback)(it->first.c_str(), it->second->GetName().c_str());
      if (!result) break;
    }

    delete callback;
    return result;
  }

  bool RegisterExtension(const char *name, ElementFactory *factory,
                         ScriptContextInterface *context) {
    Extension *extension = LoadExtension(name, false);
    if (extension && extension->IsValid()) {
      return extension->RegisterExtension(factory, context);
    }
    return false;
  }

  bool RegisterLoadedExtensions(ElementFactory *factory,
                                ScriptContextInterface *context) {
    if (extensions_.size()) {
      bool ret = true;
      for (ExtensionMap::const_iterator it = extensions_.begin();
           it != extensions_.end(); ++it) {
        if (!it->second->RegisterExtension(factory, context))
          ret = false;
      }
      return ret;
    }
    return false;
  }

  void MarkAsGlobal() {
    // Make all loaded extensions resident.
    for (ExtensionMap::iterator it = extensions_.begin();
         it != extensions_.end(); ++it) {
      it->second->MakeResident();
    }
    readonly_ = true;
  }

 public:
  static bool SetGlobalExtensionManager(ExtensionManager *manager) {
    if (!global_manager_ && manager) {
      global_manager_ = manager;
      global_manager_->impl_->MarkAsGlobal();
      return true;
    }
    return false;
  }

  static ExtensionManager *GetGlobalExtensionManager() {
    return global_manager_;
  }

  static ExtensionManager *global_manager_;

 private:
  typedef std::map<std::string, Extension *> ExtensionMap;

  MainLoopInterface *main_loop_;
  ExtensionMap extensions_;
  bool readonly_;
};

ExtensionManager* ExtensionManager::Impl::global_manager_ = NULL;

ExtensionManager::ExtensionManager()
  : impl_(new Impl()) {
}

ExtensionManager::~ExtensionManager() {
  delete impl_;
}

bool ExtensionManager::Destroy() {
  if (this && this != Impl::global_manager_) {
    delete this;
    return true;
  }

  DLOG("Try to destroy %s ExtensionManager object.",
       (this == NULL ? "an invalid" : "the global"));

  return false;
}

bool ExtensionManager::LoadExtension(const char *name, bool resident) {
  return impl_->LoadExtension(name, resident) != NULL;
}

bool ExtensionManager::UnloadExtension(const char *name) {
  return impl_->UnloadExtension(name);
}

bool ExtensionManager::EnumerateLoadedExtensions(
    Slot2<bool, const char *, const char *> *callback) const {
  return impl_->EnumerateLoadedExtensions(callback);
}

bool ExtensionManager::RegisterExtension(const char *name,
                                         ElementFactory *factory,
                                         ScriptContextInterface *context) const {
  return impl_->RegisterExtension(name, factory, context);
}

bool ExtensionManager::RegisterLoadedExtensions(
    ElementFactory *factory, ScriptContextInterface *context) const {
  return impl_->RegisterLoadedExtensions(factory, context);
}

const ExtensionManager *ExtensionManager::GetGlobalExtensionManager() {
  return Impl::GetGlobalExtensionManager();
}

bool ExtensionManager::SetGlobalExtensionManager(ExtensionManager *manager) {
  return Impl::SetGlobalExtensionManager(manager);
}

ExtensionManager *
ExtensionManager::CreateExtensionManager(MainLoopInterface *main_loop) {
  ExtensionManager *manager = new ExtensionManager();
  manager->impl_->SetMainLoop(main_loop);
  return manager;
}

} // namespace ggadget
