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
    (*ite)->Destroy();
}

int ElementsImpl::GetCount() {
  return children_.size();
}

ElementInterface *ElementsImpl::GetItem(Variant child) {
  switch (child.type()) {
    case Variant::TYPE_BOOL:
      if (VariantValue<bool>()(child))
        return NULL;
      return GetItemByIndex(0);
    case Variant::TYPE_DOUBLE:
      return GetItemByIndex(static_cast<int>(VariantValue<double>()(child)));
    case Variant::TYPE_INT64:
      return GetItemByIndex(VariantValue<int>()(child));
    case Variant::TYPE_STRING:
      return GetItemByName(VariantValue<const char *>()(child));
    default:
      return NULL;
  }
}

ElementInterface *ElementsImpl::AppendElement(const char *tag_name,
                                              const char *name) {
  ElementInterface *e = factory_->CreateElement(tag_name,
                                                owner_,
                                                owner_->GetView(),
                                                name);
  if (e == NULL)
    return NULL;
  children_.push_back(e);
  return e;
}

ElementInterface *ElementsImpl::InsertElement(
    const char *tag_name, const ElementInterface *before, const char *name) {
  ElementInterface *e = factory_->CreateElement(tag_name,
                                                owner_,
                                                owner_->GetView(),
                                                name);
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
  (*ite)->Destroy();
  children_.erase(ite);
  return true;
}

ElementInterface *ElementsImpl::GetItemByIndex(int index) {
  if (index >= 0 && index < static_cast<int>(children_.size()))
    return children_[index];
  return NULL;
}

ElementInterface *ElementsImpl::GetItemByName(const char *name) {
  return GetItemByIndex(GetIndexByName(name));
}

int ElementsImpl::GetIndexByName(const char *name) {
  if (name == NULL || strlen(name) == 0)
    return -1;
  for (std::vector<ElementInterface *>::const_iterator ite = children_.begin();
       ite != children_.end(); ++ite) {
    if (strcmp((*ite)->GetName(), name) == 0)
      return ite - children_.begin();
  }
  return -1;
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

ElementInterface *Elements::GetItem(Variant child) {
  ASSERT(impl_);
  return impl_->GetItem(child);
}

const ElementInterface *Elements::GetItem(Variant child) const {
  ASSERT(impl_);
  return impl_->GetItem(child);
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
