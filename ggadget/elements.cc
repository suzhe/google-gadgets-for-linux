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
#include "basic_element.h"
#include "element_factory.h"
#include "graphics_interface.h"
#include "logger.h"
#include "math_utils.h"
#include "scriptable_helper.h"
#include "view.h"
#include "xml_utils.h"
#include "event.h"

namespace ggadget {

class Elements::Impl {
 public:
  Impl(ElementFactory *factory, BasicElement *owner, View *view)
      : factory_(factory), owner_(owner), view_(view),
        width_(.0), height_(.0),
        scrollable_(false) {
    ASSERT(factory);
    ASSERT(view);
  }

  ~Impl() {
    RemoveAllElements();
  }

  int GetCount() {
    return children_.size();
  }

  BasicElement *AppendElement(const char *tag_name, const char *name) {
    BasicElement *e = factory_->CreateElement(tag_name, owner_, view_, name);
    if (e == NULL)
      return NULL;
    if (view_->OnElementAdd(e)) {
      children_.push_back(e);
    } else {
      delete e;
      e = NULL;
    }
    return e;
  }

  BasicElement *InsertElement(const char *tag_name,
                              const BasicElement *before,
                              const char *name) {
    BasicElement *e = factory_->CreateElement(tag_name, owner_, view_, name);
    if (e == NULL)
      return NULL;
    if (view_->OnElementAdd(e)) {
      Children::iterator ite = std::find(children_.begin(), children_.end(),
                                         before);
      children_.insert(ite, e);
    } else {
      delete e;
      e = NULL;
    }
    return e;
  }

  bool RemoveElement(BasicElement *element) {
    Children::iterator ite = std::find(children_.begin(), children_.end(),
                                       element);
    if (ite == children_.end())
      return false;
    view_->OnElementRemove(*ite);
    delete *ite;
    children_.erase(ite);
    return true;
  }

  void RemoveAllElements() {
    for (Children::iterator ite = children_.begin();
         ite != children_.end(); ++ite) {
      view_->OnElementRemove(*ite);
      delete *ite;
    }
    Children v;
    children_.swap(v);
  }

  BasicElement *GetItem(const Variant &index_or_name) {
    switch (index_or_name.type()) {
      case Variant::TYPE_INT64:
        return GetItemByIndex(VariantValue<int>()(index_or_name));
      case Variant::TYPE_STRING:
        return GetItemByName(VariantValue<const char *>()(index_or_name));
      default:
        return NULL;
    }
  }

  BasicElement *GetItemByIndex(int index) {
    if (index >= 0 && index < static_cast<int>(children_.size()))
      return children_[index];
    return NULL;
  }

  BasicElement *GetItemByName(const char *name) {
    return GetItemByIndex(GetIndexByName(name));
  }

  int GetIndexByName(const char *name) {
    if (name == NULL || strlen(name) == 0)
      return -1;
    for (Children::const_iterator ite = children_.begin();
         ite != children_.end(); ++ite) {
      if (GadgetStrCmp((*ite)->GetName().c_str(), name) == 0)
        return ite - children_.begin();
    }
    return -1;
  }

  void MapChildPositionEvent(const PositionEvent &org_event,
                             BasicElement *child,
                             PositionEvent *new_event) {
    double child_x, child_y;
    ASSERT(owner_ == child->GetParentElement());
    child->ParentCoordToSelfCoord(org_event.GetX(), org_event.GetY(),
                                  &child_x, &child_y);
    new_event->SetX(child_x);
    new_event->SetY(child_y);
  }

  void MapChildMouseEvent(const MouseEvent &org_event,
                          BasicElement *child,
                          MouseEvent *new_event) {
    MapChildPositionEvent(org_event, child, new_event);
    BasicElement::FlipMode flip = child->GetFlip();
    if (flip & BasicElement::FLIP_HORIZONTAL)
      new_event->SetWheelDeltaX(-org_event.GetWheelDeltaX());
    if (flip & BasicElement::FLIP_VERTICAL)
      new_event->SetWheelDeltaY(-org_event.GetWheelDeltaY());
  }

