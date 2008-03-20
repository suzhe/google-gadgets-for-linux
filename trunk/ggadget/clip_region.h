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

#ifndef GGADGET_CLIP_REGION_H__
#define GGADGET_CLIP_REGION_H__

#include <ggadget/common.h>

namespace ggadget {

class BasicElement;
class Rectangle;
template <typename R, typename P> class Slot1;

class ClipRegion {
 public:
  ClipRegion();
  ~ClipRegion();

  /**
   * Add a rectangle into the region.
   */
  void AddRectangle(const Rectangle &rectangle);
  /**
   * Clear the region.
   */
  void Clear();
  /**
   * Judge if a point is in the region.
   */
  bool IsPointIn(double x, double y) const;
  /**
   * Judge if a rectange is overlapped with the region.
   */
  bool IsRectangleOverlapped(const Rectangle &rectangle) const;
  /**
   * Enumerate the rectangles. The region could be represented by a list of
   * rectangles. This method offer a way to enumerate the rectangles.
   * @param slot the slot that handle each rectangle. The slot has one
   *        parameter which is a pointer point to the rectangle. The return
   *        type of the slot is boolean value, return @c true if it keep going
   *        handling the rectangles and @c false when it want to stop.
   * @return @true if all rectangles are handled once and @false otherwise.
   */
  bool EnumerateRectangles(Slot1<bool, const void*> *slot) const;
#ifdef _DEBUG
  int GetCount() const;
#endif
 private:
  class Impl;
  Impl *impl_;
  DISALLOW_EVIL_CONSTRUCTORS(ClipRegion);
};

}  // namespace ggadget

#endif  // GGADGET_CLIP_REGION_H__
