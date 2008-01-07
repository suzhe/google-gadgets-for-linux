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

#include <stdio.h>
#include "ggadget/linux/machine.h"
#include "ggadget/common.h"
#include "ggadget/logger.h"
#include "unittest/gunit.h"

using namespace ggadget;
using namespace ggadget::framework;
using namespace ggadget::framework::linux_os;

TEST(Machine, GetBiosSerialNumber) {
  Machine machine;
  std::string number = machine.GetBiosSerialNumber();
  EXPECT_TRUE(number.size());
  LOG("Serial#: %s", number.c_str());
}

TEST(Machine, GetMachineManufacturer) {
  Machine machine;
  std::string vendor = machine.GetMachineManufacturer();
  EXPECT_TRUE(vendor.size());
  LOG("vendor: %s", vendor.c_str());
}

TEST(Machine, GetMachineModel) {
  Machine machine;
  std::string model = machine.GetMachineModel();
  EXPECT_TRUE(model.size());
  LOG("model: %s", model.c_str());
}

int main(int argc, char **argv) {
  testing::ParseGUnitFlags(&argc, argv);

  return RUN_ALL_TESTS();
}
