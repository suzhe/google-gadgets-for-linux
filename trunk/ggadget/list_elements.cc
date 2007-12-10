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

#include "list_elements.h"
#include "listbox_element.h"
#include "item_element.h"
#include "basic_element.h"
#include "common.h"
#include "element_factory_interface.h"
#include "graphics_interface.h"
#include "math_utils.h"
#include "scriptable_helper.h"
#include "view.h"
#include "texture.h"

namespace ggadget {

static const char kErrorItemExpected[] = 
  "Incorrect element type: Item/ListItem expected.";

// Default values obtained from the Windows version.
static const char kDefaultItemOverColor[] = "#DEFBFF";
static const char kDefaultItemSelectedColor[] = "#C6F7F7";
static const char kDefaultItemSepColor[] = "#F7F3F7";

class ListElements::Impl {
 public:
  Impl(ListBoxElement *parent, ListElements *owner, View *view)
      : parent_(parent), owner_(owner),
        pixel_item_width_(0), pixel_item_height_(0),
        rel_item_width_(0), rel_item_height_(0),
        item_width_specified_(false), item_height_specified_(false),
        item_width_relative_(false), item_height_relative_(false),
        multiselect_(false), item_separator_(false), separator_changed_(true),
        selected_index_(-2), items_canvas_(NULL), 
        item_over_color_(view->LoadTexture(Variant(kDefaultItemOverColor))),
        item_selected_color_(view->LoadTexture(Variant(kDefaultItemSelectedColor))),
        item_separator_color_(view->LoadTexture(Variant(kDefaultItemSepColor))) {
  }

  ~Impl() {
    delete item_over_color_;
    item_over_color_ = NULL;
    delete item_selected_color_;
    item_selected_color_ = NULL;  
    delete item_separator_color_;
    item_separator_color_ = NULL;

    if (items_canvas_) {
      items_canvas_->Destroy();
    }
    items_canvas_ = NULL;
  }

  ListBoxElement *parent_;
  ListElements *owner_;
  double pixel_item_width_, pixel_item_height_;
  double rel_item_width_, rel_item_height_;
  bool item_width_specified_, item_height_specified_;
  bool item_width_relative_, item_height_relative_;
  bool multiselect_, item_separator_, separator_changed_;
  // Only used for when the index is specified in XML. This is an index 
  // of an element "pending" to become selected. Initialized to -2.
  int selected_index_;
  CanvasInterface *items_canvas_;
  Texture *item_over_color_, *item_selected_color_, *item_separator_color_;

  void Layout() { 
    // Inform children (items) that default size has changed.
    int childcount = owner_->GetCount();
    for (int i = 0; i < childcount; i++) {
      ElementInterface *child = owner_->GetItemByIndex(i);
      if (child->IsInstanceOf(ItemElement::CLASS_ID)) {
        ItemElement *item = down_cast<ItemElement *>(child);
        item->SetIndex(i);
      } else {
        LOG(kErrorItemExpected);
      }
    }

    double parent_width = parent_->GetPixelWidth();
    if (item_width_relative_) {
      pixel_item_width_ = rel_item_width_ * parent_width;
    } else {
      rel_item_width_ = parent_width > 0.0 ?
                        pixel_item_width_ / parent_width : 0.0; 
    }

    double parent_height = parent_->GetPixelHeight();
    if (item_height_relative_) {
      pixel_item_height_ = rel_item_height_ * parent_height;
    } else {
      rel_item_height_ = parent_height > 0.0 ?
                        pixel_item_height_ / parent_height : 0.0; 
    }

    // No need to destroy items_canvas_ here, since Draw() will calculate
    // the size required and resize it if necessary.
  }

