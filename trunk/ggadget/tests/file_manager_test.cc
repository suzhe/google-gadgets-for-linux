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

#include <locale.h>
#include <unistd.h>
#include <set>
#include <iostream>
#include <cstdlib>

#include "ggadget/logger.h"
#include "ggadget/file_manager_interface.h"
#include "ggadget/file_manager_wrapper.h"
#include "ggadget/file_manager_factory.h"
#include "ggadget/dir_file_manager.h"
#include "ggadget/zip_file_manager.h"
#include "ggadget/locales.h"
#include "ggadget/localized_file_manager.h"
#include "ggadget/slot.h"
#include "ggadget/system_utils.h"
#include "unittest/gtest.h"

using namespace ggadget;

const char *base_dir_path = "file_manager_test_data_dest";
const char *base_gg_path  = "file_manager_test_data_dest.gg";

const char *base_new_dir_path = "file_manager_test_data_new";
const char *base_new_gg_path  = "file_manager_test_data_new.gg";

const char *filenames[] = {
    "main.xml",
    "strings.xml",
    "global_file",
    "1033/1033_file",
    "2052/2052_file",
    "en/en_file",
    "en/global_file",
    "zh_CN/2048_file",
    "zh_CN/big_file",
    "zh_CN/global_file",
    "zh_CN/zh-CN_file",
    "zh_CN/strings.xml",
};

void TestFileManagerReadFunctions(const std::string &prefix,
                                  const std::string &base_path,
                                  FileManagerInterface *fm, bool zip) {
  ASSERT_TRUE(fm->IsValid());
  std::string data;
  std::string full_base_path = BuildFilePath(GetCurrentDirectory().c_str(),
                                             base_path.c_str(), NULL);
  std::string path;
  std::string base_filename;
  EXPECT_EQ(full_base_path, fm->GetFullPath(prefix.c_str()));
  SplitFilePath(full_base_path.c_str(), &path, &base_filename);
  ASSERT_TRUE(base_path.length());
  ASSERT_TRUE(fm->ReadFile((prefix + "global_file").c_str(), &data));
  EXPECT_STREQ("global_file at top\n", data.c_str());

  ASSERT_TRUE(fm->ReadFile((prefix + "zh_CN/./../global_file").c_str(), &data));
  EXPECT_STREQ("global_file at top\n", data.c_str());

  EXPECT_FALSE(fm->ReadFile((prefix + "non-exists").c_str(), &data));

  ASSERT_TRUE(fm->ReadFile((prefix + "zh_CN/zh-CN_file").c_str(), &data));
  EXPECT_STREQ("zh-CN_file contents\n", data.c_str());

  ASSERT_TRUE(fm->ReadFile((prefix + "zh_CN/2048_file").c_str(), &data));
  EXPECT_EQ(2048U, data.size());

  ASSERT_TRUE(fm->ReadFile((prefix + "zh_CN/big_file").c_str(), &data));
  EXPECT_EQ(32616U, data.size());

  EXPECT_TRUE(fm->FileExists((prefix + "global_file").c_str(), &path));
  EXPECT_STREQ((full_base_path + "/global_file").c_str(),
               path.c_str());
  EXPECT_STREQ(fm->GetFullPath((prefix + "global_file").c_str()).c_str(),
               path.c_str());

  EXPECT_FALSE(fm->FileExists((prefix + "non-exists").c_str(), &path));
  EXPECT_STREQ((full_base_path + "/non-exists").c_str(),
               path.c_str());
  EXPECT_STREQ(fm->GetFullPath((prefix + "non-exists").c_str()).c_str(),
               path.c_str());

  EXPECT_FALSE(fm->FileExists(("../" + base_filename).c_str(), &path));
  EXPECT_STREQ(full_base_path.c_str(), path.c_str());

  if (zip) {
    ASSERT_TRUE(fm->ReadFile((prefix + "gLoBaL_FiLe").c_str(), &data));
    EXPECT_STREQ((full_base_path + "/gLoBaL_FiLe").c_str(),
                 fm->GetFullPath((prefix + "gLoBaL_FiLe").c_str()).c_str());
    EXPECT_STREQ("global_file at top\n", data.c_str());
    EXPECT_TRUE(fm->FileExists((prefix + "1033/1033_FiLe").c_str(), &path));
    EXPECT_STREQ((full_base_path + "/1033/1033_FiLe").c_str(),
                 path.c_str());
    EXPECT_FALSE(fm->IsDirectlyAccessible((prefix + "gLoBaL_FiLe").c_str(),
                                          &path));
    EXPECT_STREQ((full_base_path + "/gLoBaL_FiLe").c_str(),
                 path.c_str());
  } else {
    // Case sensitive may not be available on some platforms.
    //EXPECT_FALSE(fm->FileExists("1033/1033_FiLe", &path));
    //EXPECT_STREQ((base_path + "/1033/1033_FiLe").c_str(), path.c_str());
    EXPECT_TRUE(fm->IsDirectlyAccessible((prefix + "gLoBaL_FiLe").c_str(),
                                         &path));
    EXPECT_STREQ((full_base_path + "/gLoBaL_FiLe").c_str(),
                 path.c_str());
  }
}

