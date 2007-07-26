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
#include "element_interface.h"
#include "element_factory_interface.h"
#include "common.h"

namespace {

class ElementsImpl {
 public:
  ElementsImpl(ggadget::ElementFactoryInterface *factory,
               ggadget::ElementInterface *owner);
  ~ElementsImpl();
  bool Init();
  int GetCount() const;
  ggadget::ElementInterface *GetItem(int index);
  const ggadget::ElementInterface *GetItem(int index) const;
  ggadget::ElementInterface *AppendElement(const char *type);
  ggadget::ElementInterface *InsertElement(
      const char *type, const ggadget::ElementInterface *before);
  bool RemoveElement(const ggadget::ElementInterface *element);

  ggadget::ElementFactoryInterface *factory_;
  ggadget::ElementInterface *owner_;
  std::vector<ggadget::ElementInterface *> children_;
};

ElementsImpl::ElementsImpl(ggadget::ElementFactoryInterface *factory,
                           ggadget::ElementInterface *owner)
    : factory_(factory), owner_(owner) {
  ASSERT(factory);
}

ElementsImpl::~ElementsImpl() {
  for (std::vector<ggadget::ElementInterface *>::iterator ite =
       children_.begin(); ite != children_.end(); ++ite)
    (*ite)->Release();
}

bool ElementsImpl::Init() {
  return true;
}

int ElementsImpl::GetCount() const {
  return children_.size();
}

ggadget::ElementInterface *ElementsImpl::GetItem(int index) {
  if (index >= 0 && index < static_cast<int>(children_.size()))
    return children_[index];
  return NULL;
}

const ggadget::ElementInterface *ElementsImpl::GetItem(int index) const {
  if (index >= 0 && index < static_cast<int>(children_.size()))
    return children_[index];
  return NULL;
}

ggadget::ElementInterface *ElementsImpl::AppendElement(const char *type) {
  ggadget::ElementInterface *e = factory_->CreateElement(type, owner_);
  if (e == NULL)
    return NULL;
  children_.push_back(e);
  return e;
}

ggadget::ElementInterface *ElementsImpl::InsertElement(
    const char *type, const ggadget::ElementInterface *before) {
  ggadget::ElementInterface *e = factory_->CreateElement(type, owner_);
  if (e == NULL)
    return NULL;
  std::vector<ggadget::ElementInterface *>::iterator ite = std::find(
      children_.begin(), children_.end(), before);
  children_.insert(ite, e);
  return e;
}

bool ElementsImpl::RemoveElement(const ggadget::ElementInterface *element) {
  std::vector<ggadget::ElementInterface *>::iterator ite = std::find(
      children_.begin(), children_.end(), element);
  if (ite == children_.end())
    return false;
  (*ite)->Release();
  children_.erase(ite);
  return true;
}

} // namespace <anonymouse>

#ifndef UNIT_TEST

namespace ggadget {

Elements::Elements(ElementFactoryInterface *factory,
                   ElementInterface *owner) {
  impl_ = new ElementsImpl(factory, owner);
}

Elements::~Elements() {
  delete impl_;
}

bool Elements::Init() {
  ASSERT(impl_);
  return impl_ != NULL;
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

ElementInterface *Elements::AppendElement(const char *type) {
  ASSERT(impl_);
  return impl_->AppendElement(type);
}

ElementInterface *Elements::InsertElement(const char *type,
                                          const ElementInterface *before) {
  ASSERT(impl_);
  return impl_->InsertElement(type, before);
}

bool Elements::RemoveElement(const ElementInterface *element) {
  ASSERT(impl_);
  return impl_->RemoveElement(element);
}

} // namespace ggadget

#endif // not UNIT_TEST