  EventResult OnMouseEvent(const MouseEvent &event,
                           BasicElement **fired_element,
                           BasicElement **in_element) {
    // The following event types are processed directly in the view.
    ASSERT(event.GetType() != Event::EVENT_MOUSE_OVER &&
           event.GetType() != Event::EVENT_MOUSE_OUT);

    ElementHolder in_element_holder;
    *fired_element = NULL;
    MouseEvent new_event(event);
    // Iterate in reverse since higher elements are listed last.
    for (Children::reverse_iterator ite = children_.rbegin();
         ite != children_.rend(); ++ite) {
      BasicElement *child = *ite;
      // Don't use child->ReallyVisible() because here we don't need to check
      // visibility of ancestors.
      if (!child->IsVisible() || child->GetOpacity() == 0.0)
        continue;
      MapChildMouseEvent(event, child, &new_event);
      if (child->IsPointIn(new_event.GetX(), new_event.GetY())) {
        BasicElement *child = *ite;
        ElementHolder child_holder(*ite);
        BasicElement *descendant_in_element = NULL;
        EventResult result = child->OnMouseEvent(new_event, false,
                                                 fired_element,
                                                 &descendant_in_element);
        // The child has been removed by some event handler, can't continue.
        if (!child_holder.Get())
          return result;

        if (!in_element_holder.Get()) {
          in_element_holder.Reset(descendant_in_element ?
                                  descendant_in_element : child);
        }
        if (*fired_element)
          return result;
      }
    }
    return EVENT_RESULT_UNHANDLED;
  }

  EventResult OnDragEvent(const DragEvent &event,
                          BasicElement **fired_element) {
    // Only the following event type is dispatched along the element tree.
    ASSERT(event.GetType() == Event::EVENT_DRAG_MOTION);

    *fired_element = NULL;
    DragEvent new_event(event);
    // Iterate in reverse since higher elements are listed last.
    for (Children::reverse_iterator ite = children_.rbegin();
         ite != children_.rend(); ++ite) {
      BasicElement *child = (*ite);
      if (!child->ReallyVisible())
        continue;

      MapChildPositionEvent(event, child, &new_event);
      if (child->IsPointIn(new_event.GetX(), new_event.GetY())) {
        ElementHolder child_holder(child);
        EventResult result = (*ite)->OnDragEvent(new_event, false,
                                                 fired_element);
        // The child has been removed by some event handler, can't continue.
        if (!child_holder.Get() || *fired_element)
          return result;
      }
    }
    return EVENT_RESULT_UNHANDLED;
  }

  // Update the maximum children extent.
  void UpdateChildExtent(BasicElement *child,
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
                             DegreesToRadians(child->GetRotation()),
                             &child_extent_width, &child_extent_height);
      if (child_extent_width > *extent_width)
        *extent_width = child_extent_width;
      if (child_extent_height > *extent_height)
        *extent_height = child_extent_height;
    }
  }

  void Layout() {
    int child_count = children_.size();
    for (int i = 0; i < child_count; i++) {
      children_[i]->Layout();
    }

    if (scrollable_) {
      // If scrollable, the canvas size is the max extent of the children.
      bool need_update_extents = false;
      for (int i = 0; i < child_count; i++) {
        if (children_[i]->IsPositionChanged() ||
            children_[i]->IsSizeChanged()) {
          need_update_extents = true;
          break;
        }
      }

      if (need_update_extents) {
        width_ = height_ = 0;
        for (int i = 0; i < child_count; i++) {
          UpdateChildExtent(children_[i], &width_, &height_);
        }
      }
    } else if (owner_) {
      // If not scrollable, the canvas size is the same as the parent.
      width_ = static_cast<size_t>(ceil(owner_->GetPixelWidth()));
      height_ = static_cast<size_t>(ceil(owner_->GetPixelHeight()));
    } else {
      width_ = view_->GetWidth();
      height_ = view_->GetHeight();
    }
  }

  void Draw(CanvasInterface *canvas) {
    ASSERT(canvas);
    if (children_.empty() || !width_ || !height_)
      return;

    // Draw children into temp array.
    int child_count = children_.size();

    BasicElement *popup = view_->GetPopupElement();
    for (int i = 0; i < child_count; i++) {
      BasicElement *element = children_[i];
      // Doesn't draw popup element here.
      if (element == popup) continue;

      canvas->PushState();
      if (element->GetRotation() == .0) {
        canvas->TranslateCoordinates(
            element->GetPixelX() - element->GetPixelPinX(),
            element->GetPixelY() - element->GetPixelPinY());
      } else {
        canvas->TranslateCoordinates(element->GetPixelX(),
                                     element->GetPixelY());
        canvas->RotateCoordinates(
            DegreesToRadians(element->GetRotation()));
        canvas->TranslateCoordinates(-element->GetPixelPinX(),
                                     -element->GetPixelPinY());
      }

      element->Draw(canvas);
      canvas->PopState();
    }

    if (view_->GetDebugMode() > 0) {
      // Draw bounding box for debug.
      canvas->DrawLine(0, 0, 0, height_, 1, Color(0, 0, 0));
      canvas->DrawLine(0, 0, width_, 0, 1, Color(0, 0, 0));
      canvas->DrawLine(width_, height_, 0, height_, 1, Color(0, 0, 0));
      canvas->DrawLine(width_, height_, width_, 0, 1, Color(0, 0, 0));
      canvas->DrawLine(0, 0, width_, height_, 1, Color(0, 0, 0));
      canvas->DrawLine(width_, 0, 0, height_, 1, Color(0, 0, 0));
    }
  }

  void SetScrollable(bool scrollable) {
    scrollable_ = scrollable;
  }

  void MarkRedraw() {
    Children::iterator it = children_.begin();
    for (; it != children_.end(); ++it)
      (*it)->MarkRedraw();
  }

  ElementFactory *factory_;
  BasicElement *owner_;
  View *view_;
  typedef std::vector<BasicElement *> Children;
  Children children_;
  double width_;
  double height_;
  bool scrollable_;
};

