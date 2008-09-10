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
#include "../file_system.h"

using namespace ggadget;
using namespace ggadget::framework;
using namespace ggadget::framework::linux_system;

//static const char *kTestDir = "/tmp/GGL_FileSystem_Test";

TEST(FileSystem, GetDrives) {
  FileSystem filesystem;
  DrivesInterface *drives = filesystem.GetDrives();
  EXPECT_TRUE(drives != NULL);
  EXPECT_EQ(1, drives->GetCount());
  DriveInterface *drive = drives->GetItem();
  EXPECT_TRUE(drive != NULL);
  drive->Destroy();
  EXPECT_FALSE(drives->AtEnd());
  drives->MoveNext();
  EXPECT_TRUE(drives->AtEnd());
  drives->Destroy();
}

TEST(FileSystem, BuildPath) {
  FileSystem filesystem;
  EXPECT_EQ("/tmp/GGL_FileSystem_Test/file.cc",
            filesystem.BuildPath("/tmp/GGL_FileSystem_Test/", "file.cc"));
  EXPECT_EQ("/tmp/GGL_FileSystem_Test/file.cc",
            filesystem.BuildPath("/tmp/GGL_FileSystem_Test", "file.cc"));
  EXPECT_EQ("/file.cc", filesystem.BuildPath("/", "file.cc"));
  EXPECT_EQ("/tmp", filesystem.BuildPath("/tmp", ""));
  EXPECT_EQ("/tmp", filesystem.BuildPath("/tmp", NULL));
  EXPECT_EQ("", filesystem.BuildPath("", NULL));
  EXPECT_EQ("", filesystem.BuildPath(NULL, NULL));
}

// test method GetParentFolderName with valid arguments
TEST(FileSystem, GetParentFolderName) {
  FileSystem filesystem;
  EXPECT_EQ("/tmp",
            filesystem.GetParentFolderName("/tmp/GGL_FileSystem_Test/"));
  EXPECT_EQ("/tmp",
            filesystem.GetParentFolderName("/tmp/GGL_FileSystem_Test"));
  EXPECT_EQ("/", filesystem.GetParentFolderName("/tmp"));
  EXPECT_EQ("", filesystem.GetParentFolderName("/"));
  EXPECT_EQ("", filesystem.GetParentFolderName(""));
  EXPECT_EQ("", filesystem.GetParentFolderName(NULL));
}

// test method GetFileName
TEST(FileSystem, GetFileName) {
  FileSystem filesystem;
  EXPECT_EQ("file.cc",
            filesystem.GetFileName("/tmp/GGL_FileSystem_Test/file.cc"));
  EXPECT_EQ("GGL_FileSystem_Test",
            filesystem.GetFileName("/tmp/GGL_FileSystem_Test"));
  EXPECT_EQ("GGL_FileSystem_Test",
            filesystem.GetFileName("/tmp/GGL_FileSystem_Test/"));
  EXPECT_EQ("", filesystem.GetFileName("/"));
  EXPECT_EQ("", filesystem.GetFileName(""));
  EXPECT_EQ("", filesystem.GetFileName(NULL));
}

// test method GetBaseName
TEST(FileSystem, GetBaseName) {
  FileSystem filesystem;
  EXPECT_EQ("file",
            filesystem.GetBaseName("/tmp/GGL_FileSystem_Test/file.cc"));
  EXPECT_EQ("file",
            filesystem.GetBaseName("/tmp/GGL_FileSystem_Test/file"));
  EXPECT_EQ("file",
            filesystem.GetBaseName("/tmp/GGL_FileSystem_Test/file.cc/"));
  EXPECT_EQ("file",
            filesystem.GetBaseName("/tmp/GGL_FileSystem_Test/file/"));
  EXPECT_EQ("", filesystem.GetFileName("/"));
  EXPECT_EQ("", filesystem.GetFileName(""));
  EXPECT_EQ("", filesystem.GetFileName(NULL));
}

