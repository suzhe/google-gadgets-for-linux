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

#include <sys/types.h>
#include <sys/stat.h>
#include <locale.h>
#include <cstdio>
#include <stdlib.h>
#include <unistd.h>
#include <string>
#include "ggadget/logger.h"
#include "ggadget/system_utils.h"
#include "ggadget/gadget_consts.h"
#include "unittest/gtest.h"

using namespace ggadget;

TEST(SystemUtils, BuildPath) {
  EXPECT_STREQ(
      "/abc/def/ghi",
      BuildPath(kDirSeparatorStr, "/", "/abc", "def/", "ghi", NULL).c_str());
  EXPECT_STREQ(
      "hello/:world",
      BuildPath("/:", "hello", "", "world", NULL).c_str());
  EXPECT_STREQ(
      "hello",
      BuildPath("//", "hello", NULL).c_str());
  EXPECT_STREQ(
      "/usr/sbin/sudo",
      BuildPath(kDirSeparatorStr, "//usr", "sbin//", "//sudo", NULL).c_str());
  EXPECT_STREQ(
      "//usr//sbin//a//sudo",
      BuildPath("//", "//usr", "//", "sbin", "////a//", "sudo", NULL).c_str());
  EXPECT_STREQ(
      "//usr",
      BuildPath("//", "////", "//////", "usr//", "////", "////", NULL).c_str());
}


TEST(SystemUtils, SplitFilePath) {
  std::string dir;
  std::string file;
  EXPECT_FALSE(SplitFilePath("/", &dir, &file));
  EXPECT_STREQ("/", dir.c_str());
  EXPECT_STREQ("", file.c_str());
  EXPECT_TRUE(SplitFilePath("/tmp", &dir, &file));
  EXPECT_STREQ("/", dir.c_str());
  EXPECT_STREQ("tmp", file.c_str());
  EXPECT_TRUE(SplitFilePath("/foo/bar/file", &dir, &file));
  EXPECT_STREQ("/foo/bar", dir.c_str());
  EXPECT_STREQ("file", file.c_str());
  EXPECT_FALSE(SplitFilePath("file", &dir, &file));
  EXPECT_STREQ("", dir.c_str());
  EXPECT_STREQ("file", file.c_str());
  EXPECT_FALSE(SplitFilePath("dir/", &dir, &file));
  EXPECT_STREQ("dir", dir.c_str());
  EXPECT_STREQ("", file.c_str());
  EXPECT_TRUE(SplitFilePath("dir///file", &dir, &file));
  EXPECT_STREQ("dir", dir.c_str());
  EXPECT_STREQ("file", file.c_str());
  EXPECT_TRUE(SplitFilePath("///dir///file", &dir, &file));
  EXPECT_STREQ("///dir", dir.c_str());
  EXPECT_STREQ("file", file.c_str());
}

TEST(SystemUtils, EnsureDirectories) {
#define TEST_HOME "/tmp/TestEnsureDirectories"
  EXPECT_FALSE(EnsureDirectories(""));
  // NOTE: The following tests are Unix/Linux specific.
  EXPECT_TRUE(EnsureDirectories("/etc"));
  EXPECT_FALSE(EnsureDirectories("/etc/hosts"));
  EXPECT_FALSE(EnsureDirectories("/etc/hosts/anything"));
  EXPECT_TRUE(EnsureDirectories("/tmp"));
  EXPECT_TRUE(EnsureDirectories("/tmp/"));
  system("rm -rf " TEST_HOME);
  EXPECT_TRUE(EnsureDirectories(TEST_HOME));
  system("rm -rf " TEST_HOME);
  EXPECT_TRUE(EnsureDirectories(TEST_HOME "/"));
  EXPECT_TRUE(EnsureDirectories(TEST_HOME "/a/b/c/d/e"));
  system("touch " TEST_HOME "/file");
  EXPECT_FALSE(EnsureDirectories(TEST_HOME "/file"));
  EXPECT_FALSE(EnsureDirectories(TEST_HOME "/file/"));
  EXPECT_FALSE(EnsureDirectories(TEST_HOME "/file/a/b/c"));

  char cwd[4096];
  ASSERT_TRUE(getcwd(cwd, sizeof(cwd)));
  chdir(TEST_HOME);
  EXPECT_TRUE(EnsureDirectories("a/b/c/d/e"));
  EXPECT_TRUE(EnsureDirectories("d/e"));
  chdir(cwd);
}