Elements::Elements(ElementFactory *factory,
                   BasicElement *owner, View *view)
    : impl_(new Impl(factory, owner, view)) {
}

void Elements::DoRegister() {
  RegisterProperty("count", NewSlot(impl_, &Impl::GetCount), NULL);
  RegisterMethod("item", NewSlot(impl_, &Impl::GetItem));
  // Register the "default" method, allowing this object be called directly
  // as a function.
  RegisterMethod("", NewSlot(impl_, &Impl::GetItem));
}

Elements::~Elements() {
  delete impl_;
}

int Elements::GetCount() const {
  return impl_->GetCount();
}

BasicElement *Elements::GetItemByIndex(int child) {
  return impl_->GetItemByIndex(child);
}

BasicElement *Elements::GetItemByName(const char *child) {
  return impl_->GetItemByName(child);
}

const BasicElement *Elements::GetItemByIndex(int child) const {
  return impl_->GetItemByIndex(child);
}

const BasicElement *Elements::GetItemByName(const char *child) const {
  return impl_->GetItemByName(child);
}

BasicElement *Elements::AppendElement(const char *tag_name,
                                          const char *name) {
  return impl_->AppendElement(tag_name, name);
}

BasicElement *Elements::InsertElement(const char *tag_name,
                                          const BasicElement *before,
                                          const char *name) {
  return impl_->InsertElement(tag_name, before, name);
}

BasicElement *Elements::AppendElementFromXML(const char *xml) {
  return ::ggadget::AppendElementFromXML(impl_->view_, this, xml);
}

BasicElement *Elements::InsertElementFromXML(
    const char *xml, const BasicElement *before) {
  return ::ggadget::InsertElementFromXML(impl_->view_, this, xml, before);
}

bool Elements::RemoveElement(BasicElement *element) {
  return impl_->RemoveElement(element);
}

void Elements::RemoveAllElements() {
  impl_->RemoveAllElements();
}

void Elements::Layout() {
  impl_->Layout();
}

void Elements::Draw(CanvasInterface *canvas) {
  impl_->Draw(canvas);
}

EventResult Elements::OnMouseEvent(const MouseEvent &event,
                                   BasicElement **fired_element,
                                   BasicElement **in_element) {
  return impl_->OnMouseEvent(event, fired_element, in_element);
}

EventResult Elements::OnDragEvent(const DragEvent &event,
                                  BasicElement **fired_element) {
  return impl_->OnDragEvent(event, fired_element);
}

void Elements::SetScrollable(bool scrollable) {
  impl_->SetScrollable(scrollable);
}

void Elements::GetChildrenExtents(double *width, double *height) {
  ASSERT(width && height);
  *width = impl_->width_;
  *height = impl_->height_;
}

void Elements::MarkRedraw() {
  impl_->MarkRedraw();
}

} // namespace ggadget
