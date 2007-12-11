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
#include "contentarea_element.h"
#include "canvas_interface.h"
#include "color.h"
#include "content_item.h"
#include "event.h"
#include "gadget_consts.h"
#include "graphics_interface.h"
#include "image.h"
#include "scriptable_array.h"
#include "view.h"

namespace ggadget {

static const size_t kDefaultMaxContentItems = 25;
static const size_t kMaxContentItemsUpperLimit = 500;
static const Color kDefaultBackground(0.98, 0.98, 0.98);
static const Color kMouseOverBackground(0.83, 0.93, 0.98);
static const Color kMouseDownBackground(0.73, 0.83, 0.88);
static const Color kSelectedBackground(0.83, 0.93, 0.98);
static const int kItemBorderWidth = 2;
static const unsigned int kRefreshInterval = 30000; // 30 seconds. 

class ContentAreaElement::Impl {
 public:
  typedef std::vector<ContentItem *> ContentItems;
  enum PinImageIndex {
    PIN_IMAGE_PINNED,
    PIN_IMAGE_PINNED_OVER,
    PIN_IMAGE_UNPINNED,
    PIN_IMAGE_COUNT,
  };

  Impl(ContentAreaElement *owner)
      : owner_(owner),
        layout_canvas_(owner->GetView()->GetGraphics()->NewCanvas(5, 5)),
        content_flags_(CONTENT_FLAG_NONE),
        target_(GadgetInterface::TARGET_SIDEBAR),
        max_content_items_(kDefaultMaxContentItems),
        pin_image_max_width_(0), pin_image_max_height_(0),
        mouse_down_(false), mouse_over_pin_(false),
        mouse_x_(-1), mouse_y_(-1), mouse_over_item_(NULL),
        content_height_(0),
        refresh_timer_(0) {
    pin_images_[PIN_IMAGE_PINNED] = owner->GetView()->LoadImageFromGlobal(
        kContentItemPinned, false);
    pin_images_[PIN_IMAGE_PINNED_OVER] = owner->GetView()->LoadImageFromGlobal(
        kContentItemPinnedOver, false);
    pin_images_[PIN_IMAGE_UNPINNED] = owner->GetView()->LoadImageFromGlobal(
        kContentItemUnpinned, false);

    // Schedule a interval timer to redraw the content area periodically,
    // to refresh the relative time stamps of the items.
    refresh_timer_ = owner->GetView()->SetInterval(
        NewSlot(this, &Impl::QueueDraw), kRefreshInterval);
  }

  ~Impl() {
    owner_->GetView()->ClearInterval(refresh_timer_);
    refresh_timer_ = 0;
    RemoveAllContentItems();
    for (size_t i = 0; i < arraysize(pin_images_); i++) {
      delete pin_images_[i];
      pin_images_[i] = NULL;
    }
    layout_canvas_->Destroy();
    layout_canvas_ = NULL;
  }

  void QueueDraw() {
    owner_->QueueDraw();
  }

  void Layout() {
    if (content_flags_ & CONTENT_FLAG_PINNABLE) {
      if (pin_image_max_width_ == 0) {
        for (size_t i = 0; i < arraysize(pin_images_); i++) {
          if (pin_images_[i]) {
            pin_image_max_width_ = std::max(
                pin_image_max_width_,
                static_cast<int>(pin_images_[i]->GetWidth()));
            pin_image_max_height_ = std::max(
                pin_image_max_height_,
                static_cast<int>(pin_images_[i]->GetHeight()));
          }
        }
        pin_image_max_width_ += kItemBorderWidth;
      }
    } else {
      pin_image_max_width_ = pin_image_max_height_ = 0;
    }

    int y = kItemBorderWidth;
    int width = static_cast<int>(ceil(owner_->GetClientWidth())) -
                2 * kItemBorderWidth;
    int item_width = width - pin_image_max_width_;

    content_height_ = 0;
    size_t item_count = content_items_.size();
    if (content_flags_ & CONTENT_FLAG_MANUAL_LAYOUT) {
      for (size_t i = 0; i < item_count; i++) {
        ContentItem *item = content_items_[i];
        ASSERT(item);
        int temp_x, temp_y, temp_width, item_height;
        item->GetRect(&temp_x, &temp_y, &temp_width, &item_height);
        content_height_ = std::max(content_height_, item_height);
      }
    } else {
      for (size_t i = 0; i < item_count; i++) {
        ContentItem *item = content_items_[i];
        ASSERT(item);
        if (item->GetFlags() & ContentItem::CONTENT_ITEM_FLAG_HIDDEN) {
          item->SetRect(0, 0, 0, 0);
        } else {
          int item_height = item->GetHeight(target_, layout_canvas_,
                                            item_width);
          item_height = std::max(item_height, pin_image_max_height_);
          // Note: SetRect still uses the width including pin_image,
          // while Draw and GetHeight use the width excluding pin_image.
          item->SetRect(kItemBorderWidth, y, width, item_height);
          y += item_height + kItemBorderWidth * 2;
        }
      }
      content_height_ = y;
    }
  }

