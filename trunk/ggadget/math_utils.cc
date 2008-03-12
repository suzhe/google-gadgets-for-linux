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

#include <algorithm>
#include <cmath>

#include "math_utils.h"
#include <ggadget/logger.h>

namespace ggadget {

void ParentCoordToChildCoord(double parent_x, double parent_y,
                             double child_x_pos, double child_y_pos,
                             double child_pin_x, double child_pin_y,
                             double rotation_radians,
                             double *child_x, double *child_y) {
  double sin_theta = sin(rotation_radians);
  double cos_theta = cos(rotation_radians);
  double a_13 = child_pin_x - child_y_pos * sin_theta - child_x_pos * cos_theta;
  double a_23 = child_pin_y + child_x_pos * sin_theta - child_y_pos * cos_theta;

  *child_x = parent_x * cos_theta + parent_y * sin_theta + a_13;
  *child_y = parent_y * cos_theta - parent_x * sin_theta + a_23;
}

void ChildCoordToParentCoord(double child_x, double child_y,
                             double child_x_pos, double child_y_pos,
                             double child_pin_x, double child_pin_y,
                             double rotation_radians,
                             double *parent_x, double *parent_y) {
  double sin_theta = sin(rotation_radians);
  double cos_theta = cos(rotation_radians);
  double x0 = child_x_pos + child_pin_y * sin_theta - child_pin_x * cos_theta;
  double y0 = child_y_pos - child_pin_x * sin_theta - child_pin_y * cos_theta;

  *parent_x = child_x * cos_theta - child_y * sin_theta + x0;
  *parent_y = child_y * cos_theta + child_x * sin_theta + y0;
}

ChildCoordCalculator::ChildCoordCalculator(double child_x_pos,
                                           double child_y_pos,
                                           double child_pin_x,
                                           double child_pin_y,
                                           double rotation_radians) {
  sin_theta_ = sin(rotation_radians);
  cos_theta_ = cos(rotation_radians);
  a_13_ = child_pin_x - child_y_pos * sin_theta_ - child_x_pos * cos_theta_;
  a_23_ = child_pin_y + child_x_pos * sin_theta_ - child_y_pos * cos_theta_;
}

void GetChildExtentInParent(double child_x_pos, double child_y_pos,
                            double child_pin_x, double child_pin_y,
                            double child_width, double child_height,
                            double rotation_radians,
                            double *extent_width, double *extent_height) {
  rotation_radians = remainder(rotation_radians, 2 * M_PI);
  double sample_width_x, sample_width_y, sample_height_x, sample_height_y;
  if (rotation_radians < -M_PI_2) {
    // The bottom-left corner is the right most.
    sample_width_x = 0;
    sample_width_y = child_height;
    // The top-left corner is the lowest.
    sample_height_x = 0;
    sample_height_y = 0;
  } else if (rotation_radians < 0) {
    // The bottom-right corner is the right most.
    sample_width_x = child_width;
    sample_width_y = child_height;
    // The bottom-left corner is the lowest.
    sample_height_x = 0;
    sample_height_y = child_height;
  } else if (rotation_radians < M_PI_2) {
    // The top-right corner is the right most.
    sample_width_x = child_width;
    sample_width_y = 0;
    // The bottom-right corner is the lowest.
    sample_height_x = child_width;
    sample_height_y = child_height;
  } else {
    // The top-left corner is the right most.
    sample_width_x = 0;
    sample_width_y = 0;
    // The top-right corner is the lowest.
    sample_height_x = child_width;
    sample_height_y = 0;
  }

  ParentCoordCalculator calculator(child_x_pos, child_y_pos,
                                   child_pin_x, child_pin_y,
                                   rotation_radians);
  *extent_width = calculator.GetParentX(sample_width_x, sample_width_y);
  *extent_height = calculator.GetParentY(sample_height_x, sample_height_y);
}

void ChildCoordCalculator::Convert(double parent_x, double parent_y,
                                   double *child_x, double *child_y) {
  *child_x = parent_x * cos_theta_ + parent_y * sin_theta_ + a_13_;
  *child_y = parent_y * cos_theta_ - parent_x * sin_theta_ + a_23_;
}

double ChildCoordCalculator::GetChildX(double parent_x, double parent_y) {
  return parent_x * cos_theta_ + parent_y * sin_theta_ + a_13_;
}

double ChildCoordCalculator::GetChildY(double parent_x, double parent_y) {
  return parent_y * cos_theta_ - parent_x * sin_theta_ + a_23_;
}

ParentCoordCalculator::ParentCoordCalculator(double child_x_pos,
                                             double child_y_pos,
                                             double child_pin_x,
                                             double child_pin_y,
                                             double rotation_radians) {
  sin_theta_ = sin(rotation_radians);
  cos_theta_ = cos(rotation_radians);
  x0_ = child_x_pos + child_pin_y * sin_theta_ - child_pin_x * cos_theta_;
  y0_ = child_y_pos - child_pin_x * sin_theta_ - child_pin_y * cos_theta_;
}

void ParentCoordCalculator::Convert(double child_x, double child_y,
                                    double *parent_x, double *parent_y) {
  *parent_x = child_x * cos_theta_ - child_y * sin_theta_ + x0_;
  *parent_y = child_y * cos_theta_ + child_x * sin_theta_ + y0_;
}

double ParentCoordCalculator::GetParentX(double child_x, double child_y) {
  return child_x * cos_theta_ - child_y * sin_theta_ + x0_;
}

double ParentCoordCalculator::GetParentY(double child_x, double child_y) {
  return child_y * cos_theta_ + child_x * sin_theta_ + y0_;
}

double DegreesToRadians(double degrees) {
  return degrees * M_PI / 180.;
}

double RadiansToDegrees(double radians) {
  return radians * 180. / M_PI;
}

bool IsPointInElement(double x, double y, double width, double height) {
  return 0. <= x && 0. <= y && x < width && y < height;
}

static inline void GetMaxMin(double x, double y, double *max, double *min) {
  if (x > y) {
    *max = x;
    *min = y;
  } else {
    *max = y;
    *min = x;
  }
}

// Get extension rectangle for a deflective rectangle
void GetRectangleExtents(const double r[8],
                         double *x, double *y,
                         double *w, double *h) {
  double max1, max2, min1, min2;
  GetMaxMin(r[0], r[2], &max1, &min1);
  GetMaxMin(r[4], r[6], &max2, &min2);
  *x = std::min(min1, min2);
  *w = std::max(max1, max2) - *x;
  GetMaxMin(r[1], r[3], &max1, &min1);
  GetMaxMin(r[5], r[7], &max2, &min2);
  *y = std::min(min1, min2);
  *h = std::max(max1, max2) - *y;
}

void GetTwoRectanglesExtents(const double r1[4], const double r2[4],
                             double r3[4]) {
  r3[0] = std::min(r1[0], r2[0]);
  r3[1] = std::min(r1[1], r2[1]);
  r3[2] = std::max(r1[0] + r1[2], r2[0] + r2[2]) - r3[0];
  r3[3] = std::max(r1[1] + r1[3], r2[1] + r2[3]) - r3[1];
}

bool RectanglesOverlapped(const double r1[4], const double r2[4]) {
  double maxx = std::min(r1[0] + r1[2], r2[0] + r2[2]);
  double minx = std::max(r1[0], r2[0]);
  double maxy = std::min(r1[1] + r1[3], r2[1] + r2[3]);
  double miny = std::max(r1[1], r2[1]);
  if (maxx <= minx || maxy <= miny) return false;
  return true;
}

} // namespace ggadget