  const CanvasInterface *Draw(bool *changed) {
    const CanvasInterface **c_canvas = NULL;  
    CanvasInterface *separator = NULL;
    bool child_changed = false;

    bool change = separator_changed_;
    separator_changed_ = false;

    int child_count = owner_->GetCount();
    if (0 == child_count) {
      if (items_canvas_) {
        items_canvas_->Destroy();
        items_canvas_ = NULL;
      }
      goto exit;
    }

    // Draw children into temp array.
    c_canvas = new const CanvasInterface*[child_count];
    for (int i = 0; i < child_count; i++) {
      ElementInterface *item = owner_->GetItemByIndex(i);
      if (item->IsInstanceOf(ItemElement::CLASS_ID)) {
        ItemElement *element = down_cast<ItemElement *>(item);

        c_canvas[i] = element->Draw(&child_changed);
        if (element->IsPositionChanged()) {
          element->ClearPositionChanged();
          child_changed = true;
        }

        change = change || child_changed;
      } else {
        c_canvas[i] = NULL;
        change = true;
        LOG(kErrorItemExpected);
      }
    }

    change = change || !items_canvas_;
    if (change) {  // Need to redraw items_canvas_.
      size_t canvas_width = static_cast<size_t>(ceil(pixel_item_width_));
      size_t canvas_height = 
        static_cast<size_t>(ceil(child_count * pixel_item_height_));

      if (!items_canvas_ || 
          items_canvas_->GetWidth() != canvas_width ||
          items_canvas_->GetHeight() != canvas_height) {
        if (items_canvas_) {
          items_canvas_->Destroy();
        }

        if (canvas_width == 0 || canvas_height == 0) {
          items_canvas_ = NULL;
          goto exit;
        }

        const GraphicsInterface *gfx = parent_->GetView()->GetGraphics();
        items_canvas_ = gfx->NewCanvas(canvas_width, canvas_height);
        if (!items_canvas_) {
          DLOG("Error: unable to create list elements canvas.");
          goto exit;
        }
      } else {
        // If not new canvas, we must remember to clear canvas before drawing.
        items_canvas_->ClearCanvas();
      }

      if (item_separator_ && item_separator_color_) {
        const GraphicsInterface *gfx = parent_->GetView()->GetGraphics();
        separator = gfx->NewCanvas(static_cast<size_t>(ceil(pixel_item_width_)), 
                                   2);
        if (!separator) {
          DLOG("Error: unable to create separator canvas.");
          goto exit;
        }

        item_separator_color_->Draw(separator);
      }

      double y = 0;
      double sep_y = pixel_item_height_ - 2;
      for (int i = 0; i < child_count; i++, y+= pixel_item_height_) {
        if (c_canvas[i]) {        
          items_canvas_->PushState();

          // This downcast must be valid since c_canvas[i] != NULL.
          ItemElement *element = 
            down_cast<ItemElement *>(owner_->GetItemByIndex(i));
          if (element->GetRotation() == .0) {
            items_canvas_->TranslateCoordinates(-element->GetPixelPinX(),
                                                y - element->GetPixelPinY());
          } else {
            items_canvas_->TranslateCoordinates(0, y);
            items_canvas_->RotateCoordinates(DegreesToRadians(element->GetRotation()));
            items_canvas_->TranslateCoordinates(-element->GetPixelPinX(),
                                                -element->GetPixelPinY());
          }

          const CanvasInterface *mask = element->GetMaskCanvas();
          if (mask) {
            items_canvas_->DrawCanvasWithMask(0, 0, c_canvas[i], 0, 0, mask);
          } else {
            items_canvas_->DrawCanvas(.0, .0, c_canvas[i]);
          }

          if (separator) {
            items_canvas_->DrawCanvas(0, sep_y, separator);
          }

          items_canvas_->PopState();
        }
      }    
    }

    *changed = change;

  exit:
    if (c_canvas) {
      delete[] c_canvas;
      c_canvas = NULL;
    }

    if (separator) {
      separator->Destroy();
      separator = NULL;
    }

    // This field is no longer used after the first draw.
    selected_index_ = -1;

    return items_canvas_;
  }

  void SetPixelItemWidth(double width) {
    if (width >= 0.0 && 
        (width != pixel_item_width_ || item_width_relative_)) {
      pixel_item_width_ = width;
      item_width_relative_ = false;
      parent_->QueueDraw();
    }
  }

  void SetPixelItemHeight(double height) {
    if (height >= 0.0 && 
        (height != pixel_item_height_ || item_height_relative_)) {
      pixel_item_height_ = height;
      item_height_relative_ = false;
      parent_->QueueDraw();
    }
  }

  void SetRelativeItemWidth(double width) {
    if (width >= 0.0 && (width != rel_item_width_ || !item_width_relative_)) {
      rel_item_width_ = width;
      item_width_relative_ = true;
      parent_->QueueDraw();
    }
  }

  void SetRelativeItemHeight(double height) {
    if (height >= 0.0 &&
        (height != rel_item_height_ || !item_height_relative_)) {
      rel_item_height_ = height;
      item_height_relative_ = true;
      parent_->QueueDraw();
    }
  }

