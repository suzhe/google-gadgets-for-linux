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

#include <cstdio>
#include <cstdlib>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <ggadget/common.h>
#include <ggadget/logger.h>
#include <unittest/gtest.h>
#include <locale.h>
#include "../file_system.h"

using namespace ggadget;
using namespace ggadget::framework;
using namespace ggadget::framework::linux_system;

TEST(FileSystem, OpenTextFile) {
  FileSystem filesystem;
  filesystem.DeleteFolder("/tmp/GGL_FileSystem_Test", true);

  mkdir("/tmp/GGL_FileSystem_Test", 0700);
  FILE *file = fopen("/tmp/GGL_FileSystem_Test/file.cc", "wb");
  fclose(file);

  // Opens an existing file for reading.
  TextStreamInterface *ti =
      filesystem.OpenTextFile("/tmp/GGL_FileSystem_Test/file.cc",
                              IO_MODE_READING,
                              false,
                              TRISTATE_USE_DEFAULT);
  ASSERT_TRUE(ti != NULL);
  ti->Close();
  ti->Destroy();

  // Opens a non-existing file for reading.
  ti = filesystem.OpenTextFile("/tmp/GGL_FileSystem_Test/file2.cc",
                               IO_MODE_READING,
                               false,
                               TRISTATE_USE_DEFAULT);
  ASSERT_TRUE(ti == NULL);

  // Opens a non-existing file for reading and create it.
  ti = filesystem.OpenTextFile("/tmp/GGL_FileSystem_Test/file2.cc",
                               IO_MODE_READING,
                               true,
                               TRISTATE_USE_DEFAULT);
  ASSERT_TRUE(ti != NULL);
  ti->Close();
  ti->Destroy();

  // Opens an existing file for writing.
  ti = filesystem.CreateTextFile("/tmp/GGL_FileSystem_Test/file.cc",
                                 false,
                                 false);
  ASSERT_TRUE(ti == NULL);

  // Opens an existing file for writing.
  ti = filesystem.CreateTextFile("/tmp/GGL_FileSystem_Test/file.cc",
                                 true,
                                 false);
  ASSERT_TRUE(ti != NULL);
  ti->Close();
  ti->Destroy();

  filesystem.DeleteFolder("/tmp/GGL_FileSystem_Test", true);
}

TEST(FileSystem, Read) {
  FileSystem filesystem;
  filesystem.DeleteFolder("/tmp/GGL_FileSystem_Test", true);

  mkdir("/tmp/GGL_FileSystem_Test", 0700);
  FILE *file = fopen("/tmp/GGL_FileSystem_Test/file.cc", "wb");
  char data[] =
      "this is a test\n"
      "\xe4\xb8\xad\xe6\x96\x87\n"
      "another test\r\n"
      "\xe5\x9d\x8f??\xe6\x96\x87\xe5\xad\x97";

  fwrite(data, 1, sizeof(data) - 1, file);
  fclose(file);

  // Opens an existing file for reading.
  TextStreamInterface *ti =
      filesystem.OpenTextFile("/tmp/GGL_FileSystem_Test/file.cc",
                              IO_MODE_READING,
                              false,
                              TRISTATE_USE_DEFAULT);
  ASSERT_TRUE(ti != NULL);

  EXPECT_EQ(1, ti->GetLine());
  EXPECT_EQ(1, ti->GetColumn());

  EXPECT_EQ("this ", ti->Read(5));
  EXPECT_FALSE(ti->IsAtEndOfLine());
  EXPECT_FALSE(ti->IsAtEndOfStream());
  EXPECT_EQ(1, ti->GetLine());
  EXPECT_EQ(6, ti->GetColumn());
  EXPECT_EQ("is a test", ti->Read(9));
  EXPECT_EQ(1, ti->GetLine());
  EXPECT_EQ(15, ti->GetColumn());
  EXPECT_TRUE(ti->IsAtEndOfLine());
  EXPECT_FALSE(ti->IsAtEndOfStream());
  ti->Skip(1);
  EXPECT_EQ(2, ti->GetLine());
  EXPECT_EQ(1, ti->GetColumn());

  EXPECT_EQ("\xe4\xb8\xad", ti->Read(1));
  EXPECT_EQ(2, ti->GetLine());
  EXPECT_EQ(2, ti->GetColumn());
  EXPECT_EQ("\xe6\x96\x87", ti->ReadLine());
  EXPECT_EQ(3, ti->GetLine());
  EXPECT_EQ(1, ti->GetColumn());

  ti->SkipLine();
  EXPECT_EQ(4, ti->GetLine());
  EXPECT_EQ(1, ti->GetColumn());
  EXPECT_EQ("\xe5\x9d\x8f??\xe6\x96\x87\xe5\xad\x97",
            ti->Read(1000));
  EXPECT_FALSE(ti->IsAtEndOfLine());
  EXPECT_TRUE(ti->IsAtEndOfStream());
  EXPECT_EQ(4, ti->GetLine());
  EXPECT_EQ(6, ti->GetColumn());

  ti->Close();
  ti->Destroy();

  ti = filesystem.OpenTextFile("/tmp/GGL_FileSystem_Test/file.cc",
                               IO_MODE_READING,
                               false,
                               TRISTATE_USE_DEFAULT);
  ASSERT_TRUE(ti != NULL);
  EXPECT_EQ(
      "this is a test\n"
      "\xe4\xb8\xad\xe6\x96\x87\n"
      "another test\n"
      "\xe5\x9d\x8f??\xe6\x96\x87\xe5\xad\x97",
      ti->ReadAll());
  EXPECT_EQ(4, ti->GetLine());
  EXPECT_EQ(6, ti->GetColumn());

  ti->Close();
  ti->Destroy();

  filesystem.DeleteFolder("/tmp/GGL_FileSystem_Test", true);
}

