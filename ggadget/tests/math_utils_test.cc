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

#include <cstdio>
#include <cmath>

#include "ggadget/math_utils.h"
#include "unittest/gunit.h"

using namespace ggadget;

const double kErrorDelta = .00000001;

TEST(MathUtilsTest, ChildCoordCalculator) {
  double child_x, child_y;

  ChildCoordCalculator calc = ChildCoordCalculator(0, 0, 50, 50, M_PI_2);
  calc.Convert(0, 0, &child_x, &child_y);
  EXPECT_DOUBLE_EQ(child_x, calc.GetChildX(0, 0));
  EXPECT_DOUBLE_EQ(child_y, calc.GetChildY(0, 0));
  EXPECT_NEAR(50., child_x, kErrorDelta);
  EXPECT_DOUBLE_EQ(50., child_y);

  calc = ChildCoordCalculator(0, 0, 50, 50, M_PI);
  calc.Convert(0, 0, &child_x, &child_y);
  EXPECT_DOUBLE_EQ(child_x, calc.GetChildX(0, 0));
  EXPECT_DOUBLE_EQ(child_y, calc.GetChildY(0, 0));
  EXPECT_DOUBLE_EQ(50., child_x);
  EXPECT_DOUBLE_EQ(50., child_y);

  calc = ChildCoordCalculator(0, 0, 50, 50, M_PI + M_PI_2);
  calc.Convert(0, 0, &child_x, &child_y);
  EXPECT_DOUBLE_EQ(child_x, calc.GetChildX(0, 0));
  EXPECT_DOUBLE_EQ(child_y, calc.GetChildY(0, 0));
  EXPECT_DOUBLE_EQ(50., child_x);
  EXPECT_DOUBLE_EQ(50., child_y);

  calc = ChildCoordCalculator(0, 0, 50, 50, 2 * M_PI);
  calc.Convert(0, 0, &child_x, &child_y);
  EXPECT_DOUBLE_EQ(child_x, calc.GetChildX(0, 0));
  EXPECT_DOUBLE_EQ(child_y, calc.GetChildY(0, 0));
  EXPECT_DOUBLE_EQ(50., child_x);
  EXPECT_DOUBLE_EQ(50., child_y);

  ChildCoordCalculator calc2(0, 0, 0, 0, 0);
  for (int i = 0; i < 360; i++) {
    calc2.Convert(i, i, &child_x, &child_y);
    EXPECT_DOUBLE_EQ(child_x, calc2.GetChildX(i, i));
    EXPECT_DOUBLE_EQ(child_y, calc2.GetChildY(i, i));
    EXPECT_DOUBLE_EQ(i, child_x);
    EXPECT_DOUBLE_EQ(i, child_y);

    calc = ChildCoordCalculator(i, i, 0, 0, 0);
    calc.Convert(0, 0, &child_x, &child_y);
    EXPECT_DOUBLE_EQ(child_x, calc.GetChildX(0, 0));
    EXPECT_DOUBLE_EQ(child_y, calc.GetChildY(0, 0));
    EXPECT_DOUBLE_EQ(-i, child_x);
    EXPECT_DOUBLE_EQ(-i, child_y);

    calc = ChildCoordCalculator(0, 0, i, i, 0);
    calc.Convert(0, 0, &child_x, &child_y);
    EXPECT_DOUBLE_EQ(child_x, calc.GetChildX(0, 0));
    EXPECT_DOUBLE_EQ(child_y, calc.GetChildY(0, 0));
    EXPECT_DOUBLE_EQ(i, child_x);
    EXPECT_DOUBLE_EQ(i, child_y);

    // distance should be constant in a circular rotation around origin
    calc = ChildCoordCalculator(0, 0, 0, 0, DegreesToRadians(i));
    calc.Convert(100, 100, &child_x, &child_y);
    EXPECT_DOUBLE_EQ(child_x, calc.GetChildX(100, 100));
    EXPECT_DOUBLE_EQ(child_y, calc.GetChildY(100, 100));
    EXPECT_DOUBLE_EQ(20000., child_x * child_x + child_y * child_y);

    // distance should be constant in a circular rotation around top-left
    calc = ChildCoordCalculator(100, 100, 0, 0, DegreesToRadians(i));
    calc.Convert(0, 0, &child_x, &child_y);
    EXPECT_DOUBLE_EQ(child_x, calc.GetChildX(0, 0));
    EXPECT_DOUBLE_EQ(child_y, calc.GetChildY(0, 0));
    EXPECT_DOUBLE_EQ(20000., child_x * child_x + child_y * child_y);

    // distance to pin should be constant in a circular rotation
    calc = ChildCoordCalculator(0, 0, 1, 1, DegreesToRadians(i));
    calc.Convert(0, 0, &child_x, &child_y);
    EXPECT_DOUBLE_EQ(child_x, calc.GetChildX(0, 0));
    EXPECT_DOUBLE_EQ(child_y, calc.GetChildY(0, 0));
    EXPECT_NEAR(0., (child_x - 1) * (child_x - 1) +
                    (child_y - 1) * (child_y - 1), kErrorDelta);
  }
}

