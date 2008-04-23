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
#include "logger.h"
#include "edit_element_base.h"
#include "elements.h"
#include "event.h"
#include "listbox_element.h"
#include "math_utils.h"
#include "gadget_consts.h"
#include "image_interface.h"
#include "item_element.h"
#include "texture.h"
#include "scriptable_event.h"
#include "view.h"
#include "graphics_interface.h"
#include "element_factory.h"

namespace ggadget {

static const char *kTypeNames[] = {
  "dropdown", "droplist"
};

class ComboBoxElement::Impl {
 public:
  Impl(ComboBoxElement *owner, View *view)
      : owner_(owner),
        mouseover_child_(NULL), grabbed_child_(NULL),
        max_items_(10),
        listbox_(new ListBoxElement(owner, view, "listbox", "")),
        edit_(NULL),
        button_over_(false),
        button_down_(false),
        update_edit_value_(true),
        item_pixel_height_(0),
        button_up_img_(view->LoadImageFromGlobal(kComboArrow, false)),
        button_down_img_(view->LoadImageFromGlobal(kComboArrowDown, false)),
        button_over_img_(view->LoadImageFromGlobal(kComboArrowOver, false)),
        background_(NULL),
        item_cache_(NULL) {
    listbox_->SetPixelX(0);
    listbox_->SetVisible(false);
    listbox_->SetAutoscroll(true);
    listbox_->ConnectOnChangeEvent(NewSlot(this, &Impl::ListBoxUpdated));
    listbox_->SetImplicit(true);
    view->OnElementAdd(listbox_); // ListBox is exposed to the View.

    CreateEdit(); // COMBO_DROPDOWN is default.
  }

  ~Impl() {
    // Close listbox before destroying it to prevent
    // ComboBoxElement::GetPixelHeight() from calling listbox methods.
    listbox_->SetVisible(false);
    owner_->GetView()->OnElementRemove(listbox_);
    delete listbox_;
    delete edit_;
    delete background_;
    DestroyImage(button_up_img_);
    DestroyImage(button_down_img_);
    DestroyImage(button_over_img_);
  }

  std::string GetSelectedText() {
    const ItemElement *item = listbox_->GetSelectedItem();
    if (item) {
      return item->GetLabelText();
    }
    return std::string();
  }

  void SetDroplistVisible(bool visible) {
    if (visible != listbox_->IsVisible()) {
      if (visible) {
        listbox_->ScrollToIndex(listbox_->GetSelectedIndex());
        listbox_->SetVisible(visible);
        owner_->GetView()->SetPopupElement(owner_);
      } else {
        // popup_out handler will turn off listbox
        owner_->GetView()->SetPopupElement(NULL);
      }
      owner_->PostSizeEvent();
    }
  }

  void CreateEdit() {
    ElementFactory *factory = owner_->GetView()->GetElementFactory();
    edit_ = down_cast<EditElementBase*>(
        factory->CreateElement("edit", owner_, owner_->GetView(), ""));
    update_edit_value_ = true;
    if (edit_) {
      edit_->ConnectOnChangeEvent(NewSlot(this, &Impl::TextChanged));
      edit_->SetImplicit(true);
    } else {
      LOG("Failed to create EditElement.");
    }
  }

  void TextChanged() {
    SimpleEvent event(Event::EVENT_CHANGE);
    ScriptableEvent s_event(&event, owner_, NULL);
    owner_->GetView()->FireEvent(&s_event, ontextchange_event_);
  }

  void ListBoxUpdated() {
    owner_->QueueDraw();

    update_edit_value_ = true;

    // Relay this event to combobox's listeners.
    SimpleEvent event(Event::EVENT_CHANGE);
    ScriptableEvent s_event(&event, owner_, NULL);
    owner_->GetView()->FireEvent(&s_event, onchange_event_);
  }