TEST(FileSystem, Write) {
  FileSystem filesystem;
  filesystem.DeleteFolder("/tmp/GGL_FileSystem_Test", true);

  mkdir("/tmp/GGL_FileSystem_Test", 0700);
  // Opens an existing file for reading.
  TextStreamInterface *ti =
      filesystem.CreateTextFile("/tmp/GGL_FileSystem_Test/file.cc",
                              true,
                              false);
  ASSERT_TRUE(ti != NULL);

  EXPECT_EQ(1, ti->GetLine());
  EXPECT_EQ(1, ti->GetColumn());

  ti->Write("this ");
  EXPECT_EQ(1, ti->GetLine());
  EXPECT_EQ(6, ti->GetColumn());
  ti->Write("is a test");
  EXPECT_EQ(1, ti->GetLine());
  EXPECT_EQ(15, ti->GetColumn());
  ti->WriteBlankLines(1);
  EXPECT_EQ(2, ti->GetLine());
  EXPECT_EQ(1, ti->GetColumn());

  ti->Write("\xe4\xb8\xad");
  EXPECT_EQ(2, ti->GetLine());
  EXPECT_EQ(2, ti->GetColumn());
  ti->WriteLine("\xe6\x96\x87");
  EXPECT_EQ(3, ti->GetLine());
  EXPECT_EQ(1, ti->GetColumn());

  ti->WriteBlankLines(1);
  EXPECT_EQ(4, ti->GetLine());
  EXPECT_EQ(1, ti->GetColumn());
  ti->Write("\xe5\x9d\x8f??\xe6\x96\x87\xe5\xad\x97");
  EXPECT_EQ(4, ti->GetLine());
  EXPECT_EQ(6, ti->GetColumn());

  ti->Close();
  ti->Destroy();

  char buffer[1024];
  FILE *file = fopen("/tmp/GGL_FileSystem_Test/file.cc", "rb");
  size_t size = fread(buffer, 1, sizeof(buffer), file);
  EXPECT_EQ(
      "this is a test\n"
      "\xe4\xb8\xad\xe6\x96\x87\n"
      "\n"
      "\xe5\x9d\x8f??\xe6\x96\x87\xe5\xad\x97",
      std::string(buffer, size));
  fclose(file);

  filesystem.DeleteFolder("/tmp/GGL_FileSystem_Test", true);
}

int main(int argc, char **argv) {
  testing::ParseGTestFlags(&argc, argv);
  setlocale(LC_ALL, "en_US.UTF-8");
  return RUN_ALL_TESTS();
}