// test method GetExtensionName
TEST(FileSystem, GetExtensionName) {
  FileSystem filesystem;
  EXPECT_EQ("cc",
            filesystem.GetExtensionName("/tmp/GGL_FileSystem_Test/file.cc"));
  EXPECT_EQ("",
            filesystem.GetExtensionName("/tmp/GGL_FileSystem_Test/file"));
  EXPECT_EQ("cc",
            filesystem.GetExtensionName("/tmp/GGL_FileSystem_Test/file.cc/"));
  EXPECT_EQ("",
            filesystem.GetExtensionName("/tmp/GGL_FileSystem_Test/file/"));
  EXPECT_EQ("file",
            filesystem.GetExtensionName("/tmp/GGL_FileSystem_Test/.file"));
  EXPECT_EQ("",
            filesystem.GetExtensionName("/tmp/GGL_FileSystem_Test/file."));
  EXPECT_EQ("",
            filesystem.GetExtensionName("/"));
  EXPECT_EQ("",
            filesystem.GetExtensionName(""));
  EXPECT_EQ("",
            filesystem.GetExtensionName(NULL));
}

// test method GetAbsolutePathName
TEST(FileSystem, GetAbsolutePathName) {
  FileSystem filesystem;
  // create the expected directory path
  char current_dir[PATH_MAX + 1];
  getcwd(current_dir, PATH_MAX);
  std::string dir(current_dir);
  if (dir[dir.size() - 1] == '/')
    dir = dir + std::string("file.cc");
  else
    dir = dir + "/"+ std::string("file.cc");
  EXPECT_EQ(dir, filesystem.GetAbsolutePathName("file.cc"));
}

// test method GetTempName
TEST(FileSystem, GetTempName) {
  FileSystem filesystem;
  std::string temp = filesystem.GetTempName();
  EXPECT_GT(temp.size(), 0U);
  LOG("Temp file name: %s", temp.c_str());
}

// test method FileExists
TEST(FileSystem, FileFolderExists) {
  FileSystem filesystem;
  mkdir("/tmp/GGL_FileSystem_Test", 0700);
  FILE *file = fopen("/tmp/GGL_FileSystem_Test/file.cc", "wb");
  fclose(file);
  EXPECT_FALSE(filesystem.FileExists("/tmp/GGL_FileSystem_Test"));
  EXPECT_TRUE(filesystem.FolderExists("/tmp/GGL_FileSystem_Test"));
  EXPECT_TRUE(filesystem.FileExists("/tmp/GGL_FileSystem_Test/file.cc"));
  EXPECT_FALSE(filesystem.FileExists("/tmp/GGL_FileSystem_Test/file2.cc"));
  EXPECT_FALSE(filesystem.FileExists(""));
  EXPECT_FALSE(filesystem.FileExists(NULL));
  EXPECT_FALSE(filesystem.FolderExists(""));
  EXPECT_FALSE(filesystem.FolderExists(NULL));
  unlink("/tmp/GGL_FileSystem_Test/file.cc");
  rmdir("/tmp/GGL_FileSystem_Test");
}

// test method GetFile.
TEST(FileSystem, GetFile) {
  FileSystem filesystem;
  mkdir("/tmp/GGL_FileSystem_Test", 0700);
  FILE *file = fopen("/tmp/GGL_FileSystem_Test/file.cc", "wb");
  fclose(file);
  FileInterface *fi = filesystem.GetFile("/tmp/GGL_FileSystem_Test/file.cc");
  EXPECT_TRUE(fi != NULL);
  fi->Destroy();
  EXPECT_TRUE(filesystem.GetFile("/tmp/GGL_FileSystem_Test") == NULL);
  EXPECT_TRUE(filesystem.GetFile("/tmp/GGL_FileSystem_Test/file2.cc") == NULL);
  EXPECT_TRUE(filesystem.GetFile("") == NULL);
  EXPECT_TRUE(filesystem.GetFile(NULL) == NULL);
  unlink("/tmp/GGL_FileSystem_Test/file.cc");
  rmdir("/tmp/GGL_FileSystem_Test");
}

