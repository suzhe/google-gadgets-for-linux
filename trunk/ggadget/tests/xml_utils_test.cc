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

#include "ggadget/xml_parser_interface.h"
#include "ggadget/xml_utils.h"
#include "unittest/gunit.h"
#include "init_extensions.h"

using namespace ggadget;

XMLParserInterface *xml_parser = NULL;

TEST(xml_utils, ReplaceXMLEntities) {
  const char *kStrings =
    "<strings>\n"
    "  <root-attr2>根元素属性2</root-attr2>\n"
    "  <second-value>第二层元素的文本内容</second-value>\n"
    "  <blank-value></blank-value>\n"
    "</strings>\n";
  const char *kMainXMLOriginalContents =
    "<root root-attr1=\"root-value1\" root-attr2=\"&root-attr2;\""
    " root-attr3=\"&lt;&amp;&gt;xyz \">\n"
    "<second>&second-value;</second>\n"
    "<third>&blank-value;&non-existence;</third>\n"
    "</root>\n";
  const char *kMainXMLTranslatedContents =
    "<root root-attr1=\"root-value1\" root-attr2=\"根元素属性2\""
    " root-attr3=\"&lt;&amp;&gt;xyz \">\n"
    "<second>第二层元素的文本内容</second>\n"
    "<third>&non-existence;</third>\n"
    "</root>\n";

  GadgetStringMap strings;
  ASSERT_TRUE(xml_parser->ParseXMLIntoXPathMap(
      std::string(kStrings), "kStrings", "strings", NULL, &strings));

  std::string xml(kMainXMLOriginalContents);
  ASSERT_TRUE(ReplaceXMLEntities(strings, &xml));
  EXPECT_STREQ(kMainXMLTranslatedContents, xml.c_str());
}

int main(int argc, char **argv) {
  testing::ParseGUnitFlags(&argc, argv);

  static const char *kExtensions[] = {
    "libxml2_xml_parser/libxml2-xml-parser",
  };
  INIT_EXTENSIONS(argc, argv, kExtensions);

  xml_parser = GetXMLParser();
  return RUN_ALL_TESTS();
}
