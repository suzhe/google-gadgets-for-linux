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

#include "options_factory.h"
#include "module.h"
#include "extension_manager.h"

namespace ggadget {

static const char kOptionsExtensionSymbolName[] = "CreateOptions";

class OptionsFactory::Impl : public ExtensionRegisterInterface {
 public:
  Impl() : func_(NULL) {
  }

  virtual bool RegisterExtension(const Module *extension) {
    ASSERT(extension);
    CreateOptionsFunc func =
        reinterpret_cast<CreateOptionsFunc>(
            extension->GetSymbol(kOptionsExtensionSymbolName));
    if (func)
      func_ = func;

    return func != NULL;
  }

  OptionsInterface *CreateOptions(const char *config_file_path) {
    if (!func_) {
      const ExtensionManager *manager =
          ExtensionManager::GetGlobalExtensionManager();
      ASSERT(manager);
      if (manager)
        manager->RegisterLoadedExtensions(this);
    }

    ASSERT(func_);
    if (func_)
      return func_(config_file_path);
    return NULL;
  }

 private:
  typedef OptionsInterface *(*CreateOptionsFunc)(const char *);
  CreateOptionsFunc func_;

 public:
  static OptionsFactory *factory_;
};

OptionsFactory *OptionsFactory::Impl::factory_ = NULL;

OptionsFactory::OptionsFactory()
  : impl_(new Impl()) {
}

OptionsFactory::~OptionsFactory() {
  DLOG("OptionsFactory singleton is destroyed, but it shouldn't.");
  ASSERT(Impl::factory_ == this);
  Impl::factory_ = NULL;
  delete impl_;
  impl_ = NULL;
}

OptionsInterface *OptionsFactory::CreateOptions(const char *config_file_path) {
  return impl_->CreateOptions(config_file_path);
}

OptionsFactory *OptionsFactory::get() {
  if (!Impl::factory_)
    Impl::factory_ = new OptionsFactory();
  return Impl::factory_;
}

} // namespace ggadget