  void Draw(CanvasInterface *canvas) {
    int width = static_cast<int>(ceil(owner_->GetClientWidth()));
    int height = static_cast<int>(ceil(owner_->GetClientHeight()));
    canvas->DrawFilledRect(0, 0, width, height, kDefaultBackground);

    int item_count = content_items_.size();
    for (int i = 0; i < item_count; i++) {
      ContentItem *item = content_items_[i];
      ASSERT(item);
      if (item->GetFlags() & ContentItem::CONTENT_ITEM_FLAG_HIDDEN)
        continue;

      int item_x = 0, item_y = 0, item_width = 0, item_height = 0;
      item->GetRect(&item_x, &item_y, &item_width, &item_height);
      item_x -= owner_->GetScrollXPosition();
      item_y -= owner_->GetScrollYPosition();
      if (item_width > 0 && item_height > 0 && item_y < height) {
        bool mouse_over = mouse_x_ != -1 && mouse_y_ != -1 &&
                          mouse_x_ >= item_x &&
                          mouse_x_ < item_x + item_width &&
                          mouse_y_ >= item_y &&
                          mouse_y_ < item_y + item_height;
        bool mouse_over_pin = false;

        if (content_flags_ & CONTENT_FLAG_PINNABLE &&
            pin_image_max_width_ > 0 && pin_image_max_height_ > 0) {
          Image *pin_image = pin_images_[PIN_IMAGE_UNPINNED];
          mouse_over_pin = mouse_over && mouse_x_ < pin_image_max_width_;
          if (mouse_over_pin) {
            const Color &color = mouse_down_ ?
                                 kMouseDownBackground : kMouseOverBackground;
            canvas->DrawFilledRect(item_x - kItemBorderWidth,
                                   item_y - kItemBorderWidth,
                                   pin_image_max_width_ + kItemBorderWidth * 2,
                                   item_height + kItemBorderWidth * 2,
                                   color);
          }
          if (item->GetFlags() & ContentItem::CONTENT_ITEM_FLAG_PINNED) {
            pin_image = pin_images_[mouse_over_pin ?
                                    PIN_IMAGE_PINNED_OVER : PIN_IMAGE_PINNED];
          }
          if (pin_image) {
            pin_image->Draw(canvas, item_x, item_y);
          }
          item_x += pin_image_max_width_;
          item_width -= pin_image_max_width_;
        }

        if (mouse_over) {
          const Color &color = mouse_down_ && !mouse_over_pin ?
                               kMouseDownBackground : kMouseOverBackground;
          canvas->DrawFilledRect(item_x - kItemBorderWidth,
                                 item_y - kItemBorderWidth,
                                 item_width + kItemBorderWidth * 2,
                                 item_height + kItemBorderWidth * 2,
                                 color);
        }
        item->Draw(target_, canvas, item_x, item_y, item_width, item_height);
      }
    }
  }

  ScriptableArray *ScriptGetContentItems() {
    return ScriptableArray::Create(content_items_.begin(),
                                   content_items_.size(), false);
  }