// test method GetFolder.
TEST(FileSystem, GetFolder) {
  FileSystem filesystem;
  mkdir("/tmp/GGL_FileSystem_Test", 0700);
  FILE *file = fopen("/tmp/GGL_FileSystem_Test/file.cc", "wb");
  fclose(file);
  FolderInterface *fi = filesystem.GetFolder("/tmp/GGL_FileSystem_Test/");
  EXPECT_TRUE(fi != NULL);
  fi->Destroy();
  fi = filesystem.GetFolder("/tmp/GGL_FileSystem_Test");
  EXPECT_TRUE(fi != NULL);
  fi->Destroy();
  fi = filesystem.GetFolder("/");
  EXPECT_TRUE(fi != NULL);
  fi->Destroy();
  EXPECT_TRUE(filesystem.GetFolder("/tmp/GGL_FileSystem_Test/file.cc") == NULL);
  EXPECT_TRUE(filesystem.GetFolder("/tmp/GGL_FileSystem_Test2") == NULL);
  EXPECT_TRUE(filesystem.GetFolder("") == NULL);
  EXPECT_TRUE(filesystem.GetFolder(NULL) == NULL);
  unlink("/tmp/GGL_FileSystem_Test/file.cc");
  rmdir("/tmp/GGL_FileSystem_Test");
}

// tests method GetSpecialFolder
TEST(FileSystem, GetSpecialFolder) {
  FileSystem filesystem;
  FolderInterface *folder;
  EXPECT_TRUE((folder = filesystem.GetSpecialFolder(SPECIAL_FOLDER_WINDOWS))
              != NULL);
  folder->Destroy();
  EXPECT_TRUE((folder = filesystem.GetSpecialFolder(SPECIAL_FOLDER_SYSTEM))
              != NULL);
  folder->Destroy();
  EXPECT_TRUE((folder = filesystem.GetSpecialFolder(SPECIAL_FOLDER_TEMPORARY))
              != NULL);
  folder->Destroy();
}

// Tests DeleteFile.
TEST(FileSystem, DeleteFile) {
  FileSystem filesystem;
  mkdir("/tmp/GGL_FileSystem_Test", 0700);
  FILE *file = fopen("/tmp/GGL_FileSystem_Test/file1.cc", "wb");
  fclose(file);
  file = fopen("/tmp/GGL_FileSystem_Test/file2.cc", "wb");
  fclose(file);
  file = fopen("/tmp/GGL_FileSystem_Test/file3.cc", "wb");
  fclose(file);

  // Deletes a single file.
  EXPECT_TRUE(filesystem.FileExists("/tmp/GGL_FileSystem_Test/file1.cc"));
  EXPECT_TRUE(filesystem.DeleteFile("/tmp/GGL_FileSystem_Test/file1.cc",
                                    true));
  EXPECT_FALSE(filesystem.FileExists("/tmp/GGL_FileSystem_Test/file1.cc"));
  EXPECT_TRUE(filesystem.FileExists("/tmp/GGL_FileSystem_Test/file2.cc"));
  EXPECT_TRUE(filesystem.FileExists("/tmp/GGL_FileSystem_Test/file3.cc"));

  // Deletes file with wild characters.
  EXPECT_TRUE(filesystem.DeleteFile("/tmp/GGL_FileSystem_Test/file*.cc",
                                    true));
  EXPECT_FALSE(filesystem.FileExists("/tmp/GGL_FileSystem_Test/file2.cc"));
  EXPECT_FALSE(filesystem.FileExists("/tmp/GGL_FileSystem_Test/file3.cc"));

  // Deletes non-existing file.
  EXPECT_FALSE(filesystem.DeleteFile("/tmp/GGL_FileSystem_Test/file4.cc",
                                     true));

  // Deletes folder.
  EXPECT_FALSE(filesystem.DeleteFile("/tmp/GGL_FileSystem_Test",
                                     true));

  EXPECT_FALSE(filesystem.DeleteFile("", true));
  EXPECT_FALSE(filesystem.DeleteFile(NULL, true));

  rmdir("/tmp/GGL_FileSystem_Test");
}

