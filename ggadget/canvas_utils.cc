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

#include "canvas_utils.h"
#include "canvas_interface.h"
#include "image_interface.h"

namespace ggadget {

void DrawCanvasArea(const CanvasInterface *src, double src_x, double src_y,
                    double src_width, double src_height,
                    CanvasInterface *dest, double dest_x, double dest_y,
                    double dest_width, double dest_height) {
  if (src_width > 0 && src_height > 0 &&
      dest_width != 0 && dest_height != 0) {
    double cx = dest_width / src_width;
    double cy = dest_height / src_height;
    dest->PushState();
    dest->IntersectRectClipRegion(dest_x, dest_y, dest_width, dest_height);
    dest->ScaleCoordinates(cx, cy);

    double draw_x = dest_x / cx - src_x;
    double draw_y = dest_y / cy - src_y;
    dest->DrawCanvas(draw_x, draw_y, src);
    dest->PopState();
  }
}

void StretchMiddleDrawCanvas(const CanvasInterface *src, CanvasInterface *dest,
                             double x, double y, double width, double height,
                             double left_border_width,
                             double top_border_height,
                             double right_border_width,
                             double bottom_border_height) {
  ASSERT(src && dest);
  if (!src || !dest)
    return;

  double src_width = src->GetWidth();
  double src_height = src->GetHeight();
  if (src_width < 4 || src_height < 4 ||
      (left_border_width <= 0 && top_border_height <= 0 &&
       right_border_width <= 0 && bottom_border_height <= 0)) {
    DrawCanvasArea(src, 0, 0, src_width, src_height, dest, x, y, width, height);
    return;
  }

  if (src && dest && src_width > 0 && src_height > 0) {
    if (src->GetWidth() == width && src->GetHeight() == height) {
      dest->DrawCanvas(x, y, src);
    } else {
      if (left_border_width == -1)
        left_border_width = src_width / 4;
      if (right_border_width == -1)
        right_border_width = src_width / 4;
      if (top_border_height == -1)
        top_border_height = src_height / 4;
      if (bottom_border_height == -1)
        bottom_border_height = src_height / 4;

      double total_border_width = left_border_width + right_border_width;
      double total_border_height = top_border_height + bottom_border_height;
      double src_middle_width = src_width - total_border_width;
      double src_middle_height = src_height - total_border_height;
      if (src_middle_width <= 0) {
        src_middle_width = src_width / 2;
        left_border_width = right_border_width = src_width / 4;
      }
      if (src_middle_height <= 0) {
        src_middle_height = src_width / 2;
        top_border_height = bottom_border_height = src_height / 4;
      }

      double dest_middle_width = width - total_border_width;
      double dest_middle_height = height - total_border_height;
      DrawCanvasArea(src, 0, 0,
                     left_border_width, top_border_height,
                     dest, x, y,
                     left_border_width, top_border_height);
      DrawCanvasArea(src, left_border_width, 0,
                     src_middle_width, top_border_height,
                     dest, left_border_width, 0,
                     dest_middle_width, top_border_height);
      DrawCanvasArea(src, src_width - right_border_width, 0,
                     right_border_width, top_border_height,
                     dest, width - right_border_width, 0,
                     right_border_width, top_border_height);
      DrawCanvasArea(src, 0, top_border_height,
                     left_border_width, src_middle_height,
                     dest, 0, top_border_height,
                     left_border_width, dest_middle_height);
      DrawCanvasArea(src, left_border_width, top_border_height,
                     src_middle_width, src_middle_height,
                     dest, left_border_width, top_border_height,
                     dest_middle_width, dest_middle_height);
      DrawCanvasArea(src, src_width - right_border_width, top_border_height,
                     right_border_width, src_middle_height,
                     dest, width - right_border_width, top_border_height,
                     right_border_width, dest_middle_height);
      DrawCanvasArea(src, 0, top_border_height,
                     left_border_width, src_middle_height,
                     dest, 0, top_border_height,
                     left_border_width, dest_middle_height);
      DrawCanvasArea(src, left_border_width, top_border_height,
                     src_middle_width, src_middle_height,
                     dest, left_border_width, top_border_height,
                     dest_middle_width, dest_middle_height);
      DrawCanvasArea(src, src_width - right_border_width, top_border_height,
                     right_border_width, src_middle_height,
                     dest, width - right_border_width, top_border_height,
                     right_border_width, dest_middle_height);
    }
  }
}

void StretchMiddleDrawImage(const ImageInterface *src, CanvasInterface *dest,
                            double x, double y, double width, double height,
                            double left_border_width,
                            double top_border_height,
                            double right_border_width,
                            double bottom_border_height) {
  ASSERT(src && dest);
  if (!src || !dest)
    return;

  double src_width = src->GetWidth();
  double src_height = src->GetHeight();
  if (src_width < 4 || src_height < 4 ||
      (left_border_width <= 0 && top_border_height <= 0 &&
       right_border_width <= 0 && bottom_border_height <= 0)) {
    src->StretchDraw(dest, x, y, width, height);
    return;
  }

  const CanvasInterface *src_canvas = src->GetCanvas();
  ASSERT(src_canvas);
  if (!src_canvas)
    return;

  StretchMiddleDrawCanvas(src_canvas, dest, x, y, width, height,
                          left_border_width, top_border_height,
                          right_border_width, bottom_border_height);
}

}  // namespace ggadget
