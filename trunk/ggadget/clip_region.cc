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
#include "clip_region.h"
#include <ggadget/math_utils.h>
#include <ggadget/slot.h>

namespace ggadget {

typedef std::vector<Rectangle> RectangleList;

class ClipRegion::Impl {
 public:
  Impl() : maximized_(false) {
  }
  ~Impl() {
  }
  void MergeRectangle(const Rectangle &r) {
    const double kMergeFactor = 0.9;
    for (RectangleList::iterator it = rectangles_.begin();
         it != rectangles_.end(); ++it) {
      if (!it->Overlaps(r)) continue;

      Rectangle big_rect;
      big_rect.ExtentsFromTwoRects(*it, r);
      // merge the two rectangles as one. We didn't check if the merged one
      // would be largely overlapped with other existed ones, it is not
      // necessary.
      if (r.x_ * r.y_ + it->x_ * it->y_ >
          kMergeFactor * big_rect.x_ * big_rect.y_) {
        *it = big_rect;
        return;
      }
    }
    rectangles_.push_back(r);
  }
 public:
  bool maximized_;
  RectangleList rectangles_;
};

ClipRegion::ClipRegion() : impl_(new Impl()) {
}

ClipRegion::~ClipRegion() {
  delete impl_;
  impl_ = NULL;
}

void ClipRegion::AddRectangle(const Rectangle &rectangle) {
  if (impl_->maximized_) return;
  impl_->MergeRectangle(rectangle);
}

void ClipRegion::Clear() {
  impl_->maximized_ = false;
  impl_->rectangles_.clear();
}

void ClipRegion::SetMaximized(bool maximized) {
  impl_->maximized_ = maximized;
}

bool ClipRegion::IsPointIn(double x, double y) const {
  if (impl_->maximized_) return true;

  for (RectangleList::const_iterator it = impl_->rectangles_.begin();
       it != impl_->rectangles_.end(); ++it)
    if (it->IsPointIn(x, y)) return true;
  return false;
}

bool ClipRegion::IsRectangleOverlapped(const Rectangle &rectangle) const {
  if (impl_->maximized_) return true;

  for (RectangleList::const_iterator it = impl_->rectangles_.begin();
       it != impl_->rectangles_.end(); ++it)
    if (it->Overlaps(rectangle)) return true;
  return false;
}

bool ClipRegion::EnumerateRectangles(Slot1<bool, const void*> *slot) const {
  if (slot) {
    for (RectangleList::const_iterator it = impl_->rectangles_.begin();
         it != impl_->rectangles_.end(); ++it) {
      if (!(*slot)(reinterpret_cast<const void*>(&(*it)))) {
        delete slot;
        return false;
      }
    }
    delete slot;
    return true;
  }
  return false;
}

#ifdef _DEBUG
int ClipRegion::GetCount() const {
  return impl_->rectangles_.size();
}
#endif

}  // namespace ggadget
