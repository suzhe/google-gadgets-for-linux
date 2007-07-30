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

#include "element_factory.h"
#include "element_factory_impl.h"
#include "common.h"

namespace ggadget {

scoped_ptr<ElementFactory> ElementFactory::instance_;

ElementFactory *ElementFactory::GetInstance(void) {
  ElementFactory *ptr;
  ptr = instance_.get();
  if (!ptr) {
    ptr = new ElementFactory();
    instance_.reset(ptr);
  }
  return ptr;
}

ElementFactory::ElementFactory()
    : impl_(new internal::ElementFactoryImpl) {
}

ElementFactory::~ElementFactory() {
  delete impl_;
}

bool ElementFactory::Init() {
  ASSERT(impl_);
  return impl_->Init();
}

ElementInterface *ElementFactory::CreateElement(const char *type,
                                                ElementInterface *parent) {
  ASSERT(impl_);
  return impl_->CreateElement(type, parent);
}

bool ElementFactory::RegisterElementClass(
    const char *type, ElementInterface *(*creator)(ElementInterface *)) {
  ASSERT(impl_);
  return impl_->RegisterElementClass(type, creator);
}

namespace internal {

bool ElementFactoryImpl::Init() {
  ASSERT(creators_.size() == 0);
  return true;
}

ElementInterface *ElementFactoryImpl::CreateElement(
    const char *type, ElementInterface *parent) {
  std::map<std::string,
      ElementInterface *(*)(ElementInterface *)>::iterator ite =
      creators_.find(type);
  if (ite == creators_.end())
    return NULL;
  return ite->second(parent);
}

bool ElementFactoryImpl::RegisterElementClass(
    const char *type, ElementInterface *(*creator)(ElementInterface *)) {
  std::map<std::string,
      ElementInterface *(*)(ElementInterface *)>::iterator ite =
      creators_.find(type);
  if (ite != creators_.end())
    return false;
  creators_[type] = creator;
  return true;
}

} // namespace internal

} // namespace ggadget
