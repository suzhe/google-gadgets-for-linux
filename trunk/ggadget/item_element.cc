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
#include "elements.h"
#include "graphics_interface.h"
#include "label_element.h"
#include "listbox_element.h"
#include "logger.h"
#include "texture.h"
#include "text_frame.h"
#include "view.h"

namespace ggadget {

class ItemElement::Impl {
 public:
  Impl(BasicElement *parent)
    : parent_(NULL), selected_(false), mouseover_(false),
      drawoverlay_(true), background_(NULL), index_(0) {
    if (parent->IsInstanceOf(ListBoxElement::CLASS_ID)) {
      parent_ = down_cast<ListBoxElement *>(parent);      
    } else {
      LOG("Item element is not contained inside a parent of the correct type.");
    }
  }

  ~Impl() {
    delete background_;
    background_ = NULL;
  }

  ListBoxElement *parent_;
  bool selected_, mouseover_, drawoverlay_;
  Texture *background_;
  int index_;
};

ItemElement::ItemElement(BasicElement *parent, View *view,
                         const char *tagname, const char *name)
    : BasicElement(parent, view, tagname, name, true),
      impl_(new Impl(parent)) {
  SetEnabled(true);
}

void ItemElement::DoRegister() {
  BasicElement::DoRegister();
  RegisterProperty("background",
                   NewSlot(this, &ItemElement::GetBackground),
                   NewSlot(this, &ItemElement::SetBackground));
  RegisterProperty("selected",
                   NewSlot(this, &ItemElement::IsSelected),
                   NewSlot(this, &ItemElement::SetSelected));
  if (impl_->parent_) {
    RegisterProperty("x", NULL, NewSlot(DummySetter));
    RegisterProperty("y", NULL, NewSlot(DummySetter));
    RegisterProperty("width", NULL, NewSlot(DummySetter));
    RegisterProperty("height", NULL, NewSlot(DummySetter));
  }

  if (impl_->parent_->IsImplicit()) {
    // This Item is in a combobox, so override BasicElement constant.
    RegisterConstant("parentElement", impl_->parent_->GetParentElement());
  }
}

ItemElement::~ItemElement() {
  delete impl_;
  impl_ = NULL;
}

void ItemElement::DoDraw(CanvasInterface *canvas) {
  if (impl_->background_) {
    impl_->background_->Draw(canvas);
  }

  if (impl_->drawoverlay_ && (impl_->selected_ || impl_->mouseover_)) {
    if (impl_->parent_) {
      const Texture *overlay;
      if (impl_->selected_) {
        overlay = impl_->parent_->GetItemSelectedTexture();
      } else {
        overlay = impl_->parent_->GetItemOverTexture();
      }
      if (overlay) {
        overlay->Draw(canvas);
      }
    }
  }

  DrawChildren(canvas);

  if (impl_->drawoverlay_ && impl_->parent_->HasItemSeparator()) {
    const Texture *item_separator = impl_->parent_->GetItemSeparatorTexture();
    if (item_separator) {
      const GraphicsInterface *gfx = GetView()->GetGraphics();
      CanvasInterface *separator = gfx->NewCanvas(canvas->GetWidth(), 2);
      if (!separator) {
        DLOG("Error: unable to create separator canvas.");
        return;
      }

      item_separator->Draw(separator);
      canvas->DrawCanvas(0, canvas->GetHeight() - 2, separator);

      separator->Destroy();
      separator = NULL;
    }
  }
}

void ItemElement::SetDrawOverlay(bool draw) {
  if (draw != impl_->drawoverlay_) {
    impl_->drawoverlay_ = draw;
  }
}

bool ItemElement::IsMouseOver() const {
  return impl_->mouseover_;
}

void ItemElement::QueueDraw() {
  if (impl_->parent_) {
    impl_->parent_->QueueDraw();
  }
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

void ItemElement::SetSelected(bool selected) {
  if (impl_->selected_ != selected) {
    impl_->selected_ = selected;
    QueueDraw();
  }
}

std::string ItemElement::GetLabelText() const {
  const BasicElement *e = GetChildren()->GetItemByIndex(0);
  if (e && e->IsInstanceOf(LabelElement::CLASS_ID)) {
    const LabelElement *label = down_cast<const LabelElement *>(e);
    return label->GetTextFrame()->GetText();
  }
  LOG("Label element not found inside Item element %s", GetName().c_str());
  return std::string();
}

void ItemElement::SetLabelText(const char *text) {
  BasicElement *e = GetChildren()->GetItemByIndex(0);
  if (e && e->IsInstanceOf(LabelElement::CLASS_ID)) {
    LabelElement *label = down_cast<LabelElement *>(e);
    label->GetTextFrame()->SetText(text);
  } else {
    LOG("Label element not found inside Item element %s", GetName().c_str());
  }
}

bool ItemElement::AddLabelWithText(const char *text) {
  BasicElement *e = GetChildren()->AppendElement("label", "");
  if (e) {
    ASSERT(e->IsInstanceOf(LabelElement::CLASS_ID));
    LabelElement *label = down_cast<LabelElement *>(e);
    label->GetTextFrame()->SetText(text);

    QueueDraw();
    return true;
  }

  return false;
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
  if (impl_->parent_) {
    *width  = impl_->parent_->GetItemPixelWidth();
    *height = impl_->parent_->GetItemPixelHeight();
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
     if (impl_->parent_) {
       // Need to invoke selection through parent, since
       // parent knows about multiselect status.
       if (event.GetModifier() & Event::MOD_SHIFT) {
         impl_->parent_->SelectRange(this);
       } else if (event.GetModifier() & Event::MOD_CONTROL) {
         impl_->parent_->AppendSelection(this);
       } else {
         impl_->parent_->SetSelectedItem(this);
       }
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

bool ItemElement::HasOpaqueBackground() const {
  return impl_->background_ ? impl_->background_->IsFullyOpaque() : false;
}

} // namespace ggadget
