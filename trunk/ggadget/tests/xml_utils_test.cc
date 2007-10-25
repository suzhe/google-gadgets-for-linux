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

#include <iostream>
#include <locale.h>

#include "ggadget/xml_utils.h"
#include "unittest/gunit.h"

using namespace ggadget;

TEST(XMLUtils, ParseXMLIntoXPathMap) {
  const char *xml =
    "<root a=\"v\" a1=\"v1\">\n"
    " <s aa=\"vv\" aa1=\"vv1\">s content</s>\n"
    " <s b=\"bv\" b1=\"bv1\"/>\n"
    " <s1 c=\"cv\" c1=\"cv1\">s1 content</s1>\n"
    " <s aa=\"vv\" aa1=\"vv1\">s content1</s>\n"
    " <s1 c=\"cv\" c1=\"cv1\">\n"
    "   s1 content1\n"
    "   <s11>s11 content</s11>"
    " </s1>\n"
    " <s2/>\n"
    "</root>";

  GadgetStringMap map;
  ASSERT_TRUE(ParseXMLIntoXPathMap(xml, "TheFileName", "root", &map));
  ASSERT_EQ(19U, map.size());
  EXPECT_STREQ("v", map.find("@a")->second.c_str());
  EXPECT_STREQ("v1", map.find("@a1")->second.c_str());
  EXPECT_STREQ("s content", map.find("s")->second.c_str());
  EXPECT_STREQ("vv", map.find("s@aa")->second.c_str());
  EXPECT_STREQ("s1 content", map.find("s1")->second.c_str());
  EXPECT_STREQ("", map.find("s[2]")->second.c_str());
  EXPECT_STREQ("s content1", map.find("s[3]")->second.c_str());
  EXPECT_STREQ("vv", map.find("s[3]@aa")->second.c_str());
  EXPECT_STREQ("", map.find("s2")->second.c_str());

  ASSERT_FALSE(ParseXMLIntoXPathMap(xml, "TheFileName", "another", &map));
  ASSERT_FALSE(ParseXMLIntoXPathMap("<a></b>", "Bad", "a", &map));
}

TEST(XMLUtils, CheckXMLName) {
  ASSERT_TRUE(CheckXMLName("abcde:def_.123-456"));
  ASSERT_TRUE(CheckXMLName("\344\270\200-\344\270\201"));
  ASSERT_FALSE(CheckXMLName("&#@Q!#"));
}

int main(int argc, char **argv) {
  testing::ParseGUnitFlags(&argc, argv);
  return RUN_ALL_TESTS();
}
