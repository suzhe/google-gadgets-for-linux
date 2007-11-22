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

#include "listbox_element.h"
#include "canvas_interface.h"
#include "elements.h"
#include "event.h"
#include "item_element.h"
#include "texture.h"
#include "view_interface.h"

namespace ggadget {

class ListBoxElement::Impl {
 public:
  Impl(ListBoxElement *owner)
      : owner_(owner),
        selected_index_(-1),
        item_over_color_(NULL), // TODO: default color
        item_selected_color_(NULL) {  // TODO: default color
  }

  ~Impl() {
    delete item_over_color_;
    item_over_color_ = NULL;
    delete item_selected_color_;
    item_selected_color_ = NULL;
  }

  ListBoxElement *owner_;
  Variant item_width_, item_height_;
  int selected_index_;
  Texture *item_over_color_, *item_selected_color_;
  EventSignal onchange_event_;
};

ListBoxElement::ListBoxElement(ElementInterface *parent,
                               ViewInterface *view,
                               const char *tag_name,
                               const char *name)
    : DivElement(parent, view, tag_name, name, true),
      impl_(new Impl(this)) {
  SetEnabled(true);
  RegisterProperty("background",
                   NewSlot(implicit_cast<DivElement *>(this),
                           &DivElement::GetBackground),
                   NewSlot(implicit_cast<DivElement *>(this),
                           &DivElement::SetBackground));
  RegisterProperty("autoscroll",
                   NewSlot(implicit_cast<DivElement *>(this),
                           &DivElement::IsAutoscroll),
                   NewSlot(implicit_cast<DivElement *>(this),
                           &DivElement::SetAutoscroll));
  RegisterProperty("itemHeight",
                   NewSlot(this, &ListBoxElement::GetItemHeight),
                   NewSlot(this, &ListBoxElement::SetItemHeight));
  RegisterProperty("itemWidth",
                   NewSlot(this, &ListBoxElement::GetItemWidth),
                   NewSlot(this, &ListBoxElement::SetItemWidth));
  RegisterProperty("itemOverColor",
                   NewSlot(this, &ListBoxElement::GetItemOverColor),
                   NewSlot(this, &ListBoxElement::SetItemOverColor));
  RegisterProperty("itemSelectedColor",
                   NewSlot(this, &ListBoxElement::GetItemSelectedColor),
                   NewSlot(this, &ListBoxElement::SetItemSelectedColor));
  RegisterProperty("itemSeparator",
                   NewSlot(this, &ListBoxElement::HasItemSeparator),
                   NewSlot(this, &ListBoxElement::SetItemSeparator));
  RegisterProperty("multiSelect",
                   NewSlot(this, &ListBoxElement::IsMultiSelect),
                   NewSlot(this, &ListBoxElement::SetMultiSelect));
  RegisterProperty("selectedIndex",
                   NewSlot(this, &ListBoxElement::GetSelectedIndex),
                   NewSlot(this, &ListBoxElement::SetSelectedIndex));
  RegisterProperty("selectedItem",
                   NewSlot(this, &ListBoxElement::GetSelectedItem),
                   NewSlot(this, &ListBoxElement::SetSelectedItem));
  RegisterMethod("ClearSelection",
                 NewSlot(this, &ListBoxElement::ClearSelection));
  RegisterSignal("onchange", &impl_->onchange_event_);
}

ListBoxElement::~ListBoxElement() {
  delete impl_;
  impl_ = NULL;
}

void ListBoxElement::DoDraw(CanvasInterface *canvas,
                         const CanvasInterface *children_canvas) {
  DivElement::DoDraw(canvas, children_canvas);
}

Variant ListBoxElement::GetItemWidth() const {
  return impl_->item_width_;
}

void ListBoxElement::SetItemWidth(const Variant &width) {
  impl_->item_width_ = width;
  // TODO: set children height.
}

Variant ListBoxElement::GetItemHeight() const {
  return impl_->item_height_;
}

void ListBoxElement::SetItemHeight(const Variant &height) {
  impl_->item_height_ = height;
  // TODO: set children height.
}

/** Gets or sets the background texture of the item under the mouse cursor. */
Variant ListBoxElement::GetItemOverColor() const {
  return Variant(Texture::GetSrc(impl_->item_over_color_));
}

void ListBoxElement::SetItemOverColor(const Variant &color) {
  delete impl_->item_over_color_;
  impl_->item_over_color_ = GetView()->LoadTexture(color);
  QueueDraw();
}

Variant ListBoxElement::GetItemSelectedColor() const {
  return Variant(Texture::GetSrc(impl_->item_selected_color_));
}

void ListBoxElement::SetItemSelectedColor(const Variant &color) {
  delete impl_->item_selected_color_;
  impl_->item_selected_color_ = GetView()->LoadTexture(color);
  QueueDraw();
}

bool ListBoxElement::HasItemSeparator() const {
  // TODO:
  return false;
}

void ListBoxElement::SetItemSeparator(bool separator) {
  // TODO:
}

bool ListBoxElement::IsMultiSelect() const {
  // TODO:
  return false;
}

void ListBoxElement::SetMultiSelect(bool multiSelect) {
  // TODO:
}

int ListBoxElement::GetSelectedIndex() const {
  if (impl_->selected_index_ >= GetChildren()->GetCount())
    impl_->selected_index_ = -1;
  return impl_->selected_index_;
}

void ListBoxElement::SetSelectedIndex(int index) {
  if (impl_->selected_index_ != index) {
    if (index < -1 || index >= GetChildren()->GetCount()) {
      impl_->selected_index_ = -1;
    } else {
      impl_->selected_index_ = 0;
    }
    QueueDraw();
  }
}

ItemElement *ListBoxElement::GetSelectedItem() const {
  return NULL;
}

void ListBoxElement::SetSelectedItem(ItemElement *item) {
  // TODO:
}

void ListBoxElement::ClearSelection() {
  // TODO:
}

bool ListBoxElement::OnMouseEvent(MouseEvent *event, bool direct,
                              ElementInterface **fired_element) {
  return DivElement::OnMouseEvent(event, direct, fired_element);
}

bool ListBoxElement::OnKeyEvent(KeyboardEvent *event) {
  return DivElement::OnKeyEvent(event);
}

ElementInterface *ListBoxElement::CreateInstance(ElementInterface *parent,
                                             ViewInterface *view,
                                             const char *name) {
  return new ListBoxElement(parent, view, "listbox", name);
}

} // namespace ggadget
