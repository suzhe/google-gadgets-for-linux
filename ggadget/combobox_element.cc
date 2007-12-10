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

#include <algorithm>

#include "combobox_element.h"
#include "canvas_interface.h"
#include "edit_element.h"
#include "elements.h"
#include "event.h"
#include "listbox_element.h"
#include "list_elements.h"
#include "math_utils.h"
#include "gadget_consts.h"
#include "image.h"
#include "item_element.h"
#include "texture.h"
#include "scriptable_event.h"
#include "view.h"

namespace ggadget {

static const char kOnChangeEvent[] = "onchange";

static const char *kTypeNames[] = {
  "dropdown", "droplist"
};

class ComboBoxElement::Impl {
 public:
  Impl(ComboBoxElement *owner, View *view)
      : owner_(owner), elements_(NULL), 
        mouseover_child_(NULL), grabbed_child_(NULL),
        type_(COMBO_DROPDOWN), maxitems_(10),
        listbox_(new ListBoxElement(owner, view, "listbox", "")),
        edit_(NULL), // TODO
        button_over_(false), button_down_(false),
        button_up_img_(view->LoadImageFromGlobal(kScrollDefaultRight, false)), 
        button_down_img_(view->LoadImageFromGlobal(kScrollDefaultRightDown, false)), 
        button_over_img_(view->LoadImageFromGlobal(kScrollDefaultRightOver, false)),
        background_(NULL) {
    ASSERT(listbox_->GetChildren()->IsInstanceOf(ListElements::CLASS_ID));
    elements_ = down_cast<ListElements *>(listbox_->GetChildren());

    // Register container methods since combobox is really a container.
    owner_->RegisterConstant("children", elements_);
    owner_->RegisterMethod("appendElement",
                           NewSlot(implicit_cast<Elements *>(elements_), 
                                   &ListElements::AppendElementFromXML));
    owner_->RegisterMethod("insertElement",
                           NewSlot(implicit_cast<Elements *>(elements_), 
                                   &ListElements::InsertElementFromXML));
    owner_->RegisterMethod("removeElement",
                           NewSlot(implicit_cast<Elements *>(elements_), 
                                   &ListElements::RemoveElement));
    owner_->RegisterMethod("removeAllElements",
                           NewSlot(implicit_cast<Elements *>(elements_), 
                                   &ListElements::RemoveAllElements));

    listbox_->SetPixelX(0);
    listbox_->SetVisible(false);
    listbox_->SetAutoscroll(true);
    Slot0<void> *slot = NewSlot(this, &Impl::ListBoxUpdated);
    listbox_->ConnectOnChangeEvent(slot);
    slot = NewSlot(this, &Impl::ListBoxRedraw);
    listbox_->ConnectOnRedrawEvent(slot);

    // TODO edit control

  }

  ~Impl() {
    delete listbox_;
    listbox_ = NULL;

    delete edit_;
    edit_ = NULL;

    delete background_;
    background_ = NULL;
  }

  ComboBoxElement *owner_;
  ListElements *elements_;
  BasicElement *mouseover_child_, *grabbed_child_;
  Type type_;
  int maxitems_;
  ListBoxElement *listbox_;
  EditElement *edit_; // TODO
  bool button_over_, button_down_;
  Image *button_up_img_, *button_down_img_, *button_over_img_;
  Texture *background_;
  EventSignal onchange_event_;

  void ListBoxRedraw() {
    owner_->QueueDraw();
  }

  void ListBoxUpdated() {
    // Relay this event to combobox's listeners.
    Event event(Event::EVENT_CHANGE);
    ScriptableEvent s_event(&event, owner_, NULL);
    owner_->GetView()->FireEvent(&s_event, onchange_event_);
  }

