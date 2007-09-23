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
#include "common.h"
#include "element_interface.h"
#include "element_factory_interface.h"
#include "view_interface.h"
#include "xml_utils.h"
#include "math_utils.h"
#include "graphics_interface.h"

namespace ggadget {

namespace internal {

ElementsImpl::ElementsImpl(ElementFactoryInterface *factory,
                           ElementInterface *owner,
                           ViewInterface *view)
    : factory_(factory), owner_(owner), view_(view), 
    width_(.0), height_(.0), canvas_(NULL), count_changed_(true) {
  ASSERT(factory);
  ASSERT(view);

  RegisterProperty("count", NewSlot(this, &ElementsImpl::GetCount), NULL);
  RegisterMethod("item", NewSlot(this, &ElementsImpl::GetItem));
  SetArrayHandler(NewSlot(this, &ElementsImpl::GetItemByIndex), NULL);
  SetDynamicPropertyHandler(NewSlot(this, &ElementsImpl::GetItemByName), NULL);
}

ElementsImpl::~ElementsImpl() {
  RemoveAllElements();
}

int ElementsImpl::GetCount() {
  return children_.size();
}

ElementInterface *ElementsImpl::AppendElement(const char *tag_name,
                                              const char *name) {
  ElementInterface *e = factory_->CreateElement(tag_name,
                                                owner_,
                                                view_,
                                                name);
  if (e == NULL)
    return NULL;
  children_.push_back(e);
  count_changed_ = true;
  view_->OnElementAdd(e);
  return e;
}

ElementInterface *ElementsImpl::InsertElement(
    const char *tag_name, const ElementInterface *before, const char *name) {
  ElementInterface *e = factory_->CreateElement(tag_name,
                                                owner_,
                                                view_,
                                                name);
  if (e == NULL)
    return NULL;
  std::vector<ElementInterface *>::iterator ite = std::find(
      children_.begin(), children_.end(), before);
  children_.insert(ite, e);
  count_changed_ = true;
  view_->OnElementAdd(e);
  return e;
}

bool ElementsImpl::RemoveElement(ElementInterface *element) {
  std::vector<ElementInterface *>::iterator ite = std::find(
      children_.begin(), children_.end(), element);
  if (ite == children_.end())
    return false;
  view_->OnElementRemove(*ite);
  (*ite)->Destroy();
  children_.erase(ite);
  count_changed_ = true;
  return true;
}

void ElementsImpl::RemoveAllElements() {
  for (std::vector<ElementInterface *>::iterator ite =
       children_.begin(); ite != children_.end(); ++ite) {
    view_->OnElementRemove(*ite);
    (*ite)->Destroy();
  }
  std::vector<ElementInterface *> v;
  children_.swap(v);
  count_changed_ = true;
}

ElementInterface *ElementsImpl::GetItem(const Variant &index_or_name) {
  switch (index_or_name.type()) {
    case Variant::TYPE_INT64:
      return GetItemByIndex(VariantValue<int>()(index_or_name));
    case Variant::TYPE_STRING:
      return GetItemByName(VariantValue<const char *>()(index_or_name));
    default:
      return NULL;
  }
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
    if (GadgetStrCmp((*ite)->GetName(), name) == 0)
      return ite - children_.begin();
  }
  return -1;
}

void ElementsImpl::OnParentWidthChange(double width) {
  if (width_ != width) {
    width_ = width;
    if (canvas_) {
      canvas_->Destroy();
      canvas_ = NULL;
    }
    for (std::vector<ElementInterface *>::iterator ite = children_.begin();
         ite != children_.end(); ++ite) {
      (*ite)->OnParentWidthChange(width);
    }
  }
}

void ElementsImpl::OnParentHeightChange(double height) {
  if (height != height_) {
    height_ = height;
    if (canvas_) {
      canvas_->Destroy();
      canvas_ = NULL;
    }
    for (std::vector<ElementInterface *>::iterator ite = children_.begin();
         ite != children_.end(); ++ite) {
      (*ite)->OnParentHeightChange(height);
    }
  }
}

const CanvasInterface *ElementsImpl::Draw(bool *changed) {
  ElementInterface *element;
  CanvasInterface *canvas = NULL;
  const CanvasInterface **children_canvas = NULL;
  int child_count;
  bool child_changed = false;
  bool change = count_changed_;
  
  count_changed_ = false;
  if (children_.empty()) {
    *changed = change;
    goto exit;
  }
  
  // draw children into temp array
  child_count = children_.size();
  children_canvas = new const CanvasInterface*[child_count];
  for (int i = 0; i < child_count; i++) {
    element = children_[i];
    children_canvas[i] = element->Draw(&child_changed);
    if (child_changed || element->IsPositionChanged()) {
      element->ClearPositionChanged();
      change = true;
    }    
  }
  
  if (!canvas_ || change) {
    // Need to redraw
    change = true;
    
    if (!canvas_) {
      const GraphicsInterface *gfx = view_->GetGraphics();
      canvas_ = gfx->NewCanvas(static_cast<size_t>(width_) + 1, 
                               static_cast<size_t>(height_) + 1);
      if (!canvas_) {
        DLOG("Error: unable to create canvas.");
        change = true;
        goto exit;
      }
    }
    else {
      // If not new canvas, we must remember to clear canvas before drawing.
      canvas_->ClearCanvas();
    }
    canvas_->IntersectRectClipRegion(0., 0., width_, height_);

    for (int i = 0; i < child_count; i++) {
      if (children_canvas[i]) {
        canvas_->PushState();

        element = children_[i];
        canvas_->TranslateCoordinates(element->GetPixelX(), 
                                      element->GetPixelY());
        
        if (element->GetRotation() != .0) {
          canvas_->TranslateCoordinates(element->GetPixelPinX(), 
                                        element->GetPixelPinY());
          canvas_->RotateCoordinates(DegreesToRadians(element->GetRotation()));
          canvas_->TranslateCoordinates(-element->GetPixelPinX(), 
                                        -element->GetPixelPinY());
        }

        const CanvasInterface *mask = element->GetMaskCanvas();
        if (mask) {
          canvas_->DrawCanvasWithMask(.0, .0, children_canvas[i], .0, .0, mask);
        }
        else {
          canvas_->DrawCanvas(.0, .0, children_canvas[i]);
        }
        
        canvas_->PopState();  
      }          
    }
  }
  
  // Draw bounding box
  canvas_->DrawLine(0, 0, 0, height_, 1, Color(0, 0, 0));
  canvas_->DrawLine(0, 0, width_, 0, 1, Color(0, 0, 0));
  canvas_->DrawLine(width_, height_, 0, height_, 1, Color(0, 0, 0));
  canvas_->DrawLine(width_, height_, width_, 0, 1, Color(0, 0, 0));
  canvas_->DrawLine(0, 0, width_, height_, 1, Color(0, 0, 0));
  canvas_->DrawLine(width_, 0, 0, height_, 1, Color(0, 0, 0));
  
  canvas = canvas_;
  
exit:  
  if (children_canvas) {
    delete[] children_canvas;
    children_canvas = NULL;
  }

  *changed = change;
  return canvas;
}

} // namespace internal

