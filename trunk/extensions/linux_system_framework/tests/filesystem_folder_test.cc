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
#include <set>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "ggadget/common.h"
#include "ggadget/logger.h"
#include "unittest/gtest.h"
#include "../file_system.h"

using namespace ggadget;
using namespace ggadget::framework;
using namespace ggadget::framework::linux_system;

TEST(FileSystem, GetInformation) {
  FileSystem filesystem;
  mkdir("/tmp/GGL_FileSystem_Test", 0700);
  FILE *file = fopen("/tmp/GGL_FileSystem_Test/file.cc", "wb");
  fwrite("test", 1, 4, file);
  fclose(file);

  FolderInterface *fi = filesystem.GetFolder("/tmp/GGL_FileSystem_Test");
  ASSERT_TRUE(fi != NULL);

  EXPECT_EQ("/tmp/GGL_FileSystem_Test", fi->GetPath());
  EXPECT_EQ("GGL_FileSystem_Test", fi->GetName());
  EXPECT_GT(fi->GetSize(), 4);
  EXPECT_GT(fi->GetDateLastModified().value, 0U);
  EXPECT_GT(fi->GetDateLastAccessed().value, 0U);

  fi->Destroy();
  filesystem.DeleteFolder("/tmp/GGL_FileSystem_Test", true);
}

TEST(FileSystem, SetName) {
  FileSystem filesystem;
  mkdir("/tmp/GGL_FileSystem_Test", 0700);
  FILE *file = fopen("/tmp/GGL_FileSystem_Test/file.cc", "wb");
  fwrite("test", 1, 4, file);
  fclose(file);

  FolderInterface *fi = filesystem.GetFolder("/tmp/GGL_FileSystem_Test");
  ASSERT_TRUE(fi != NULL);

  EXPECT_TRUE(fi->SetName("GGL_FileSystem_Test2"));
  EXPECT_EQ("/tmp/GGL_FileSystem_Test2", fi->GetPath());
  EXPECT_TRUE(filesystem.FileExists("/tmp/GGL_FileSystem_Test2/file.cc"));
  EXPECT_FALSE(filesystem.FileExists("/tmp/GGL_FileSystem_Test/file.cc"));

  // SetName() doesn't support moving file.
  EXPECT_FALSE(fi->SetName("/tmp/file3"));
  EXPECT_EQ("/tmp/GGL_FileSystem_Test2", fi->GetPath());
  EXPECT_TRUE(filesystem.FileExists("/tmp/GGL_FileSystem_Test2/file.cc"));
  EXPECT_FALSE(filesystem.FileExists("/tmp/GGL_FileSystem_Test/file.cc"));

  fi->Destroy();
  filesystem.DeleteFolder("/tmp/GGL_FileSystem_Test2", true);
}

TEST(FileSystem, GetParentFolder) {
  FileSystem filesystem;
  mkdir("/tmp/GGL_FileSystem_Test", 0700);
  FILE *file = fopen("/tmp/GGL_FileSystem_Test/file.cc", "wb");
  fwrite("test", 1, 4, file);
  fclose(file);

  FolderInterface *fi = filesystem.GetFolder("/tmp/GGL_FileSystem_Test/");
  ASSERT_TRUE(fi != NULL);

  FolderInterface *folder = fi->GetParentFolder();
  EXPECT_EQ("/tmp", folder->GetPath());
  folder->Destroy();

  fi->Destroy();
  filesystem.DeleteFolder("/tmp/GGL_FileSystem_Test", true);
}

TEST(FileSystem, Delete) {
  FileSystem filesystem;
  mkdir("/tmp/GGL_FileSystem_Test", 0700);
  FILE *file = fopen("/tmp/GGL_FileSystem_Test/file.cc", "wb");
  fwrite("test", 1, 4, file);
  fclose(file);

  FolderInterface *fi = filesystem.GetFolder("/tmp/GGL_FileSystem_Test/");
  ASSERT_TRUE(fi != NULL);

  fi->Delete(true);
  EXPECT_FALSE(filesystem.FolderExists("/tmp/GGL_FileSystem_Test/"));

  fi->Destroy();
}

