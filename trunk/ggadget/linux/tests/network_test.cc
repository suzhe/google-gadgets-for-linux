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
#include <ggadget/common.h>
#include <ggadget/framework_interface.h>
#include <ggadget/linux/network.h>
#include <ggadget/logger.h>
#include <unittest/gunit.h>

using namespace ggadget;
using namespace ggadget::framework;
using namespace ggadget::framework::linux_os;

TEST(Network, GetConnectionType) {
  Network network;
  if (network.IsOnline())
    EXPECT_NE(NetworkInterface::CONNECTION_TYPE_UNKNOWN,
              network.GetConnectionType());
  else
    EXPECT_EQ(NetworkInterface::CONNECTION_TYPE_UNKNOWN,
              network.GetConnectionType());
}

TEST(Network, GetPhysicalMediaType) {
  Network network;
  if (network.IsOnline())
    EXPECT_NE(NetworkInterface::PHISICAL_MEDIA_TYPE_UNSPECIFIED,
              network.GetPhysicalMediaType());
  else
    EXPECT_EQ(NetworkInterface::PHISICAL_MEDIA_TYPE_UNSPECIFIED,
              network.GetPhysicalMediaType());
}

int main(int argc, char **argv) {
  testing::ParseGUnitFlags(&argc, argv);
  return RUN_ALL_TESTS();
}
