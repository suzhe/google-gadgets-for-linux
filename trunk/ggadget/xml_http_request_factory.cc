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

#include "xml_http_request_factory.h"
#include "module.h"
#include "extension_manager.h"

namespace ggadget {

static const char kXMLHttpRequestExtensionSymbolName[] = "CreateXMLHttpRequest";

class XMLHttpRequestFactory::Impl : public ExtensionRegisterInterface {
 public:
  Impl() : func_(NULL) {
  }

  virtual bool RegisterExtension(const Module *extension) {
    ASSERT(extension);
    CreateXMLHttpRequestFunc func =
        reinterpret_cast<CreateXMLHttpRequestFunc>(
            extension->GetSymbol(kXMLHttpRequestExtensionSymbolName));
    if (func)
      func_ = func;

    return func != NULL;
  }

  XMLHttpRequestInterface *CreateXMLHttpRequest(XMLParserInterface *parser) {
    if (!func_) {
      const ExtensionManager *manager =
          ExtensionManager::GetGlobalExtensionManager();

      ASSERT(manager);

      if (manager)
        manager->RegisterLoadedExtensions(this);
    }

    ASSERT(func_);

    if (func_)
      return func_(parser);

    return NULL;
  }

 private:
  typedef XMLHttpRequestInterface *
      (*CreateXMLHttpRequestFunc)(XMLParserInterface *);

  CreateXMLHttpRequestFunc func_;

 public:
  static XMLHttpRequestFactory *factory_;
};

XMLHttpRequestFactory *XMLHttpRequestFactory::Impl::factory_ = NULL;

XMLHttpRequestFactory::XMLHttpRequestFactory()
  : impl_(new Impl()) {
}

XMLHttpRequestFactory::~XMLHttpRequestFactory() {
  DLOG("XMLHttpRequestFactory singleton is destroyed, but it shouldn't.");
  ASSERT(Impl::factory_ == this);
  Impl::factory_ = NULL;
  delete impl_;
  impl_ = NULL;
}

XMLHttpRequestInterface *
XMLHttpRequestFactory::CreateXMLHttpRequest(XMLParserInterface *parser) {
  return impl_->CreateXMLHttpRequest(parser);
}

XMLHttpRequestFactory *XMLHttpRequestFactory::get() {
  if (!Impl::factory_)
    Impl::factory_ = new XMLHttpRequestFactory();
  return Impl::factory_;
}

} // namespace ggadget
