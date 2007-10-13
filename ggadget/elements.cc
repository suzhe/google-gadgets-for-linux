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
#include "common.h"
#include "element_interface.h"
#include "element_factory_interface.h"
#include "graphics_interface.h"
#include "math_utils.h"
#include "scriptable_helper.h"
#include "view_interface.h"
#include "xml_utils.h"
#include "event.h"

namespace ggadget {

class Elements::Impl {
 public:
  Impl(ElementFactoryInterface *factory,
       ElementInterface *owner,
       ViewInterface *view)
      : factory_(factory), owner_(owner), view_(view), 
        width_(.0), height_(.0),
        canvas_(NULL),
        count_changed_(true),
        scrollable_(false) {
    ASSERT(factory);
    ASSERT(view);

    RegisterProperty("count", NewSlot(this, &Impl::GetCount), NULL);
    RegisterMethod("item", NewSlot(this, &Impl::GetItem));
    // Disable the following for now, because they are not in the public
    // API document.
    // SetArrayHandler(NewSlot(this, &Impl::GetItemByIndex), NULL);
    // SetDynamicPropertyHandler(NewSlot(this, &Impl::GetItemByNameVariant),
    //                           NULL);
  }

  ~Impl() {
    RemoveAllElements();
  }

  int GetCount() {
    return children_.size();
  }

