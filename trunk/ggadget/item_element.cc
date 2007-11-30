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

#include "item_element.h"
#include "canvas_interface.h"
#include "listbox_element.h"
#include "view.h"

namespace ggadget {

class ItemElement::Impl {
 public:
  Impl() : selected_(false) {
  }

  ~Impl() {
  }

  bool selected_;
};

ItemElement::ItemElement(BasicElement *parent, View *view,
                         const char *tagname, const char *name)
    : DivElement(parent, view, tagname, name),
      impl_(new Impl) {
  SetEnabled(true);
  RegisterProperty("background",
                   NewSlot(implicit_cast<DivElement *>(this),
                           &DivElement::GetBackground),
                   NewSlot(implicit_cast<DivElement *>(this),
                           &DivElement::SetBackground));
  RegisterProperty("selected",
                   NewSlot(this, &ItemElement::IsSelected),
                   NewSlot(this, &ItemElement::SetSelected));
}

ItemElement::~ItemElement() {
  delete impl_;
  impl_ = NULL;
}

void ItemElement::DoDraw(CanvasInterface *canvas,
                         const CanvasInterface *children_canvas) {
  DivElement::DoDraw(canvas, children_canvas);
}

bool ItemElement::IsSelected() const {
  return impl_->selected_;
}

void ItemElement::SetSelected(bool selected) {
  impl_->selected_ = true;
  // TODO:
}

BasicElement *ItemElement::CreateInstance(BasicElement *parent, View *view,
                                          const char *name) {
  return new ItemElement(parent, view, "item", name);
}

BasicElement *ItemElement::CreateListItemInstance(BasicElement *parent,
                                                  View *view,
                                                  const char *name) {
  return new ItemElement(parent, view, "listitem", name);
}

} // namespace ggadget
