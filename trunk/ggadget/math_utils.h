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

#ifndef GGADGET_MATH_UTILS_H__
#define GGADGET_MATH_UTILS_H__

#include <cmath>

namespace ggadget {

/**
 * Converts coordinates in a parent element's space to coordinates in a
 * child element.
 * @param parent_x X-coordinate in the parent space to convert.
 * @param parent_y Y-coordinate in the parent space to convert. 
 * @param child_x_pos X-coordinate of the child (0, 0) point in parent space.
 * @param child_y_pos Y-coordinate of the child (0, 0) point in parent space.
 * @param child_pin_x X-coordinate of the child rotation pin in child space.
 * @param child_pin_y Y-coordinate of the child rotation pin in child space.
 * @param rotation_radians The rotation of the child element in radians.
 * @param[out] child_x Parameter to store the converted child X-coordinate.
 * @param[out] child_y Parameter to store the converted child Y-coordinate.
 */
void ChildCoordFromParentCoord(double parent_x, double parent_y,
                               double child_x_pos, double child_y_pos,
                               double child_pin_x, double child_pin_y,
                               double rotation_radians,
                               double *child_x, double *child_y);

/**
 * Calculator object used to convert a parent element's coordinate space to 
 * that of a child element. This struct is a better choice if the multiple
 * coordinate conversions need to be done for the same child element.
 */
class ChildCoordCalculator {
 public:
  /**
   * Constructs the coordinate calculator object.
   * @param child_x_pos X-coordinate of the child (0, 0) point in parent space.
   * @param child_y_pos Y-coordinate of the child (0, 0) point in parent space.
   * @param child_pin_x X-coordinate of the child rotation pin in child space.
   * @param child_pin_y Y-coordinate of the child rotation pin in child space.
   * @param rotation_radians The rotation of the child element in radians.
   */
  ChildCoordCalculator(double child_x_pos, double child_y_pos,
                       double child_pin_x, double child_pin_y,
                       double rotation_radians);
  
  /**
   * Converts coordinates the given coordinates.
   * @param parent_x X-coordinate in the parent space to convert.
   * @param parent_y Y-coordinate in the parent space to convert. 
   * @param[out] child_x Parameter to store the converted child X-coordinate.
   * @param[out] child_y Parameter to store the converted child Y-coordinate.
   */
  void Convert(double parent_x, double parent_y, 
               double *child_x, double *child_y);
  
  /**
   * @param parent_x X-coordinate in the parent space to convert.
   * @param parent_y Y-coordinate in the parent space to convert. 
   * @return The converted child X-coordinate.
   */
  double GetChildX(double parent_x, double parent_y);
  
  /**
   * @param parent_x X-coordinate in the parent space to convert.
   * @param parent_y Y-coordinate in the parent space to convert. 
   * @return The converted child Y-coordinate.
   */
  double GetChildY(double parent_x, double parent_y);

 private:
  double sin_theta, cos_theta;
  double a_13, a_23;
};

/**
 * @return The radian measure of the input parameter.
 */
double DegreesToRadians(double degrees);

/**
 * Checks to see if the given (x, y) is contained in an element.
 * @param x X-coordinate of the element's (0, 0) point in parent space.
 * @param y Y-coordinate of the element's (0, 0) point in parent space.
 * @param width Width of element.
 * @param height Height of element.
 */
bool IsPointInElement(double x, double y, double width, double height);

}

#endif // GGADGET_MATH_UTILS_H__
