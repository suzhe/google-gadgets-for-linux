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

#include "ggadget/file_manager_interface.h"
#include "ggadget/file_manager_wrapper.h"
#include "ggadget/file_manager_factory.h"
#include "ggadget/dir_file_manager.h"
#include "ggadget/zip_file_manager.h"
#include "ggadget/localized_file_manager.h"
#include "ggadget/system_utils.h"
#include "unittest/gunit.h"

using namespace ggadget;

const char *base_dir_path = "file_manager_test_data_dest";
const char *base_gg_path  = "file_manager_test_data_dest.gg";

const char *base_new_dir_path = "file_manager_test_data_new";
const char *base_new_gg_path  = "file_manager_test_data_new.gg";

void TestFileManagerReadFunctions(FileManagerInterface *fm, bool zip) {
  ASSERT_TRUE(fm->IsValid());
  std::string data;
  std::string path;
  std::string base_path;
  std::string base_filename;
  base_path = fm->GetFullPath(NULL);
  SplitFilePath(base_path.c_str(), &path, &base_filename);
  ASSERT_TRUE(base_path.length());
  ASSERT_TRUE(fm->ReadFile("global_file", &data));
  EXPECT_STREQ("global_file at top\n", data.c_str());

  ASSERT_TRUE(fm->ReadFile("zh_CN/./../global_file", &data));
  EXPECT_STREQ("global_file at top\n", data.c_str());

  EXPECT_FALSE(fm->ReadFile("non-exists", &data));

  ASSERT_TRUE(fm->ReadFile("zh_CN/zh_CN_file", &data));
  EXPECT_STREQ("zh_CN_file contents\n", data.c_str());

  ASSERT_TRUE(fm->ReadFile("zh_CN/2048_file", &data));
  EXPECT_EQ(2048U, data.size());

  ASSERT_TRUE(fm->ReadFile("zh_CN/big_file", &data));
  EXPECT_EQ(32616U, data.size());

  EXPECT_TRUE(fm->FileExists("global_file", &path));
  EXPECT_STREQ((base_path + "/global_file").c_str(), path.c_str());
  EXPECT_STREQ(fm->GetFullPath("global_file").c_str(), path.c_str());

  EXPECT_FALSE(fm->FileExists("non-exists", &path));
  EXPECT_STREQ((base_path + "/non-exists").c_str(), path.c_str());
  EXPECT_STREQ(fm->GetFullPath("non-exists").c_str(), path.c_str());

  EXPECT_FALSE(fm->FileExists(("../" + base_filename).c_str(), &path));
  EXPECT_STREQ(base_path.c_str(), path.c_str());

  if (zip) {
    ASSERT_TRUE(fm->ReadFile("gLoBaL_FiLe", &data));
    EXPECT_STREQ((base_path + "/gLoBaL_FiLe").c_str(),
                 fm->GetFullPath("gLoBaL_FiLe").c_str());
    EXPECT_STREQ("global_file at top\n", data.c_str());
    EXPECT_TRUE(fm->FileExists("1033/1033_FiLe", &path));
    EXPECT_STREQ((base_path + "/1033/1033_FiLe").c_str(), path.c_str());
    EXPECT_FALSE(fm->IsDirectlyAccessible("gLoBaL_FiLe", &path));
    EXPECT_STREQ((base_path + "/gLoBaL_FiLe").c_str(), path.c_str());
  } else {
    // Case sensitive may not be available on some platforms.
    //EXPECT_FALSE(fm->FileExists("1033/1033_FiLe", &path));
    //EXPECT_STREQ((base_path + "/1033/1033_FiLe").c_str(), path.c_str());
    EXPECT_TRUE(fm->IsDirectlyAccessible("gLoBaL_FiLe", &path));
    EXPECT_STREQ((base_path + "/gLoBaL_FiLe").c_str(), path.c_str());
  }
}