Elements::Elements(ElementFactoryInterface *factory,
                   ElementInterface *owner,
                   ViewInterface *view)
    : impl_(new internal::ElementsImpl(factory, owner, view)) {
}

Elements::~Elements() {
  delete impl_;
}

int Elements::GetCount() const {
  ASSERT(impl_);
  return impl_->GetCount();
}

ElementInterface *Elements::GetItemByIndex(int child) {
  ASSERT(impl_);
  return impl_->GetItemByIndex(child);
}

ElementInterface *Elements::GetItemByName(const char *child) {
  ASSERT(impl_);
  return impl_->GetItemByName(child);
}

const ElementInterface *Elements::GetItemByIndex(int child) const {
  ASSERT(impl_);
  return impl_->GetItemByIndex(child);
}

const ElementInterface *Elements::GetItemByName(const char *child) const {
  ASSERT(impl_);
  return impl_->GetItemByName(child);
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

ElementInterface *Elements::AppendElementFromXML(const char *xml) {
  return ::ggadget::AppendElementFromXML(this, xml);
}

ElementInterface *Elements::InsertElementFromXML(
    const char *xml, const ElementInterface *before) {
  return ::ggadget::InsertElementFromXML(this, xml, before);
}

bool Elements::RemoveElement(ElementInterface *element) {
  ASSERT(impl_);
  return impl_->RemoveElement(element);
}

void Elements::RemoveAllElements() {
  ASSERT(impl_);
  impl_->RemoveAllElements();
}

void Elements::OnParentWidthChange(double width) {
  ASSERT(impl_);
  impl_->OnParentWidthChange(width);
}

void Elements::OnParentHeightChange(double height) {
  ASSERT(impl_);
  impl_->OnParentHeightChange(height);
}

const CanvasInterface *Elements::Draw(bool *changed) {
  ASSERT(impl_);
  return impl_->Draw(changed);
}

DELEGATE_SCRIPTABLE_INTERFACE_IMPL(Elements, impl_->scriptable_helper_)

} // namespace ggadget