// test method DeleteFolder with existing folder
TEST(FileSystem, DeleteFolder) {
  FileSystem filesystem;
  mkdir("/tmp/GGL_FileSystem_Test", 0700);
  FILE *file = fopen("/tmp/GGL_FileSystem_Test/file1.cc", "wb");
  fclose(file);
  file = fopen("/tmp/GGL_FileSystem_Test/file2.cc", "wb");
  fclose(file);
  file = fopen("/tmp/GGL_FileSystem_Test/file3.cc", "wb");
  fclose(file);

  // Deletes files.
  EXPECT_FALSE(filesystem.DeleteFolder("/tmp/GGL_FileSystem_Test/file1.cc",
                                       true));
  EXPECT_FALSE(filesystem.DeleteFolder("/tmp/GGL_FileSystem_Test/file2.cc",
                                       true));
  EXPECT_FALSE(filesystem.DeleteFolder("/tmp/GGL_FileSystem_Test/file3.cc",
                                       true));
  EXPECT_FALSE(filesystem.DeleteFolder("/tmp/GGL_FileSystem_Test/file4.cc",
                                       true));
  // Deletes folder.
  EXPECT_TRUE(filesystem.DeleteFolder("/tmp/GGL_FileSystem_Test/",
                                      true));

  EXPECT_FALSE(filesystem.FolderExists("/tmp/GGL_FileSystem_Test/"));

  EXPECT_FALSE(filesystem.DeleteFolder("", true));
  EXPECT_FALSE(filesystem.DeleteFolder(NULL, true));
}

// test method MoveFile.
TEST(FileSystem, MoveFile) {
  FileSystem filesystem;
  mkdir("/tmp/GGL_FileSystem_Test", 0700);
  mkdir("/tmp/GGL_FileSystem_Test2", 0700);
  FILE *file = fopen("/tmp/GGL_FileSystem_Test/file1.cc", "wb");
  fclose(file);
  file = fopen("/tmp/GGL_FileSystem_Test/file2.cc", "wb");
  fclose(file);
  file = fopen("/tmp/GGL_FileSystem_Test/file3.cc", "wb");
  fclose(file);

  EXPECT_FALSE(filesystem.MoveFile("/tmp/GGL_FileSystem_Test/file1.cc",
                                   "/tmp/GGL_FileSystem_Test/file1.cc"));

  // Moves an existing file to a non-existing file.
  EXPECT_TRUE(filesystem.MoveFile("/tmp/GGL_FileSystem_Test/file1.cc",
                                  "/tmp/GGL_FileSystem_Test/file4.cc"));
  EXPECT_FALSE(filesystem.FileExists("/tmp/GGL_FileSystem_Test/file1.cc"));
  EXPECT_TRUE(filesystem.FileExists("/tmp/GGL_FileSystem_Test/file4.cc"));

  // Moves an existing file to an existing file.
  EXPECT_FALSE(filesystem.MoveFile("/tmp/GGL_FileSystem_Test/file2.cc",
                                   "/tmp/GGL_FileSystem_Test/file3.cc"));
  EXPECT_TRUE(filesystem.FileExists("/tmp/GGL_FileSystem_Test/file2.cc"));
  EXPECT_TRUE(filesystem.FileExists("/tmp/GGL_FileSystem_Test/file3.cc"));

  // Moves an existing file to an existing folder.
  EXPECT_TRUE(filesystem.MoveFile("/tmp/GGL_FileSystem_Test/file*.cc",
                                  "/tmp/GGL_FileSystem_Test2/"));
  EXPECT_FALSE(filesystem.FileExists("/tmp/GGL_FileSystem_Test/file2.cc"));
  EXPECT_FALSE(filesystem.FileExists("/tmp/GGL_FileSystem_Test/file3.cc"));
  EXPECT_FALSE(filesystem.FileExists("/tmp/GGL_FileSystem_Test/file4.cc"));
  EXPECT_TRUE(filesystem.FileExists("/tmp/GGL_FileSystem_Test2/file2.cc"));
  EXPECT_TRUE(filesystem.FileExists("/tmp/GGL_FileSystem_Test2/file3.cc"));
  EXPECT_TRUE(filesystem.FileExists("/tmp/GGL_FileSystem_Test2/file4.cc"));

  filesystem.DeleteFolder("/tmp/GGL_FileSystem_Test", true);
  filesystem.DeleteFolder("/tmp/GGL_FileSystem_Test2", true);

  EXPECT_FALSE(filesystem.MoveFile("", ""));
  EXPECT_FALSE(filesystem.MoveFile(NULL, NULL));
}

