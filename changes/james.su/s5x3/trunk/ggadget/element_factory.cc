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
#include "common.h"
#include "basic_element.h"
#include "view.h"

#include "anchor_element.h"
#include "button_element.h"
#include "checkbox_element.h"
#include "combobox_element.h"
#include "contentarea_element.h"
#include "div_element.h"
#include "edit_element.h"
#include "img_element.h"
#include "item_element.h"
#include "label_element.h"
#include "listbox_element.h"
#include "progressbar_element.h"
#include "scrollbar_element.h"

namespace ggadget {

ElementFactory::ElementFactory()
    : impl_(new internal::ElementFactoryImpl) {
  RegisterElementClass("a", &ggadget::AnchorElement::CreateInstance);
  RegisterElementClass("button", &ggadget::ButtonElement::CreateInstance);
  RegisterElementClass("checkbox", 
                       &ggadget::CheckBoxElement::CreateCheckBoxInstance);
  RegisterElementClass("combobox", &ggadget::ComboBoxElement::CreateInstance);
  RegisterElementClass("contentarea", 
                       &ggadget::ContentAreaElement::CreateInstance);
  RegisterElementClass("div", &ggadget::DivElement::CreateInstance);
  RegisterElementClass("edit", &ggadget::EditElement::CreateInstance);
  RegisterElementClass("img", &ggadget::ImgElement::CreateInstance);
  RegisterElementClass("item", &ggadget::ItemElement::CreateInstance);
  RegisterElementClass("label", &ggadget::LabelElement::CreateInstance);
  RegisterElementClass("listbox", &ggadget::ListBoxElement::CreateInstance);
  RegisterElementClass("listitem", 
                       &ggadget::ItemElement::CreateListItemInstance);
  RegisterElementClass("progressbar", 
                       &ggadget::ProgressBarElement::CreateInstance);
  RegisterElementClass("radio", &ggadget::CheckBoxElement::CreateRadioInstance);
  RegisterElementClass("scrollbar", &ggadget::ScrollBarElement::CreateInstance);
}

ElementFactory::~ElementFactory() {
  delete impl_;
}

BasicElement *ElementFactory::CreateElement(const char *tag_name,
                                            BasicElement *parent,
                                            ViewInterface *view,
                                            const char *name) {
  ASSERT(impl_);
  return impl_->CreateElement(tag_name, parent, view, name);
}

bool ElementFactory::RegisterElementClass(const char *tag_name,
                                          ElementCreator creator) {
  ASSERT(impl_);
  return impl_->RegisterElementClass(tag_name, creator);
}

namespace internal {

BasicElement *ElementFactoryImpl::CreateElement(const char *tag_name,
                                                BasicElement *parent,
                                                ViewInterface *view,
                                                const char *name) {
  CreatorMap::iterator ite = creators_.find(tag_name);
  if (ite == creators_.end())
    return NULL;
  return ite->second(down_cast<BasicElement *>(parent),
                     down_cast<View *>(view),
                     name);
}

bool ElementFactoryImpl::RegisterElementClass(
    const char *tag_name, ElementFactory::ElementCreator creator) {
  CreatorMap::iterator ite = creators_.find(tag_name);
  if (ite != creators_.end())
    return false;
  creators_[tag_name] = creator;
  return true;
}

} // namespace internal

} // namespace ggadget