TEST(SystemUtils, GetCurrentDirectory) {
  std::string curdir = GetCurrentDirectory();
  ASSERT_TRUE(curdir.length() > 0);
  ASSERT_EQ(0, chdir("/"));
  ASSERT_STREQ("/", GetCurrentDirectory().c_str());
  ASSERT_EQ(0, chdir(curdir.c_str()));
}

TEST(SystemUtils, CreateTempDirectory) {
  std::string path1;
  std::string path2;
  ASSERT_TRUE(CreateTempDirectory("abc", &path1));
  ASSERT_TRUE(CreateTempDirectory("abc", &path2));
  ASSERT_STRNE(path1.c_str(), path2.c_str());
  ASSERT_EQ(0, access(path1.c_str(), R_OK|X_OK|W_OK|F_OK));
  ASSERT_EQ(0, access(path2.c_str(), R_OK|X_OK|W_OK|F_OK));

  struct stat stat_value;
  ASSERT_EQ(0, stat(path1.c_str(), &stat_value));
  ASSERT_TRUE(S_ISDIR(stat_value.st_mode));
  ASSERT_EQ(0, stat(path2.c_str(), &stat_value));
  ASSERT_TRUE(S_ISDIR(stat_value.st_mode));

  rmdir(path1.c_str());
  rmdir(path2.c_str());
}

TEST(SystemUtils, RemoveDirectory) {
  std::string tempdir;
  ASSERT_TRUE(CreateTempDirectory("removeme", &tempdir));
  std::string subdir = BuildFilePath(tempdir.c_str(), "subdir", NULL);
  std::string file = BuildFilePath(tempdir.c_str(), "file", NULL);
  std::string subfile = BuildFilePath(subdir.c_str(), "file", NULL);
  ASSERT_EQ(0, system(("mkdir " + subdir).c_str()));
  ASSERT_EQ(0, system(("touch " + file).c_str()));
  ASSERT_EQ(0, system(("touch " + subfile).c_str()));
  ASSERT_TRUE(RemoveDirectory(tempdir.c_str(), true));

  ASSERT_TRUE(CreateTempDirectory("removeme1", &tempdir));
  subdir = BuildFilePath(tempdir.c_str(), "subdir", NULL);
  file = BuildFilePath(tempdir.c_str(), "file", NULL);
  subfile = BuildFilePath(subdir.c_str(), "file", NULL);
  ASSERT_EQ(0, system(("mkdir " + subdir).c_str()));
  ASSERT_EQ(0, system(("touch " + file).c_str()));
  ASSERT_EQ(0, system(("touch " + subfile).c_str()));
  ASSERT_EQ(0, system(("chmod a-w " + subfile).c_str()));
  ASSERT_FALSE(RemoveDirectory(tempdir.c_str(), false));
  ASSERT_TRUE(RemoveDirectory(tempdir.c_str(), true));
}

TEST(SystemUtils, NormalizeFilePath) {
  ASSERT_STREQ("/", NormalizeFilePath("/").c_str());
  ASSERT_STREQ("/", NormalizeFilePath("//").c_str());
  ASSERT_STREQ("/abc", NormalizeFilePath("/abc").c_str());
  ASSERT_STREQ("/abc", NormalizeFilePath("/abc/").c_str());
  ASSERT_STREQ("/abc", NormalizeFilePath("/abc/def/..").c_str());
  ASSERT_STREQ("/abc", NormalizeFilePath("//abc/.///def/..").c_str());
  ASSERT_STREQ("/abc", NormalizeFilePath("//abc/./def/../../abc/").c_str());
  ASSERT_STREQ("/", NormalizeFilePath("//abc/./def/../../").c_str());
}

TEST(Locales, GetSystemLocaleInfo) {
  std::string lang, terr;
  setlocale(LC_MESSAGES, "en_US.UTF-8");
  ASSERT_TRUE(GetSystemLocaleInfo(&lang, &terr));
  ASSERT_STREQ("en", lang.c_str());
  ASSERT_STREQ("US", terr.c_str());
  setlocale(LC_MESSAGES, "en_US");
  ASSERT_TRUE(GetSystemLocaleInfo(&lang, &terr));
  ASSERT_STREQ("en", lang.c_str());
  ASSERT_STREQ("US", terr.c_str());
}

int main(int argc, char **argv) {
  testing::ParseGTestFlags(&argc, argv);
  return RUN_ALL_TESTS();
}
