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
#include "image_interface.h"
#include "menu_interface.h"
#include "scriptable_array.h"
#include "view.h"
#include "view_host_interface.h"

namespace ggadget {

static const size_t kDefaultMaxContentItems = 25;
static const size_t kMaxContentItemsUpperLimit = 500;
static const Color kDefaultBackground(0.98, 0.98, 0.98);
static const Color kMouseOverBackground(0.83, 0.93, 0.98);
static const Color kMouseDownBackground(0.73, 0.83, 0.88);
static const Color kSelectedBackground(0.83, 0.93, 0.98);
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
        mouse_x_(-1), mouse_y_(-1),
        mouse_over_item_(NULL),
        content_height_(0),
        refresh_timer_(0),
        modified_(false),
        death_detector_(NULL),
        context_menu_time_(0) {
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
    if (death_detector_) {
      // Inform the death detector that this element is dying.
      *death_detector_ = true;
    }

    owner_->GetView()->ClearInterval(refresh_timer_);
    refresh_timer_ = 0;
    RemoveAllContentItems();
    for (size_t i = 0; i < arraysize(pin_images_); i++) {
      DestroyImage(pin_images_[i]);
    }
    layout_canvas_->Destroy();
  }

  void QueueDraw() {
    owner_->QueueDraw();
  }