// test method MoveFolder.
TEST(FileSystem, MoveFolder) {
  FileSystem filesystem;
  mkdir("/tmp/GGL_FileSystem_Test", 0700);
  mkdir("/tmp/GGL_FileSystem_Test2", 0700);
  FILE *file = fopen("/tmp/GGL_FileSystem_Test/file1.cc", "wb");
  fclose(file);
  file = fopen("/tmp/GGL_FileSystem_Test/file2.cc", "wb");
  fclose(file);
  file = fopen("/tmp/GGL_FileSystem_Test/file3.cc", "wb");
  fclose(file);
  file = fopen("/tmp/GGL_FileSystem_Test3", "wb");
  fclose(file);

  EXPECT_FALSE(filesystem.MoveFolder("/tmp/GGL_FileSystem_Test/",
                                     "/tmp/GGL_FileSystem_Test/"));

  // Moves a folder into its sub-folder.
  EXPECT_FALSE(filesystem.MoveFolder("/tmp/GGL_FileSystem_Test/",
                                     "/tmp/GGL_FileSystem_Test/subfolder"));
  // Moves a folder into another folder.
  EXPECT_TRUE(filesystem.MoveFolder("/tmp/GGL_FileSystem_Test/",
                                     "/tmp/GGL_FileSystem_Test2"));
  EXPECT_FALSE(filesystem.FolderExists("/tmp/GGL_FileSystem_Test"));
  EXPECT_TRUE(filesystem.FolderExists(
      "/tmp/GGL_FileSystem_Test2/GGL_FileSystem_Test"));
  EXPECT_TRUE(filesystem.FileExists(
      "/tmp/GGL_FileSystem_Test2/GGL_FileSystem_Test/file1.cc"));
  EXPECT_TRUE(filesystem.FileExists(
      "/tmp/GGL_FileSystem_Test2/GGL_FileSystem_Test/file2.cc"));
  EXPECT_TRUE(filesystem.FileExists(
      "/tmp/GGL_FileSystem_Test2/GGL_FileSystem_Test/file3.cc"));

  // Moves a folder into another folder and rename.
  EXPECT_TRUE(filesystem.MoveFolder(
      "/tmp/GGL_FileSystem_Test2/GGL_FileSystem_Test",
      "/tmp/GGL_FileSystem_Test4"));
  EXPECT_FALSE(filesystem.FolderExists(
      "/tmp/GGL_FileSystem_Test2/GGL_FileSystem_Test"));
  EXPECT_TRUE(filesystem.FolderExists("/tmp/GGL_FileSystem_Test4"));

  // Moves a folder into another folder and rename.
  EXPECT_FALSE(filesystem.MoveFolder(
      "/tmp/GGL_FileSystem_Test4",
      "/tmp/GGL_FileSystem_Test3"));
  EXPECT_TRUE(filesystem.FolderExists("/tmp/GGL_FileSystem_Test4"));
  EXPECT_TRUE(filesystem.FileExists("/tmp/GGL_FileSystem_Test3"));

  filesystem.DeleteFolder("/tmp/GGL_FileSystem_Test4", true);
  filesystem.DeleteFolder("/tmp/GGL_FileSystem_Test2", true);
  unlink("/tmp/GGL_FileSystem_Test3");

  EXPECT_FALSE(filesystem.MoveFolder("", ""));
  EXPECT_FALSE(filesystem.MoveFolder(NULL, NULL));
}