  void SetListBoxHeight() {
    double elem_height = owner_->GetPixelHeight();
    double max_height = max_items_ * item_pixel_height_;
    double height = std::min(max_height, elem_height - item_pixel_height_);
    if (height < 0) {
      height = 0;
    }

    listbox_->SetPixelHeight(height);
  }

  void ScrollList(bool down) {
    int count = listbox_->GetChildren()->GetCount();
    if (count == 0) {
      return;
    }

    int index = listbox_->GetSelectedIndex();
    // Scroll wraps around.
    index += count + (down ? 1 : -1);
    index %= count;
    listbox_->SetSelectedIndex(index);
    listbox_->ScrollToIndex(index);
  }

  ImageInterface *GetButtonImage() {
    if (button_down_) {
      return button_down_img_;
    } else if (button_over_) {
      return button_over_img_;
    } else {
      return button_up_img_;
    }
  }

  void MarkRedraw() {
    if (edit_)
      edit_->MarkRedraw();
    listbox_->MarkRedraw();
  }

  ComboBoxElement *owner_;
  BasicElement *mouseover_child_, *grabbed_child_;
  int max_items_;
  ListBoxElement *listbox_;
  EditElementBase *edit_; // is NULL if and only if COMBO_DROPLIST mode
  bool button_over_, button_down_;
  bool update_edit_value_;
  double item_pixel_height_;
  ImageInterface *button_up_img_, *button_down_img_, *button_over_img_;
  Texture *background_;
  CanvasInterface *item_cache_;
  EventSignal onchange_event_, ontextchange_event_;
};

ComboBoxElement::ComboBoxElement(BasicElement *parent, View *view,
                                 const char *name)
    : BasicElement(parent, view, "combobox", name, false),
      impl_(new Impl(this, view)) {
  SetEnabled(true);
}

void ComboBoxElement::DoRegister() {
  BasicElement::DoRegister();
  RegisterProperty("background",
                   NewSlot(this, &ComboBoxElement::GetBackground),
                   NewSlot(this, &ComboBoxElement::SetBackground));

  // Register container methods since combobox is really a container.
  Elements *elements = impl_->listbox_->GetChildren();
  RegisterConstant("children", elements);
  RegisterMethod("appendElement",
                 NewSlot(elements, &Elements::AppendElementFromXML));
  RegisterMethod("insertElement",
                 NewSlot(elements, &Elements::InsertElementFromXML));
  RegisterMethod("removeElement",
                 NewSlot(elements, &Elements::RemoveElement));
  RegisterMethod("removeAllElements",
                 NewSlot(elements, &Elements::RemoveAllElements));

  RegisterProperty("itemHeight",
                   NewSlot(impl_->listbox_, &ListBoxElement::GetItemHeight),
                   NewSlot(impl_->listbox_, &ListBoxElement::SetItemHeight));
  RegisterProperty("itemWidth",
                   NewSlot(impl_->listbox_, &ListBoxElement::GetItemWidth),
                   NewSlot(impl_->listbox_, &ListBoxElement::SetItemWidth));
  RegisterProperty("itemOverColor",
                   NewSlot(impl_->listbox_, &ListBoxElement::GetItemOverColor),
                   NewSlot(impl_->listbox_, &ListBoxElement::SetItemOverColor));
  RegisterProperty("itemSelectedColor",
                   NewSlot(impl_->listbox_, &ListBoxElement::GetItemSelectedColor),
                   NewSlot(impl_->listbox_, &ListBoxElement::SetItemSelectedColor));
  RegisterProperty("itemSeparator",
                   NewSlot(impl_->listbox_, &ListBoxElement::HasItemSeparator),
                   NewSlot(impl_->listbox_, &ListBoxElement::SetItemSeparator));
  RegisterProperty("selectedIndex",
                   NewSlot(impl_->listbox_, &ListBoxElement::GetSelectedIndex),
                   NewSlot(impl_->listbox_, &ListBoxElement::SetSelectedIndex));
  RegisterProperty("selectedItem",
                   NewSlot(impl_->listbox_, &ListBoxElement::GetSelectedItem),
                   NewSlot(impl_->listbox_, &ListBoxElement::SetSelectedItem));
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
                 NewSlot(impl_->listbox_, &ListBoxElement::ClearSelection));