void TestFileManagerWriteFunctions(const std::string &prefix,
                                   const std::string &base_path,
                                   FileManagerInterface *fm, bool zip) {
  ASSERT_TRUE(fm->IsValid());
  std::string data;
  std::string path;
  std::string path2;
  std::string full_base_path = BuildFilePath(GetCurrentDirectory().c_str(),
                                             base_path.c_str(), NULL);
  // Test write file in top dir.
  data = "new_file contents\n";
  uint64_t t = time(NULL) * UINT64_C(1000);
  ASSERT_TRUE(fm->WriteFile((prefix + "new_file").c_str(), data, false));
  ASSERT_TRUE(fm->FileExists((prefix + "new_file").c_str(), &path));
  EXPECT_LE(abs(static_cast<int>(
      fm->GetLastModifiedTime((prefix + "new_file").c_str()) - t)), 1000);
  EXPECT_STREQ((full_base_path + "/new_file").c_str(), path.c_str());
  ASSERT_TRUE(fm->ReadFile((prefix + "new_file").c_str(), &data));
  EXPECT_STREQ("new_file contents\n", data.c_str());
  path.clear();
  ASSERT_TRUE(fm->ExtractFile((prefix + "new_file").c_str(), &path));
  ASSERT_TRUE(ReadFileContents(path.c_str(), &data));
  EXPECT_STREQ("new_file contents\n", data.c_str());
  ::unlink(path.c_str());
  EXPECT_FALSE(ReadFileContents(path.c_str(), &data));
  path2 = path;
  ASSERT_TRUE(fm->ExtractFile((prefix + "new_file").c_str(), &path));
  EXPECT_STREQ(path2.c_str(), path.c_str());
  ASSERT_TRUE(ReadFileContents(path.c_str(), &data));
  EXPECT_STREQ("new_file contents\n", data.c_str());
  ASSERT_TRUE(fm->FileExists((prefix + "new_file").c_str(), &path));

  // Test write file in sub dir.
  data = "en_new_file contents\n";
  ASSERT_TRUE(fm->WriteFile((prefix + "en/new_file").c_str(), data, false));
  ASSERT_TRUE(fm->FileExists((prefix + "en/new_file").c_str(), &path));
  EXPECT_STREQ((full_base_path + "/en/new_file").c_str(),
               path.c_str());
  ASSERT_TRUE(fm->ReadFile((prefix + "en/new_file").c_str(), &data));
  EXPECT_STREQ("en_new_file contents\n", data.c_str());
  path.clear();
  ASSERT_TRUE(fm->ExtractFile((prefix + "en/new_file").c_str(), &path));
  ASSERT_TRUE(ReadFileContents(path.c_str(), &data));
  EXPECT_STREQ("en_new_file contents\n", data.c_str());
  ::unlink(path.c_str());
  EXPECT_FALSE(ReadFileContents(path.c_str(), &data));
  path2 = path;
  ASSERT_TRUE(fm->ExtractFile((prefix + "en/new_file").c_str(), &path));
  EXPECT_STREQ(path2.c_str(), path.c_str());
  ASSERT_TRUE(ReadFileContents(path.c_str(), &data));
  EXPECT_STREQ("en_new_file contents\n", data.c_str());
  ASSERT_TRUE(fm->FileExists((prefix + "en/new_file").c_str(), &path));

  // Test overwrite an existing file.
  EXPECT_FALSE(fm->WriteFile((prefix + "en/new_file").c_str(), data, false));

  EXPECT_TRUE(fm->WriteFile((prefix + "en/new_file").c_str(), data, true));
  EXPECT_TRUE(fm->RemoveFile((prefix + "new_file").c_str()));
  EXPECT_TRUE(fm->RemoveFile((prefix + "en/new_file").c_str()));
  EXPECT_FALSE(fm->FileExists((prefix + "new_file").c_str(), &path));
  EXPECT_FALSE(fm->FileExists((prefix + "en/new_file").c_str(), &path));
}

