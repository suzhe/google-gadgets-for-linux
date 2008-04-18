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
#include "messages.h"
#include "scriptable_array.h"
#include "scriptable_image.h"
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
    PIN_IMAGE_UNPINNED,
    PIN_IMAGE_UNPINNED_OVER,
    PIN_IMAGE_PINNED,
    PIN_IMAGE_COUNT,
  };

  Impl(ContentAreaElement *owner)
      : owner_(owner),
        layout_canvas_(owner->GetView()->GetGraphics()->NewCanvas(5, 5)),
        content_flags_(CONTENT_FLAG_NONE),
        target_(Gadget::TARGET_SIDEBAR),
        max_content_items_(kDefaultMaxContentItems),
        pin_image_max_width_(0), pin_image_max_height_(0),
        mouse_down_(false), mouse_over_pin_(false),
        mouse_x_(-1), mouse_y_(-1),
        mouse_over_item_(NULL),
        content_height_(0),
        scrolling_line_step_(0),
        refresh_timer_(0),
        modified_(false),
        death_detector_(NULL),
        context_menu_time_(0), 
        background_color_src_(kDefaultBackground.ToString()), 
        mouseover_color_src_(kMouseOverBackground.ToString()), 
        mousedown_color_src_(kMouseDownBackground.ToString()), 
        background_opacity_(1.),
        mouseover_opacity_(1.),
        mousedown_opacity_(1.),
        background_color_(kDefaultBackground),
        mouseover_color_(kMouseOverBackground),
        mousedown_color_(kMouseDownBackground) {
    pin_images_[PIN_IMAGE_UNPINNED].Reset(new ScriptableImage(
        owner->GetView()->LoadImageFromGlobal(kContentItemUnpinned, false)));
    pin_images_[PIN_IMAGE_UNPINNED_OVER].Reset(new ScriptableImage(
        owner->GetView()->LoadImageFromGlobal(kContentItemUnpinnedOver, false)));
    pin_images_[PIN_IMAGE_PINNED].Reset(new ScriptableImage(
        owner->GetView()->LoadImageFromGlobal(kContentItemPinned, false)));

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
          const ImageInterface *image = pin_images_[i].Get()->GetImage();
          if (image) {
            pin_image_max_width_ = std::max(
                pin_image_max_width_, static_cast<int>(image->GetWidth()));
            pin_image_max_height_ = std::max(
                pin_image_max_height_, static_cast<int>(image->GetHeight()));
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
      scrolling_line_step_ = 1;
      for (size_t i = 0; i < item_count && !dead && !modified_; i++) {
        ContentItem *item = content_items_[i];
        ASSERT(item);
        int item_x, item_y, item_width, item_height;
        bool x_relative, y_relative, width_relative, height_relative;
        item->GetRect(&item_x, &item_y, &item_width, &item_height,
                      &x_relative, &y_relative,
                      &width_relative, &height_relative);
        if (dead)
          break;

        double client_width = owner_->GetClientWidth() / 100;
        double client_height = owner_->GetClientHeight() / 100;
        if (x_relative)
          item_x = static_cast<int>(round(item_x * client_width));
        if (y_relative)
          item_y = static_cast<int>(round(item_y * client_height));
        if (width_relative)
          item_width = static_cast<int>(ceil(item_width * client_width));
        if (height_relative)
          item_height = static_cast<int>(ceil(item_height * client_height));
        item->SetLayoutRect(item_x, item_y, item_width, item_height);
        content_height_ = std::max(content_height_, item_y + item_height);
      }
    } else {
      scrolling_line_step_ = 0;
      // Pinned items first.
      if (content_flags_ & CONTENT_FLAG_PINNABLE) {
        for (size_t i = 0; i < item_count && !dead && !modified_ ; i++) {
          ContentItem *item = content_items_[i];
          ASSERT(item);
          int item_flags = item->GetFlags();
          if (item_flags & ContentItem::CONTENT_ITEM_FLAG_HIDDEN) {
            item->SetLayoutRect(0, 0, 0, 0);
          } else if (item_flags & ContentItem::CONTENT_ITEM_FLAG_PINNED) {
            int item_height = item->GetHeight(target_, layout_canvas_,
                                              item_width);
            if (dead)
              break;
            item_height = std::max(item_height, pin_image_max_height_);
            // Note: SetRect still uses the width including pin_image,
            // while Draw and GetHeight use the width excluding pin_image.
            item->SetLayoutRect(0, y, width, item_height);
            y += item_height;
            if (!scrolling_line_step_ || scrolling_line_step_ > item_height)
              scrolling_line_step_ = item_height;
          }
        }
      }
      // Then unpinned items.
      for (size_t i = 0; i < item_count && !dead && !modified_ ; i++) {
        ContentItem *item = content_items_[i];
        ASSERT(item);
        int item_flags = item->GetFlags();
        if (!(item_flags & ContentItem::CONTENT_ITEM_FLAG_HIDDEN) &&
            (!(content_flags_ & CONTENT_FLAG_PINNABLE) ||
             !(item_flags & ContentItem::CONTENT_ITEM_FLAG_PINNED))) {
          int item_height = item->GetHeight(target_, layout_canvas_,
                                            item_width);
          if (dead)
            break;
          item_height = std::max(item_height, pin_image_max_height_);
          // Note: SetRect still uses the width including pin_image,
          // while Draw and GetHeight use the width excluding pin_image.
          item->SetLayoutRect(0, y, width, item_height);
          y += item_height;
          if (!scrolling_line_step_ || scrolling_line_step_ > item_height)
            scrolling_line_step_ = item_height;
        }
      }
      if (!dead)
        content_height_ = y;
    }

    if (!dead)
      death_detector_ = NULL;
  }

  void Draw(CanvasInterface *canvas) {    
    int height = static_cast<int>(ceil(owner_->GetClientHeight()));
    if (background_opacity_ > 0.) {
      if (background_opacity_ != 1.) {
        canvas->PushState();
        canvas->MultiplyOpacity(background_opacity_);
      }
      int width = static_cast<int>(ceil(owner_->GetClientWidth()));
      canvas->DrawFilledRect(0, 0, width, height, background_color_);
      if (background_opacity_ != 1.) {
        canvas->PopState(); 
      }
    }
    
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
      item->GetLayoutRect(&item_x, &item_y, &item_width, &item_height);
      item_x -= owner_->GetScrollXPosition();
      item_y -= owner_->GetScrollYPosition();
      if (item_width > 0 && item_height > 0 && item_y < height) {
        bool mouse_over = mouse_x_ != -1 && mouse_y_ != -1 &&
                          mouse_x_ >= item_x &&
                          mouse_x_ < item_x + item_width &&
                          mouse_y_ >= item_y &&
                          mouse_y_ < item_y + item_height;
        bool mouse_over_pin = false;

        if ((content_flags_ & CONTENT_FLAG_PINNABLE) &&
            pin_image_max_width_ > 0 && pin_image_max_height_ > 0) {
          mouse_over_pin = mouse_over && mouse_x_ < pin_image_max_width_;
          if (mouse_over_pin) {
            const Color &color = mouse_down_ ?
                                  mousedown_color_ : mouseover_color_;
            canvas->DrawFilledRect(item_x, item_y,
                                   pin_image_max_width_, item_height,
                                   color);
          }

          const ImageInterface *pin_image = NULL;
          if (item->GetFlags() & ContentItem::CONTENT_ITEM_FLAG_PINNED) {
            pin_image = pin_images_[PIN_IMAGE_PINNED].Get()->GetImage();
          } else {
            pin_image = pin_images_[mouse_over_pin ? PIN_IMAGE_UNPINNED_OVER :
                                    PIN_IMAGE_UNPINNED].Get()->GetImage();
          }
          if (pin_image) {
            int image_width = static_cast<int>(pin_image->GetWidth());
            int image_height = static_cast<int>(pin_image->GetHeight());
            pin_image->Draw(canvas,
                            item_x + (pin_image_max_width_ - image_width) / 2,
                            item_y + (item_height - image_height) / 2);
          }
          item_x += pin_image_max_width_;
          item_width -= pin_image_max_width_;
        }

        if (mouse_over &&
            !(item->GetFlags() & ContentItem::CONTENT_ITEM_FLAG_STATIC)) {
          const Color &color = mouse_down_ && !mouse_over_pin ?
                                mousedown_color_ : mouseover_color_;
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

  void GetPinImages(ScriptableImage **unpinned,
                    ScriptableImage **unpinned_over,
                    ScriptableImage **pinned) {
    ASSERT(unpinned && unpinned_over && pinned);
    *unpinned = pin_images_[PIN_IMAGE_UNPINNED].Get();
    *unpinned_over = pin_images_[PIN_IMAGE_UNPINNED_OVER].Get();
    *pinned = pin_images_[PIN_IMAGE_PINNED].Get();
  }

  void SetPinImage(PinImageIndex index, ScriptableImage *image) {
    if (image)
      pin_images_[index].Reset(image);
  }

  void SetPinImages(ScriptableImage *unpinned,
                    ScriptableImage *unpinned_over,
                    ScriptableImage *pinned) {
    SetPinImage(PIN_IMAGE_UNPINNED, unpinned);
    SetPinImage(PIN_IMAGE_UNPINNED_OVER, unpinned_over);
    SetPinImage(PIN_IMAGE_PINNED, pinned);
    // To be updated in Layout().
    pin_image_max_width_ = pin_image_max_height_ = 0;
    QueueDraw();
  }

  ScriptableArray *ScriptGetPinImages() {
    Variant *values = new Variant[3];
    values[0] = Variant(pin_images_[PIN_IMAGE_UNPINNED].Get());
    values[1] = Variant(pin_images_[PIN_IMAGE_UNPINNED_OVER].Get());
    values[2] = Variant(pin_images_[PIN_IMAGE_PINNED].Get());
    return ScriptableArray::Create(values, 3);
  }

  void ScriptSetPinImage(PinImageIndex index, const Variant &v) {
    if (v.type() == Variant::TYPE_SCRIPTABLE)
      SetPinImage(index, VariantValue<ScriptableImage *>()(v));
  }

  void ScriptSetPinImages(ScriptableInterface *array) {
    if (array) {
      ScriptSetPinImage(PIN_IMAGE_UNPINNED, array->GetProperty(0));
      ScriptSetPinImage(PIN_IMAGE_UNPINNED_OVER, array->GetProperty(1));
      ScriptSetPinImage(PIN_IMAGE_PINNED, array->GetProperty(2));
    }
  }

  bool SetMaxContentItems(size_t max_content_items) {
    max_content_items = std::min(std::max(size_t(1), max_content_items),
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
          (*it)->GetLayoutRect(&x, &y, &w, &h);
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
              } else if ((content_flags_ & CONTENT_FLAG_HAVE_DETAILS) &&
                         !(mouse_over_item_->GetFlags() &
                           ContentItem::CONTENT_ITEM_FLAG_STATIC)) {
                std::string title;
                DetailsViewData *details_view_data = NULL;
                int flags = 0;
                if (!mouse_over_item_->OnDetailsView(
                       &title, &details_view_data, &flags) &&
                    details_view_data) {
                  owner_->GetView()->GetGadget()->ShowDetailsView(
                      details_view_data, title.c_str(), flags,
                      NewSlot(this, &Impl::ProcessDetailsViewFeedback));
                }
              }
            }
            break;
          case Event::EVENT_MOUSE_DBLCLICK:
            if (mouse_over_item_ && !mouse_over_pin_ &&
                !(mouse_over_item_->GetFlags() &
                  ContentItem::CONTENT_ITEM_FLAG_STATIC))
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
    if (flags & ViewInterface::DETAILS_VIEW_FLAG_TOOLBAR_OPEN)
      OnItemOpen(NULL);
    if (flags & ViewInterface::DETAILS_VIEW_FLAG_NEGATIVE_FEEDBACK)
      OnItemNegativeFeedback(NULL);
    if (flags & ViewInterface::DETAILS_VIEW_FLAG_REMOVE_BUTTON)
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
              ViewInterface::DETAILS_VIEW_FLAG_REMOVE_BUTTON) &&
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
              ViewInterface::DETAILS_VIEW_FLAG_REMOVE_BUTTON) &&
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
  Gadget::DisplayTarget target_;
  size_t max_content_items_;
  ContentItems content_items_;
  ScriptableHolder<ScriptableImage> pin_images_[PIN_IMAGE_COUNT];
  int pin_image_max_width_, pin_image_max_height_;
  bool mouse_down_, mouse_over_pin_;
  int mouse_x_, mouse_y_;
  ContentItem *mouse_over_item_;
  int content_height_;
  int scrolling_line_step_;
  int refresh_timer_;
  bool modified_; // Flags whether items were added, removed or reordered.
  bool *death_detector_;
  uint64_t context_menu_time_;
  std::string background_color_src_, mouseover_color_src_, mousedown_color_src_;
  double background_opacity_, mouseover_opacity_, mousedown_opacity_;
  Color background_color_, mouseover_color_, mousedown_color_;
};