  // Version 5.5 newly added methods and properties.
  RegisterProperty("itemSeparatorColor",
                   NewSlot(impl_->listbox_,
                           &ListBoxElement::GetItemSeparatorColor),
                   NewSlot(impl_->listbox_,
                           &ListBoxElement::SetItemSeparatorColor));
  RegisterMethod("appendString",
                 NewSlot(impl_->listbox_, &ListBoxElement::AppendString));
  RegisterMethod("insertStringAt",
                 NewSlot(impl_->listbox_, &ListBoxElement::InsertStringAt));
  RegisterMethod("removeString",
                 NewSlot(impl_->listbox_, &ListBoxElement::RemoveString));

  // Disabled
  RegisterProperty("autoscroll",
                   NewSlot(this, &ComboBoxElement::IsAutoscroll),
                   NewSlot(this, &ComboBoxElement::SetAutoscroll));
  RegisterProperty("multiSelect",
                   NewSlot(this, &ComboBoxElement::IsMultiSelect),
                   NewSlot(this, &ComboBoxElement::SetMultiSelect));

  RegisterSignal(kOnChangeEvent, &impl_->onchange_event_);
  RegisterSignal(kOnTextChangeEvent, &impl_->ontextchange_event_);
}

ComboBoxElement::~ComboBoxElement() {
  delete impl_;
  impl_ = NULL;
}

void ComboBoxElement::MarkRedraw() {
  BasicElement::MarkRedraw();
  impl_->MarkRedraw();
}

void ComboBoxElement::DoDraw(CanvasInterface *canvas) {
  bool expanded = impl_->listbox_->IsVisible();
  double elem_width = GetPixelWidth();

  if (impl_->background_) {
    // Crop before drawing background.
    double crop_height = impl_->item_pixel_height_;
    if (expanded) {
      crop_height += impl_->listbox_->GetPixelHeight();
    }
    canvas->IntersectRectClipRegion(0, 0, elem_width, crop_height);
    impl_->background_->Draw(canvas);
  }

  if (impl_->edit_) {
    impl_->edit_->Draw(canvas);
  } else {
    // Draw item
    ItemElement *item = impl_->listbox_->GetSelectedItem();
    // If the selected item is outside the View's clip region, it won't be
    // drawn correctly. In this case the stored canvas cache of this item will
    // be used.
    if (item && GetView()->IsElementInClipRegion(item)) {
      if (!impl_->item_cache_ ||
          impl_->item_cache_->GetHeight() != impl_->item_pixel_height_ ||
          impl_->item_cache_->GetWidth() != elem_width) {
        if (impl_->item_cache_)
          impl_->item_cache_->Destroy();
        impl_->item_cache_ =
            GetView()->GetGraphics()->NewCanvas(elem_width,
                                                impl_->item_pixel_height_);
      } else {
        impl_->item_cache_->ClearCanvas();
      }

      item->SetDrawOverlay(false);
      // Support rotations, masks, etc. here. Windows version supports these,
      // but is this really intended?
      double rotation = item->GetRotation();
      double pinx = item->GetPixelPinX(), piny = item->GetPixelPinY();
      bool transform = (rotation != 0 || pinx != 0 || piny != 0);
      if (transform) {
        impl_->item_cache_->PushState();
        impl_->item_cache_->RotateCoordinates(DegreesToRadians(rotation));
        impl_->item_cache_->TranslateCoordinates(-pinx, -piny);
      }

      item->Draw(impl_->item_cache_);

      if (transform) {
        impl_->item_cache_->PopState();
      }
      item->SetDrawOverlay(true);
    }

    if (impl_->item_cache_)
      canvas->DrawCanvas(0, 0, impl_->item_cache_);
  }

  // Draw button
  ImageInterface *img = impl_->GetButtonImage();
  if (img) {
    double imgw = img->GetWidth();
    double x = elem_width - imgw;
    // Windows default color is 206 203 206 and leaves a 1px margin.
    canvas->DrawFilledRect(x, 1, imgw - 1, impl_->item_pixel_height_ - 2,
                           Color::FromChars(206, 203, 206));
    img->Draw(canvas, x, (impl_->item_pixel_height_ - img->GetHeight()) / 2);
  }

  // Draw listbox
  if (expanded) {
    canvas->TranslateCoordinates(0, impl_->item_pixel_height_);
    impl_->listbox_->Draw(canvas);
  }
}