std::set<std::string> actual_set;
bool EnumerateCallback(const char *name) {
  LOG("EnumerateCallback: %zu %s", actual_set.size(), name);
  EXPECT_TRUE(actual_set.find(name) == actual_set.end());
  actual_set.insert(name);
  return true;
}

void TestFileManagerEnumerate(FileManagerInterface *fm) {
  actual_set.clear();
  std::set<std::string> expected_set;
  for (size_t i = 0; i < arraysize(filenames); i++)
    expected_set.insert(filenames[i]);
  EXPECT_TRUE(fm->EnumerateFiles("", NewSlot(EnumerateCallback)));
  EXPECT_TRUE(expected_set == actual_set);

  actual_set.clear();
  expected_set.clear();
  expected_set.insert("2048_file");
  expected_set.insert("big_file");
  expected_set.insert("global_file");
  expected_set.insert("zh-CN_file");
  expected_set.insert("strings.xml");
  EXPECT_TRUE(fm->EnumerateFiles("zh_CN", NewSlot(EnumerateCallback)));
  EXPECT_TRUE(expected_set == actual_set);
  actual_set.clear();
  EXPECT_TRUE(fm->EnumerateFiles("zh_CN/", NewSlot(EnumerateCallback)));
  EXPECT_TRUE(expected_set == actual_set);
}

void TestFileManagerLocalized(FileManagerInterface *fm,
                              const std::string &locale,
                              const std::string &alternative_locale) {
  std::string contents(" contents\n");
  std::string data;
  std::string filename;

  filename = locale + "_file";
  ASSERT_TRUE(fm->ReadFile(filename.c_str(), &data));
  EXPECT_STREQ((filename + contents).c_str(), data.c_str());

  filename = alternative_locale + "_file";
  ASSERT_TRUE(fm->ReadFile(filename.c_str(), &data));
  EXPECT_STREQ((filename + contents).c_str(), data.c_str());

  filename = "en_file";
  ASSERT_TRUE(fm->ReadFile(filename.c_str(), &data));
  EXPECT_STREQ((filename + contents).c_str(), data.c_str());

  filename = "1033_file";
  ASSERT_TRUE(fm->ReadFile(filename.c_str(), &data));
  EXPECT_STREQ((filename + contents).c_str(), data.c_str());

  // Global files always have higher priority.
  filename = "global_file";
  ASSERT_TRUE(fm->ReadFile(filename.c_str(), &data));
  EXPECT_STREQ((filename + " at top\n").c_str(), data.c_str());
}

TEST(FileManager, DirRead) {
  FileManagerInterface *fm = new DirFileManager();
  ASSERT_TRUE(fm->Init(base_dir_path, false));
  TestFileManagerReadFunctions("", base_dir_path, fm, false);
  delete fm;
}

TEST(FileManager, ZipRead) {
  FileManagerInterface *fm = new ZipFileManager();
  ASSERT_TRUE(fm->Init(base_gg_path, false));
  TestFileManagerReadFunctions("", base_gg_path, fm, true);
  delete fm;
}

TEST(FileManager, DirWrite) {
  FileManagerInterface *fm = new DirFileManager();
  ASSERT_TRUE(fm->Init(base_new_dir_path, true));
  TestFileManagerWriteFunctions("", base_new_dir_path, fm, false);
  delete fm;
  RemoveDirectory(base_new_dir_path);
}

TEST(FileManager, ZipWrite) {
  FileManagerInterface *fm = new ZipFileManager();
  ASSERT_TRUE(fm->Init(base_new_gg_path, true));
  TestFileManagerWriteFunctions("", base_new_gg_path, fm, true);
  delete fm;
  ::unlink(base_new_gg_path);
}

TEST(FileManager, DirEnumerate) {
  FileManagerInterface *fm = new DirFileManager();
  ASSERT_TRUE(fm->Init(base_dir_path, true));
  TestFileManagerEnumerate(fm);
  delete fm;
}

TEST(FileManager, ZipEnumerate) {
  FileManagerInterface *fm = new ZipFileManager();
  ASSERT_TRUE(fm->Init(base_gg_path, true));
  TestFileManagerEnumerate(fm);
  delete fm;
}