TEST(MathUtilsTest, GetChildCoord) {
  double child_x, child_y;

  ParentCoordToChildCoord(0, 0, 0, 0, 50, 50, 0, &child_x, &child_y);
  EXPECT_DOUBLE_EQ(50, child_x);
  EXPECT_DOUBLE_EQ(50, child_y);

  ParentCoordToChildCoord(0, 0, 0, 0, 50, 50, M_PI_2, &child_x, &child_y);
  EXPECT_NEAR(50., child_x, kErrorDelta);
  EXPECT_DOUBLE_EQ(50., child_y);

  ParentCoordToChildCoord(0, 0, 0, 0, 50, 50, M_PI, &child_x, &child_y);
  EXPECT_DOUBLE_EQ(50., child_x);
  EXPECT_DOUBLE_EQ(50., child_y);

  ParentCoordToChildCoord(0, 0, 0, 0, 50, 50, M_PI + M_PI_2,
                            &child_x, &child_y);
  EXPECT_DOUBLE_EQ(50., child_x);
  EXPECT_DOUBLE_EQ(50., child_y);

  ParentCoordToChildCoord(0, 0, 0, 0, 50, 50, 2 * M_PI, &child_x, &child_y);
  EXPECT_DOUBLE_EQ(50., child_x);
  EXPECT_DOUBLE_EQ(50., child_y);

  for (int i = 0; i < 360; i++) {
    ParentCoordToChildCoord(i, i, 0, 0, 0, 0, 0, &child_x, &child_y);
    EXPECT_DOUBLE_EQ(i, child_x);
    EXPECT_DOUBLE_EQ(i, child_y);

    ParentCoordToChildCoord(0, 0, i, i, 0, 0, 0, &child_x, &child_y);
    EXPECT_DOUBLE_EQ(-i, child_x);
    EXPECT_DOUBLE_EQ(-i, child_y);

    ParentCoordToChildCoord(0, 0, 0, 0, i, i, 0, &child_x, &child_y);
    EXPECT_DOUBLE_EQ(i, child_x);
    EXPECT_DOUBLE_EQ(i, child_y);

    // distance should be constant in a circular rotation around origin
    ParentCoordToChildCoord(100, 100, 0, 0, 0, 0, DegreesToRadians(i), 
                            &child_x, &child_y);
    EXPECT_DOUBLE_EQ(20000., child_x * child_x + child_y * child_y);

    // distance should be constant in a circular rotation around top-left
    ParentCoordToChildCoord(0, 0, 100, 100, 0, 0, DegreesToRadians(i), 
                            &child_x, &child_y);
    EXPECT_DOUBLE_EQ(20000., child_x * child_x + child_y * child_y);

    // distance to pin should be constant in a circular rotation
    ParentCoordToChildCoord(0, 0, 0, 0, 1, 1, DegreesToRadians(i), 
                            &child_x, &child_y);
    EXPECT_NEAR(0., (child_x - 1) * (child_x - 1) +
                    (child_y - 1) * (child_y - 1), kErrorDelta);
  }
}

