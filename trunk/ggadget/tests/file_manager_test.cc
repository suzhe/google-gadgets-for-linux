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

#include "ggadget/file_manager.h"
#include "unittest/gunit.h"

using namespace ggadget;
using namespace ggadget::internal;

TEST(file_manager, InitLocaleStrings) {
  FileManagerImpl impl;
  ASSERT_STREQ("C", setlocale(LC_MESSAGES, "C"));
  impl.InitLocaleStrings();
  EXPECT_STREQ("C/", impl.locale_lang_prefix_.c_str());
  EXPECT_STREQ("C/", impl.locale_prefix_.c_str());
  EXPECT_STREQ("", impl.locale_id_prefix_.c_str());

  ASSERT_STREQ("en_US", setlocale(LC_MESSAGES, "en_US"));
  impl.InitLocaleStrings();
  EXPECT_STREQ("en/", impl.locale_lang_prefix_.c_str());
  EXPECT_STREQ("en_US/", impl.locale_prefix_.c_str());
  EXPECT_STREQ("1033/", impl.locale_id_prefix_.c_str());

  ASSERT_STREQ("zh_CN.UTF8", setlocale(LC_MESSAGES, "zh_CN.UTF8"));
  impl.InitLocaleStrings();
  EXPECT_STREQ("zh/", impl.locale_lang_prefix_.c_str());
  EXPECT_STREQ("zh_CN/", impl.locale_prefix_.c_str());
  EXPECT_STREQ("2052/", impl.locale_id_prefix_.c_str());
}

TEST(file_manager, SplitPathFilename) {
  std::string path;
  std::string filename;
  FileManagerImpl::SplitPathFilename("/", &path, &filename);
  EXPECT_STREQ("/", path.c_str());
  EXPECT_STREQ("", filename.c_str());

  FileManagerImpl::SplitPathFilename("/etc", &path, &filename);
  EXPECT_STREQ("/", path.c_str());
  EXPECT_STREQ("etc", filename.c_str());

  FileManagerImpl::SplitPathFilename("/etc/", &path, &filename);
  EXPECT_STREQ("/etc", path.c_str());
  EXPECT_STREQ("", filename.c_str());

  FileManagerImpl::SplitPathFilename("/etc/xyz.conf", &path, &filename);
  EXPECT_STREQ("/etc", path.c_str());
  EXPECT_STREQ("xyz.conf", filename.c_str());
}

TEST(file_manager, FindLocalizedFile) {
  FileManagerImpl impl;
  impl.files_["en/en_file"];
  impl.files_["en_US/strings.xml"];
  impl.files_["en_US/en_US_file"];
  impl.files_["1033/1033_file"];
  impl.files_["1033/strings.xml"];
  impl.files_["1033/en_file"];
  impl.files_["2052/2052_file"];
  impl.files_["zh/zh_file"];
  impl.files_["zh/strings.xml"];
  impl.files_["zh_CN/zh_CN_file"];
  impl.files_["zh_CN/strings.xml"];
  impl.files_["2052/zh_file"];
  impl.files_["2052/2052_file"];
  impl.files_["2052/strings.xml"];
  impl.files_["strings.xml"];
  impl.files_["global"];

  ASSERT_STREQ("C", setlocale(LC_MESSAGES, "C"));
  impl.InitLocaleStrings();
  EXPECT_STREQ("en_US/strings.xml",
               impl.FindLocalizedFile("strings.xml")->first.c_str());
  EXPECT_STREQ("en/en_file",
               impl.FindLocalizedFile("en_file")->first.c_str());
  EXPECT_STREQ("1033/1033_file",
               impl.FindLocalizedFile("1033_file")->first.c_str());
  EXPECT_TRUE(impl.files_.end() == impl.FindLocalizedFile("zh_CN_file"));

  ASSERT_STREQ("en_US", setlocale(LC_MESSAGES, "en_US"));
  impl.InitLocaleStrings();
  EXPECT_STREQ("en_US/strings.xml",
               impl.FindLocalizedFile("strings.xml")->first.c_str());
  EXPECT_STREQ("en/en_file",
               impl.FindLocalizedFile("en_file")->first.c_str());
  EXPECT_STREQ("1033/1033_file",
               impl.FindLocalizedFile("1033_file")->first.c_str());
  EXPECT_TRUE(impl.files_.end() == impl.FindLocalizedFile("zh_CN_file"));

  ASSERT_STREQ("zh_CN.UTF8", setlocale(LC_MESSAGES, "zh_CN.UTF8"));
  impl.InitLocaleStrings();
  EXPECT_STREQ("zh_CN/strings.xml",
               impl.FindLocalizedFile("strings.xml")->first.c_str());
  EXPECT_STREQ("zh/zh_file",
               impl.FindLocalizedFile("zh_file")->first.c_str());
  EXPECT_STREQ("2052/2052_file",
               impl.FindLocalizedFile("2052_file")->first.c_str());
  EXPECT_STREQ("en/en_file",
               impl.FindLocalizedFile("en_file")->first.c_str());
  EXPECT_STREQ("1033/1033_file",
               impl.FindLocalizedFile("1033_file")->first.c_str());
  EXPECT_STREQ("en_US/en_US_file",
               impl.FindLocalizedFile("en_US_file")->first.c_str());
  EXPECT_TRUE(impl.files_.end() == impl.FindLocalizedFile("global"));
}