// test method CopyFile
TEST(FileSystem, CopyFile) {
  FileSystem filesystem;
  mkdir("/tmp/GGL_FileSystem_Test", 0700);
  mkdir("/tmp/GGL_FileSystem_Test2", 0700);
  FILE *file = fopen("/tmp/GGL_FileSystem_Test/file1.cc", "wb");
  fputs("test", file);
  fclose(file);
  file = fopen("/tmp/GGL_FileSystem_Test/file2.cc", "wb");
  fclose(file);
  file = fopen("/tmp/GGL_FileSystem_Test/file3.cc", "wb");
  fclose(file);

  EXPECT_FALSE(filesystem.CopyFile("/tmp/GGL_FileSystem_Test/file1.cc",
                                   "/tmp/GGL_FileSystem_Test/file1.cc",
                                   false));
  EXPECT_FALSE(filesystem.CopyFile("/tmp/GGL_FileSystem_Test/file1.cc",
                                   "/tmp/GGL_FileSystem_Test/file1.cc",
                                   true));

  // Copies an existing file to a non-existing file.
  EXPECT_TRUE(filesystem.CopyFile("/tmp/GGL_FileSystem_Test/file1.cc",
                                  "/tmp/GGL_FileSystem_Test/file4.cc",
                                  false));
  file = fopen("/tmp/GGL_FileSystem_Test/file4.cc", "rb");
  char buffer[32];
  EXPECT_EQ(4U, fread(buffer, 1, 32, file));
  fclose(file);
  EXPECT_EQ("test", std::string(buffer, 4));
  EXPECT_TRUE(filesystem.FileExists("/tmp/GGL_FileSystem_Test/file1.cc"));
  EXPECT_TRUE(filesystem.FileExists("/tmp/GGL_FileSystem_Test/file4.cc"));

  // Copies an existing file to an existing file.
  EXPECT_FALSE(filesystem.CopyFile("/tmp/GGL_FileSystem_Test/file2.cc",
                                   "/tmp/GGL_FileSystem_Test/file3.cc",
                                   false));
  EXPECT_TRUE(filesystem.FileExists("/tmp/GGL_FileSystem_Test/file2.cc"));
  EXPECT_TRUE(filesystem.FileExists("/tmp/GGL_FileSystem_Test/file3.cc"));
  EXPECT_TRUE(filesystem.CopyFile("/tmp/GGL_FileSystem_Test/file2.cc",
                                  "/tmp/GGL_FileSystem_Test/file3.cc",
                                  true));

  // Copies an existing file to an existing folder.
  EXPECT_TRUE(filesystem.CopyFile("/tmp/GGL_FileSystem_Test/file*.cc",
                                  "/tmp/GGL_FileSystem_Test2/",
                                  false));
  EXPECT_TRUE(filesystem.FileExists("/tmp/GGL_FileSystem_Test/file2.cc"));
  EXPECT_TRUE(filesystem.FileExists("/tmp/GGL_FileSystem_Test/file3.cc"));
  EXPECT_TRUE(filesystem.FileExists("/tmp/GGL_FileSystem_Test/file4.cc"));
  EXPECT_TRUE(filesystem.FileExists("/tmp/GGL_FileSystem_Test2/file2.cc"));
  EXPECT_TRUE(filesystem.FileExists("/tmp/GGL_FileSystem_Test2/file3.cc"));
  EXPECT_TRUE(filesystem.FileExists("/tmp/GGL_FileSystem_Test2/file4.cc"));

  EXPECT_FALSE(filesystem.CopyFile("/tmp/GGL_FileSystem_Test/file*.cc",
                                   "/tmp/GGL_FileSystem_Test2/",
                                   false));
  EXPECT_TRUE(filesystem.CopyFile("/tmp/GGL_FileSystem_Test/file*.cc",
                                  "/tmp/GGL_FileSystem_Test2/",
                                  true));

  filesystem.DeleteFolder("/tmp/GGL_FileSystem_Test", true);
  filesystem.DeleteFolder("/tmp/GGL_FileSystem_Test2", true);

  EXPECT_FALSE(filesystem.CopyFile("", "", false));
  EXPECT_FALSE(filesystem.CopyFile(NULL, NULL, false));
}