  void ScriptSetContentItems(ScriptableArray *array) {
    RemoveAllContentItems();
    if (array) {
      for (size_t i = 0; i < array->GetCount(); i++) {
        Variant v = array->GetItem(i);
        if (v.type() != Variant::TYPE_SCRIPTABLE) {
          ContentItem *item = VariantValue<ContentItem *>()(v);
          if (item) {
            AddContentItem(item, ITEM_DISPLAY_IN_SIDEBAR);
          }
        }
      }
    }
    QueueDraw();
  }

  void GetPinImages(Variant *pinned, Variant *pinned_over, Variant *unpinned) {
    ASSERT(pinned && pinned_over && unpinned);
    *pinned = Variant(Image::GetSrc(pin_images_[PIN_IMAGE_PINNED]));
    *pinned_over = Variant(Image::GetSrc(pin_images_[PIN_IMAGE_PINNED_OVER]));
    *unpinned = Variant(Image::GetSrc(pin_images_[PIN_IMAGE_UNPINNED]));
  }

  void SetPinImages(const Variant &pinned,
                    const Variant &pinned_over,
                    const Variant &unpinned) {
    delete pin_images_[PIN_IMAGE_PINNED];
    pin_images_[PIN_IMAGE_PINNED] = owner_->GetView()->LoadImage(pinned, false);
    delete pin_images_[PIN_IMAGE_PINNED_OVER];
    pin_images_[PIN_IMAGE_PINNED_OVER] = owner_->GetView()->LoadImage(
        pinned_over, false);
    delete pin_images_[PIN_IMAGE_UNPINNED];
    pin_images_[PIN_IMAGE_UNPINNED] = owner_->GetView()->LoadImage(unpinned,
                                                                   false);
    pin_image_max_width_ = pin_image_max_height_ = 0;
    QueueDraw();
  }

  ScriptableArray *ScriptGetPinImages() {
    return ScriptableArray::Create(pin_images_, arraysize(pin_images_), false);
  }

  void ScriptSetPinImages(ScriptableArray *array) {
    if (array && array->GetCount() == arraysize(pin_images_)) {
      SetPinImages(array->GetItem(0), array->GetItem(1), array->GetItem(2));
    }
  }

  bool SetMaxContentItems(size_t max_content_items) {
    max_content_items = std::min(std::max(1U, max_content_items),
                                 kMaxContentItemsUpperLimit);
    if (max_content_items_ != max_content_items) {
      max_content_items_ = max_content_items;
      return RemoveExtraItems(content_items_.begin());
    }
    return false;
  }

  bool AddContentItem(ContentItem *item, DisplayOptions options) {
    ContentItems::iterator it = std::find(content_items_.begin(),
                                          content_items_.end(),
                                          item);
    if (it == content_items_.end()) {
      item->AttachContentArea(owner_);
      content_items_.insert(content_items_.begin(), item);
      RemoveExtraItems(content_items_.begin() + 1);
      return true;
    }
    return false;
  }

  bool RemoveExtraItems(ContentItems::iterator begin) {
    if (content_items_.size() <= max_content_items_)
      return false;

    bool all_pinned = false;
    while (content_items_.size() > max_content_items_) {
      ContentItems::iterator it = content_items_.end() - 1;
      if (!all_pinned && (content_flags_ & CONTENT_FLAG_PINNABLE)) {
        // Find the first unpinned item which can be removed. If can't find
        // anything last item will be removed.
        for (; it > begin; --it) {
          if (!((*it)->GetFlags() & ContentItem::CONTENT_ITEM_FLAG_PINNED))
            break;
        }
        if (it == begin &&
            ((*it)->GetFlags() & ContentItem::CONTENT_ITEM_FLAG_PINNED)) {
          all_pinned = true;
          it = content_items_.end() - 1;
        }
      }

      (*it)->DetachContentArea(owner_);
      content_items_.erase(it);
    }
    return true;
  }

  bool RemoveContentItem(ContentItem *item) {
    ContentItems::iterator it = std::find(content_items_.begin(),
                                          content_items_.end(),
                                          item);
    if (it != content_items_.end()) {
      (*it)->DetachContentArea(owner_);
      content_items_.erase(it);
      return true;
    }
    return false;
  }

