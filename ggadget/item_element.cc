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
#include "label_element.h"
#include "listbox_element.h"
#include "list_elements.h"
#include "texture.h"
#include "text_frame.h"
#include "view.h"

namespace ggadget {

class ItemElement::Impl {
 public:
  Impl(BasicElement *parent) 
    : parent_(parent), selected_(false), mouseover_(false), 
      drawoverlay_(true), background_(NULL), index_(0) {
    ElementsInterface *children;
    if (parent && 
        (children = parent->GetChildren())->IsInstanceOf(ListElements::CLASS_ID)) {
      elements_ = down_cast<ListElements *>(children);
    } else {
	  LOG("Item element is not contained inside a parent of the correct type");
    }
  }

  ~Impl() {    
    delete background_;
    background_ = NULL;
  }

  BasicElement *parent_;
  bool selected_, mouseover_, drawoverlay_;
  Texture *background_;
  int index_;
  ListElements *elements_;
};

ItemElement::ItemElement(BasicElement *parent, View *view,
                         const char *tagname, const char *name)
    : BasicElement(parent, view, tagname, name, 
                   new Elements(view->GetElementFactory(), this, view)),
      impl_(new Impl(parent)) {
  SetEnabled(true);
  RegisterProperty("background",
                   NewSlot(this, &ItemElement::GetBackground),
                   NewSlot(this, &ItemElement::SetBackground));
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
  if (impl_->background_) {
    impl_->background_->Draw(canvas);
  }

  if (impl_->drawoverlay_ && (impl_->selected_ || impl_->mouseover_)) {
    if (impl_->elements_) {
      const Texture *overlay; 
      if (impl_->selected_) {
        overlay = impl_->elements_->GetItemSelectedTexture();
      } else {
        overlay = impl_->elements_->GetItemOverTexture();
      }       
      if (overlay) {
        overlay->Draw(canvas);
      }
    }
  }

  if (children_canvas) {
    canvas->DrawCanvas(0, 0, children_canvas);
  }
}

void ItemElement::SetDrawOverlay(bool draw) {
  if (draw != impl_->drawoverlay_) {
    impl_->drawoverlay_ = draw;
    // This method is used inside Draw() calls to temporarily disable
    // overlays, so don't queue another draw.
    QueueDraw();
  }
}

bool ItemElement::IsMouseOver() const {
  return impl_->mouseover_;
}

void ItemElement::QueueDraw() {
  impl_->parent_->QueueDraw();
  BasicElement::QueueDraw();
}

void ItemElement::SetIndex(int index) {
  if (impl_->index_ != index) {
    impl_->index_ = index;
  }
}

Variant ItemElement::GetBackground() const {
  return Variant(Texture::GetSrc(impl_->background_));
}

void ItemElement::SetBackground(const Variant &background) {
  delete impl_->background_;
  impl_->background_ = GetView()->LoadTexture(background);
  QueueDraw();
}

bool ItemElement::IsSelected() const {
  return impl_->selected_;
}

void ItemElement::SetSelectedNoRedraw(bool selected) {
  if (impl_->selected_ != selected) {
    impl_->selected_ = selected;
    QueueDraw();
  }
}

void ItemElement::SetSelected(bool selected) {
  if (impl_->selected_ != selected) {
    impl_->selected_ = selected;
    QueueDraw();
  }
}

void ItemElement::SetWidth(const Variant &width) {
  // Disabled
}
void ItemElement::SetHeight(const Variant &height) {
  // Disabled
}

void ItemElement::SetX(const Variant &x) {
  // Disabled
}

void ItemElement::SetY(const Variant &y) {
  // Disabled
}

const char *ItemElement::GetLabelText() const {
  const ElementInterface *e = GetChildren()->GetItemByIndex(0);
  if (e && e->IsInstanceOf(LabelElement::CLASS_ID)) {
    const LabelElement *label = down_cast<const LabelElement *>(e);
    return label->GetTextFrame()->GetText();
  }
  LOG("Label element not found inside Item element %s", GetName());
  return NULL;
}

void ItemElement::SetLabelText(const char *text) {
  ElementInterface *e = GetChildren()->GetItemByIndex(0);
  if (e && e->IsInstanceOf(LabelElement::CLASS_ID)) {
    LabelElement *label = down_cast<LabelElement *>(e);
    label->GetTextFrame()->SetText(text);
  } else {
    LOG("Label element not found inside Item element %s", GetName());
  }
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

void ItemElement::GetDefaultSize(double *width, double *height) const {
  if (impl_->elements_) {
    *width  = impl_->elements_->GetItemPixelWidth();
    *height = impl_->elements_->GetItemPixelHeight();
  } else {
    *width = *height = 0;
  }  
}

void ItemElement::GetDefaultPosition(double *x, double *y) const {
  *x = 0;
  *y = impl_->index_ * GetPixelHeight();
}

EventResult ItemElement::HandleMouseEvent(const MouseEvent &event) {
  EventResult result = EVENT_RESULT_HANDLED;
  switch (event.GetType()) {
   case Event::EVENT_MOUSE_CLICK:   
     if (impl_->elements_) {
       // Need to invoke selection through parent, since
       // parent knows about multiselect status.
       impl_->elements_->AppendSelection(this);
     }     
    break;
   case Event::EVENT_MOUSE_OUT:
    impl_->mouseover_ = false;    
    QueueDraw();
    break;
   case Event::EVENT_MOUSE_OVER:
    impl_->mouseover_ = true;
    QueueDraw();
    break;
   default:
    result = EVENT_RESULT_UNHANDLED;
    break;
  }
  return result;
}

} // namespace ggadget