EditElementBase *ComboBoxElement::GetEdit() {
  return impl_->edit_;
}

const EditElementBase *ComboBoxElement::GetEdit() const {
  return impl_->edit_;
}

ListBoxElement *ComboBoxElement::GetListBox() {
  return impl_->listbox_;
}

const ListBoxElement *ComboBoxElement::GetListBox() const {
  return impl_->listbox_;
}

const Elements *ComboBoxElement::GetChildren() const {
  return impl_->listbox_->GetChildren();
}

Elements *ComboBoxElement::GetChildren() {
  return impl_->listbox_->GetChildren();
}

bool ComboBoxElement::IsDroplistVisible() const {
  return impl_->listbox_->IsVisible();
}

void ComboBoxElement::SetDroplistVisible(bool visible) {
  impl_->SetDroplistVisible(visible);
}

int ComboBoxElement::GetMaxDroplistItems() const {
  return impl_->max_items_;
}

void ComboBoxElement::SetMaxDroplistItems(int max_droplist_items) {
  if (max_droplist_items != impl_->max_items_) {
    impl_->max_items_ = max_droplist_items;
    QueueDraw();
  }
}

ComboBoxElement::Type ComboBoxElement::GetType() const {
  if (impl_->edit_) {
    return COMBO_DROPDOWN;
  } else {
    return COMBO_DROPLIST;
  }
}

void ComboBoxElement::SetType(Type type) {
  if (type == COMBO_DROPDOWN) {
    if (!impl_->edit_) {
      impl_->CreateEdit();
      QueueDraw();
    }
    // It's not necessary to cache selected item in dropdown mode.
    if (impl_->item_cache_) {
      impl_->item_cache_->Destroy();
      impl_->item_cache_ = NULL;
    }
  } else if (impl_->edit_) {
    delete impl_->edit_;
    impl_->edit_ = NULL;
    QueueDraw();
  }
}

std::string ComboBoxElement::GetValue() const {
  if (impl_->edit_) {
    return impl_->edit_->GetValue();
  } else {
    // The release notes are wrong here: the value property can be read 
    // but not modified in droplist mode. 
    return impl_->GetSelectedText();
  }
}

