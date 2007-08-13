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

#include "ggadget/utility.h"
#include "unittest/gunit.h"

using namespace ggadget;

const double kErrorDelta = .00001;

TEST(UtilityTest, ChildCoordCalculator) {
  double child_x, child_y;
  
  ChildCoordCalculator calc = ChildCoordCalculator(0, 0, 50, 50, M_PI/2);
  calc.Convert(0, 0, &child_x, &child_y);
  EXPECT_EQ(child_x, calc.GetChildX(0, 0));
  EXPECT_EQ(child_y, calc.GetChildY(0, 0));
  EXPECT_LT(-kErrorDelta, child_x); 
  EXPECT_GT(kErrorDelta, child_x);
  EXPECT_EQ(100., child_y);

  calc = ChildCoordCalculator(0, 0, 50, 50, M_PI);
  calc.Convert(0, 0, &child_x, &child_y);
  EXPECT_EQ(child_x, calc.GetChildX(0, 0));
  EXPECT_EQ(child_y, calc.GetChildY(0, 0));
  EXPECT_EQ(100., child_x);
  EXPECT_EQ(100., child_y);

  calc = ChildCoordCalculator(0, 0, 50, 50, 1.5 * M_PI);
  calc.Convert(0, 0, &child_x, &child_y);
  EXPECT_EQ(child_x, calc.GetChildX(0, 0));
  EXPECT_EQ(child_y, calc.GetChildY(0, 0));
  EXPECT_LT(100 - kErrorDelta, child_x);
  EXPECT_GT(100 + kErrorDelta, child_x);
  EXPECT_LT(-kErrorDelta, child_y); 
  EXPECT_GT(kErrorDelta, child_y);

  calc = ChildCoordCalculator(0, 0, 50, 50, 2 * M_PI);
  calc.Convert(0, 0, &child_x, &child_y);
  EXPECT_EQ(child_x, calc.GetChildX(0, 0));
  EXPECT_EQ(child_y, calc.GetChildY(0, 0));
  EXPECT_LT(-kErrorDelta, child_x); 
  EXPECT_GT(kErrorDelta, child_x);
  EXPECT_LT(-kErrorDelta, child_y); 
  EXPECT_GT(kErrorDelta, child_y);

  ChildCoordCalculator calc2(0, 0, 0, 0, 0);
  for (int i = 0; i < 360; i++) {
    calc2.Convert(i, i, &child_x, &child_y);
    EXPECT_EQ(child_x, calc2.GetChildX(i, i));
    EXPECT_EQ(child_y, calc2.GetChildY(i, i));
    EXPECT_EQ(i, child_x);
    EXPECT_EQ(i, child_y);

    calc = ChildCoordCalculator(i, i, 0, 0, 0);
    calc.Convert(0, 0, &child_x, &child_y);
    EXPECT_EQ(child_x, calc.GetChildX(0, 0));
    EXPECT_EQ(child_y, calc.GetChildY(0, 0));
    EXPECT_EQ(-i, child_x);
    EXPECT_EQ(-i, child_y);
    
    calc = ChildCoordCalculator(0, 0, i, i, 0);
    calc.Convert(0, 0, &child_x, &child_y);
    EXPECT_EQ(child_x, calc.GetChildX(0, 0));
    EXPECT_EQ(child_y, calc.GetChildY(0, 0));
    EXPECT_EQ(0, child_x);
    EXPECT_EQ(0, child_y);

    // distance should be constant in a circular rotation around origin
    calc = ChildCoordCalculator(0, 0, 0, 0, DegreesToRadians(i));
    calc.Convert(100, 100, &child_x, &child_y);
    EXPECT_EQ(child_x, calc.GetChildX(100, 100));
    EXPECT_EQ(child_y, calc.GetChildY(100, 100));
    EXPECT_GT(20000 + kErrorDelta, child_x * child_x + child_y * child_y);
    EXPECT_LT(20000 - kErrorDelta, child_x * child_x + child_y * child_y);

    // distance should be constant in a circular rotation around top-left
    calc = ChildCoordCalculator(100, 100, 0, 0, DegreesToRadians(i));
    calc.Convert(0, 0, &child_x, &child_y);
    EXPECT_EQ(child_x, calc.GetChildX(0, 0));
    EXPECT_EQ(child_y, calc.GetChildY(0, 0));
    EXPECT_GT(20000 + kErrorDelta, child_x * child_x + child_y * child_y);
    EXPECT_LT(20000 - kErrorDelta, child_x * child_x + child_y * child_y);

    // distance to pin should be constant in a circular rotation
    calc = ChildCoordCalculator(0, 0, 1, 1, DegreesToRadians(i));
    calc.Convert(0, 0, &child_x, &child_y);
    EXPECT_EQ(child_x, calc.GetChildX(0, 0));
    EXPECT_EQ(child_y, calc.GetChildY(0, 0));
    EXPECT_GT(2 + kErrorDelta, 
          (child_x - 1) * (child_x - 1) + (child_y - 1) * (child_y - 1));
    EXPECT_LT(2 - kErrorDelta, 
          (child_x - 1) * (child_x - 1) + (child_y - 1) * (child_y - 1));
  }
}