  ElementInterface *AppendElement(const char *tag_name, const char *name) {
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

  ElementInterface *InsertElement(const char *tag_name,
                                  const ElementInterface *before,
                                  const char *name) {
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

  bool RemoveElement(ElementInterface *element) {
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

  void RemoveAllElements() {
    for (std::vector<ElementInterface *>::iterator ite =
         children_.begin(); ite != children_.end(); ++ite) {
      view_->OnElementRemove(*ite);
      (*ite)->Destroy();
    }
    std::vector<ElementInterface *> v;
    children_.swap(v);
    count_changed_ = true;
  }

  ElementInterface *GetItem(const Variant &index_or_name) {
    switch (index_or_name.type()) {
      case Variant::TYPE_INT64:
        return GetItemByIndex(VariantValue<int>()(index_or_name));
      case Variant::TYPE_STRING:
        return GetItemByName(VariantValue<const char *>()(index_or_name));
      default:
        return NULL;
    }
  }

  ElementInterface *GetItemByIndex(int index) {
    if (index >= 0 && index < static_cast<int>(children_.size()))
      return children_[index];
    return NULL;
  }

  ElementInterface *GetItemByName(const char *name) {
    return GetItemByIndex(GetIndexByName(name));
  }

  Variant GetItemByNameVariant(const char *name) {
    ElementInterface *result = GetItemByName(name);
    return result ? Variant(result) : Variant();
  }

  int GetIndexByName(const char *name) {
    if (name == NULL || strlen(name) == 0)
      return -1;
    for (std::vector<ElementInterface *>::const_iterator ite =
             children_.begin();
         ite != children_.end(); ++ite) {
      if (GadgetStrCmp((*ite)->GetName(), name) == 0)
        return ite - children_.begin();
    }
    return -1;
  }

  void OnParentWidthChange(double width) {
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

  void OnParentHeightChange(double height) {
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

  void MapChildMouseEvent(MouseEvent *org_event, ElementInterface *child,
                          MouseEvent *new_event) {
    double child_x, child_y;
    if (owner_) {
      owner_->SelfCoordToChildCoord(child, org_event->GetX(), org_event->GetY(),
                                    &child_x, &child_y);
    } else {
      ParentCoordToChildCoord(org_event->GetX(), org_event->GetY(),
                              child->GetPixelX(), child->GetPixelY(),
                              child->GetPixelPinX(), child->GetPixelPinY(),
                              DegreesToRadians(child->GetRotation()),
                              &child_x, &child_y);
    }

    new_event->SetX(child_x);
    new_event->SetY(child_y);
  }

  ElementInterface *OnMouseEvent(MouseEvent *event) {
    // The following event types are processed directly in the view.
    ASSERT(event->GetType() != Event::EVENT_MOUSE_OVER &&
           event->GetType() != Event::EVENT_MOUSE_OUT);

    MouseEvent new_event(*event);
    // Iterate in reverse since higher elements are listed last.
    for (std::vector<ElementInterface *>::reverse_iterator ite =
             children_.rbegin();
         ite != children_.rend(); ++ite) {
      MapChildMouseEvent(event, *ite, &new_event);
      if ((*ite)->IsMouseEventIn(&new_event)) {
        ElementInterface *fired_element = (*ite)->OnMouseEvent(&new_event,
                                                               false);
        if (fired_element)
          return fired_element;
      }
    }
    return NULL;
  }

  // Update the maximum children extent.
  void UpdateChildExtent(ElementInterface *child,
                         double *extent_width, double *extent_height) {
    double x = child->GetPixelX();
    double y = child->GetPixelY();
    double pin_x = child->GetPixelPinX();
    double pin_y = child->GetPixelPinY();
    double width = child->GetPixelWidth();
    double height = child->GetPixelHeight();
    // Estimate the biggest possible extent with low cost.
    double est_maximum_extent = std::max(pin_x, width - pin_x) +
                                std::max(pin_y, height - pin_y);
    double child_extent_width = x + est_maximum_extent;
    double child_extent_height = y + est_maximum_extent;
    // Calculate the actual extent only if the estimated value is bigger than
    // current extent.
    if (child_extent_width > *extent_width ||
        child_extent_height > *extent_height) {
      GetChildExtentInParent(x, y, pin_x, pin_y, width, height,
                             child->GetRotation(),
                             &child_extent_width, &child_extent_height);
      if (child_extent_width > *extent_width)
        *extent_width = child_extent_width;
      if (child_extent_height > *extent_height)
        *extent_height = child_extent_height;
    }
  }

  void SetScrollable(bool scrollable) {
    scrollable_ = scrollable;
  }

  const CanvasInterface *Draw(bool *changed);

  DELEGATE_SCRIPTABLE_REGISTER(scriptable_helper_)

  ScriptableHelper scriptable_helper_;
  ElementFactoryInterface *factory_;
  ElementInterface *owner_;
  ViewInterface *view_;
  std::vector<ElementInterface *> children_;
  double width_;
  double height_;
  CanvasInterface *canvas_;
  bool count_changed_;
  bool scrollable_;
};

const CanvasInterface *Elements::Impl::Draw(bool *changed) {
  ElementInterface *element;
  CanvasInterface *canvas = NULL;
  const CanvasInterface **children_canvas = NULL;
  int child_count;
  bool child_changed = false;
  bool change = count_changed_;
  
  count_changed_ = false;
  if (children_.empty()) {
    goto exit;
  }

  // Draw children into temp array.
  child_count = children_.size();
  children_canvas = new const CanvasInterface*[child_count];
  for (int i = 0; i < child_count; i++) {
    element = children_[i];
    children_canvas[i] = element->Draw(&child_changed);
    if (element->IsPositionChanged()) {
      element->ClearPositionChanged();
      child_changed = true;
    }
    change = change || child_changed;
  }

  change = change || !canvas;
  if (change) {
    size_t canvas_width, canvas_height;
    if (scrollable_) {
      if (child_changed || !canvas_) {
        double children_extent_width = 0;
        double children_extent_height = 0;
        for (int i = 0; i < child_count; i++) {
          UpdateChildExtent(children_[i],
                            &children_extent_width,
                            &children_extent_height);
        }
        canvas_width = static_cast<size_t>(ceil(children_extent_width));
        canvas_height = static_cast<size_t>(ceil(children_extent_height));
      } else {
        canvas_width = canvas_->GetWidth();
        canvas_height = canvas_->GetHeight();
      }
    } else {
      canvas_width = static_cast<size_t>(ceil(width_));
      canvas_height = static_cast<size_t>(ceil(height_));
    }

    // Need to redraw.
    if (!canvas_ || canvas_->GetWidth() != canvas_width ||
        canvas_->GetHeight() != canvas_height) {
      if (canvas_)
        canvas_->Destroy();

      if (canvas_width == 0 || canvas_height == 0) {
        canvas_ = NULL;
        goto exit;
      }

      const GraphicsInterface *gfx = view_->GetGraphics();
      canvas_ = gfx->NewCanvas(canvas_width, canvas_height);
      if (!canvas_) {
        DLOG("Error: unable to create canvas.");
        goto exit;
      }
    }
    else {
      // If not new canvas, we must remember to clear canvas before drawing.
      canvas_->ClearCanvas();
    }

    // TODO: Ensure whether it is useful.
    if (!scrollable_)
      canvas_->IntersectRectClipRegion(0., 0., width_, height_);

    for (int i = 0; i < child_count; i++) {
      if (children_canvas[i]) {
        canvas_->PushState();

        element = children_[i];
        if (element->GetRotation() == .0) {
          canvas_->TranslateCoordinates(
              element->GetPixelX() - element->GetPixelPinX(),
              element->GetPixelY() - element->GetPixelPinY());
        }
        else {
          canvas_->TranslateCoordinates(element->GetPixelX(),
                                        element->GetPixelY());
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

  if (view_->GetDebugMode() > 0) {
    // Draw bounding box for debug.
    double w = canvas_->GetWidth();
    double h = canvas_->GetHeight();
    canvas_->DrawLine(0, 0, 0, h, 1, Color(0, 0, 0));
    canvas_->DrawLine(0, 0, w, 0, 1, Color(0, 0, 0));
    canvas_->DrawLine(w, h, 0, h, 1, Color(0, 0, 0));
    canvas_->DrawLine(w, h, w, 0, 1, Color(0, 0, 0));
    canvas_->DrawLine(0, 0, w, h, 1, Color(0, 0, 0));
    canvas_->DrawLine(w, 0, 0, h, 1, Color(0, 0, 0));
  }

  canvas = canvas_;

exit:
  if (children_canvas) {
    delete[] children_canvas;
    children_canvas = NULL;
  }

  *changed = change;
  return canvas;
}

Elements::Elements(ElementFactoryInterface *factory,
                   ElementInterface *owner,
                   ViewInterface *view)
    : impl_(new Impl(factory, owner, view)) {
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

ElementInterface *Elements::OnMouseEvent(MouseEvent *event) {
  ASSERT(impl_);
  return impl_->OnMouseEvent(event);
}

void Elements::SetScrollable(bool scrollable) {
  ASSERT(impl_);
  impl_->SetScrollable(scrollable);
}

DELEGATE_SCRIPTABLE_INTERFACE_IMPL(Elements, impl_->scriptable_helper_)

} // namespace ggadget