void ComboBoxElement::SetValue(const char *value) {
  if (impl_->edit_) {
    impl_->edit_->SetValue(value);
  }
  // The release notes are wrong here: the value property can be read 
  // but not modified in droplist mode. 
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

void ComboBoxElement::Layout() {
  BasicElement::Layout();
  impl_->item_pixel_height_ = impl_->listbox_->GetItemPixelHeight();
  double elem_width = GetPixelWidth();
  impl_->listbox_->SetPixelY(impl_->item_pixel_height_);
  impl_->listbox_->SetPixelWidth(elem_width);
  impl_->SetListBoxHeight();
  impl_->listbox_->Layout();
  if (impl_->edit_) {
    ImageInterface *img = impl_->GetButtonImage();
    impl_->edit_->SetPixelWidth(elem_width - (img ? img->GetWidth() : 0));
    impl_->edit_->SetPixelHeight(impl_->item_pixel_height_);

    if (impl_->update_edit_value_) {
      impl_->edit_->SetValue(impl_->GetSelectedText().c_str());
    }

    impl_->edit_->Layout();
  }
  impl_->update_edit_value_ = false;
}

EventResult ComboBoxElement::OnMouseEvent(const MouseEvent &event, bool direct,
                                     BasicElement **fired_element,
                                     BasicElement **in_element) {
  BasicElement *new_fired = NULL, *new_in = NULL;
  double new_y = event.GetY() - impl_->listbox_->GetPixelY();
  Event::Type t = event.GetType();
  bool expanded = impl_->listbox_->IsVisible();

  if (!expanded && new_y >= 0 && !direct) {
    // In listbox
    // This combobox will need to appear to be transparent to the elements
    // below it if listbox is invisible.
    return EVENT_RESULT_UNHANDLED;
  }

  if (impl_->edit_) {
    EventResult r;
    if (t == Event::EVENT_MOUSE_OUT && impl_->mouseover_child_) {
      // Case: Mouse moved out of parent and child at same time.
      // Clone mouse out event and send to child in addition to parent.
      MouseEvent new_event(event);
      impl_->mouseover_child_->OnMouseEvent(new_event, true,
                                            &new_fired, &new_in);
      impl_->mouseover_child_ = NULL;

      // Do not return, parent needs to handle this mouse out event too.
    } else if (impl_->grabbed_child_ &&
               (t == Event::EVENT_MOUSE_MOVE || t == Event::EVENT_MOUSE_UP
                || t == Event::EVENT_MOUSE_CLICK)) {
      // Case: Mouse is grabbed by child. Send to child regardless of position.
      // Send to child directly.
      MouseEvent new_event(event);
      r = impl_->grabbed_child_->OnMouseEvent(new_event, true,
                                              fired_element, in_element);
      if (t == Event::EVENT_MOUSE_CLICK) {
        impl_->grabbed_child_->Focus();
      }
      if (t == Event::EVENT_MOUSE_CLICK ||
          !(event.GetButton() & MouseEvent::BUTTON_LEFT)) {
        impl_->grabbed_child_ = NULL;
      }
      // Make editbox invisible to caller
      if (*fired_element == impl_->edit_) {
        *fired_element = this;
      }
      if (*in_element == impl_->edit_) {
        *in_element = this;
      }
      return r;
    } else if (event.GetX() < impl_->edit_->GetPixelWidth() &&
               new_y < 0 && !direct) {
      // !direct is necessary to eliminate events grabbed when clicked on
      // inactive parts of the combobox.
      // Case: Mouse is inside child. Dispatch event to child,
      // except in the case where the event is a mouse over event
      // (when the mouse enters the child and parent at the same time).
      if (!impl_->mouseover_child_) {
        // Case: Mouse just moved inside child. Turn on mouseover bit and
        // synthesize a mouse over event. The original event still needs to
        // be dispatched to child.
        impl_->mouseover_child_ = impl_->edit_;
        MouseEvent in(Event::EVENT_MOUSE_OVER, event.GetX(), event.GetY(),
                      event.GetWheelDeltaX(), event.GetWheelDeltaY(),
                      event.GetButton(), event.GetModifier());
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
      r = impl_->edit_->OnMouseEvent(new_event, direct,
                                     fired_element, in_element);
      // Make child invisible to caller
      if (*fired_element == impl_->edit_) {
        // Only grab events fired on combobox, and not its children
        if (t == Event::EVENT_MOUSE_DOWN &&
            (event.GetButton() & MouseEvent::BUTTON_LEFT)) {
          impl_->grabbed_child_ = impl_->edit_;
        }
        *fired_element = this;
      }
      if (*in_element == impl_->edit_) {
        *in_element = this;
      }
      return r;
    } else if (impl_->mouseover_child_) {
      // Case: Mouse isn't in child, but mouseover bit is on, so turn
      // it off and send a mouse out event to child. The original event is
      // still dispatched to parent.
      MouseEvent new_event(Event::EVENT_MOUSE_OUT, event.GetX(), event.GetY(),
                           event.GetWheelDeltaX(), event.GetWheelDeltaY(),
                           event.GetButton(), event.GetModifier());
      impl_->mouseover_child_->OnMouseEvent(new_event, true,
                                            &new_fired, &new_in);
      impl_->mouseover_child_ = NULL;

      // Don't return, dispatch event to parent.
    }

    // Else not handled, send to BasicElement::OnMouseEvent
  }

  if (expanded && new_y >= 0 && !direct) {
    MouseEvent new_event(event);
    new_event.SetY(new_y);
    return impl_->listbox_->OnMouseEvent(new_event, direct,
                                         fired_element, in_element);
  }

  return  BasicElement::OnMouseEvent(event, direct, fired_element, in_element);
}

EventResult ComboBoxElement::OnDragEvent(const DragEvent &event, bool direct,
                                     BasicElement **fired_element) {
  double new_y = event.GetY() - impl_->listbox_->GetPixelY();
  if (!direct) {
    if (new_y >= 0) {
      // In the listbox region.
      if (impl_->listbox_->IsVisible()) {
        DragEvent new_event(event);
        new_event.SetY(new_y);
        EventResult r = impl_->listbox_->OnDragEvent(new_event,
                                                     direct, fired_element);
        if (*fired_element == impl_->listbox_) {
          *fired_element = this;
        }
        return r;
      } else  {
        // This combobox will need to appear to be transparent to the elements
        // below it if listbox is invisible.
        return EVENT_RESULT_UNHANDLED;
      }
    } else if (impl_->edit_ && event.GetX() < impl_->edit_->GetPixelWidth()) {
      // In editbox.
      EventResult r = impl_->edit_->OnDragEvent(event, direct, fired_element);
      if (*fired_element == impl_->edit_) {
        *fired_element = this;
      }
      return r;
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
  double button_width =
      impl_->button_up_img_ ? impl_->button_up_img_->GetWidth() : 0;
  bool in_button = event.GetY() < impl_->listbox_->GetPixelY() &&
        event.GetX() >= (GetPixelWidth() - button_width);
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
     if (in_button && event.GetButton() & MouseEvent::BUTTON_LEFT) {
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

      // Close dropdown on selection.
      SetDroplistVisible(false);
      break;
     default:
      result = EVENT_RESULT_UNHANDLED;
      break;
    }
  }
  return result;
}

void ComboBoxElement::OnPopupOff() {
  QueueDraw();
  impl_->listbox_->SetVisible(false);
  PostSizeEvent();
}

double ComboBoxElement::GetPixelHeight() const {
  return impl_->listbox_->IsVisible() ? BasicElement::GetPixelHeight() :
         impl_->item_pixel_height_;
}

bool ComboBoxElement::IsChildInVisibleArea(const BasicElement *child) const {
  ASSERT(child);

  if (child == impl_->edit_)
    return true;
  else if (child == impl_->listbox_)
    return impl_->listbox_->IsVisible();

  return impl_->listbox_->IsVisible() &&
         impl_->listbox_->IsChildInVisibleArea(child);
}

bool ComboBoxElement::HasOpaqueBackground() const {
  return impl_->background_ ? impl_->background_->IsFullyOpaque() : false;
}

Connection *ComboBoxElement::ConnectOnChangeEvent(Slot0<void> *slot) {
  return impl_->onchange_event_.Connect(slot);
}

BasicElement *ComboBoxElement::CreateInstance(BasicElement *parent,
                                              View *view,
                                              const char *name) {
  return new ComboBoxElement(parent, view, name);
}

} // namespace ggadget