void TestFileManagerWriteFunctions(FileManagerInterface *fm, bool zip) {
  ASSERT_TRUE(fm->IsValid());
  std::string data;
  std::string path;
  std::string path2;
  std::string base_path;
  base_path = fm->GetFullPath(NULL);
  ASSERT_TRUE(base_path.length());

  // Test write file in top dir.
  data = "new_file contents\n";
  ASSERT_TRUE(fm->WriteFile("new_file", data, false));
  ASSERT_TRUE(fm->FileExists("new_file", &path));
  EXPECT_STREQ((base_path + "/new_file").c_str(), path.c_str());
  ASSERT_TRUE(fm->ReadFile("new_file", &data));
  EXPECT_STREQ("new_file contents\n", data.c_str());
  path.clear();
  ASSERT_TRUE(fm->ExtractFile("new_file", &path));
  ASSERT_TRUE(ReadFileContents(path.c_str(), &data));
  EXPECT_STREQ("new_file contents\n", data.c_str());
  ::unlink(path.c_str());
  EXPECT_FALSE(ReadFileContents(path.c_str(), &data));
  path2 = path;
  ASSERT_TRUE(fm->ExtractFile("new_file", &path));
  EXPECT_STREQ(path2.c_str(), path.c_str());
  ASSERT_TRUE(ReadFileContents(path.c_str(), &data));
  EXPECT_STREQ("new_file contents\n", data.c_str());
  ASSERT_TRUE(fm->FileExists("new_file", &path));

  // Test write file in sub dir.
  data = "en_new_file contents\n";
  ASSERT_TRUE(fm->WriteFile("en/new_file", data, false));
  ASSERT_TRUE(fm->FileExists("en/new_file", &path));
  EXPECT_STREQ((base_path + "/en/new_file").c_str(), path.c_str());
  ASSERT_TRUE(fm->ReadFile("en/new_file", &data));
  EXPECT_STREQ("en_new_file contents\n", data.c_str());
  path.clear();
  ASSERT_TRUE(fm->ExtractFile("en/new_file", &path));
  ASSERT_TRUE(ReadFileContents(path.c_str(), &data));
  EXPECT_STREQ("en_new_file contents\n", data.c_str());
  ::unlink(path.c_str());
  EXPECT_FALSE(ReadFileContents(path.c_str(), &data));
  path2 = path;
  ASSERT_TRUE(fm->ExtractFile("en/new_file", &path));
  EXPECT_STREQ(path2.c_str(), path.c_str());
  ASSERT_TRUE(ReadFileContents(path.c_str(), &data));
  EXPECT_STREQ("en_new_file contents\n", data.c_str());
  ASSERT_TRUE(fm->FileExists("en/new_file", &path));

  // Test overwrite an existing file.
  EXPECT_FALSE(fm->WriteFile("en/new_file", data, false));

  if (zip) {
    EXPECT_FALSE(fm->RemoveFile("new_file"));
    EXPECT_FALSE(fm->RemoveFile("en/new_file"));
  } else {
    EXPECT_TRUE(fm->WriteFile("en/new_file", data, true));
    EXPECT_TRUE(fm->RemoveFile("new_file"));
    EXPECT_TRUE(fm->RemoveFile("en/new_file"));
    EXPECT_FALSE(fm->FileExists("new_file", &path));
    EXPECT_FALSE(fm->FileExists("en/new_file", &path));
  }
}

void TestFileManagerLocalized(FileManagerInterface *fm,
                              const char *lang,
                              const char *territory) {
  std::string contents(" contents\n");
  std::string locale(lang);
  std::string data;
  std::string filename;

  filename = locale + "_file";
  ASSERT_TRUE(fm->ReadFile(filename.c_str(), &data));
  EXPECT_STREQ((filename + contents).c_str(), data.c_str());

  locale.append("_");
  locale.append(territory);
  filename = locale + "_file";
  ASSERT_TRUE(fm->ReadFile(filename.c_str(), &data));
  EXPECT_STREQ((filename + contents).c_str(), data.c_str());

  if (locale != "en_US") {
    locale = "en";
    filename = locale + "_file";
    ASSERT_TRUE(fm->ReadFile(filename.c_str(), &data));
    EXPECT_STREQ((filename + contents).c_str(), data.c_str());

    locale.append("_US");
    filename = locale + "_file";
    ASSERT_TRUE(fm->ReadFile(filename.c_str(), &data));
    EXPECT_STREQ((filename + contents).c_str(), data.c_str());
  }

  locale = "1033";
  filename = locale + "_file";
  ASSERT_TRUE(fm->ReadFile(filename.c_str(), &data));
  EXPECT_STREQ((filename + contents).c_str(), data.c_str());
}

TEST(FileManager, DirRead) {
  FileManagerInterface *fm = new DirFileManager();
  ASSERT_TRUE(fm->Init(base_dir_path, false));
  TestFileManagerReadFunctions(fm, false);
  delete fm;
}

TEST(FileManager, ZipRead) {
  FileManagerInterface *fm = new ZipFileManager();
  ASSERT_TRUE(fm->Init(base_gg_path, false));
  TestFileManagerReadFunctions(fm, true);
  delete fm;
}

TEST(FileManager, DirWrite) {
  FileManagerInterface *fm = new DirFileManager();
  ASSERT_TRUE(fm->Init(base_new_dir_path, true));
  TestFileManagerWriteFunctions(fm, false);
  delete fm;
  RemoveDirectory(base_new_dir_path);
}

TEST(FileManager, ZipWrite) {
  FileManagerInterface *fm = new ZipFileManager();
  ASSERT_TRUE(fm->Init(base_new_gg_path, true));
  TestFileManagerWriteFunctions(fm, true);
  delete fm;
  ::unlink(base_new_gg_path);
}

TEST(FileManager, LocalizedFile) {
  static const char *locales[] = { "en_US", "zh_CN", NULL };
  std::string lang, territory;

  for (size_t i = 0; locales[i]; ++i) {
    setlocale(LC_MESSAGES, locales[i]);
    ASSERT_TRUE(GetSystemLocaleInfo(&lang, &territory));

    FileManagerInterface *fm = new LocalizedFileManager(new DirFileManager());
    ASSERT_TRUE(fm->Init(base_dir_path, false));
    TestFileManagerLocalized(fm, lang.c_str(), territory.c_str());
    delete fm;

    fm = new LocalizedFileManager(new ZipFileManager());
    ASSERT_TRUE(fm->Init(base_gg_path, false));
    TestFileManagerLocalized(fm, lang.c_str(), territory.c_str());
    delete fm;
  }
}

int main(int argc, char **argv) {
  testing::ParseGUnitFlags(&argc, argv);
  return RUN_ALL_TESTS();
}
