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

#include "ggadget/light_map.h"
#include "ggadget/string_utils.h"
#include "unittest/gtest.h"

using namespace ggadget;

void use(void *v1, void *v2, void *v3) {
}

TEST(LightMap, Value) {
  typedef LightMap<std::string, int> Map1;
  typedef LightMap<std::string, void *> Map2;
  typedef LightMap<std::string, const Map2 *> Map3;
  // The int value map and const Map2 * map should all use LightMap<...,void *>
  // as super type.
  void *v0 = NULL;
  // The following assignment statements should be compiled without error,
  // in order to ensure they are all "void *".
  Map1::super_mapped_type v1 = v0;
  Map2::mapped_type v2 = v0;
  Map3::super_mapped_type v3 = v0;
  use(v1, v2, v3); // To avoid "not used variable" errors.
}

TEST(LightMap, Both) {
  typedef LightMap<int, const int *> Map1;
  typedef LightMap<Map1 *, size_t> Map2;
  typedef LightMap<void *, Map2 *> Map3;
  typedef LightMap<const Map2 *, const Map3 *> Map4;
  void *v0 = NULL;
  Map1::super_key_type v1a = v0;
  Map1::super_mapped_type v1b = v0;
  Map2::super_key_type v2a = v0;
  Map2::super_mapped_type v2b = v0;
  Map3::key_type v3a = v0;
  Map3::super_mapped_type v3b = v0;
  Map4::super_key_type v4a = v0;
  Map4::super_mapped_type v4b = v0;
  use(v1a, v1b, v2a); // To avoid "not used variable" errors.
  use(v2b, v3a, v3b);
  use(v4a, v4b, NULL);
}

TEST(LightMap, Operations) {
  typedef LightMap<const char *, int, CharPtrComparator> Map;
  Map map;
  EXPECT_TRUE(map.empty());

  map.insert(std::make_pair("a", 1));
  EXPECT_FALSE(map.empty());
  map["b"] = 2;
  EXPECT_EQ(2, map["b"]);
  map["b"] = 3;
  int *c_ptr = &map["c"];
  *c_ptr = 4;
  EXPECT_EQ(1, map["a"]);
  EXPECT_EQ(3, map["b"]);
  EXPECT_EQ(4, map["c"]);
  EXPECT_EQ(3U, map.size());

  Map::iterator it = map.begin();
  EXPECT_STREQ("a", it->first);
  EXPECT_EQ(1, it->second);
  Map::const_reverse_iterator const_rit = map.rbegin();
  EXPECT_STREQ("c", const_rit->first);
  EXPECT_EQ(4, const_rit->second);

  const char *keys[] = { "a", "b", "c" };
  const int values[] = { 1, 3, 4 };
  int i = 0;
  for (Map::iterator it = map.begin(); it != map.end(); ++it, ++i) {
    EXPECT_STREQ(keys[i], it->first);
    EXPECT_EQ(values[i], it->second);
  }
  i = 0;
  for (Map::const_iterator it = map.begin(); it != map.end(); ++it, ++i) {
    EXPECT_STREQ(keys[i], it->first);
    EXPECT_EQ(values[i], it->second);
  }
  i = 2;
  for (Map::reverse_iterator it = map.rbegin(); it != map.rend(); ++it, --i) {
    EXPECT_STREQ(keys[i], it->first);
    EXPECT_EQ(values[i], it->second);
  }
  i = 2;
  for (Map::const_reverse_iterator it = map.rbegin(); it != map.rend();
       ++it, --i) {
    EXPECT_STREQ(keys[i], it->first);
    EXPECT_EQ(values[i], it->second);
  }

  // Assignment to 'second' is not supported for now.
  // it = map.find("b");
  // it->second = 5;
  ///EXPECT_EQ(5, map["b"]);

  it = map.begin();
  EXPECT_TRUE(it++ == map.begin());
  map.erase(it);
  EXPECT_EQ(0U, map.count("b"));
  EXPECT_EQ(2U, map.size());
  EXPECT_EQ(1U, map.count("a"));
  EXPECT_EQ(1U, map.count("c"));

  map.erase("c");
  EXPECT_EQ(0U, map.count("c"));
  EXPECT_EQ(1U, map.count("a"));
  EXPECT_EQ(1U, map.size());

  map.clear();
  EXPECT_EQ(0U, map.size());
  EXPECT_TRUE(map.empty());
}

int main(int argc, char **argv) {
  testing::ParseGTestFlags(&argc, argv);
  return RUN_ALL_TESTS();
}