  // Called when content items are added, removed or reordered.
  void Modified() {
    modified_ = true;
    mouse_over_item_ = NULL;
    QueueDraw();
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
      }
    } else {
      pin_image_max_width_ = pin_image_max_height_ = 0;
    }

    int y = 0;
    int width = static_cast<int>(ceil(owner_->GetClientWidth()));
    int item_width = width - pin_image_max_width_;

    // Add a modification checker to detect whether the set of content items
    // or this object itself is modified during the following loop. If such
    // things happen, break the loop and return to ensure safety.
    modified_ = false;
    bool dead = false;
    death_detector_ = &dead;

    content_height_ = 0;
    size_t item_count = content_items_.size();
    if (content_flags_ & CONTENT_FLAG_MANUAL_LAYOUT) {
      for (size_t i = 0; i < item_count && dead && !modified_; i++) {
        ContentItem *item = content_items_[i];
        ASSERT(item);
        int temp_x, temp_y, temp_width, temp_height;
        item->GetRect(&temp_x, &temp_y, &temp_width, &temp_height);
        if (dead)
          break;
        content_height_ = std::max(content_height_, temp_y + temp_height);
      }
    } else {
      for (size_t i = 0; i < item_count && !dead && !modified_ ; i++) {
        ContentItem *item = content_items_[i];
        ASSERT(item);
        if (item->GetFlags() & ContentItem::CONTENT_ITEM_FLAG_HIDDEN) {
          item->SetRect(0, 0, 0, 0);
        } else {
          int item_height = item->GetHeight(target_, layout_canvas_,
                                            item_width);
          if (dead)
            break;
          item_height = std::max(item_height, pin_image_max_height_);
          // Note: SetRect still uses the width including pin_image,
          // while Draw and GetHeight use the width excluding pin_image.
          item->SetRect(0, y, width, item_height);
          y += item_height;
        }
      }
      if (!dead)
        content_height_ = y;
    }

    if (!dead)
      death_detector_ = NULL;
  }

  void Draw(CanvasInterface *canvas) {
    int width = static_cast<int>(ceil(owner_->GetClientWidth()));
    int height = static_cast<int>(ceil(owner_->GetClientHeight()));
    canvas->DrawFilledRect(0, 0, width, height, kDefaultBackground);

    // Add a modification checker to detect whether the set of content items
    // or this object itself is modified during the following loop. If such
    // things happen, break the loop and return to ensure safety.
    modified_ = false;
    bool dead = false;
    death_detector_ = &dead;

    int item_count = content_items_.size();
    for (int i = 0; i < item_count && !dead && !modified_; i++) {
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
          ImageInterface *pin_image = pin_images_[PIN_IMAGE_UNPINNED];
          mouse_over_pin = mouse_over && mouse_x_ < pin_image_max_width_;
          if (mouse_over_pin) {
            const Color &color = mouse_down_ ?
                                 kMouseDownBackground : kMouseOverBackground;
            canvas->DrawFilledRect(item_x, item_y,
                                   pin_image_max_width_, item_height,
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

        if (mouse_over &&
            !(item->GetFlags() & ContentItem::CONTENT_ITEM_FLAG_STATIC)) {
          const Color &color = mouse_down_ && !mouse_over_pin ?
                               kMouseDownBackground : kMouseOverBackground;
          canvas->DrawFilledRect(item_x, item_y,
                                 item_width, item_height, color);
        }
        item->Draw(target_, canvas, item_x, item_y, item_width, item_height);
      }
    }
    if (!dead)
      death_detector_ = NULL;
  }

  ScriptableArray *ScriptGetContentItems() {
    return ScriptableArray::Create(content_items_.begin(),
                                   content_items_.size());
  }

  void ScriptSetContentItems(ScriptableInterface *array) {
    RemoveAllContentItems();
    if (array) {
      Variant length_v = GetPropertyByName(array, "length");
      int length;
      if (length_v.ConvertToInt(&length)) {
        if (static_cast<size_t>(length) > max_content_items_)
          length = max_content_items_;

        for (int i = 0; i < length; i++) {
          Variant v = array->GetProperty(i);
          if (v.type() == Variant::TYPE_SCRIPTABLE) {
            ContentItem *item = VariantValue<ContentItem *>()(v);
            if (item)
              AddContentItem(item, ITEM_DISPLAY_IN_SIDEBAR);
          }
        }
      }
    }
    QueueDraw();
  }

  void GetPinImages(Variant *pinned, Variant *pinned_over, Variant *unpinned) {
    ASSERT(pinned && pinned_over && unpinned);
    *pinned = Variant(GetImageTag(pin_images_[PIN_IMAGE_PINNED]));
    *pinned_over = Variant(GetImageTag(pin_images_[PIN_IMAGE_PINNED_OVER]));
    *unpinned = Variant(GetImageTag(pin_images_[PIN_IMAGE_UNPINNED]));
  }

  void SetPinImages(const Variant &pinned,
                    const Variant &pinned_over,
                    const Variant &unpinned) {
    DestroyImage(pin_images_[PIN_IMAGE_PINNED]);
    pin_images_[PIN_IMAGE_PINNED] = owner_->GetView()->LoadImage(pinned, false);
    DestroyImage(pin_images_[PIN_IMAGE_PINNED_OVER]);
    pin_images_[PIN_IMAGE_PINNED_OVER] = owner_->GetView()->LoadImage(
        pinned_over, false);
    DestroyImage(pin_images_[PIN_IMAGE_UNPINNED]);
    pin_images_[PIN_IMAGE_UNPINNED] = owner_->GetView()->LoadImage(unpinned,
                                                                   false);
    pin_image_max_width_ = pin_image_max_height_ = 0;
    QueueDraw();
  }

  ScriptableArray *ScriptGetPinImages() {
    Variant *values = new Variant[3];
    GetPinImages(&values[0], &values[1], &values[2]);
    return ScriptableArray::Create(values, 3);
  }

  void ScriptSetPinImages(ScriptableInterface *array) {
    if (array) {
      SetPinImages(array->GetProperty(0),
                   array->GetProperty(1),
                   array->GetProperty(2));
    }
  }

  bool SetMaxContentItems(size_t max_content_items) {
    max_content_items = std::min(std::max((size_t)1, max_content_items),
                                 kMaxContentItemsUpperLimit);
    if (max_content_items_ != max_content_items) {
      max_content_items_ = max_content_items;
      if (RemoveExtraItems(content_items_.begin())) {
        Modified();
        return true;
      }
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
      Modified();
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
      Modified();
      return true;
    }
    return false;
  }

  void RemoveAllContentItems() {
    for (ContentItems::iterator it = content_items_.begin();
         it != content_items_.end(); ++it) {
      (*it)->DetachContentArea(owner_);
    }
    content_items_.clear();
    Modified();
  }

  static const unsigned int kContextMenuMouseOutInterval = 50;
  EventResult HandleMouseEvent(const MouseEvent &event) {
    bool queue_draw = false;
    EventResult result = EVENT_RESULT_UNHANDLED;
    if (event.GetType() == Event::EVENT_MOUSE_OUT) {
      // Ignore the mouseout event caused by the context menu.
      if (owner_->GetView()->GetCurrentTime() - context_menu_time_ >
          kContextMenuMouseOutInterval) {
        mouse_over_pin_ = false;
        mouse_over_item_ = NULL;
        mouse_x_ = mouse_y_ = -1;
        mouse_down_ = false;
        queue_draw = true;
      }
      result = EVENT_RESULT_HANDLED;
    } else {
      mouse_x_ = static_cast<int>(round(event.GetX()));
      mouse_y_ = static_cast<int>(round(event.GetY()));
      ContentItem *new_mouse_over_item = NULL;
      bool tooltip_required = false;
      for (ContentItems::iterator it = content_items_.begin();
           it != content_items_.end(); ++it) {
        if (!((*it)->GetFlags() & ContentItem::CONTENT_ITEM_FLAG_HIDDEN)) {
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
      }

      bool new_mouse_over_pin = (mouse_x_ < pin_image_max_width_);
      if (mouse_over_item_ != new_mouse_over_item) {
        mouse_over_item_ = new_mouse_over_item;
        mouse_over_pin_ = new_mouse_over_pin;
        const char *tooltip = tooltip_required ?
                              new_mouse_over_item->GetTooltip().c_str() : NULL;
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
          (event.GetButton() & MouseEvent::BUTTON_LEFT)) {
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
              } else if (content_flags_ & CONTENT_FLAG_HAVE_DETAILS) {
                std::string title;
                DetailsView *details_view = NULL;
                int flags = 0;
                if (!mouse_over_item_->OnDetailsView(&title, &details_view,
                                                     &flags) &&
                    details_view) {
                  owner_->GetView()->ShowDetailsView(
                      details_view, title.c_str(), flags,
                      NewSlot(this, &Impl::ProcessDetailsViewFeedback));
                }
              }
            }
            break;
          case Event::EVENT_MOUSE_DBLCLICK:
            if (mouse_over_item_ && !mouse_over_pin_)
              mouse_over_item_->OpenItem();
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

  void ProcessDetailsViewFeedback(int flags) {
    if (flags & ViewHostInterface::DETAILS_VIEW_FLAG_TOOLBAR_OPEN)
      OnItemOpen(NULL);
    if (flags & ViewHostInterface::DETAILS_VIEW_FLAG_NEGATIVE_FEEDBACK)
      OnItemNegativeFeedback(NULL);
    if (flags & ViewHostInterface::DETAILS_VIEW_FLAG_REMOVE_BUTTON)
      OnItemRemove(NULL);
  }

  // Handler of the "Open" menu item.
  void OnItemOpen(const char *menu_item) {
    if (mouse_over_item_)
      mouse_over_item_->OpenItem();
  }

  // Handler of the "Remove" menu item.
  void OnItemRemove(const char *menu_item) {
    if (mouse_over_item_) {
      bool dead = false;
      death_detector_ = &dead;
      if (!mouse_over_item_->ProcessDetailsViewFeedback(
              ViewHostInterface::DETAILS_VIEW_FLAG_REMOVE_BUTTON) &&
          !dead && mouse_over_item_ &&
          !mouse_over_item_->OnUserRemove() &&
          !dead && mouse_over_item_) {
        RemoveContentItem(mouse_over_item_);
      }
      if (!dead)
        death_detector_ = NULL;
    }
  }

  // Handler of the "Don't show me ..." menu item.
  void OnItemNegativeFeedback(const char *menu_item) {
    if (mouse_over_item_) {
      bool dead = false;
      death_detector_ = &dead;
      if (!mouse_over_item_->ProcessDetailsViewFeedback(
              ViewHostInterface::DETAILS_VIEW_FLAG_REMOVE_BUTTON) &&
          !dead && mouse_over_item_) {
        RemoveContentItem(mouse_over_item_);
      }
      if (!dead)
        death_detector_ = NULL;
    }
  }

  ContentAreaElement *owner_;
  CanvasInterface *layout_canvas_; // Only used during Layout().
  int content_flags_;
  GadgetInterface::DisplayTarget target_;
  size_t max_content_items_;
  ContentItems content_items_;
  ImageInterface *pin_images_[PIN_IMAGE_COUNT];
  int pin_image_max_width_, pin_image_max_height_;
  bool mouse_down_, mouse_over_pin_;
  int mouse_x_, mouse_y_;
  ContentItem *mouse_over_item_;
  int content_height_;
  int refresh_timer_;
  bool modified_; // Flags whether items were added, removed or reordered.
  bool *death_detector_;
  uint64_t context_menu_time_;
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
  impl_->SetMaxContentItems(max_content_items);
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
  impl_->AddContentItem(item, options);
}

void ContentAreaElement::RemoveContentItem(ContentItem *item) {
  impl_->RemoveContentItem(item);
}

void ContentAreaElement::RemoveAllContentItems() {
  impl_->RemoveAllContentItems();
}

void ContentAreaElement::Layout() {
  static int recurse_depth = 0;
  // Check to prevent infinite recursion when updating scroll bar.
  // This may be caused by a bad GetHeight() handler of a ContentItem.
  if (++recurse_depth > 2)
    return;

  ScrollingElement::Layout();
  impl_->Layout();

  int y_range = static_cast<int>(ceil(impl_->content_height_ -
                                      GetClientHeight()));
  if (y_range < 0) y_range = 0;
  if (UpdateScrollBar(0, y_range)) {
    // Layout again to reflect change of the scroll bar.
    Layout();
  }
  --recurse_depth;
}

void ContentAreaElement::DoDraw(CanvasInterface *canvas) {
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

bool ContentAreaElement::OnAddContextMenuItems(MenuInterface *menu) {
  if (impl_->mouse_over_item_) {
    impl_->context_menu_time_ = GetView()->GetCurrentTime();
    if (impl_->mouse_over_item_->CanOpen()) {
      menu->AddItem("Open", 0,
                    new SlotProxy1<void, const char *>(
                        GetView()->NewDeathDetectedSlot(
                            this, NewSlot(impl_, &Impl::OnItemOpen))));
    }
    if (!(impl_->mouse_over_item_->GetFlags() &
        ContentItem::CONTENT_ITEM_FLAG_NO_REMOVE)) {
      menu->AddItem("Remove", 0,
                    new SlotProxy1<void, const char *>(
                        GetView()->NewDeathDetectedSlot(
                            this, NewSlot(impl_, &Impl::OnItemRemove))));
    }
    if (impl_->mouse_over_item_->GetFlags() &
        ContentItem::CONTENT_ITEM_FLAG_NEGATIVE_FEEDBACK) {
      menu->AddItem("Don't show me items like this", 0,
                    new SlotProxy1<void, const char *>(
                        GetView()->NewDeathDetectedSlot(
                            this,
                            NewSlot(impl_, &Impl::OnItemNegativeFeedback))));
    }
  }
  // To keep compatible with the Windows version, don't show default menu items.
  return false;
}

} // namespace ggadget