TEST(FileSystem, Copy) {
  FileSystem filesystem;
  mkdir("/tmp/GGL_FileSystem_Test", 0700);
  FILE *file = fopen("/tmp/GGL_FileSystem_Test/file.cc", "wb");
  fwrite("test", 1, 4, file);
  fclose(file);

  FolderInterface *fi = filesystem.GetFolder("/tmp/GGL_FileSystem_Test/");
  ASSERT_TRUE(fi != NULL);

  // Copies a directory to another directory.
  EXPECT_TRUE(fi->Copy("/tmp/GGL_FileSystem_Test2", false));
  EXPECT_FALSE(fi->Copy("/tmp", false));
  EXPECT_TRUE(filesystem.FileExists("/tmp/GGL_FileSystem_Test/file.cc"));
  EXPECT_TRUE(filesystem.FileExists("/tmp/GGL_FileSystem_Test2/file.cc"));

  // Copies a file to another folder.
  EXPECT_TRUE(fi->Copy("/tmp/GGL_FileSystem_Test2", false));
  EXPECT_FALSE(fi->Copy("/tmp/GGL_FileSystem_Test2/", false));
  EXPECT_TRUE(fi->Copy("/tmp/GGL_FileSystem_Test2/", true));
  EXPECT_TRUE(
      filesystem.FolderExists("/tmp/GGL_FileSystem_Test2/GGL_FileSystem_Test"));

  fi->Destroy();
  filesystem.DeleteFolder("/tmp/GGL_FileSystem_Test", true);
  filesystem.DeleteFolder("/tmp/GGL_FileSystem_Test2", true);
}

TEST(FileSystem, Move) {
  FileSystem filesystem;
  mkdir("/tmp/GGL_FileSystem_Test", 0700);
  FILE *file = fopen("/tmp/GGL_FileSystem_Test/file.cc", "wb");
  fwrite("test", 1, 4, file);
  fclose(file);

  FolderInterface *fi = filesystem.GetFolder("/tmp/GGL_FileSystem_Test/");
  ASSERT_TRUE(fi != NULL);

  // Moves a directory to another directory.
  EXPECT_TRUE(fi->Move("/tmp/GGL_FileSystem_Test2"));
  EXPECT_FALSE(filesystem.FileExists("/tmp/GGL_FileSystem_Test/file.cc"));
  EXPECT_TRUE(filesystem.FileExists("/tmp/GGL_FileSystem_Test2/file.cc"));

  // Moves a file to another folder.
  EXPECT_TRUE(fi->Move("/tmp/GGL_FileSystem_Test"));
  EXPECT_TRUE(filesystem.FolderExists("/tmp/GGL_FileSystem_Test"));

  fi->Destroy();
  filesystem.DeleteFolder("/tmp/GGL_FileSystem_Test", true);
}

TEST(FileSystem, FilesAndFolders) {
  FileSystem filesystem;
  mkdir("/tmp/GGL_FileSystem_Test", 0700);
  FILE *file = fopen("/tmp/GGL_FileSystem_Test/file1.cc", "wb");
  fwrite("test1", 1, 5, file);
  fclose(file);
  file = fopen("/tmp/GGL_FileSystem_Test/file2.cc", "wb");
  fwrite("test2", 1, 5, file);
  fclose(file);
  file = fopen("/tmp/GGL_FileSystem_Test/file3.cc", "wb");
  fwrite("test3", 1, 5, file);
  fclose(file);
  mkdir("/tmp/GGL_FileSystem_Test/sub1", 0700);
  mkdir("/tmp/GGL_FileSystem_Test/sub2", 0700);

  FolderInterface *fi = filesystem.GetFolder("/tmp/GGL_FileSystem_Test");
  ASSERT_TRUE(fi != NULL);

  std::set<std::string> filesset;
  FilesInterface *files;
  for (files = fi->GetFiles();
       !files->AtEnd();
       files->MoveNext()) {
    FileInterface *f = files->GetItem();
    ASSERT_TRUE(f != NULL);
    filesset.insert(f->GetName());
    f->Destroy();
  }
  EXPECT_EQ(3, files->GetCount());
  files->Destroy();
  EXPECT_TRUE(filesset.find("file1.cc") != filesset.end());
  EXPECT_TRUE(filesset.find("file2.cc") != filesset.end());
  EXPECT_TRUE(filesset.find("file3.cc") != filesset.end());
  EXPECT_EQ(3U, filesset.size());

  std::set<std::string> foldersset;
  FoldersInterface *folders;
  for (folders = fi->GetSubFolders();
       !folders->AtEnd();
       folders->MoveNext()) {
    FolderInterface *f = folders->GetItem();
    ASSERT_TRUE(f != NULL);
    foldersset.insert(f->GetName());
    f->Destroy();
  }
  EXPECT_EQ(2, folders->GetCount());
  folders->Destroy();
  EXPECT_TRUE(foldersset.find("sub1") != foldersset.end());
  EXPECT_TRUE(foldersset.find("sub2") != foldersset.end());
  EXPECT_EQ(2U, foldersset.size());

  filesystem.DeleteFolder("/tmp/GGL_FileSystem_Test", true);
}

int main(int argc, char **argv) {
  testing::ParseGTestFlags(&argc, argv);

  return RUN_ALL_TESTS();
}