  void RemoveAllContentItems() {
    for (ContentItems::iterator it = content_items_.begin();
         it != content_items_.end(); ++it) {
      (*it)->DetachContentArea(owner_);
    }
  }

  EventResult HandleMouseEvent(const MouseEvent &event) {
    bool queue_draw = false;
    EventResult result = EVENT_RESULT_UNHANDLED;
    if (event.GetType() == Event::EVENT_MOUSE_OUT) {
      mouse_over_pin_ = false;
      mouse_over_item_ = NULL;
      mouse_x_ = mouse_y_ = -1;
      mouse_down_ = false;
      queue_draw = true;
      result = EVENT_RESULT_HANDLED;
    } else {
      mouse_x_ = static_cast<int>(round(event.GetX()));
      mouse_y_ = static_cast<int>(round(event.GetY()));
      ContentItem *new_mouse_over_item = NULL;
      bool tooltip_required = false;
      for (ContentItems::iterator it = content_items_.begin();
           it != content_items_.end(); ++it) {
        int x, y, w, h;
        (*it)->GetRect(&x, &y, &w, &h);
        x -= owner_->GetScrollXPosition();
        y -= owner_->GetScrollYPosition();
        if (mouse_x_ >= x && mouse_x_ < x + w &&
            mouse_y_ >= y && mouse_y_ < y + h) {
          new_mouse_over_item = *it;
          tooltip_required = new_mouse_over_item->IsTooltipRequired(
              target_, layout_canvas_, x, y, w, h);
          break;
        }
      }
  
      bool new_mouse_over_pin = (mouse_x_ < pin_image_max_width_);
      if (mouse_over_item_ != new_mouse_over_item) {
        mouse_over_item_ = new_mouse_over_item;
        mouse_over_pin_ = new_mouse_over_pin;
        const char *tooltip = tooltip_required ?
                              new_mouse_over_item->GetTooltip() : NULL;
        // Store the tooltip to let view display it when appropriate using
        // the default mouse-in logic.
        owner_->SetTooltip(tooltip);
        // Display the tooltip now, because view only displays tooltip when
        // the mouse-in element changes.
        owner_->GetView()->SetTooltip(tooltip);
        queue_draw = true;
      } else if (new_mouse_over_pin != mouse_over_pin_) {
        mouse_over_pin_ = new_mouse_over_pin;
        queue_draw = true;
      }
  
      if (event.GetType() != Event::EVENT_MOUSE_MOVE &&
          event.GetButton() == MouseEvent::BUTTON_LEFT) {
        result = EVENT_RESULT_HANDLED;
  
        switch (event.GetType()) {
          case Event::EVENT_MOUSE_DOWN:
            mouse_down_ = true;
            queue_draw = true;
            break;
          case Event::EVENT_MOUSE_UP:
            mouse_down_ = false;
            queue_draw = true;
            break;
          case Event::EVENT_MOUSE_CLICK:
            if (mouse_over_item_) {
              if (mouse_over_pin_) {
                mouse_over_item_->ToggleItemPinnedState();
              } else {
                // TODO: show details view for this item.
              }
            }
            break;
          default:
            result = EVENT_RESULT_UNHANDLED;
            break;
        }
      }
    }

    if (queue_draw)
      QueueDraw();
    return result;
  }