void TestFileManagerFunctions(const std::string &base_path,
                              const std::string &actual_path) {
  FileManagerImpl impl;
  ASSERT_STREQ("zh_CN.UTF8", setlocale(LC_MESSAGES, "zh_CN.UTF8"));
  impl.Init(base_path.c_str());

  for (FileManagerImpl::FileMap::const_iterator it = impl.files_.begin();
       it != impl.files_.end(); ++it)
    printf("%s\n", it->first.c_str());

  ASSERT_EQ(8U, impl.files_.size());
  std::string data;
  std::string path;
  ASSERT_TRUE(impl.GetFileContents("global_file", &data, &path));
  EXPECT_STREQ("global_file at top\n", data.c_str());
  EXPECT_STREQ((actual_path + "/global_file").c_str(), path.c_str());
  EXPECT_FALSE(impl.GetFileContents("non-exists", &data, &path));
  ASSERT_TRUE(impl.GetFileContents("zh_CN_file", &data, &path));
  EXPECT_STREQ((actual_path + "/zh_CN/zh_CN_file").c_str(), path.c_str());
  EXPECT_STREQ("zh_CN_file contents\n", data.c_str());
  ASSERT_TRUE(impl.GetFileContents("2048_file", &data, &path));
  EXPECT_EQ(2048U, data.size());
  ASSERT_TRUE(impl.GetFileContents("big_file", &data, &path));
  EXPECT_EQ(32616U, data.size());

  ASSERT_TRUE(impl.GetFileContents("gLoBaL_FiLe", &data, &path));
  EXPECT_STREQ((actual_path + "/global_file").c_str(), path.c_str());
  EXPECT_STREQ("global_file at top\n", data.c_str());
  ASSERT_TRUE(impl.GetFileContents("ZH_cn_File", &data, &path));
  EXPECT_STREQ((actual_path + "/zh_CN/zh_CN_file").c_str(), path.c_str());
  EXPECT_STREQ("zh_CN_file contents\n", data.c_str());
}

TEST(file_manager, FileManagerDir) {
  TestFileManagerFunctions("file_manager_test_data", "file_manager_test_data");
}

TEST(file_manager, FileManagerZip) {
  TestFileManagerFunctions("file_manager_test_data.gg",
                           "file_manager_test_data.gg");
}

TEST(file_manager, FileManagerDirManifest) {
  TestFileManagerFunctions("file_manager_test_data/gadget.gmanifest",
                           "file_manager_test_data");
}

TEST(file_manager, StringTable) {
  FileManagerImpl impl;
  ASSERT_STREQ("zh_CN.UTF8", setlocale(LC_MESSAGES, "zh_CN.UTF8"));
  impl.Init("file_manager_test_data");

  EXPECT_EQ(3U, impl.string_table_.size());
  EXPECT_STREQ("", impl.string_table_["blank-value"].c_str());
  EXPECT_STREQ("根元素属性2", impl.string_table_["root-attr2"].c_str());
  EXPECT_STREQ("第二层元素的文本内容", impl.string_table_["second-value"].c_str());
}

TEST(file_manager, GetTranslatedFileContents) {
  FileManagerImpl impl;
  ASSERT_STREQ("zh_CN.UTF8", setlocale(LC_MESSAGES, "zh_CN.UTF8"));
  impl.Init("file_manager_test_data");

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

  std::string data;
  std::string path;
  ASSERT_TRUE(impl.GetFileContents("main.xml", &data, &path));
  EXPECT_STREQ("file_manager_test_data/main.xml", path.c_str());
  EXPECT_STREQ(kMainXMLOriginalContents, data.c_str());
  ASSERT_TRUE(impl.GetXMLFileContents("main.xml", &data, &path));
  EXPECT_STREQ(kMainXMLTranslatedContents, data.c_str());
}

int main(int argc, char **argv) {
  testing::ParseGUnitFlags(&argc, argv);
  return RUN_ALL_TESTS();
}
