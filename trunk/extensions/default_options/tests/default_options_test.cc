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

#include "ggadget/logger.h"
#include "ggadget/options_factory.h"
#include "ggadget/system_utils.h"
#include "ggadget/tests/init_extensions.h"
#include "unittest/gunit.h"

using namespace ggadget;

// TODO: system dependent.
#define TEST_DIRECTORY "/tmp/TestDefaultOptions"

TEST(DefaultOptions, Test) {
  OptionsInterface *options =
      OptionsFactory::get()->CreateOptions(TEST_DIRECTORY "/options1");
  ASSERT_TRUE(options);
  const char kBinaryData[] = "\x01\0\x02xyz\n\r\"\'\\\xff\x7f<>&";
  const std::string kBinaryStr(kBinaryData, sizeof(kBinaryData) - 1);

  typedef std::map<std::string, Variant> TestData;
  TestData test_data;
  test_data["itemint"] = Variant(1);
  test_data["itembooltrue"] = Variant(true);
  test_data["itemboolfalse"] = Variant(false);
  test_data["itemdouble"] = Variant(1.234);
  test_data["itemstring"] = Variant("string");
  test_data["itemstringnull"] = Variant(static_cast<const char *>(NULL));
  test_data["itembinary"] = Variant(kBinaryStr);
  test_data["itemjson"] = Variant(JSONString("233456"));
  test_data["itemdate"] = Variant(Date(123456789));

  for (TestData::const_iterator it = test_data.begin();
       it != test_data.end(); ++it) {
    EXPECT_EQ(Variant(), options->GetValue(it->first.c_str()));
    options->PutValue(it->first.c_str(), it->second);
    options->PutValue((it->first + "_encrypted").c_str(), it->second);
    options->EncryptValue((it->first + "_encrypted").c_str()); 
  }

  for (TestData::const_iterator it = test_data.begin();
       it != test_data.end(); ++it) {
    EXPECT_EQ(Variant(), options->GetDefaultValue(it->first.c_str()));
    EXPECT_EQ(it->second, options->GetValue(it->first.c_str()));
    EXPECT_FALSE(options->IsEncrypted(it->first.c_str()));
    EXPECT_EQ(it->second, options->GetValue((it->first + "_encrypted").c_str()));
    EXPECT_TRUE(options->IsEncrypted((it->first + "_encrypted").c_str())); 
  }

  options->PutDefaultValue("test_default", Variant("default"));
  options->PutInternalValue("test_internal", Variant("internal"));
  EXPECT_EQ(Variant("default"), options->GetDefaultValue("test_default"));
  EXPECT_EQ(Variant("default"), options->GetValue("test_default"));
  EXPECT_EQ(Variant("internal"), options->GetInternalValue("test_internal"));
  // Default and internal items don't affect count.
  EXPECT_EQ(test_data.size() * 2, options->GetCount());

  options->Flush();
  delete options;

  // NULL string become blank string when persisted and loaded.
  test_data["itemstringnull"] = Variant("");

  options = OptionsFactory::get()->CreateOptions(TEST_DIRECTORY "/options1");
  ASSERT_TRUE(options);
  for (TestData::const_iterator it = test_data.begin();
       it != test_data.end(); ++it) {
    EXPECT_EQ(Variant(), options->GetDefaultValue(it->first.c_str()));
    EXPECT_EQ(it->second, options->GetValue(it->first.c_str()));
    EXPECT_FALSE(options->IsEncrypted(it->first.c_str()));
    EXPECT_EQ(it->second, options->GetValue((it->first + "_encrypted").c_str()));
    EXPECT_TRUE(options->IsEncrypted((it->first + "_encrypted").c_str())); 
  }
  EXPECT_EQ(Variant("internal"), options->GetInternalValue("test_internal"));
  // Default values won't get persisted.
  EXPECT_EQ(Variant(), options->GetDefaultValue("test_default"));
  EXPECT_EQ(Variant(), options->GetValue("test_default"));

  // Test addditional default value logic.
  options->PutDefaultValue("itemdouble", Variant(456.7));
  options->Remove("itemdouble");
  EXPECT_EQ(Variant(456.7), options->GetValue("itemdouble"));
  options->PutValue("itemdouble", Variant(789));
  EXPECT_EQ(Variant(789), options->GetValue("itemdouble"));

  // If new value is set, encrypted state will be cleared.
  options->PutValue("itemdouble_encrypted", Variant(432.1));
  EXPECT_FALSE(options->IsEncrypted("itemdouble_encrypted"));
  delete options;
}

int main(int argc, char **argv) {
  testing::ParseGUnitFlags(&argc, argv);

  static const char *kExtensions[] = {
    "libxml2_xml_parser/libxml2-xml-parser",
    "default_options/default-options",
  };
  INIT_EXTENSIONS(argc, argv, kExtensions);

  system("rm -rf " TEST_DIRECTORY);
  EnsureDirectories(TEST_DIRECTORY);
  int result = RUN_ALL_TESTS();
  // TODO: system dependent.
  // system("cat " TEST_DIRECTORY "/options1");
  system("rm -rf " TEST_DIRECTORY);
  return result;
}
