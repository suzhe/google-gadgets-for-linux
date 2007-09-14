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

#include "math_utils.h"

namespace ggadget {

void ChildCoordFromParentCoord(double parent_x, double parent_y,
                               double child_x_pos, double child_y_pos,
                               double child_pin_x, double child_pin_y,
                               double rotation_radians,
                               double *child_x, double *child_y) {
  double sin_theta = sin(rotation_radians);
  double cos_theta = cos(rotation_radians);
  double px_x0 = child_pin_x + child_x_pos;
  double py_y0 = child_pin_y + child_y_pos;
  double a_13 = child_pin_x - py_y0 * sin_theta - px_x0 * cos_theta;
  double a_23 = child_pin_y + px_x0 * sin_theta - py_y0 * cos_theta;
  
  *child_x = parent_x * cos_theta + parent_y * sin_theta + a_13;
  *child_y = parent_y * cos_theta - parent_x * sin_theta + a_23;
}

ChildCoordCalculator::ChildCoordCalculator(double child_x_pos, double child_y_pos,
                                           double child_pin_x, double child_pin_y,
                                           double rotation_radians) {
  double px_x0 = child_pin_x + child_x_pos;
  double py_y0 = child_pin_y + child_y_pos;
  
  sin_theta = sin(rotation_radians);
  cos_theta = cos(rotation_radians);
  a_13 = child_pin_x - py_y0 * sin_theta - px_x0 * cos_theta;
  a_23 = child_pin_y + px_x0 * sin_theta - py_y0 * cos_theta;  
}

void ChildCoordCalculator::Convert(double parent_x, double parent_y, 
                                   double *child_x, double *child_y) {
  *child_x = parent_x * cos_theta + parent_y * sin_theta + a_13;
  *child_y = parent_y * cos_theta - parent_x * sin_theta + a_23;    
}  

double ChildCoordCalculator::GetChildX(double parent_x, double parent_y) {
  return parent_x * cos_theta + parent_y * sin_theta + a_13;
}

/**
 * @param parent_x X-coordinate in the parent space to convert.
 * @param parent_y Y-coordinate in the parent space to convert. 
 * @return The converted child Y-coordinate.
 */
double ChildCoordCalculator::GetChildY(double parent_x, double parent_y) {
  return parent_y * cos_theta - parent_x * sin_theta + a_23;  
}

} // namespace ggadget