  void SetListBoxHeight() {
    // GetPixelHeight is overridden, so specify BasicElement::
    double elem_height = owner_->BasicElement::GetPixelHeight();
    double item_height = elements_->GetItemPixelHeight();

    double max_height = maxitems_ * item_height;    
    double height = std::min(max_height, elem_height - item_height);
    if (height < 0) {
      height = 0;
    }

    listbox_->SetPixelHeight(height);
  }

  void ScrollList(bool down) {
    int count = elements_->GetCount();
    if (count == 0) {
      return;
    }

    int index = elements_->GetSelectedIndex();
    // Scroll wraps around.
    index += count + (down ? 1 : -1);
    index %= count;
    elements_->SetSelectedIndex(index);
    listbox_->ScrollToIndex(index);
  }
};

ComboBoxElement::ComboBoxElement(BasicElement *parent, View *view,
                                 const char *name)
    : BasicElement(parent, view, "combobox", name, NULL),
      impl_(new Impl(this, view)) {
  SetEnabled(true);  

  RegisterProperty("background",
                   NewSlot(this, &ComboBoxElement::GetBackground),
                   NewSlot(this, &ComboBoxElement::SetBackground));
  RegisterProperty("itemHeight",
                   NewSlot(impl_->elements_, &ListElements::GetItemHeight),
                   NewSlot(this, &ComboBoxElement::SetItemHeight));
  RegisterProperty("itemWidth",
                   NewSlot(impl_->elements_, &ListElements::GetItemWidth),
                   NewSlot(this, &ComboBoxElement::SetItemWidth));
  RegisterProperty("itemOverColor",
                   NewSlot(impl_->elements_, &ListElements::GetItemOverColor),
                   NewSlot(impl_->elements_, &ListElements::SetItemOverColor));
  RegisterProperty("itemSelectedColor",
                   NewSlot(impl_->elements_, &ListElements::GetItemSelectedColor),
                   NewSlot(impl_->elements_, &ListElements::SetItemSelectedColor));
  RegisterProperty("itemSeparator",
                   NewSlot(impl_->elements_, &ListElements::HasItemSeparator),
                   NewSlot(impl_->elements_, &ListElements::SetItemSeparator));
  RegisterProperty("selectedIndex",
                   NewSlot(impl_->elements_, &ListElements::GetSelectedIndex),
                   NewSlot(impl_->elements_, &ListElements::SetSelectedIndex));
  RegisterProperty("selectedItem",
                   NewSlot(impl_->elements_, &ListElements::GetSelectedItem),
                   NewSlot(impl_->elements_, &ListElements::SetSelectedItem));
  RegisterProperty("droplistVisible",
                   NewSlot(this, &ComboBoxElement::IsDroplistVisible),
                   NewSlot(this, &ComboBoxElement::SetDroplistVisible));
  RegisterProperty("maxDroplistItems",
                   NewSlot(this, &ComboBoxElement::GetMaxDroplistItems),
                   NewSlot(this, &ComboBoxElement::SetMaxDroplistItems));
  RegisterProperty("value",
                   NewSlot(this, &ComboBoxElement::GetValue),
                   NewSlot(this, &ComboBoxElement::SetValue));
  RegisterStringEnumProperty("type",
                             NewSlot(this, &ComboBoxElement::GetType),
                             NewSlot(this, &ComboBoxElement::SetType),
                             kTypeNames, arraysize(kTypeNames));

  RegisterMethod("clearSelection",
                 NewSlot(impl_->elements_, &ListElements::ClearSelection));

  // Version 5.5 newly added methods and properties.
  RegisterProperty("itemSeparatorColor",
                   NewSlot(impl_->elements_, &ListElements::GetItemSeparatorColor),
                   NewSlot(impl_->elements_, &ListElements::SetItemSeparatorColor));
  RegisterMethod("appendString",
                 NewSlot(impl_->elements_, &ListElements::AppendString));
  RegisterMethod("insertStringAt",
                 NewSlot(impl_->elements_, &ListElements::InsertStringAt));
  RegisterMethod("removeString",
                 NewSlot(impl_->elements_, &ListElements::RemoveString));

  // Disabled
  RegisterProperty("autoscroll",
                   NewSlot(this, &ComboBoxElement::IsAutoscroll),
                   NewSlot(this, &ComboBoxElement::SetAutoscroll));
  RegisterProperty("multiSelect",
                   NewSlot(this, &ComboBoxElement::IsMultiSelect),
                   NewSlot(this, &ComboBoxElement::SetMultiSelect));

  RegisterSignal(kOnChangeEvent, &impl_->onchange_event_);
}

ComboBoxElement::~ComboBoxElement() {
  delete impl_;
  impl_ = NULL;
}

void ComboBoxElement::DoDraw(CanvasInterface *canvas,
                             const CanvasInterface *children_canvas) {
  bool expanded = impl_->listbox_->IsVisible();
  double item_height = impl_->elements_->GetItemPixelHeight();
  double elem_width = GetPixelWidth();
  bool c; // Used for drawing.

  if (impl_->background_) {
    // Crop before drawing background.
    double crop_height = item_height;
    if (expanded) {
      crop_height += impl_->listbox_->GetPixelHeight();
    }
    canvas->IntersectRectClipRegion(0, 0, elem_width, crop_height);
    impl_->background_->Draw(canvas);
  }

  // TODO dropdown type isn't supported right now

  // Draw item
  ItemElement *item = impl_->elements_->GetSelectedItem();
  if (item) {
    item->SetDrawOverlay(false);
    const CanvasInterface *item_canvas = item->Draw(&c);
    item->SetDrawOverlay(true);

    // Support rotations, masks, etc. here. Windows version supports these, 
    // but is this really intended?
    double rotation = item->GetRotation();
    double pinx = item->GetPixelPinX(), piny = item->GetPixelPinY();
    bool transform = (rotation != 0 || pinx != 0 || piny != 0); 
    if (transform) {
      canvas->PushState();

      canvas->IntersectRectClipRegion(0, 0, elem_width, item_height);
      canvas->RotateCoordinates(DegreesToRadians(rotation));
      canvas->TranslateCoordinates(-pinx, -piny);
    }  

    const CanvasInterface *mask = item->GetMaskCanvas();
    if (mask) {
      canvas->DrawCanvasWithMask(0, 0, item_canvas, 0, 0, mask);
    } else {
      canvas->DrawCanvas(.0, .0, item_canvas);
    }

    if (transform) {
      canvas->PopState();
    }      
  }  

  // Draw button
  Image *img;
  if (impl_->button_down_) {
    img = impl_->button_down_img_;
  } else if (impl_->button_over_) {
    img = impl_->button_over_img_;
  } else {
    img = impl_->button_up_img_;
  }
  if (img) {
    double imgw = img->GetWidth();
    double x = elem_width - imgw;
    // Windows default color is 206 203 206 and leaves a 1px margin.
    canvas->DrawFilledRect(x, 1, imgw - 1, item_height - 2, 
                           Color::ColorFromChars(206, 203, 206));
    img->Draw(canvas, x, (item_height - img->GetHeight()) / 2);
  }

  // Draw listbox
  if (expanded) {
    const CanvasInterface *listbox = impl_->listbox_->Draw(&c);
    canvas->DrawCanvas(0, item_height, listbox);
  } 
}

const ElementsInterface *ComboBoxElement::GetChildren() const {
  return impl_->elements_;
}

ElementsInterface *ComboBoxElement::GetChildren() {
  return impl_->elements_;  
}

double ComboBoxElement::GetPixelHeight() const {
  if (impl_->listbox_->IsVisible()) {
    return BasicElement::GetPixelHeight(); 
  } else {
    return impl_->elements_->GetItemPixelHeight();
  }
}

bool ComboBoxElement::IsDroplistVisible() const {
  return impl_->listbox_->IsVisible();
}

void ComboBoxElement::SetDroplistVisible(bool visible) {
  if (visible != impl_->listbox_->IsVisible()) {
    if (visible) {
      impl_->listbox_->ScrollToIndex(impl_->elements_->GetSelectedIndex());
    }
    impl_->listbox_->SetVisible(visible);
  }
}

int ComboBoxElement::GetMaxDroplistItems() const {
  return impl_->maxitems_;
}

void ComboBoxElement::SetMaxDroplistItems(int max_droplist_items) {
  if (max_droplist_items != impl_->maxitems_) {
    impl_->maxitems_ = max_droplist_items;
    QueueDraw();
    impl_->SetListBoxHeight();
  }
}

ComboBoxElement::Type ComboBoxElement::GetType() const {
  return impl_->type_;
}

void ComboBoxElement::SetType(Type type) {
  if (type != impl_->type_) {
    impl_->type_ = type;
    // TODO
    QueueDraw();
  }
}

const char *ComboBoxElement::GetValue() const {
  const ItemElement *item = impl_->elements_->GetSelectedItem();
  if (item) {
    return item->GetLabelText();
  }
  LOG("ComboBox: No item selected");
  return NULL;
}

void ComboBoxElement::SetValue(const char *value) {
  ItemElement *item = impl_->elements_->GetSelectedItem();
  if (item) {
    item->SetLabelText(value);
  } else {
    LOG("ComboBox: No item selected");
  }
}

bool ComboBoxElement::IsAutoscroll() const {
  return false; // Disabled
}

void ComboBoxElement::SetAutoscroll(bool autoscroll) {
  // Disabled
}

bool ComboBoxElement::IsMultiSelect() const {
  return false; // Disabled
}

void ComboBoxElement::SetMultiSelect(bool multiselect) {
  // Disabled
}

Variant ComboBoxElement::GetBackground() const {
  return Variant(Texture::GetSrc(impl_->background_));
}

void ComboBoxElement::SetBackground(const Variant &background) {
  delete impl_->background_;
  impl_->background_ = GetView()->LoadTexture(background);
  QueueDraw();
}

void ComboBoxElement::SetItemWidth(const Variant &width) {
  impl_->elements_->SetItemWidth(width);
  QueueDraw();
}

void ComboBoxElement::SetItemHeight(const Variant &height) {
  impl_->elements_->SetItemHeight(height);
  QueueDraw();
}

void ComboBoxElement::Layout() {
  BasicElement::Layout();
  impl_->listbox_->SetPixelY(impl_->elements_->GetItemPixelHeight());
  impl_->listbox_->SetPixelWidth(GetPixelWidth());
  impl_->SetListBoxHeight();
  impl_->listbox_->Layout();
  // TODO: Call layout of edit.
}

void ComboBoxElement::SelfCoordToChildCoord(const BasicElement *child,
                                       double x, double y,
                                       double *child_x, double *child_y) const {  
  //TODO
  BasicElement::SelfCoordToChildCoord(child, x, y, child_x, child_y);
}

EventResult ComboBoxElement::OnMouseEvent(const MouseEvent &event, bool direct,
                                     BasicElement **fired_element,
                                     BasicElement **in_element) {
  BasicElement *new_fired = NULL, *new_in = NULL;
  double new_y = event.GetY() - impl_->listbox_->GetPixelY();
  Event::Type t = event.GetType();

  if (impl_->listbox_->IsVisible()) {
    EventResult r;    
    if (t == Event::EVENT_MOUSE_OUT && impl_->mouseover_child_) {
      // Case: Mouse moved out of parent and child at same time.
      // Clone mouse out event and send to child in addition to parent.
      MouseEvent new_event(event);
      new_event.SetY(new_y);
      impl_->mouseover_child_->OnMouseEvent(new_event, true, 
                                            &new_fired, &new_in);
      impl_->mouseover_child_ = NULL;

      // Do not return, parent needs to handle this mouse out event too.
    } else if (impl_->grabbed_child_ &&  
               (t == Event::EVENT_MOUSE_MOVE || t == Event::EVENT_MOUSE_UP)) { 
      // Case: Mouse is grabbed by child. Send to child regardless of position.
      // Send to child directly.
      MouseEvent new_event(event);
      new_event.SetY(new_y);
      r = impl_->grabbed_child_->OnMouseEvent(new_event, true, 
                                              fired_element, in_element);
      if (t == Event::EVENT_MOUSE_UP) {
        impl_->grabbed_child_ = NULL; 
      }
      // Make listbox invisible to caller
      if (*fired_element == impl_->listbox_) {
        *fired_element = this;
      }
      if (*in_element == impl_->listbox_) {
        *in_element = this;
      }
      return r;
    } else if (new_y >= 0 && !direct) { 
      // !direct is necessary to eliminate events grabbed when clicked on 
      // inactive parts of the combobox.
      // Case: Mouse is inside child. Dispatch event to child,
      // except in the case where the event is a mouse over event 
      // (when the mouse enters the child and parent at the same time).
      if (!impl_->mouseover_child_) {
        // Case: Mouse just moved inside child. Turn on mouseover bit and  
        // synthesize a mouse over event. The original event still needs to 
        // be dispatched to child.
        impl_->mouseover_child_ = impl_->listbox_;
        MouseEvent in(Event::EVENT_MOUSE_OVER, event.GetX(), new_y, 
                      event.GetButton(), event.GetWheelDelta(), 
                      event.GetModifier());
        impl_->mouseover_child_->OnMouseEvent(in, true, &new_fired, &new_in);
        // Ignore return from handler and don't return to continue processing.
        if (t == Event::EVENT_MOUSE_OVER) {
          // Case: Mouse entered child and parent at same time.
          // Parent also needs this event, so send to parent
          // and return.
          return BasicElement::OnMouseEvent(event, direct,  
                                            fired_element, in_element);         
        } 
      }
      // Send event to child.
      MouseEvent new_event(event);
      new_event.SetY(new_y);
      r = impl_->listbox_->OnMouseEvent(new_event, direct,
                                        fired_element, in_element);
      // Make listbox invisible to caller
      if (*fired_element == impl_->listbox_) {
        // Only grab events fired on combobox, and not its children 
        if (t == Event::EVENT_MOUSE_DOWN) {
          impl_->grabbed_child_ = impl_->listbox_;
        }
        *fired_element = this;
      }
      if (*in_element == impl_->listbox_) {
        *in_element = this;
      }
      if (r == EVENT_RESULT_HANDLED && t == Event::EVENT_MOUSE_CLICK &&
          impl_->listbox_->IsVisible()) {
        // Close dropdown on selection.
        impl_->listbox_->SetVisible(false);
      }
      return r;
    } else if (impl_->mouseover_child_) {
      // Case: Mouse isn't in child, but mouseover bit is on, so turn 
      // it off and send a mouse out event to child. The original event is 
      // still dispatched to parent.
      MouseEvent new_event(Event::EVENT_MOUSE_OUT, event.GetX(), new_y, 
                           event.GetButton(), event.GetWheelDelta(),
                           event.GetModifier());
      impl_->mouseover_child_->OnMouseEvent(new_event, true, 
                                            &new_fired, &new_in);
      impl_->mouseover_child_ = NULL;

      // Don't return, dispatch event to parent.
    }

    // Else not handled, send to BasicElement::OnMouseEvent
  } else {
    // Visible state may change while grabbed or hovered.
    impl_->grabbed_child_ = NULL;
    impl_->mouseover_child_ = NULL;

    if (new_y >= 0) {
      // In listbox
      if (!direct) {        
        // This combobox will need to appear to be transparent to the elements 
        // below it if listbox is invisible.      
        return EVENT_RESULT_UNHANDLED;
      }
    }    
  }

  return BasicElement::OnMouseEvent(event, direct, fired_element, in_element);
}

EventResult ComboBoxElement::OnDragEvent(const DragEvent &event, bool direct,
                                     BasicElement **fired_element) {
  double new_y = event.GetY() - impl_->listbox_->GetPixelY();
  if (!direct && new_y >= 0) {
    // In the listbox region.
    if (impl_->listbox_->IsVisible()) {
      DragEvent new_event(event);
      new_event.SetY(new_y);
      EventResult r = impl_->listbox_->OnDragEvent(new_event, direct, fired_element);
      if (*fired_element == impl_->listbox_) {
        *fired_element = this;
      }
      return r;
    } else  {
      // This combobox will need to appear to be transparent to the elements 
      // below it if listbox is invisible.
      return EVENT_RESULT_UNHANDLED;
    }    
  }

  return BasicElement::OnDragEvent(event, direct, fired_element);
}

EventResult ComboBoxElement::HandleMouseEvent(const MouseEvent &event) {
  // Only events NOT in the listbox region are ever passed to this handler.
  // So it's save to assume that these events are not for the listbox, with the 
  // exception of mouse wheel events.
  EventResult r = EVENT_RESULT_HANDLED;
  bool oldvalue;
  bool in_button = event.GetY() < impl_->listbox_->GetPixelY() &&
        event.GetX() >= (GetPixelWidth() - impl_->button_up_img_->GetWidth());
  switch (event.GetType()) {
    case Event::EVENT_MOUSE_MOVE:
      r = EVENT_RESULT_UNHANDLED;
      // Fall through.
    case Event::EVENT_MOUSE_OVER:
      oldvalue = impl_->button_over_;
      impl_->button_over_ = in_button;
      if (oldvalue != impl_->button_over_) {
        QueueDraw();
      }    
     break;
    case Event::EVENT_MOUSE_UP:
     if (impl_->button_down_) {
       impl_->button_down_ = false;
       QueueDraw();
     }
     break;    
    case Event::EVENT_MOUSE_DOWN:
     if (in_button) {
       impl_->button_down_ = true;
       QueueDraw();
     }
     break;
    case Event::EVENT_MOUSE_CLICK:
      // Toggle droplist visibility.
      SetDroplistVisible(!impl_->listbox_->IsVisible());
     break;
    case Event::EVENT_MOUSE_OUT:
     if (impl_->button_over_) {
       impl_->button_over_ = false;
       QueueDraw();
     }
     break;
    case Event::EVENT_MOUSE_WHEEL: 
     if (impl_->listbox_->IsVisible()) {
       r = impl_->listbox_->HandleMouseEvent(event);
     }
     break;   
   default:
    r = EVENT_RESULT_UNHANDLED;
    break; 
  }

  return r;
}

EventResult ComboBoxElement::HandleKeyEvent(const KeyboardEvent &event) {
  EventResult result = EVENT_RESULT_UNHANDLED;
  if (event.GetType() == Event::EVENT_KEY_DOWN) {
    result = EVENT_RESULT_HANDLED;
    switch (event.GetKeyCode()) {
     case KeyboardEvent::KEY_UP:
      impl_->ScrollList(false);
      break;
     case KeyboardEvent::KEY_DOWN:
      impl_->ScrollList(true);
      break;
     case KeyboardEvent::KEY_RETURN:
       // Windows only allows the box to be closed with the enter key,
       // not opened. Weird.
      if (impl_->listbox_->IsVisible()) {
        // Close dropdown on selection.
        impl_->listbox_->SetVisible(false);
      }
      break;
     default:
      result = EVENT_RESULT_UNHANDLED;
      break;
    }
  }
  return result;
}

BasicElement *ComboBoxElement::CreateInstance(BasicElement *parent,
                                              View *view,
                                              const char *name) {
  return new ComboBoxElement(parent, view, name);
}

} // namespace ggadget