TEST(MathUtilsTest, GetParentCoord) {
  double parent_x, parent_y;

  ChildCoordToParentCoord(40, 50, 0, 0, 40, 50, 0, &parent_x, &parent_y);
  EXPECT_NEAR(0, parent_x, kErrorDelta);
  EXPECT_NEAR(0, parent_y, kErrorDelta);

  ChildCoordToParentCoord(40, 50, 0, 0, 40, 50, M_PI_2,
                          &parent_x, &parent_y);
  EXPECT_NEAR(0, parent_x, kErrorDelta);
  EXPECT_NEAR(0, parent_y, kErrorDelta);

  ChildCoordToParentCoord(40, 50, 0, 0, 40, 50, M_PI, &parent_x, &parent_y);
  EXPECT_NEAR(0, parent_x, kErrorDelta);
  EXPECT_NEAR(0, parent_y, kErrorDelta);

  ChildCoordToParentCoord(40, 50, 0, 0, 40, 50, M_PI + M_PI_2,
                          &parent_x, &parent_y);
  EXPECT_NEAR(0, parent_x, kErrorDelta);
  EXPECT_NEAR(0, parent_y, kErrorDelta);

  ChildCoordToParentCoord(40, 50, 0, 0, 40, 50, 2 * M_PI,
                          &parent_x, &parent_y);
  EXPECT_NEAR(0, parent_x, kErrorDelta);
  EXPECT_NEAR(0, parent_y, kErrorDelta);

  for (int i = 0; i < 360; i++) {
    ChildCoordToParentCoord(i, i, 0, 0, 0, 0, 0, &parent_x, &parent_y);
    EXPECT_DOUBLE_EQ(i, parent_x);
    EXPECT_DOUBLE_EQ(i, parent_y);

    ChildCoordToParentCoord(0, 0, i, i, 0, 0, 0, &parent_x, &parent_y);
    EXPECT_DOUBLE_EQ(i, parent_x);
    EXPECT_DOUBLE_EQ(i, parent_y);

    ChildCoordToParentCoord(0, 0, 0, 0, i, i, 0, &parent_x, &parent_y);
    EXPECT_DOUBLE_EQ(-i, parent_x);
    EXPECT_DOUBLE_EQ(-i, parent_y);

    // distance should be constant in a circular rotation around origin
    ChildCoordToParentCoord(100, 100, 0, 0, 0, 0, DegreesToRadians(i), 
                            &parent_x, &parent_y);
    EXPECT_DOUBLE_EQ(20000., parent_x * parent_x + parent_y * parent_y);

    // distance should be constant in a circular rotation around top-left
    ChildCoordToParentCoord(0, 0, 100, 100, 0, 0, DegreesToRadians(i), 
                            &parent_x, &parent_y);
    EXPECT_DOUBLE_EQ(20000., parent_x * parent_x + parent_y * parent_y);

    // distance to pin should be constant in a circular rotation
    ChildCoordToParentCoord(0, 0, 0, 0, 1, 1, DegreesToRadians(i), 
                            &parent_x, &parent_y);
    EXPECT_NEAR(2., parent_x * parent_x + parent_y * parent_y, kErrorDelta);
  }
}

TEST(MathUtilsTest, TestBackAndForth) {
  const double child_x_pos = 25;
  const double child_y_pos = 48;
  const double pin_x = 77;
  const double pin_y = 71;
  const double parent_x = 123.4;
  const double parent_y = 432.1;

  for (int i = 0; i < 360; i++) {
    double child_x, child_y;
    ParentCoordToChildCoord(parent_x, parent_y, child_x_pos, child_y_pos,
                            pin_x, pin_y, DegreesToRadians(i),
                            &child_x, &child_y);
    double parent_x1, parent_y1;
    ChildCoordToParentCoord(child_x, child_y, child_x_pos, child_y_pos,
                            pin_x, pin_y, DegreesToRadians(i),
                            &parent_x1, &parent_y1);
    EXPECT_NEAR(parent_x, parent_x1, kErrorDelta);
    EXPECT_NEAR(parent_y, parent_y1, kErrorDelta);
  }
}

TEST(MathUtilsTest, CheckPointInElement) {
  EXPECT_TRUE(IsPointInElement(0, 0, 50, 20));
  EXPECT_TRUE(IsPointInElement(1, 1, 50, 20));
  EXPECT_TRUE(IsPointInElement(49.9, 19.9, 50, 20));
  EXPECT_FALSE(IsPointInElement(-5, 0, 50, 20));
  EXPECT_FALSE(IsPointInElement(0, -5, 50, 20));
  EXPECT_FALSE(IsPointInElement(0, 30, 50, 20));
  EXPECT_FALSE(IsPointInElement(60, 0, 50, 20));
}

