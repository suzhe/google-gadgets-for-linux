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

#include <vector>
#include <algorithm>
#include "elements.h"
#include "elements_impl.h"
#include "element_interface.h"
#include "element_factory_interface.h"
#include "common.h"

namespace ggadget {

namespace internal {

ElementsImpl::ElementsImpl(ElementFactoryInterface *factory,
                           ElementInterface *owner)
    : factory_(factory), owner_(owner) {
  ASSERT(factory);
}

ElementsImpl::~ElementsImpl() {
  for (std::vector<ElementInterface *>::iterator ite =
       children_.begin(); ite != children_.end(); ++ite)
    (*ite)->Release();
}

int ElementsImpl::GetCount() const {
  return children_.size();
}

ElementInterface *ElementsImpl::GetItem(int index) {
  if (index >= 0 && index < static_cast<int>(children_.size()))
    return children_[index];
  return NULL;
}

const ElementInterface *ElementsImpl::GetItem(int index) const {
  if (index >= 0 && index < static_cast<int>(children_.size()))
    return children_[index];
  return NULL;
}

ElementInterface *ElementsImpl::AppendElement(const char *tag_name,
                                              const char *name) {
  ElementInterface *e = factory_->CreateElement(tag_name, owner_, name);
  if (e == NULL)
    return NULL;
  children_.push_back(e);
  return e;
}

ElementInterface *ElementsImpl::InsertElement(
    const char *tag_name, const ElementInterface *before, const char *name) {
  ElementInterface *e = factory_->CreateElement(tag_name, owner_, name);
  if (e == NULL)
    return NULL;
  std::vector<ElementInterface *>::iterator ite = std::find(
      children_.begin(), children_.end(), before);
  children_.insert(ite, e);
  return e;
}

bool ElementsImpl::RemoveElement(ElementInterface *element) {
  std::vector<ElementInterface *>::iterator ite = std::find(
      children_.begin(), children_.end(), element);
  if (ite == children_.end())
    return false;
  (*ite)->Release();
  children_.erase(ite);
  return true;
}

} // namespace internal

Elements::Elements(ElementFactoryInterface *factory,
                   ElementInterface *owner)
    : impl_(new internal::ElementsImpl(factory, owner)) {
}

Elements::~Elements() {
  delete impl_;
}

int Elements::GetCount() const {
  ASSERT(impl_);
  return impl_->GetCount();
}

ElementInterface *Elements::GetItem(int index) {
  ASSERT(impl_);
  return impl_->GetItem(index);
}

const ElementInterface *Elements::GetItem(int index) const {
  ASSERT(impl_);
  return impl_->GetItem(index);
}

ElementInterface *Elements::AppendElement(const char *tag_name,
                                          const char *name) {
  ASSERT(impl_);
  return impl_->AppendElement(tag_name, name);
}

ElementInterface *Elements::InsertElement(const char *tag_name,
                                          const ElementInterface *before,
                                          const char *name) {
  ASSERT(impl_);
  return impl_->InsertElement(tag_name, before, name);
}

bool Elements::RemoveElement(ElementInterface *element) {
  ASSERT(impl_);
  return impl_->RemoveElement(element);
}

} // namespace ggadget
