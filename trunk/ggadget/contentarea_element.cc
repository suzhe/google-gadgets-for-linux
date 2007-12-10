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
        pin_image_max_width_(0), pin_image_max_height_(0) {
    pin_images_[PIN_IMAGE_PINNED] = owner->GetView()->LoadImageFromGlobal(
        kContentItemPinned, false);
    pin_images_[PIN_IMAGE_PINNED_OVER] = owner->GetView()->LoadImageFromGlobal(
        kContentItemPinnedOver, false);
    pin_images_[PIN_IMAGE_UNPINNED] = owner->GetView()->LoadImageFromGlobal(
        kContentItemUnpinned, false);
  }

  ~Impl() {
    RemoveAllContentItems();
    for (size_t i = 0; i < arraysize(pin_images_); i++) {
      delete pin_images_[i];
      pin_images_[i] = NULL;
    }
    layout_canvas_->Destroy();
    layout_canvas_ = NULL;
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

    int y = kItemBorderWidth;
    int width = static_cast<int>(ceil(owner_->GetPixelWidth())) -
                2 * kItemBorderWidth;
    int item_width = width - pin_image_max_width_;

    if (!(content_flags_ & CONTENT_FLAG_MANUAL_LAYOUT)) {
      size_t item_count = content_items_.size();
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
    }
  }

  void Draw(ContentAreaElement *owner, CanvasInterface *canvas) {
    int width = static_cast<int>(ceil(owner->GetPixelWidth()));
    int height = static_cast<int>(ceil(owner->GetPixelHeight()));
    canvas->DrawFilledRect(0, 0, width, height, kDefaultBackground);

    int item_count = content_items_.size();
    for (int i = 0; i < item_count; i++) {
      ContentItem *item = content_items_[i];
      ASSERT(item);
      if (item->GetFlags() & ContentItem::CONTENT_ITEM_FLAG_HIDDEN)
        continue;

      int item_x = 0, item_y = 0, item_width = 0, item_height = 0;
      item->GetRect(&item_x, &item_y, &item_width, &item_height);
      if (item_width > 0 && item_height > 0 && item_y < height) {
        if (content_flags_ & CONTENT_FLAG_PINNABLE &&
            pin_image_max_width_ > 0 && pin_image_max_height_ > 0) {
          Image *pin_image = pin_images_[PIN_IMAGE_UNPINNED];
          if (item->GetFlags() & ContentItem::CONTENT_ITEM_FLAG_PINNED) {
            pin_image = pin_images_[PIN_IMAGE_PINNED];
            // TODO: mouseover
          }
          if (pin_image)
            pin_image->Draw(canvas, item_x, item_y);

          item_x += pin_image_max_width_;
          item_width -= pin_image_max_width_;
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
    // TODO:
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
    owner_->QueueDraw();
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
    max_content_items = std::min(max_content_items, kMaxContentItemsUpperLimit);
    if (max_content_items_ != max_content_items) {
      max_content_items_ = max_content_items;
      return RemoveExtraItems();
    }
    return false;
  }

  bool AddContentItem(ContentItem *item, DisplayOptions options) {
    ContentItems::iterator it = std::find(content_items_.begin(),
                                          content_items_.end(),
                                          item);
    if (it == content_items_.end()) {
      // Insert the new item at the beginning on default.
      it = content_items_.begin();
      if (content_flags_ & CONTENT_FLAG_PINNABLE) {
        // Pinned items are shown in the same order that they were added.
        for (; it != content_items_.end(); ++it) {
          if ((*it)->GetFlags() & ContentItem::CONTENT_ITEM_FLAG_PINNED)
            break;
        }
      }

      item->AttachContentArea(owner_);
      content_items_.insert(it, item);
      RemoveExtraItems();
      return true;
    }
    return false;
  }

  bool RemoveExtraItems() {
    if (content_items_.size() <= max_content_items_)
      return false;

    bool all_pinned = false;
    while (content_items_.size() > max_content_items_) {
      ContentItems::iterator it = content_items_.end() - 1;
      if (!all_pinned && (content_flags_ & CONTENT_FLAG_PINNABLE)) {
        // Find the first unpinned item which can be removed. If cant find
        // anything last item will be removed.
        for (; it != content_items_.begin(); --it) {
          if ((*it)->GetFlags() & ContentItem::CONTENT_ITEM_FLAG_PINNED)
            break;
        }
        if (it == content_items_.begin() && 
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
    return EVENT_RESULT_UNHANDLED;
  }

  ContentAreaElement *owner_;
  CanvasInterface *layout_canvas_; // Only used during Layout().
  int content_flags_;
  GadgetInterface::DisplayTarget target_;
  size_t max_content_items_;
  ContentItems content_items_;
  Image *pin_images_[PIN_IMAGE_COUNT];
  int pin_image_max_width_, pin_image_max_height_;
};

ContentAreaElement::ContentAreaElement(BasicElement *parent, View *view,
                                       const char *name)
    : BasicElement(parent, view, "contentarea", name, false),
      impl_(new Impl(this)) {
  SetEnabled(true);
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
  BasicElement::Layout();
  impl_->Layout();
}

void ContentAreaElement::DoDraw(CanvasInterface *canvas,
                                const CanvasInterface *children_canvas) {
  impl_->Draw(this, canvas);
}

EventResult ContentAreaElement::HandleMouseEvent(const MouseEvent &event) {
  return impl_->HandleMouseEvent(event);
}

BasicElement *ContentAreaElement::CreateInstance(BasicElement *parent,
                                                 View *view,
                                                 const char *name) {
  return new ContentAreaElement(parent, view, name);
}

} // namespace ggadget