// test method CopyFolder.
TEST(FileSystem, CopyFolder) {
  FileSystem filesystem;
  mkdir("/tmp/GGL_FileSystem_Test", 0700);
  mkdir("/tmp/GGL_FileSystem_Test2", 0700);
  FILE *file = fopen("/tmp/GGL_FileSystem_Test/file1.cc", "wb");
  fclose(file);
  file = fopen("/tmp/GGL_FileSystem_Test/file2.cc", "wb");
  fclose(file);
  file = fopen("/tmp/GGL_FileSystem_Test/file3.cc", "wb");
  fclose(file);
  file = fopen("/tmp/GGL_FileSystem_Test3", "wb");
  fclose(file);

  EXPECT_FALSE(filesystem.CopyFolder("/tmp/GGL_FileSystem_Test/",
                                     "/tmp/GGL_FileSystem_Test/",
                                     false));
  EXPECT_FALSE(filesystem.CopyFolder("/tmp/GGL_FileSystem_Test/",
                                     "/tmp/GGL_FileSystem_Test/",
                                     true));

  // Copies a folder into its sub-folder.
  EXPECT_FALSE(filesystem.CopyFolder("/tmp/GGL_FileSystem_Test/",
                                     "/tmp/GGL_FileSystem_Test/subfolder",
                                     false));
  EXPECT_FALSE(filesystem.CopyFolder("/tmp/GGL_FileSystem_Test/",
                                     "/tmp/GGL_FileSystem_Test/subfolder",
                                     true));
  // Copies a folder into another folder.
  EXPECT_TRUE(filesystem.CopyFolder("/tmp/GGL_FileSystem_Test/",
                                     "/tmp/GGL_FileSystem_Test2",
                                     false));
  EXPECT_TRUE(filesystem.FolderExists("/tmp/GGL_FileSystem_Test"));
  EXPECT_TRUE(filesystem.FolderExists(
      "/tmp/GGL_FileSystem_Test2/GGL_FileSystem_Test"));
  EXPECT_TRUE(filesystem.FileExists(
      "/tmp/GGL_FileSystem_Test2/GGL_FileSystem_Test/file1.cc"));
  EXPECT_TRUE(filesystem.FileExists(
      "/tmp/GGL_FileSystem_Test2/GGL_FileSystem_Test/file2.cc"));
  EXPECT_TRUE(filesystem.FileExists(
      "/tmp/GGL_FileSystem_Test2/GGL_FileSystem_Test/file3.cc"));

  // Copies a folder into another folder and rename.
  EXPECT_TRUE(filesystem.CopyFolder(
      "/tmp/GGL_FileSystem_Test2/GGL_FileSystem_Test",
      "/tmp/GGL_FileSystem_Test4",
      false));
  EXPECT_TRUE(filesystem.FolderExists(
      "/tmp/GGL_FileSystem_Test2/GGL_FileSystem_Test"));
  EXPECT_TRUE(filesystem.FolderExists("/tmp/GGL_FileSystem_Test4"));

  EXPECT_FALSE(filesystem.CopyFolder(
      "/tmp/GGL_FileSystem_Test2/GGL_FileSystem_Test",
      "/tmp/",
      false));
  EXPECT_TRUE(filesystem.CopyFolder(
      "/tmp/GGL_FileSystem_Test2/GGL_FileSystem_Test",
      "/tmp/",
      true));

  // Copies a folder into another folder and rename.
  EXPECT_FALSE(filesystem.CopyFolder(
      "/tmp/GGL_FileSystem_Test4",
      "/tmp/GGL_FileSystem_Test3",
      false));
  EXPECT_TRUE(filesystem.FolderExists("/tmp/GGL_FileSystem_Test4"));
  EXPECT_TRUE(filesystem.FileExists("/tmp/GGL_FileSystem_Test3"));

  filesystem.DeleteFolder("/tmp/GGL_FileSystem_Test4", true);
  filesystem.DeleteFolder("/tmp/GGL_FileSystem_Test2", true);
  filesystem.DeleteFolder("/tmp/GGL_FileSystem_Test", true);
  unlink("/tmp/GGL_FileSystem_Test3");

  EXPECT_FALSE(filesystem.CopyFolder("", "", false));
  EXPECT_FALSE(filesystem.CopyFolder(NULL, NULL, false));
  EXPECT_FALSE(filesystem.CopyFolder("", "", true));
  EXPECT_FALSE(filesystem.CopyFolder(NULL, NULL, true));
}

// test method CreateFolder.
TEST(FileSystem, CreateFolder) {
  FileSystem filesystem;
  mkdir("/tmp/GGL_FileSystem_Test", 0700);
  FILE *file = fopen("/tmp/GGL_FileSystem_Test/file.cc", "wb");
  fclose(file);

  EXPECT_FALSE(filesystem.CreateFolder("/tmp/GGL_FileSystem_Test/file.cc"));
  EXPECT_TRUE(filesystem.CreateFolder("/tmp/GGL_FileSystem_Test/folder"));
  EXPECT_FALSE(filesystem.CreateFolder(""));
  EXPECT_FALSE(filesystem.CreateFolder(NULL));
  EXPECT_TRUE(filesystem.FolderExists("/tmp/GGL_FileSystem_Test/folder"));

  filesystem.DeleteFolder("/tmp/GGL_FileSystem_Test", true);
}

int main(int argc, char **argv) {
  testing::ParseGTestFlags(&argc, argv);

  return RUN_ALL_TESTS();
}