TEST(UtilityTest, GetChildCoord) {
  double child_x, child_y;

  ChildCoordFromParentCoord(0, 0, 0, 0, 50, 50, M_PI / 2, &child_x, &child_y);
  EXPECT_LT(-kErrorDelta, child_x); 
  EXPECT_GT(kErrorDelta, child_x);
  EXPECT_EQ(100., child_y);

  ChildCoordFromParentCoord(0, 0, 0, 0, 50, 50, M_PI, &child_x, &child_y);
  EXPECT_EQ(100., child_x);
  EXPECT_EQ(100., child_y);

  ChildCoordFromParentCoord(0, 0, 0, 0, 50, 50, 1.5 * M_PI, &child_x, &child_y);
  EXPECT_LT(100 - kErrorDelta, child_x);
  EXPECT_GT(100 + kErrorDelta, child_x);
  EXPECT_LT(-kErrorDelta, child_y); 
  EXPECT_GT(kErrorDelta, child_y);

  ChildCoordFromParentCoord(0, 0, 0, 0, 50, 50, 2 * M_PI, &child_x, &child_y);
  EXPECT_LT(-kErrorDelta, child_x); 
  EXPECT_GT(kErrorDelta, child_x);
  EXPECT_LT(-kErrorDelta, child_y); 
  EXPECT_GT(kErrorDelta, child_y);
    
  for (int i = 0; i < 360; i++) {
    ChildCoordFromParentCoord(i, i, 0, 0, 0, 0, 0, &child_x, &child_y);
    EXPECT_EQ(i, child_x);
    EXPECT_EQ(i, child_y);

    ChildCoordFromParentCoord(0, 0, i, i, 0, 0, 0, &child_x, &child_y);
    EXPECT_EQ(-i, child_x);
    EXPECT_EQ(-i, child_y);
    
    ChildCoordFromParentCoord(0, 0, 0, 0, i, i, 0, &child_x, &child_y);
    EXPECT_EQ(0, child_x);
    EXPECT_EQ(0, child_y);

    // distance should be constant in a circular rotation around origin
    ChildCoordFromParentCoord(100, 100, 0, 0, 0, 0, DegreesToRadians(i), 
			      &child_x, &child_y);
    EXPECT_GT(20000 + kErrorDelta, child_x * child_x + child_y * child_y);
    EXPECT_LT(20000 - kErrorDelta, child_x * child_x + child_y * child_y);

    // distance should be constant in a circular rotation around top-left
    ChildCoordFromParentCoord(0, 0, 100, 100, 0, 0, DegreesToRadians(i), 
			      &child_x, &child_y);
    EXPECT_GT(20000 + kErrorDelta, child_x * child_x + child_y * child_y);
    EXPECT_LT(20000 - kErrorDelta, child_x * child_x + child_y * child_y);

    // distance to pin should be constant in a circular rotation
    ChildCoordFromParentCoord(0, 0, 0, 0, 1, 1, DegreesToRadians(i), 
			      &child_x, &child_y);
    EXPECT_GT(2 + kErrorDelta, 
	      (child_x - 1) * (child_x - 1) + (child_y - 1) * (child_y - 1));
    EXPECT_LT(2 - kErrorDelta, 
	      (child_x - 1) * (child_x - 1) + (child_y - 1) * (child_y - 1));
  }
}

TEST(UtilityTest, CheckPointInElement) {
  EXPECT_TRUE(IsPointInElement(0, 0, 50, 20));
  EXPECT_TRUE(IsPointInElement(1, 1, 50, 20));
  EXPECT_TRUE(IsPointInElement(49.9, 19.9, 50, 20));
  EXPECT_FALSE(IsPointInElement(-5, 0, 50, 20));
  EXPECT_FALSE(IsPointInElement(0, -5, 50, 20));
  EXPECT_FALSE(IsPointInElement(0, 30, 50, 20));
  EXPECT_FALSE(IsPointInElement(60, 0, 50, 20));
}

TEST(UtilityTest, DegreesToRadians) {
  EXPECT_EQ(2 * M_PI, DegreesToRadians(360.));
  EXPECT_EQ(0., DegreesToRadians(0.));
  EXPECT_EQ(M_PI, DegreesToRadians(180.));
}

int main(int argc, char **argv) {
  testing::ParseGUnitFlags(&argc, argv);
  
  return RUN_ALL_TESTS();
}
