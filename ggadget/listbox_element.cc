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

#include <cmath>

#include "listbox_element.h"
#include "color.h"
#include "elements_interface.h"
#include "event.h"
#include "item_element.h"
#include "logger.h"
#include "scriptable_event.h"
#include "string_utils.h"
#include "texture.h"
#include "view.h"

namespace ggadget {

static const char kErrorItemExpected[] =
  "Incorrect element type: Item/ListItem expected.";

// Default values obtained from the Windows version.
static const Color kDefaultItemOverColor(0xDE/255.0, 0xFB/255.0, 1.0);
static const Color kDefaultItemSelectedColor(0xC6/255.0,
                                             0xF7/255.0,
                                             0xF7/255.0);
static const Color kDefaultItemSepColor(0xF7/255.0, 0xF3/255.0, 0xF7/255.0);

class ListBoxElement::Impl {
 public:
  Impl(ListBoxElement *owner, View *view) : 
    owner_(owner),
    item_width_(0), item_height_(0),
    item_width_specified_(false), item_height_specified_(false),
    item_width_relative_(false), item_height_relative_(false),
    multiselect_(false), item_separator_(false),
    selected_index_(-2), 
    item_over_color_(new Texture(kDefaultItemOverColor, 1.0)),
    item_selected_color_(new Texture(kDefaultItemSelectedColor, 1.0)),
    item_separator_color_(new Texture(kDefaultItemSepColor, 1.0)) { 
  }

  ~Impl() {
    delete item_over_color_;
    item_over_color_ = NULL;
    delete item_selected_color_;
    item_selected_color_ = NULL;
    delete item_separator_color_;
    item_separator_color_ = NULL;
  }

  void SetPixelItemWidth(double width) {
    if (width >= 0.0 && 
        (width != item_width_ || item_width_relative_)) {
      item_width_ = width;
      item_width_relative_ = false;
      owner_->QueueDraw();
    }
  }

  void SetPixelItemHeight(double height) {
    if (height >= 0.0 && 
        (height != item_height_ || item_height_relative_)) {
      item_height_ = height;
      item_height_relative_ = false;
      owner_->QueueDraw();
    }
  }

  void SetRelativeItemWidth(double width) {
    if (width >= 0.0 && (width != item_width_ || !item_width_relative_)) {
      item_width_ = width;
      item_width_relative_ = true;
      owner_->QueueDraw();
    }
  }

  void SetRelativeItemHeight(double height) {
    if (height >= 0.0 &&
        (height != item_height_ || !item_height_relative_)) {
      item_height_ = height;
      item_height_relative_ = true;
      owner_->QueueDraw();
    }
  }

  void SetPendingSelection() {
    ElementInterface *selected = 
      owner_->GetChildren()->GetItemByIndex(selected_index_);
    if (selected) {
      if (selected->IsInstanceOf(ItemElement::CLASS_ID)) {
        ItemElement *item = down_cast<ItemElement *>(selected);
        item->SetSelected(true);
      } else {
        LOG(kErrorItemExpected);
      }

      selected_index_ = -1;
    }
  }

  // Returns true if anything was cleared.
  bool ClearSelection(ItemElement *avoid) {   
    bool result = false;
    ElementsInterface *elements = owner_->GetChildren();
    int childcount = elements->GetCount();
    for (int i = 0; i < childcount; i++) {
      ElementInterface *child = elements->GetItemByIndex(i);
      if (child != avoid) {
        if (child->IsInstanceOf(ItemElement::CLASS_ID)) {
          ItemElement *item = down_cast<ItemElement *>(child);
          if (item->IsSelected()) {
            result = true;
            item->SetSelected(false);
          }
        } else {
          LOG(kErrorItemExpected);
        }
      }
    }  
    return result;
  }

  void FireOnChangeEvent() {
    SimpleEvent event(Event::EVENT_CHANGE);
    ScriptableEvent s_event(&event, owner_, NULL);
    owner_->GetView()->FireEvent(&s_event, onchange_event_);
  }

