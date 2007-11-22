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

#include "combobox_element.h"
#include "canvas_interface.h"
#include "elements.h"
#include "event.h"
#include "item_element.h"
#include "texture.h"
#include "view_interface.h"

namespace ggadget {

class ComboBoxElement::Impl {
 public:
  Impl(ComboBoxElement *owner)
      : owner_(owner) {
  }

  ~Impl() {
  }

  ComboBoxElement *owner_;
};

static const char *kTypeNames[] = {
  "dropdown", "droplist"
};

ComboBoxElement::ComboBoxElement(ElementInterface *parent,
                                 ViewInterface *view,
                                 const char *name)
    : ListBoxElement(parent, view, "combobox", name),
      impl_(new Impl(this)) {
  SetAutoscroll(true);
  SetEnabled(true);
  RegisterProperty("droplistVisible",
                   NewSlot(this, &ComboBoxElement::IsDroplistVisible),
                   NewSlot(this, &ComboBoxElement::SetDroplistVisible));
  RegisterProperty("maxDroplistItems",
                   NewSlot(this, &ComboBoxElement::GetMaxDroplistItems),
                   NewSlot(this, &ComboBoxElement::SetMaxDroplistItems));
  RegisterStringEnumProperty("type",
                             NewSlot(this, &ComboBoxElement::GetType),
                             NewSlot(this, &ComboBoxElement::SetType),
                             kTypeNames, arraysize(kTypeNames));
  RegisterProperty("value",
                   NewSlot(this, &ComboBoxElement::GetValue),
                   NewSlot(this, &ComboBoxElement::SetValue));
}

ComboBoxElement::~ComboBoxElement() {
  delete impl_;
  impl_ = NULL;
}

void ComboBoxElement::DoDraw(CanvasInterface *canvas,
                         const CanvasInterface *children_canvas) {
  // TODO: different drawing flow.
  ListBoxElement::DoDraw(canvas, children_canvas);
}

bool ComboBoxElement::IsDroplistVisible() const {
  // TODO:
  return false;
}

void ComboBoxElement::SetDroplistVisible(bool visible) {
  // TODO:
}

int ComboBoxElement::GetMaxDroplistItems() const {
  // TODO:
  return 2;
}

void ComboBoxElement::SetMaxDroplistItems(int max_droplist_items) {
  // TODO:
}

ComboBoxElement::Type ComboBoxElement::GetType() const {
  // TODO:
  return DROPDOWN;
}

void ComboBoxElement::SetType(Type type) {
  // TODO:
}

const char *ComboBoxElement::GetValue() const {
  // TODO:
  return "Combobox";
}

void ComboBoxElement::SetValue(const char *value) {
  // TODO:
}

bool ComboBoxElement::OnMouseEvent(MouseEvent *event, bool direct,
                              ElementInterface **fired_element) {
  return ListBoxElement::OnMouseEvent(event, direct, fired_element);
}

bool ComboBoxElement::OnKeyEvent(KeyboardEvent *event) {
  return ListBoxElement::OnKeyEvent(event);
}

ElementInterface *ComboBoxElement::CreateInstance(ElementInterface *parent,
                                                  ViewInterface *view,
                                                  const char *name) {
  return new ComboBoxElement(parent, view, name);
}

} // namespace ggadget