ContentAreaElement::ContentAreaElement(BasicElement *parent, View *view,
                                       const char *name)
    : ScrollingElement(parent, view, "contentarea", name, false),
      impl_(new Impl(this)) {
  SetEnabled(true);
  SetAutoscroll(true);
}

void ContentAreaElement::DoRegister() {
  ScrollingElement::DoRegister();
  RegisterProperty("contentFlags", NULL, // Write only.
                   NewSlot(this, &ContentAreaElement::SetContentFlags));
  RegisterProperty("maxContentItems",
                   NewSlot(this, &ContentAreaElement::GetMaxContentItems),
                   NewSlot(this, &ContentAreaElement::SetMaxContentItems));
  RegisterProperty("backgroundColor",
                   NewSlot(this, &ContentAreaElement::GetBackgroundColor),
                   NewSlot(this, &ContentAreaElement::SetBackgroundColor));
  RegisterProperty("overColor",
                   NewSlot(this, &ContentAreaElement::GetOverColor),
                   NewSlot(this, &ContentAreaElement::SetOverColor));
  RegisterProperty("downColor",
                   NewSlot(this, &ContentAreaElement::GetDownColor),
                   NewSlot(this, &ContentAreaElement::SetDownColor));
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

std::string ContentAreaElement::GetBackgroundColor() const {
  return impl_->background_color_src_;
}

void ContentAreaElement::SetBackgroundColor(const char *color) {
  if (impl_->background_color_src_ != color) {
    if (Color::FromString(color, &impl_->background_color_, 
                          &impl_->background_opacity_)) {
      impl_->background_color_src_ = color;
      
      QueueDraw();
    }    
  }  
}

std::string ContentAreaElement::GetDownColor() const {
  return impl_->mousedown_color_src_;
}

void ContentAreaElement::SetDownColor(const char *color) {
  if (impl_->mousedown_color_src_ != color) {
    if (Color::FromString(color, &impl_->mousedown_color_, 
                          &impl_->mousedown_opacity_)) {
      impl_->mousedown_color_src_ = color;
      
      QueueDraw();
    }    
  }  
}

std::string ContentAreaElement::GetOverColor() const {
  return impl_->mouseover_color_src_;
}

void ContentAreaElement::SetOverColor(const char *color) {
  if (impl_->mouseover_color_src_ != color) {
    if (Color::FromString(color, &impl_->mouseover_color_, 
                          &impl_->mouseover_opacity_)) {
      impl_->mouseover_color_src_ = color;
      
      QueueDraw();
    }    
  }  
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

void ContentAreaElement::GetPinImages(ScriptableImage **unpinned,
                                      ScriptableImage **unpinned_over,
                                      ScriptableImage **pinned) {
  impl_->GetPinImages(unpinned, unpinned_over, pinned);
}

void ContentAreaElement::SetPinImages(ScriptableImage *unpinned,
                                      ScriptableImage *unpinned_over,
                                      ScriptableImage *pinned) {
  impl_->SetPinImages(unpinned, unpinned_over, pinned);
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
  } else {
    // Set reasonable scrolling step length.
    SetYPageStep(static_cast<int>(round(GetClientHeight())));
    SetYLineStep(impl_->scrolling_line_step_);
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

class FeedbackSlot : public Slot1<void, const char *> {
 public:
  FeedbackSlot(ContentAreaElement *content_area,
               Slot1<void, const char *> *slot)
      : content_area_(content_area), slot_(slot) {
  }

  virtual Variant Call(int argc, const Variant argv[]) const {
    // Test if the slot owner is still valid.
    if (content_area_.Get())
      return slot_->Call(argc, argv);
    return Variant();
  }
  // Not used.
  virtual bool operator==(const Slot &another) const { return false; }
 private:
  ElementHolder content_area_;
  Slot1<void, const char *> *slot_;
};

bool ContentAreaElement::OnAddContextMenuItems(MenuInterface *menu) {
  if (impl_->mouse_over_item_) {
    int item_flags = impl_->mouse_over_item_->GetFlags();
    if (!(item_flags & ContentItem::CONTENT_ITEM_FLAG_STATIC)) {
      impl_->context_menu_time_ = GetView()->GetCurrentTime();
      if (impl_->mouse_over_item_->CanOpen()) {
        menu->AddItem(GM_("OPEN_CONTENT_ITEM"), 0,
            new FeedbackSlot(this, NewSlot(impl_, &Impl::OnItemOpen)));
      }
      if (!(item_flags & ContentItem::CONTENT_ITEM_FLAG_NO_REMOVE)) {
        menu->AddItem(GM_("REMOVE_CONTENT_ITEM"), 0,
            new FeedbackSlot(this, NewSlot(impl_, &Impl::OnItemRemove)));
      }
      if (item_flags & ContentItem::CONTENT_ITEM_FLAG_NEGATIVE_FEEDBACK) {
        menu->AddItem(GM_("DONT_SHOW_CONTENT_ITEM"), 0,
            new FeedbackSlot(this,
                             NewSlot(impl_, &Impl::OnItemNegativeFeedback)));
      }
    }
  }
  // To keep compatible with the Windows version, don't show default menu items.
  return false;
}

ScriptableArray *ContentAreaElement::ScriptGetContentItems() {
  return impl_->ScriptGetContentItems();
}

void ContentAreaElement::ScriptSetContentItems(ScriptableInterface *array) {
  impl_->ScriptSetContentItems(array);
}

ScriptableArray *ContentAreaElement::ScriptGetPinImages() {
  return impl_->ScriptGetPinImages();
}

void ContentAreaElement::ScriptSetPinImages(ScriptableInterface *array) {
  impl_->ScriptSetPinImages(array);
}

bool ContentAreaElement::HasOpaqueBackground() const {
  return true;
}

} // namespace ggadget