  ItemElement *FindItemByString(const char *str) {
    ElementsInterface *elements = owner_->GetChildren();
    int childcount = elements->GetCount();
    for (int i = 0; i < childcount; i++) {
      ElementInterface *child = elements->GetItemByIndex(i);
      if (child->IsInstanceOf(ItemElement::CLASS_ID)) {
        ItemElement *item = down_cast<ItemElement *>(child);
        std::string text = item->GetLabelText();
        if (text == str) {
          return item;
        }
      }
    }
    return NULL;
  }

  ListBoxElement *owner_;
  double item_width_, item_height_;
  bool item_width_specified_, item_height_specified_;
  bool item_width_relative_, item_height_relative_;
  bool multiselect_, item_separator_;
  // Only used for when the index is specified in XML. This is an index 
  // of an element "pending" to become selected. Initialized to -2.
  int selected_index_;
  Texture *item_over_color_, *item_selected_color_, *item_separator_color_;
  EventSignal onchange_event_;
};

ListBoxElement::ListBoxElement(BasicElement *parent, View *view,
                               const char *tag_name, const char *name)
    : DivElement(parent, view, tag_name, name),
      impl_(new Impl(this, view)) {
  SetEnabled(true);  

  RegisterProperty("background",
                   NewSlot(implicit_cast<DivElement *>(this),
                           &DivElement::GetBackground),
                   NewSlot(implicit_cast<DivElement *>(this),
                           &DivElement::SetBackground));
  RegisterProperty("autoscroll",
                   NewSlot(implicit_cast<ScrollingElement *>(this), 
                           &ScrollingElement::IsAutoscroll),
                   NewSlot(implicit_cast<ScrollingElement *>(this), 
                           &ScrollingElement::SetAutoscroll));
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

  RegisterMethod("clearSelection",
                 NewSlot(this, &ListBoxElement::ClearSelection));

  // Version 5.5 newly added methods and properties.
  RegisterProperty("itemSeparatorColor",
                   NewSlot(this, &ListBoxElement::GetItemSeparatorColor),
                   NewSlot(this, &ListBoxElement::SetItemSeparatorColor));
  RegisterMethod("appendString",
                 NewSlot(this, &ListBoxElement::AppendString));
  RegisterMethod("insertStringAt",
                 NewSlot(this, &ListBoxElement::InsertStringAt));
  RegisterMethod("removeString",
                 NewSlot(this, &ListBoxElement::RemoveString));

  RegisterSignal(kOnChangeEvent, &impl_->onchange_event_);
}

ListBoxElement::~ListBoxElement() {
  delete impl_;
  impl_ = NULL;
}

void ListBoxElement::ScrollToIndex(int index) {
  SetScrollYPosition(index * 
                     static_cast<int>(GetItemPixelHeight()));
}

Connection *ListBoxElement::ConnectOnChangeEvent(Slot0<void> *slot) {
  return impl_->onchange_event_.Connect(slot);
}

EventResult ListBoxElement::OnMouseEvent(const MouseEvent &event, bool direct,
                                     BasicElement **fired_element,
                                     BasicElement **in_element) {
  // Interecept mouse wheel events from Item elements and send to Div 
  // directly to enable wheel scrolling.
  bool wheel = (event.GetType() == Event::EVENT_MOUSE_WHEEL);
  return DivElement::OnMouseEvent(event, wheel ? true : direct, 
                                  fired_element, in_element);  
}

Variant ListBoxElement::GetItemWidth() const {
  return BasicElement::GetPixelOrRelative(impl_->item_width_relative_, 
                                          impl_->item_width_specified_,
                                          impl_->item_width_, 
                                          impl_->item_width_);
}

void ListBoxElement::SetItemWidth(const Variant &width) {
  double v;
  switch (BasicElement::ParsePixelOrRelative(width, &v)) {
    case BasicElement::PR_PIXEL:
      impl_->item_width_specified_ = true;
      impl_->SetPixelItemWidth(v);
      break;
    case BasicElement::PR_RELATIVE:
      impl_->item_width_specified_ = true;
      impl_->SetRelativeItemWidth(v);
      break;
    case BasicElement::PR_UNSPECIFIED:
      impl_->item_width_specified_ = false;
      impl_->SetPixelItemWidth(0);
      break;
    default:
      break;
  }
}

Variant ListBoxElement::GetItemHeight() const {
  return BasicElement::GetPixelOrRelative(impl_->item_height_relative_, 
                                          impl_->item_height_specified_,
                                          impl_->item_height_, 
                                          impl_->item_height_);
}

void ListBoxElement::SetItemHeight(const Variant &height) {
  double v;
  switch (BasicElement::ParsePixelOrRelative(height, &v)) {
    case BasicElement::PR_PIXEL:
      impl_->item_height_specified_ = true;
      impl_->SetPixelItemHeight(v);
      break;
    case BasicElement::PR_RELATIVE:
      impl_->item_height_specified_ = true;
      impl_->SetRelativeItemHeight(v);
      break;
    case BasicElement::PR_UNSPECIFIED:
      impl_->item_height_specified_ = false;
      impl_->SetPixelItemHeight(0);
      break;
    default:
      break;
  }
}

double ListBoxElement::GetItemPixelWidth() const {
  return impl_->item_width_relative_ ?
         impl_->item_width_ * GetClientWidth() : impl_->item_width_;
}

double ListBoxElement::GetItemPixelHeight() const {
  return impl_->item_height_relative_ ?
         impl_->item_height_ * GetClientHeight() : impl_->item_height_;
}

Variant ListBoxElement::GetItemOverColor() const {
  return Variant(Texture::GetSrc(impl_->item_over_color_));
}

const Texture *ListBoxElement::GetItemOverTexture() const {
  return impl_->item_over_color_;
}

void ListBoxElement::SetItemOverColor(const Variant &color) {
  delete impl_->item_over_color_;
  impl_->item_over_color_ = GetView()->LoadTexture(color);

  ElementsInterface *elements = GetChildren();
  int childcount = elements->GetCount();
  for (int i = 0; i < childcount; i++) {
    ElementInterface *child = elements->GetItemByIndex(i);
    if (child->IsInstanceOf(ItemElement::CLASS_ID)) {
      ItemElement *item = down_cast<ItemElement *>(child);
      if (item->IsMouseOver()) {
        item->QueueDraw();
        break;
      }
    } else {
      LOG(kErrorItemExpected);
    }
  }  
}

Variant ListBoxElement::GetItemSelectedColor() const {
  return Variant(Texture::GetSrc(impl_->item_selected_color_));
}

const Texture *ListBoxElement::GetItemSelectedTexture() const {
  return impl_->item_selected_color_;
}

void ListBoxElement::SetItemSelectedColor(const Variant &color) {
  delete impl_->item_selected_color_;
  impl_->item_selected_color_ = GetView()->LoadTexture(color);

  ElementsInterface *elements = GetChildren();
  int childcount = elements->GetCount();
  for (int i = 0; i < childcount; i++) {
    ElementInterface *child = elements->GetItemByIndex(i);
    if (child->IsInstanceOf(ItemElement::CLASS_ID)) {
      ItemElement *item = down_cast<ItemElement *>(child);
      if (item->IsSelected()) {
        item->QueueDraw();
      }
    } else {
      LOG(kErrorItemExpected);
    }
  }  
}

Variant ListBoxElement::GetItemSeparatorColor() const {
  return Variant(Texture::GetSrc(impl_->item_separator_color_));
}

const Texture *ListBoxElement::GetItemSeparatorTexture() const {
  return impl_->item_separator_color_;
}

void ListBoxElement::SetItemSeparatorColor(const Variant &color) {
  delete impl_->item_separator_color_;
  impl_->item_separator_color_ = GetView()->LoadTexture(color);

  ElementsInterface *elements = GetChildren();
  int childcount = elements->GetCount();
  for (int i = 0; i < childcount; i++) {
    ElementInterface *child = elements->GetItemByIndex(i);
    if (child->IsInstanceOf(ItemElement::CLASS_ID)) {
      ItemElement *item = down_cast<ItemElement *>(child);
      item->QueueDraw();
    } else {
      LOG(kErrorItemExpected);
    }
  }
}

bool ListBoxElement::HasItemSeparator() const {
  return impl_->item_separator_;
}

void ListBoxElement::SetItemSeparator(bool separator) {
  if (separator != impl_->item_separator_) {
    impl_->item_separator_ = separator;

    ElementsInterface *elements = GetChildren();
    int childcount = elements->GetCount();
    for (int i = 0; i < childcount; i++) {
      ElementInterface *child = elements->GetItemByIndex(i);
      if (child->IsInstanceOf(ItemElement::CLASS_ID)) {
        ItemElement *item = down_cast<ItemElement *>(child);
        item->QueueDraw();
      } else {
        LOG(kErrorItemExpected);
      }
    }
  }
}

bool ListBoxElement::IsMultiSelect() const {
  return impl_->multiselect_;
}

void ListBoxElement::SetMultiSelect(bool multiselect) {
  impl_->multiselect_ = multiselect; // No need to redraw.
}

int ListBoxElement::GetSelectedIndex() const {
  const ElementsInterface *elements = GetChildren();
  int childcount = elements->GetCount();
  for (int i = 0; i < childcount; i++) {
    const ElementInterface *child = elements->GetItemByIndex(i);
    if (child->IsInstanceOf(ItemElement::CLASS_ID)) {
      const ItemElement *item = down_cast<const ItemElement *>(child);
      if (item->IsSelected()) {
        return i;
      }
    } else {
      LOG(kErrorItemExpected);
    }
  }  

  if (impl_->selected_index_ >= 0) {
    return impl_->selected_index_;
  }

  return -1;
}

void ListBoxElement::SetSelectedIndex(int index) {
  ElementInterface *item = GetChildren()->GetItemByIndex(index);
  if (!item) {
    if (impl_->selected_index_ == -2) { // Mark selection as pending.
      impl_->selected_index_ = index;
    }
    return;
  }

  if (item->IsInstanceOf(ItemElement::CLASS_ID)) {
    SetSelectedItem(down_cast<ItemElement *>(item));  
  } else {
    LOG(kErrorItemExpected);
  }  
}

ItemElement *ListBoxElement::GetSelectedItem() {
  ElementsInterface *elements = GetChildren();
  int childcount = elements->GetCount();
  for (int i = 0; i < childcount; i++) {
    ElementInterface *child = elements->GetItemByIndex(i);
    if (child->IsInstanceOf(ItemElement::CLASS_ID)) {
      ItemElement *item = down_cast<ItemElement *>(child);
      if (item->IsSelected()) {
        return item;
      }
    } else {
      LOG(kErrorItemExpected);
    }
  }  

  return NULL;  
}

const ItemElement *ListBoxElement::GetSelectedItem() const {
  const ElementsInterface *elements = GetChildren();
  int childcount = elements->GetCount();
  for (int i = 0; i < childcount; i++) {
    const ElementInterface *child = elements->GetItemByIndex(i);
    if (child->IsInstanceOf(ItemElement::CLASS_ID)) {
      const ItemElement *item = down_cast<const ItemElement *>(child);
      if (item->IsSelected()) {
        return item;
      }
    } else {
      LOG(kErrorItemExpected);
    }
  }  

  return NULL;  
}

void ListBoxElement::SetSelectedItem(ItemElement *item) {
  bool changed = impl_->ClearSelection(item);
  if (item && !item->IsSelected()) {
    item->SetSelected(true);
    changed = true;
  }

  if (changed) {
    impl_->FireOnChangeEvent();
  }
}

void ListBoxElement::ClearSelection() {
  bool changed = impl_->ClearSelection(NULL);
  if (changed) {
    impl_->FireOnChangeEvent();
  }
}

void ListBoxElement::AppendSelection(ItemElement *item) {
  ASSERT(item);

  if (!impl_->multiselect_) {
    SetSelectedItem(item);
    return;
  }

  if (!item->IsSelected()) {
    item->SetSelected(true);
    impl_->FireOnChangeEvent();
  }
}

void ListBoxElement::SelectRange(ItemElement *endpoint) {
  ASSERT(endpoint);

  if (!impl_->multiselect_) {
    SetSelectedItem(endpoint);
    return;
  }

  bool changed = false;
  ItemElement *endpoint2 = GetSelectedItem();
  if (endpoint2 == NULL || endpoint == endpoint2) {
    if (!endpoint->IsSelected()) {
      changed = true;
      endpoint->SetSelected(true);
    }
  } else {
    bool started = false;
    ElementsInterface *elements = GetChildren();
    int childcount = elements->GetCount();
    for (int i = 0; i < childcount; i++) {
      ElementInterface *child = elements->GetItemByIndex(i);
      if (child->IsInstanceOf(ItemElement::CLASS_ID)) {
        ItemElement *item = down_cast<ItemElement *>(child);
        if (item == endpoint || item == endpoint2) {
          started = !started;
          if (!started) { // started has just been turned off
            if (!item->IsSelected()) {
              item->SetSelected(true);
              changed = true;
            }
            break;
          }
        }       

        if (started && !item->IsSelected()) {
          item->SetSelected(true);
          changed = true;
        }
      } else {
        LOG(kErrorItemExpected);
      }
    }      
  }

  if (changed) {
    impl_->FireOnChangeEvent();
  }
}

bool ListBoxElement::AppendString(const char *str) {
  ElementsInterface *elements = GetChildren();
  ElementInterface *child = elements->AppendElement("item", "");
  if (!child) {
    return false;
  }

  ASSERT(child->IsInstanceOf(ItemElement::CLASS_ID));
  ItemElement *item = down_cast<ItemElement *>(child);
  bool result = item->AddLabelWithText(str);
  if (!result) {
    // Cleanup on failure.
    elements->RemoveElement(child);
  }    
  return result;
}

bool ListBoxElement::InsertStringAt(const char *str, int index) {
  ElementsInterface *elements = GetChildren();
  if (elements->GetCount() == index) {
    return AppendString(str);
  }

  const ElementInterface *before = elements->GetItemByIndex(index);
  if (!before) {
    return false;
  }

  ElementInterface *child = elements->InsertElement("item", before, ""); 
  if (!child) {
    return false;
  }

  ASSERT(child->IsInstanceOf(ItemElement::CLASS_ID));
  ItemElement *item = down_cast<ItemElement *>(child);
  bool result = item->AddLabelWithText(str);  
  if (!result) {
    // Cleanup on failure.
    elements->RemoveElement(child);
  }    
  return result;
}

void ListBoxElement::RemoveString(const char *str) {
  ItemElement *item = FindItemByString(str);
  if (item) {
    GetChildren()->RemoveElement(item);
  }
}

void ListBoxElement::Layout() { 
  impl_->SetPendingSelection();
  // This field is no longer used after the first layout.
  impl_->selected_index_ = -1;

  // Inform children (items) of their index.
  ElementsInterface *elements = GetChildren();
  int childcount = elements->GetCount();
  for (int i = 0; i < childcount; i++) {
    ElementInterface *child = elements->GetItemByIndex(i);
    if (child->IsInstanceOf(ItemElement::CLASS_ID)) {
      ItemElement *item = down_cast<ItemElement *>(child);
      item->SetIndex(i);
    } else {
      LOG(kErrorItemExpected);
    }
  }

  // Call parent Layout() after SetIndex().
  DivElement::Layout();

  // No need to destroy items_canvas_ here, since Draw() will calculate
  // the size required and resize it if necessary.
}

ItemElement *ListBoxElement::FindItemByString(const char *str) {
  return impl_->FindItemByString(str);
}

const ItemElement *ListBoxElement::FindItemByString(const char *str) const {
  return impl_->FindItemByString(str);
}

BasicElement *ListBoxElement::CreateInstance(BasicElement *parent, View *view,
                                             const char *name) {
  return new ListBoxElement(parent, view, "listbox", name);
}

} // namespace ggadget