  void SetPendingSelection() {
    ElementInterface *selected = owner_->GetItemByIndex(selected_index_);
    if (selected) {
      if (selected->IsInstanceOf(ItemElement::CLASS_ID)) {
        ItemElement *item = down_cast<ItemElement *>(selected);
        item->SetSelectedNoRedraw(true);

      } else {
        LOG(kErrorItemExpected);
      }

      selected_index_ = -1;
    }
  }

  // Returns true if anything was cleared.
  bool ClearSelection(ItemElement *avoid) {   
    bool result = false;
    int childcount = owner_->GetCount();
    for (int i = 0; i < childcount; i++) {
      ElementInterface *child = owner_->GetItemByIndex(i);
      if (child != avoid) {
        if (child->IsInstanceOf(ItemElement::CLASS_ID)) {
          ItemElement *item = down_cast<ItemElement *>(child);
          if (item->IsSelected()) {
            result = true;
            item->SetSelectedNoRedraw(false);
          }
        } else {
          LOG(kErrorItemExpected);
        }
      }
    }  
    return result;
  }
};

ListElements::ListElements(ElementFactoryInterface *factory,
                   ListBoxElement *parent, View *view)
    : Elements(factory, parent, view), impl_(new Impl(parent, this, view)) {
}

ListElements::~ListElements() {
  delete impl_;
}

ElementInterface *ListElements::AppendElement(const char *tag_name,
                                              const char *name) {
  ElementInterface *e = Elements::AppendElement(tag_name, name);
  // Windows version still allows non-Item insertions.
  impl_->SetPendingSelection();
  return e;
}

ElementInterface *ListElements::InsertElement(const char *tag_name,
                                              const ElementInterface *before,
                                              const char *name) {
  ElementInterface *e = Elements::InsertElement(tag_name, before, name);
  // Windows version still allows non-Item insertions.
  impl_->SetPendingSelection();
  return e; 
}

const CanvasInterface *ListElements::Draw(bool *changed) {
  //return Elements::Draw(changed);
  return impl_->Draw(changed);
}

Variant ListElements::GetItemWidth() const {
  return BasicElement::GetPixelOrRelative(impl_->item_width_relative_, 
                                          impl_->item_width_specified_,
                                          impl_->pixel_item_width_, 
                                          impl_->rel_item_width_);
}

void ListElements::SetItemWidth(const Variant &width) {
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

Variant ListElements::GetItemHeight() const {
  return BasicElement::GetPixelOrRelative(impl_->item_height_relative_, 
                                          impl_->item_height_specified_,
                                          impl_->pixel_item_height_, 
                                          impl_->rel_item_height_);
}

void ListElements::SetItemHeight(const Variant &height) {
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

double ListElements::GetItemPixelWidth() const {
  return impl_->pixel_item_width_;  
}

double ListElements::GetItemPixelHeight() const {
  return impl_->pixel_item_height_;
}

Variant ListElements::GetItemOverColor() const {
  return Variant(Texture::GetSrc(impl_->item_over_color_));
}

const Texture *ListElements::GetItemOverTexture() const {
  return impl_->item_over_color_;
}

void ListElements::SetItemOverColor(const Variant &color) {
  delete impl_->item_over_color_;
  impl_->item_over_color_ = impl_->parent_->GetView()->LoadTexture(color);

  int childcount = GetCount();
  for (int i = 0; i < childcount; i++) {
    ElementInterface *child = GetItemByIndex(i);
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

Variant ListElements::GetItemSelectedColor() const {
  return Variant(Texture::GetSrc(impl_->item_selected_color_));
}

const Texture *ListElements::GetItemSelectedTexture() const {
  return impl_->item_selected_color_;
}

void ListElements::SetItemSelectedColor(const Variant &color) {
  delete impl_->item_selected_color_;
  impl_->item_selected_color_ = impl_->parent_->GetView()->LoadTexture(color);

  int childcount = GetCount();
  for (int i = 0; i < childcount; i++) {
    ElementInterface *child = GetItemByIndex(i);
    if (child->IsInstanceOf(ItemElement::CLASS_ID)) {
      ItemElement *item = down_cast<ItemElement *>(child);
      if (item->IsSelected()) {
        item->QueueDraw();
      }
    } else {
      LOG(kErrorItemExpected);
    }
  }  
  impl_->parent_->QueueDraw();
}

Variant ListElements::GetItemSeparatorColor() const {
  return Variant(Texture::GetSrc(impl_->item_separator_color_));
}

void ListElements::SetItemSeparatorColor(const Variant &color) {
  delete impl_->item_separator_color_;
  impl_->item_separator_color_ = impl_->parent_->GetView()->LoadTexture(color);
  impl_->parent_->QueueDraw();
}

bool ListElements::HasItemSeparator() const {
  return impl_->item_separator_;
}

void ListElements::SetItemSeparator(bool separator) {
  if (separator != impl_->item_separator_) {
    impl_->item_separator_ = separator;
    impl_->separator_changed_ = true;
    impl_->parent_->QueueDraw();
  }
}

bool ListElements::IsMultiSelect() const {
  return impl_->multiselect_;
}

void ListElements::SetMultiSelect(bool multiselect) {
  impl_->multiselect_ = multiselect; // No need to redraw.
}

int ListElements::GetSelectedIndex() const {
  int childcount = GetCount();
  for (int i = 0; i < childcount; i++) {
    const ElementInterface *child = GetItemByIndex(i);
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

void ListElements::SetSelectedIndex(int index) {
  ElementInterface *item = GetItemByIndex(index);
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

ItemElement *ListElements::GetSelectedItem() {
  int childcount = GetCount();
  for (int i = 0; i < childcount; i++) {
    ElementInterface *child = GetItemByIndex(i);
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

const ItemElement *ListElements::GetSelectedItem() const {
  int childcount = GetCount();
  for (int i = 0; i < childcount; i++) {
    const ElementInterface *child = GetItemByIndex(i);
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

void ListElements::SetSelectedItem(ItemElement *item) {
  bool changed = impl_->ClearSelection(item);
  if (item && !item->IsSelected()) {
    item->SetSelected(true);
    changed = true;
  }

  if (changed) {
    impl_->parent_->FireOnChangeEvent();
  }
}

void ListElements::ClearSelection() {
  bool changed = impl_->ClearSelection(NULL);
  if (changed) {
    impl_->parent_->QueueDraw();
    impl_->parent_->FireOnChangeEvent();
  }
}

void ListElements::AppendSelection(ItemElement *item) {
  ASSERT(item);

  if (!impl_->multiselect_) {
    SetSelectedItem(item);
    return;
  }

  if (!item->IsSelected()) {
    item->SetSelected(true);
    impl_->parent_->FireOnChangeEvent();
  }
}

void ListElements::SelectRange(ItemElement *endpoint) {
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
    int childcount = GetCount();
    for (int i = 0; i < childcount; i++) {
      ElementInterface *child = GetItemByIndex(i);
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
    impl_->parent_->FireOnChangeEvent();
  }
}

bool ListElements::AppendString(const char *str) {
  ElementInterface *child = AppendElement("item", "");
  if (!child) {
    return false;
  }

  ASSERT(child->IsInstanceOf(ItemElement::CLASS_ID));
  ItemElement *item = down_cast<ItemElement *>(child);
  bool result = item->AddLabelWithText(str);
  if (!result) {
    // Cleanup on failure.
    RemoveElement(child);
  }    
  return result;
}

bool ListElements::InsertStringAt(const char *str, int index) {
  if (GetCount() == index) {
    return AppendString(str);
  }

  const ElementInterface *before = GetItemByIndex(index);
  if (!before) {
    return false;
  }

  ElementInterface *child = InsertElement("item", before, ""); 
  if (!child) {
    return false;
  }

  ASSERT(child->IsInstanceOf(ItemElement::CLASS_ID));
  ItemElement *item = down_cast<ItemElement *>(child);
  bool result = item->AddLabelWithText(str);  
  if (!result) {
    // Cleanup on failure.
    RemoveElement(child);
  }    
  return result;
}

void ListElements::RemoveString(const char *str) {
  int childcount = GetCount();
  for (int i = 0; i < childcount; i++) {
    ElementInterface *child = GetItemByIndex(i);
    if (child->IsInstanceOf(ItemElement::CLASS_ID)) {
      ItemElement *item = down_cast<ItemElement *>(child);
      const char *text = item->GetLabelText();
      if (text && strcmp(str, text) == 0) {
        RemoveElement(child);
        return;
      }
    } else {
      LOG(kErrorItemExpected);
    }
  }  
}

void ListElements::Layout() {
  impl_->Layout();
  Elements::Layout();
}

} // namespace ggadget
