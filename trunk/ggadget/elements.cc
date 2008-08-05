/*
  Copyright 2008 Google Inc.

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
#include "event.h"
#include "element_factory.h"
#include "gadget.h"
#include "graphics_interface.h"
#include "logger.h"
#include "math_utils.h"
#include "scriptable_helper.h"
#include "view.h"
#include "view_element.h"
#include "xml_dom_interface.h"
#include "xml_parser_interface.h"
#include "xml_utils.h"

namespace ggadget {

class Elements::Impl {
 public:
  Impl(ElementFactory *factory, BasicElement *owner, View *view)
      : factory_(factory), owner_(owner), view_(view),
        width_(.0), height_(.0),
        scrollable_(false), element_removed_(false) {
    ASSERT(view);
  }

  ~Impl() {
    RemoveAllElements();
  }

  int GetCount() {
    return static_cast<int>(children_.size());
  }

  BasicElement *AppendElement(const char *tag_name, const char *name) {
    if (!factory_) return NULL;

    BasicElement *e = factory_->CreateElement(tag_name, owner_, view_, name);
    if (e == NULL)
      return NULL;
    if (view_->OnElementAdd(e)) {
      e->QueueDraw();
      children_.push_back(e);
    } else {
      delete e;
      e = NULL;
    }
    return e;
  }

  bool InsertElement(BasicElement *element, const BasicElement *before) {
    // firstly erase the element from children, then insert to proper position
    Children::iterator first = std::find(children_.begin(), children_.end(),
                                         element);
    Children::iterator second = std::find(children_.begin(), children_.end(),
                                          before);
    if (first != children_.end()) {
      children_.erase(first);
      second = std::find(children_.begin(), children_.end(), before);
    }
    if (view_->OnElementAdd(element)) {
      element->QueueDraw();
      if (!before || second == children_.end()) {
        children_.push_back(element);
        return true;
      }
      children_.insert(second, element);
    } else {
      return false;
    }
    return true;
  }

  BasicElement *InsertElement(const char *tag_name,
                              const BasicElement *before,
                              const char *name) {
    if (!factory_) return NULL;

    BasicElement *e = factory_->CreateElement(tag_name, owner_, view_, name);
    if (e == NULL)
      return NULL;
    if (view_->OnElementAdd(e)) {
      e->QueueDraw();
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
    element_removed_ = true;
    return true;
  }

  void RemoveAllElements() {
    if (children_.size()) {
      for (Children::iterator ite = children_.begin();
           ite != children_.end(); ++ite) {
        view_->OnElementRemove(*ite);
        delete *ite;
      }
      Children v;
      children_.swap(v);
      element_removed_ = true;
    }
  }

  BasicElement *GetItem(const Variant &index_or_name) {
    switch (index_or_name.type()) {
      case Variant::TYPE_INT64:
        return GetItemByIndex(VariantValue<int>()(index_or_name));
      case Variant::TYPE_STRING:
        return GetItemByName(VariantValue<const char *>()(index_or_name));
      case Variant::TYPE_DOUBLE:
        return GetItemByIndex(
            static_cast<int>(VariantValue<double>()(index_or_name)));
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
        return static_cast<int>(ite - children_.begin());
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

    ElementHolder in_element_holder(*in_element);
    *fired_element = NULL;
    MouseEvent new_event(event);
    // Iterate in reverse since higher elements are listed last.
    for (Children::reverse_iterator ite = children_.rbegin();
         ite != children_.rend(); ++ite) {
      BasicElement *child = *ite;
      // Don't use child->IsReallyVisible() because here we don't need to check
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
        if (!child_holder.Get()) {
          *in_element = in_element_holder.Get();
          return result;
        }
        if (!in_element_holder.Get()) {
          in_element_holder.Reset(descendant_in_element);
        }
        if (*fired_element) {
          *in_element = in_element_holder.Get();
          return result;
        }
      }
    }
    *in_element = in_element_holder.Get();
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
      if (!child->IsReallyVisible())
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
    Children::iterator it = children_.begin();
    Children::iterator end = children_.end();
    bool need_update_extents = element_removed_;
    for (; it != end; ++it) {
      (*it)->Layout();
      if ((*it)->IsPositionChanged() || (*it)->IsSizeChanged())
        need_update_extents = true;
      // Clear the size and position changed state here, because children's
      // Draw() method might not be called.
      (*it)->ClearPositionChanged();
      (*it)->ClearSizeChanged();
    }

    if (scrollable_) {
      if (need_update_extents) {
        width_ = height_ = 0;
        for (it = children_.begin(); it != end; ++it)
          UpdateChildExtent(*it, &width_, &height_);
      }
    } else if (owner_) {
      // If not scrollable, the canvas size is the same as the parent.
      width_ = owner_->GetPixelWidth();
      height_ = owner_->GetPixelHeight();
    } else {
      width_ = view_->GetWidth();
      height_ = view_->GetHeight();
    }

    element_removed_ = false;
  }

  void Draw(CanvasInterface *canvas) {
    ASSERT(canvas);
    if (children_.empty() || !width_ || !height_)
      return;

    size_t child_count = children_.size();

    BasicElement *popup = view_->GetPopupElement();
    for (size_t i = 0; i < child_count; i++) {
      BasicElement *element = children_[i];
      // Doesn't draw popup element here.
      if (element == popup) continue;

      // Doesn't draw elements that outside visible area.
      // Conditions to determine if an element is outside visible area:
      // 1. If it's outside view's clip region
      // 2. If it's outside parent's visible area.
      if (!view_->IsElementInClipRegion(element) ||
          (owner_ && !owner_->IsChildInVisibleArea(element))) {
        //DLOG("pass child: %p(%s)", element, element->GetName().c_str());
        continue;
      }

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

#ifdef _DEBUG
    if (view_->GetDebugMode() & ViewInterface::DEBUG_CONTAINER) {
      // Draw bounding box for debug.
      canvas->DrawLine(0, 0, 0, height_, 1, Color(0, 0, 0));
      canvas->DrawLine(0, 0, width_, 0, 1, Color(0, 0, 0));
      canvas->DrawLine(width_, height_, 0, height_, 1, Color(0, 0, 0));
      canvas->DrawLine(width_, height_, width_, 0, 1, Color(0, 0, 0));
      canvas->DrawLine(0, 0, width_, height_, 1, Color(0, 0, 0));
      canvas->DrawLine(width_, 0, 0, height_, 1, Color(0, 0, 0));
    }
#endif
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
  bool element_removed_;
};

Elements::Elements(ElementFactory *factory,
                   BasicElement *owner, View *view)
    : impl_(new Impl(factory, owner, view)) {
}

void Elements::DoClassRegister() {
  RegisterProperty("count", NewSlot(&Impl::GetCount, &Elements::impl_), NULL);
  RegisterMethod("item", NewSlot(&Impl::GetItem, &Elements::impl_));
  // Register the "default" method, allowing this object be called directly
  // as a function.
  RegisterMethod("", NewSlot(&Impl::GetItem, &Elements::impl_));
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

bool Elements::InsertElement(BasicElement *element,
                             const BasicElement *before) {
  return impl_->InsertElement(element, before);
}

BasicElement *Elements::AppendElementFromXML(const std::string &xml) {
  return InsertElementFromXML(xml, NULL);
}

BasicElement *Elements::InsertElementFromXML(const std::string &xml,
                                             const BasicElement *before) {
  DOMDocumentInterface *xmldoc = GetXMLParser()->CreateDOMDocument();
  xmldoc->Ref();
  Gadget *gadget = impl_->view_->GetGadget();
  bool success = false;
  if (gadget) {
    success = gadget->ParseLocalizedXML(xml, xml.c_str(), xmldoc);
  } else {
    // For unittest. Parse without encoding fallback and localization.
    success = GetXMLParser()->ParseContentIntoDOM(xml, NULL, xml.c_str(),
                                                  NULL, NULL, NULL,
                                                  xmldoc, NULL, NULL);
  }

  BasicElement *result = NULL;
  if (success) {
    DOMElementInterface *xml_element = xmldoc->GetDocumentElement();
    if (!xml_element) {
      LOG("No root element in xml definition: %s", xml.c_str());
    } else {
      // Disable events during parsing XML into Elements.
      impl_->view_->EnableEvents(false);
      result = InsertElementFromDOM(this, impl_->view_->GetScriptContext(),
                                    xml_element, before, "");
      impl_->view_->EnableEvents(true);
    }
  }
  xmldoc->Unref();
  return result;
}

bool Elements::RemoveElement(BasicElement *element) {
  return impl_->RemoveElement(element);
}

void Elements::RemoveAllElements() {
  impl_->RemoveAllElements();
  if (impl_->element_removed_)
    impl_->owner_->QueueDraw();
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