TEST(MathUtilsTest, DegreesToRadians) {
  EXPECT_EQ(2 * M_PI, DegreesToRadians(360.));
  EXPECT_EQ(0., DegreesToRadians(0.));
  EXPECT_EQ(M_PI, DegreesToRadians(180.));
}

TEST(MathUtilsTest, GetChildExtentInParent) {
  double extent_width, extent_height;
  GetChildExtentInParent(40, 50, 0, 0, 7, 8, 0, &extent_width, &extent_height);
  EXPECT_EQ(47, extent_width);
  EXPECT_EQ(58, extent_height);
  GetChildExtentInParent(40, 50, 3, 4, 7, 8, 0, &extent_width, &extent_height);
  EXPECT_EQ(44, extent_width);
  EXPECT_EQ(54, extent_height);
  // TODO: Add more tests.
}

// Test if x is really a rectangle
#define IS_RECT(x) \
    do { \
      EXPECT_EQ(0, (x[0]-x[6])*(x[3]-x[5])-(x[1]-x[7])*(x[2]-x[4])); \
      EXPECT_EQ(0, (x[0]-x[2])*(x[7]-x[5])-(x[1]-x[3])*(x[6]-x[4])); \
      EXPECT_EQ(0, (x[0]-x[2])*(x[0]-x[6])+(x[1]-x[3])*(x[1]-x[7])); \
    } while (false)
#define DEQ(x, y) (fabs((x) - (y)) < 1e-6)

TEST(MathUtilsTest, GetRectangleExtents) {
  const double r1[] = {0, 0, 0, 1, 1, 1, 1, 0}; IS_RECT(r1);
  const double r2[] = {.4, 2, .6, 2, .6, -2, .4, -2}; IS_RECT(r2);
  const double r3[] = {.5, 1.5, 1.5, .5, .5, -.5, -.5, .5}; IS_RECT(r3);
  double x, y, w, h;
  GetRectangleExtents(r1, &x, &y, &w, &h);
  EXPECT_TRUE(DEQ(x,0) && DEQ(y,0) && DEQ(w,1) && DEQ(h,1));
  GetRectangleExtents(r2, &x, &y, &w, &h);
  EXPECT_TRUE(DEQ(x,.4) && DEQ(y,-2) && DEQ(w,.2) && DEQ(h,4));
  GetRectangleExtents(r3, &x, &y, &w, &h);
  EXPECT_TRUE(DEQ(x,-.5) && DEQ(y,-.5) && DEQ(w,2) && DEQ(h,2));
}

TEST(MathUtilsTest, GetTwoRectanglesExtents) {
  const double r1[] = {2, 2, 2, 3};
  const double r2[] = {0, 1, 2, 1};
  double r[4];
  GetTwoRectanglesExtents(r1, r2, r);
  EXPECT_TRUE(DEQ(r[0],0) && DEQ(r[1],1) && DEQ(r[2],4) && DEQ(r[3],4));
}

TEST(MathUtilsTest, RectanglesOverlapped) {
  const double r1[] = {2, 2, 2, 3};
  const double r2[] = {0, 1, 2, 1};
  const double r3[] = {1, 1, 4, 3};
  const double r4[] = {6, 6, 1, 1};
  EXPECT_TRUE(RectanglesOverlapped(r1, r1));
  EXPECT_TRUE(RectanglesOverlapped(r1, r3));
  EXPECT_TRUE(RectanglesOverlapped(r2, r3));
  EXPECT_FALSE(RectanglesOverlapped(r1, r2));
  EXPECT_FALSE(RectanglesOverlapped(r2, r1));
  EXPECT_FALSE(RectanglesOverlapped(r1, r4));
  EXPECT_FALSE(RectanglesOverlapped(r2, r4));
  EXPECT_FALSE(RectanglesOverlapped(r3, r4));
}

int main(int argc, char **argv) {
  testing::ParseGUnitFlags(&argc, argv);

  return RUN_ALL_TESTS();
}
