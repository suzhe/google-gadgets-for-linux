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
#include "clip_region.h"
#include "math_utils.h"
#include "slot.h"
#include "small_object.h"

namespace ggadget {

typedef std::vector<Rectangle> RectangleVector;

class ClipRegion::Impl : public SmallObject<> {
 public:
  Impl(double fuzzy_ratio)
    : fuzzy_ratio_(Clamp(fuzzy_ratio, 0.5, 1.0)) {
  }

  /**
   * This method merges two overlapped rectangles a and b into one rectangle,
   * and stores the result into out.
   *
   * @return 0 means the overlap ratio of a and b is less than fuzzy_ratio,
   *         thus can't merge them.
   *         1 means the overlap ratio is large enough and a is larger than b,
   *         so b is merged into a.
   *         -1 means a is merged into b.
   */
  int MergeRectangles(const Rectangle &a, const Rectangle &b, Rectangle *out) {
    Rectangle rect(a);
    if (a == b) {
      *out = rect;
      return 1;
    } else if (a.Overlaps(b)) {
      rect.Union(b);
      double fuzzy_area = rect.w * rect.h * fuzzy_ratio_;
      double a_area = a.w * a.h;
      double b_area = b.w * b.h;
      if (a_area >= fuzzy_area || b_area >= fuzzy_area) {
        *out = rect;
        return a_area >= b_area ? 1 : -1;
      }
    } else if ((a.y == b.y && a.h == b.h &&
                ((a.x + a.w >= b.x && a.x <= b.x) ||
                 (b.x + b.w >= a.x && b.x <= a.x))) ||
               (a.x == b.x && a.w == b.w &&
                ((a.y + a.h >= b.y && a.y <= b.y) ||
                 (b.y + b.h >= a.y && b.y <= a.y)))) {
      // Merge neighbor rectangles.
      rect.Union(b);
      *out = rect;
      return 1;
    }
    return 0;
  }

 public:
  double fuzzy_ratio_;
  RectangleVector rectangles_;
};

ClipRegion::ClipRegion()
  : impl_(new Impl(1.0)) {
}

ClipRegion::ClipRegion(double fuzzy_ratio)
  : impl_(new Impl(fuzzy_ratio)) {
}

ClipRegion::ClipRegion(const ClipRegion &region)
  : impl_(new Impl(region.impl_->fuzzy_ratio_)) {
  impl_->rectangles_ = region.impl_->rectangles_;
}

ClipRegion::~ClipRegion() {
  delete impl_;
  impl_ = NULL;
}

const ClipRegion& ClipRegion::operator = (const ClipRegion &region) {
  impl_->fuzzy_ratio_ = region.impl_->fuzzy_ratio_;
  impl_->rectangles_ = region.impl_->rectangles_;
  return *this;
}

double ClipRegion::GetFuzzyRatio() const {
  return impl_->fuzzy_ratio_;
}

void ClipRegion::SetFuzzyRatio(double fuzzy_ratio) {
  impl_->fuzzy_ratio_ = Clamp(fuzzy_ratio, 0.5, 1.0);
}

void ClipRegion::AddRectangle(const Rectangle &rect) {
  if (!rect.w || !rect.h) return;

  RectangleVector new_rectangles;
  Rectangle big_rect(rect);
  for (RectangleVector::iterator it = impl_->rectangles_.begin();
       it != impl_->rectangles_.end(); ++it) {
    if (impl_->MergeRectangles(big_rect, *it, &big_rect) == 0)
      new_rectangles.push_back(*it);
  }
  new_rectangles.push_back(big_rect);
  impl_->rectangles_.swap(new_rectangles);
}

bool ClipRegion::IsEmpty() const {
  return impl_->rectangles_.size() == 0;
}

void ClipRegion::Clear() {
  impl_->rectangles_.clear();
}

bool ClipRegion::IsPointIn(double x, double y) const {
  for (RectangleVector::const_iterator it = impl_->rectangles_.begin();
       it != impl_->rectangles_.end(); ++it)
    if (it->IsPointIn(x, y)) return true;
  return false;
}

bool ClipRegion::Overlaps(const Rectangle &rect) const {
  for (RectangleVector::const_iterator it = impl_->rectangles_.begin();
       it != impl_->rectangles_.end(); ++it)
    if (it->Overlaps(rect)) return true;
  return false;
}

bool ClipRegion::IsInside(const Rectangle &rect) const {
  for (RectangleVector::const_iterator it = impl_->rectangles_.begin();
       it != impl_->rectangles_.end(); ++it)
    if (!it->IsInside(rect)) return false;
  // If the clip region is empty then return false.
  return impl_->rectangles_.size() != 0;
}

Rectangle ClipRegion::GetExtents() const {
  RectangleVector::const_iterator it = impl_->rectangles_.begin();
  Rectangle rect(*it);
  for (++it; it != impl_->rectangles_.end(); ++it)
    rect.Union(*it);
  return rect;
}

void ClipRegion::Integerize() {
  for (RectangleVector::iterator it = impl_->rectangles_.begin();
       it != impl_->rectangles_.end(); ++it)
    it->Integerize(true);
}

bool ClipRegion::EnumerateRectangles(RectangleSlot *slot) const {
  bool result = false;
  if (slot) {
    for (RectangleVector::const_iterator it = impl_->rectangles_.begin();
         it != impl_->rectangles_.end(); ++it) {
      if (!(result = (*slot)(it->x, it->y, it->w, it->h)))
        break;
    }
    delete slot;
  }
  return result;
}

void ClipRegion::PrintLog() const {
#ifdef _DEBUG
  DLOG("%zu Clip Regions:", impl_->rectangles_.size());
  for (RectangleVector::const_iterator it = impl_->rectangles_.begin();
       it != impl_->rectangles_.end(); ++it) {
    DLOG("(%.1lf,%.1lf) - (%.1lf,%.1lf); w: %.1lf h: %.1lf",
         it->x, it->y, it->x + it->w, it->y + it->h, it->w, it->h);
  }
#endif
}

}  // namespace ggadget