TEST(FileManager, LocalizedFile) {
  static const char *locales_full[] = { "en_US", "zh_CN.UTF8", NULL };
  static const char *locales[] = { "en", "zh-CN", NULL };
  static const char *alt_locales[] = { "1033", "2052", NULL };

  for (size_t i = 0; locales[i]; ++i) {
    setlocale(LC_MESSAGES, locales_full[i]);

    FileManagerInterface *fm = new LocalizedFileManager(new DirFileManager());
    ASSERT_TRUE(fm->Init(base_dir_path, false));
    TestFileManagerLocalized(fm, locales[i], alt_locales[i]);
    delete fm;

    fm = new LocalizedFileManager(new ZipFileManager());
    ASSERT_TRUE(fm->Init(base_gg_path, false));
    TestFileManagerLocalized(fm, locales[i], alt_locales[i]);
    delete fm;
  }
}

TEST(FileManager, FileManagerWrapper) {
  FileManagerWrapper *fm = new FileManagerWrapper();
  FileManagerInterface *dir_fm = new DirFileManager();
  ASSERT_TRUE(dir_fm->Init(base_dir_path, true));
  FileManagerInterface *zip_fm = new ZipFileManager();
  ASSERT_TRUE(zip_fm->Init(base_gg_path, true));
  FileManagerInterface *dir_write_fm = new DirFileManager();
  ASSERT_TRUE(dir_write_fm->Init(base_new_dir_path, true));
  FileManagerInterface *zip_write_fm = new ZipFileManager();
  ASSERT_TRUE(zip_write_fm->Init(base_new_gg_path, true));
  EXPECT_TRUE(fm->RegisterFileManager("", dir_fm));
  EXPECT_TRUE(fm->RegisterFileManager("zip/", zip_fm));
  EXPECT_TRUE(fm->RegisterFileManager("dir_write/", dir_write_fm));
  EXPECT_TRUE(fm->RegisterFileManager("zip_write/", zip_write_fm));

  TestFileManagerReadFunctions("", base_dir_path, fm, false);
  TestFileManagerWriteFunctions("dir_write/", base_new_dir_path, fm, false);
  TestFileManagerReadFunctions("zip/", base_gg_path, fm, true);
  // TestFileManagerWriteFunctions("zip_write/", base_new_gg_path, fm, true);

  actual_set.clear();
  std::set<std::string> expected_set;
  for (size_t i = 0; i < arraysize(filenames); i++) {
    expected_set.insert(filenames[i]);
    expected_set.insert(std::string("zip/") + filenames[i]);
  }
  EXPECT_TRUE(fm->EnumerateFiles("", NewSlot(EnumerateCallback)));
  EXPECT_TRUE(expected_set == actual_set);

  actual_set.clear();
  expected_set.clear();
  expected_set.insert("2048_file");
  expected_set.insert("big_file");
  expected_set.insert("global_file");
  expected_set.insert("zh-CN_file");
  expected_set.insert("strings.xml");
  EXPECT_TRUE(fm->EnumerateFiles("zh_CN", NewSlot(EnumerateCallback)));
  EXPECT_TRUE(expected_set == actual_set);
  actual_set.clear();
  EXPECT_TRUE(fm->EnumerateFiles("zip/zh_CN", NewSlot(EnumerateCallback)));
  EXPECT_TRUE(expected_set == actual_set);
  actual_set.clear();
  EXPECT_TRUE(fm->EnumerateFiles("zh_CN/", NewSlot(EnumerateCallback)));
  EXPECT_TRUE(expected_set == actual_set);
  actual_set.clear();
  EXPECT_TRUE(fm->EnumerateFiles("zip/zh_CN/", NewSlot(EnumerateCallback)));
  EXPECT_TRUE(expected_set == actual_set);

  expected_set.clear();
  for (size_t i = 0; i < arraysize(filenames); i++)
    expected_set.insert(filenames[i]);
  actual_set.clear();
  EXPECT_TRUE(fm->EnumerateFiles("zip", NewSlot(EnumerateCallback)));
  EXPECT_TRUE(expected_set == actual_set);
  actual_set.clear();
  EXPECT_TRUE(fm->EnumerateFiles("zip/", NewSlot(EnumerateCallback)));
  EXPECT_TRUE(expected_set == actual_set);
  delete fm;
  RemoveDirectory(base_new_dir_path);
  ::unlink(base_new_gg_path);
}

int main(int argc, char **argv) {
  testing::ParseGTestFlags(&argc, argv);

  return RUN_ALL_TESTS();
}