  ContentAreaElement *owner_;
  CanvasInterface *layout_canvas_; // Only used during Layout().
  int content_flags_;
  GadgetInterface::DisplayTarget target_;
  size_t max_content_items_;
  ContentItems content_items_;
  Image *pin_images_[PIN_IMAGE_COUNT];
  int pin_image_max_width_, pin_image_max_height_;
  bool mouse_down_, mouse_over_pin_;
  int mouse_x_, mouse_y_;
  // This pointer is only used in HandleMouseEvent() to check if mouse over
  // item changes. Don't dereference the pointer because it may be stale.
  ContentItem *mouse_over_item_;
  int content_height_;
  int refresh_timer_;
};

ContentAreaElement::ContentAreaElement(BasicElement *parent, View *view,
                                       const char *name)
    : ScrollingElement(parent, view, "contentarea", name, false),
      impl_(new Impl(this)) {
  SetEnabled(true);
  SetAutoscroll(true);
  RegisterProperty("contentFlags", NULL, // Write only.
                   NewSlot(this, &ContentAreaElement::SetContentFlags));
  RegisterProperty("maxContentItems",
                   NewSlot(this, &ContentAreaElement::GetMaxContentItems),
                   NewSlot(this, &ContentAreaElement::SetMaxContentItems));
  RegisterProperty("contentItems",
                   NewSlot(impl_, &Impl::ScriptGetContentItems),
                   NewSlot(impl_, &Impl::ScriptSetContentItems));
  RegisterProperty("pinImages",
                   NewSlot(impl_, &Impl::ScriptGetPinImages),
                   NewSlot(impl_, &Impl::ScriptSetPinImages));
  RegisterMethod("addContentItem",
                 NewSlot(this, &ContentAreaElement::AddContentItem));
  RegisterMethod("removeContentItem",
                 NewSlot(this, &ContentAreaElement::RemoveContentItem));
  RegisterMethod("removeAllContentItems",
                 NewSlot(this, &ContentAreaElement::RemoveAllContentItems));
}

ContentAreaElement::~ContentAreaElement() {
  delete impl_;
  impl_ = NULL;
}

int ContentAreaElement::GetContentFlags() const {
  return impl_->content_flags_;
}

void ContentAreaElement::SetContentFlags(int flags) {
  if (impl_->content_flags_ != flags) {
    impl_->content_flags_ = flags;
    QueueDraw();
  }
}

size_t ContentAreaElement::GetMaxContentItems() const {
  return impl_->max_content_items_;
}

void ContentAreaElement::SetMaxContentItems(size_t max_content_items) {
  if (impl_->SetMaxContentItems(max_content_items))
    QueueDraw();
}

const std::vector<ContentItem *> &ContentAreaElement::GetContentItems() {
  return impl_->content_items_;
}

void ContentAreaElement::GetPinImages(Variant *pinned,
                                      Variant *pinned_over,
                                      Variant *unpinned) {
  impl_->GetPinImages(pinned, pinned_over, unpinned);
}

void ContentAreaElement::SetPinImages(const Variant &pinned,
                                      const Variant &pinned_over,
                                      const Variant &unpinned) {
  impl_->SetPinImages(pinned, pinned_over, unpinned);
}

void ContentAreaElement::AddContentItem(ContentItem *item,
                                        DisplayOptions options) {
  if (impl_->AddContentItem(item, options))
    QueueDraw();
}

void ContentAreaElement::RemoveContentItem(ContentItem *item) {
  if (impl_->RemoveContentItem(item))
    QueueDraw();
}

void ContentAreaElement::RemoveAllContentItems() {
  impl_->RemoveAllContentItems();
  QueueDraw();
}

void ContentAreaElement::Layout() {
  static int recurse_depth = 0;
  // Check to prevent infinite recursion when updating scroll bar.
  // This may be caused by a bad GetHeight() handler of a ContentItem.
  if (++recurse_depth > 2)
    return;

  ScrollingElement::Layout();
  impl_->Layout();

  if (UpdateScrollBar(static_cast<int>(ceil(GetClientWidth())),
                      impl_->content_height_)) {
    // Layout again to reflect change of the scroll bar.
    Layout();
  }
  --recurse_depth;
}

void ContentAreaElement::DoDraw(CanvasInterface *canvas,
                                const CanvasInterface *children_canvas) {
  impl_->Draw(canvas);
  DrawScrollbar(canvas);
}

EventResult ContentAreaElement::HandleMouseEvent(const MouseEvent &event) {
  EventResult result = impl_->HandleMouseEvent(event);
  return result == EVENT_RESULT_UNHANDLED ?
         ScrollingElement::HandleMouseEvent(event) : result;
}

BasicElement *ContentAreaElement::CreateInstance(BasicElement *parent,
                                                 View *view,
                                                 const char *name) {
  return new ContentAreaElement(parent, view, name);
}

} // namespace ggadget
