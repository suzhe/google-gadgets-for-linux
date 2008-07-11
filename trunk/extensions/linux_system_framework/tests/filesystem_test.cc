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
#include <ggadget/common.h>
#include <ggadget/logger.h>
#include <unittest/gtest.h>
#include "../file_system.h"

using namespace ggadget;
using namespace ggadget::framework;
using namespace ggadget::framework::linux_system;

static const char *kTestDir = "/tmp/GGL_FileSystem_Test";

TEST(FileSystem, GetDrives) {
  FileSystem filesystem;
  // it will always return NULL
  EXPECT_TRUE(filesystem.GetDrives() == NULL);
}

TEST(FileSystem, BuildPath_Accuracy1) {
  FileSystem filesystem;
  char path[] = "/tmp/GGL_FileSystem_Test/";
  char name[] = "file.cc";
  EXPECT_EQ("/tmp/GGL_FileSystem_Test/file.cc",
            filesystem.BuildPath(path, name));
}

// tests whether it adds '/' between filename and path
TEST(FileSystem, BuildPath_Accuracy2) {
  FileSystem filesystem;
  const char *path = kTestDir;
  char name[] = "file.cc";
  EXPECT_EQ("/tmp/GGL_FileSystem_Test/file.cc",
            filesystem.BuildPath(path, name));
}

TEST(FileSystem, BuildPath_Accuracy3) {
  FileSystem filesystem;
  char path[] = "/"; // path is just a '/'
  char name[] = "file.cc";
  EXPECT_EQ("/file.cc", filesystem.BuildPath(path, name));
}

// test with '\' in path
TEST(FileSystem, BuildPath_Accuracy4) {
  FileSystem filesystem;
  char path[] = "\\tmp\\GGL_FileSystem_Test\\";
  char name[] = "file";
  EXPECT_EQ("/tmp/GGL_FileSystem_Test/file", filesystem.BuildPath(path, name));
}

// test with '\' in path
TEST(FileSystem, BuildPath_Accuracy5) {
  FileSystem filesystem;
  char path[] = "\\tmp\\GGL_FileSystem_Test";
  char name[] = "file";
  EXPECT_EQ("/tmp/GGL_FileSystem_Test/file", filesystem.BuildPath(path, name));
}

// test with '\' in path
TEST(FileSystem, BuildPath_Accuracy6) {
  FileSystem filesystem;
  char path[] = "\\";
  char name[] = "file";
  EXPECT_EQ("/file", filesystem.BuildPath(path, name));
}

// test with NULL path
TEST(FileSystem, BuildPath_Failure1) {
  FileSystem filesystem;
  char name[] = "file.cc";
  EXPECT_EQ("", filesystem.BuildPath(NULL, name));
}

// test with NULL name
TEST(FileSystem, BuildPath_Failure2) {
  FileSystem filesystem;
  char path[] = "/";
  EXPECT_EQ("", filesystem.BuildPath(path, NULL));
}

// test with empty path
TEST(FileSystem, BuildPath_Failure3) {
  FileSystem filesystem;
  char path[] = "";
  char name[] = "file.cc";
  EXPECT_EQ("", filesystem.BuildPath(path, name));
}

// test with empty name
TEST(FileSystem, BuildPath_Failure4) {
  FileSystem filesystem;
  char path[] = "/";
  EXPECT_EQ("", filesystem.BuildPath(path, ""));
}

TEST(FileSystem, GetDriveName) {
  FileSystem filesystem;
  EXPECT_EQ("", filesystem.GetDriveName(NULL));
  EXPECT_EQ("", filesystem.GetDriveName(kTestDir));
}

// test method GetParentFolderName with valid arguments
TEST(FileSystem, GetParentFolderName_Accuracy1) {
  FileSystem filesystem;
  const char *path = kTestDir;
  EXPECT_EQ("/tmp", filesystem.GetParentFolderName(path));
}

TEST(FileSystem, GetParentFolderName_Accuracy2) {
  FileSystem filesystem;
  char path[] = "/tmp";
  EXPECT_EQ("/", filesystem.GetParentFolderName(path));
}

TEST(FileSystem, GetParentFolderName_Accuracy3) {
  FileSystem filesystem;
  char path[] = "/";
  EXPECT_EQ("", filesystem.GetParentFolderName(path));
}

// test method GetParentFolderName with '\' arguments
TEST(FileSystem, GetParentFolderName_Accuracy4) {
  FileSystem filesystem;
  char path[] = "\\tmp\\GGL_FileSystem_Test";
  EXPECT_EQ("/tmp", filesystem.GetParentFolderName(path));
}

TEST(FileSystem, GetParentFolderName_Accuracy5) {
  FileSystem filesystem;
  char path[] = "\\tmp";
  EXPECT_EQ("/", filesystem.GetParentFolderName(path));
}

TEST(FileSystem, GetParentFolderName_Accuracy6) {
  FileSystem filesystem;
  char path[] = "\\";
  EXPECT_EQ("", filesystem.GetParentFolderName(path));
}

// test with NULL argument
// empty string expected
TEST(FileSystem, GetParentFolderName_Failure1) {
  FileSystem filesystem;
  EXPECT_EQ("", filesystem.GetParentFolderName(NULL));
}

// test with empty string argument
// empty string expected
TEST(FileSystem, GetParentFolderName_Failure2) {
  FileSystem filesystem;
  EXPECT_EQ("", filesystem.GetParentFolderName(""));
}

// test method GetFileName
TEST(FileSystem, GetFileName_Accuracy1) {
  FileSystem filesystem;
  char path[] = "/tmp/GGL_FileSystem_Test/file.cc";
  EXPECT_EQ("file.cc", filesystem.GetFileName(path));
}

TEST(FileSystem, GetFileName_Accuracy2) {
  FileSystem filesystem;
  char path[] = "/tmp/GGL_FileSystem_Test/file";
  EXPECT_EQ("file", filesystem.GetFileName(path));
}

TEST(FileSystem, GetFileName_Accuracy3) {
  FileSystem filesystem;
  char path[] = "/";
  EXPECT_EQ("", filesystem.GetFileName(path));
}

TEST(FileSystem, GetFileName_Accuracy4) {
  FileSystem filesystem;
  char path[] = "\\tmp\\GGL_FileSystem_Test\\file.cc";
  EXPECT_EQ("file.cc", filesystem.GetFileName(path));
}

TEST(FileSystem, GetFileName_Accuracy5) {
  FileSystem filesystem;
  char path[] = "\\tmp\\GGL_FileSystem_Test\\file";
  EXPECT_EQ("file", filesystem.GetFileName(path));
}

TEST(FileSystem, GetFileName_Accuracy6) {
  FileSystem filesystem;
  char path[] = "\\";
  EXPECT_EQ("", filesystem.GetFileName(path));
}

// test with NULL argument
// empty string expected
TEST(FileSystem, GetFileName_Failure1) {
  FileSystem filesystem;
  EXPECT_EQ("", filesystem.GetFileName(NULL));
}

// test with empty string argument
// empty string expected
TEST(FileSystem, GetFileName_Failure2) {
  FileSystem filesystem;
  EXPECT_EQ("", filesystem.GetFileName(""));
}

// test method GetBaseName
TEST(FileSystem, GetBaseName_Accuracy1) {
  FileSystem filesystem;
  char path[] = "/tmp/GGL_FileSystem_Test/file.cc";
  EXPECT_EQ("file", filesystem.GetBaseName(path));
}

TEST(FileSystem, GetBaseName_Accuracy2) {
  FileSystem filesystem;
  char path[] = "/tmp/GGL_FileSystem_Test/file";
  EXPECT_EQ("file", filesystem.GetBaseName(path));
}

TEST(FileSystem, GetBaseName_Accuracy3) {
  FileSystem filesystem;
  char path[] = "/";
  EXPECT_EQ("", filesystem.GetBaseName(path));
}

TEST(FileSystem, GetBaseName_Accuracy4) {
  FileSystem filesystem;
  char path[] = "\\tmp\\GGL_FileSystem_Test\\file.cc";
  EXPECT_EQ("file", filesystem.GetBaseName(path));
}

TEST(FileSystem, GetBaseName_Accuracy5) {
  FileSystem filesystem;
  char path[] = "\\tmp\\GGL_FileSystem_Test\\file";
  EXPECT_EQ("file", filesystem.GetBaseName(path));
}

TEST(FileSystem, GetBaseName_Accuracy6) {
  FileSystem filesystem;
  char path[] = "\\";
  EXPECT_EQ("", filesystem.GetBaseName(path));
}

// test with NULL argument
// empty string expected
TEST(FileSystem, GetBaseName_Failure1) {
  FileSystem filesystem;
  EXPECT_EQ("", filesystem.GetBaseName(NULL));
}

// test with empty string argument
// empty string expected
TEST(FileSystem, GetBaseName_Failure2) {
  FileSystem filesystem;
  EXPECT_EQ("", filesystem.GetBaseName(""));
}

// test method GetExtensionName
TEST(FileSystem, GetExtensionName_Accuracy1) {
  FileSystem filesystem;
  char path[] = "/tmp/GGL_FileSystem_Test/file.cc";
  EXPECT_EQ("cc", filesystem.GetExtensionName(path));
}

TEST(FileSystem, GetExtensionName_Accuracy1a) {
  FileSystem filesystem;
  char path[] = "file.cc";
  EXPECT_EQ("cc", filesystem.GetExtensionName(path));
}

TEST(FileSystem, GetExtensionName_Accuracy2) {
  FileSystem filesystem;
  char path[] = "/tmp/GGL_FileSystem_Test/file";
  EXPECT_EQ("", filesystem.GetExtensionName(path));
}

TEST(FileSystem, GetExtensionName_Accuracy3) {
  FileSystem filesystem;
  char path[] = "/";
  EXPECT_EQ("", filesystem.GetExtensionName(path));
}

TEST(FileSystem, GetExtensionName_Accuracy4) {
  FileSystem filesystem;
  char path[] = "\\tmp\\GGL_FileSystem_Test\\file.cc";
  EXPECT_EQ("cc", filesystem.GetExtensionName(path));
}

TEST(FileSystem, GetExtensionName_Accuracy5) {
  FileSystem filesystem;
  char path[] = "\\tmp\\GGL_FileSystem_Test\\file";
  EXPECT_EQ("", filesystem.GetExtensionName(path));
}

TEST(FileSystem, GetExtensionName_Accuracy6) {
  FileSystem filesystem;
  char path[] = "\\";
  EXPECT_EQ("", filesystem.GetExtensionName(path));
}

// test with NULL argument
// empty string expected
TEST(FileSystem, GetExtensionName_Failure1) {
  FileSystem filesystem;
  EXPECT_EQ("", filesystem.GetExtensionName(NULL));
}

// test with empty string argument
// empty string expected
TEST(FileSystem, GetExtensionName_Failure2) {
  FileSystem filesystem;
  EXPECT_EQ("", filesystem.GetExtensionName(""));
}

// test method GetAbsolutePathName
TEST(FileSystem, GetAbsolutePathName_Accuracy1) {
  FileSystem filesystem;
  char path[] = "file.cc";

  // create the expected directory path
  char current_dir[PATH_MAX + 1];
  getcwd(current_dir, PATH_MAX);
  std::string dir(current_dir);
  if (dir[dir.size() - 1] == '/')
    dir = dir + std::string(path);
  else
    dir = dir + "/"+ std::string(path);

  EXPECT_EQ(dir, filesystem.GetAbsolutePathName(path));
}

TEST(FileSystem, GetAbsolutePathName_Accuracy2) {
  FileSystem filesystem;
  char path[] = "sub-folder/file.cc";

  // create the expected directory path
  char current_dir[PATH_MAX + 1];
  getcwd(current_dir, PATH_MAX);
  std::string dir(current_dir);
  if (dir[dir.size() - 1] == '/')
    dir = dir + std::string(path);
  else
    dir = dir + "/"+ std::string(path);

  EXPECT_EQ(dir, filesystem.GetAbsolutePathName(path));
}

TEST(FileSystem, GetAbsolutePathName_Accuracy3) {
  FileSystem filesystem;
  char path[] = "/tmp/GGL_FileSystem_Test/file";
  EXPECT_EQ(path, filesystem.GetAbsolutePathName(path));
}

TEST(FileSystem, GetAbsolutePathName_Accuracy4) {
  FileSystem filesystem;
  char path[] = "/";
  EXPECT_EQ("/", filesystem.GetAbsolutePathName(path));
}

TEST(FileSystem, GetAbsolutePathName_Accuracy5) {
  FileSystem filesystem;
  char path[] = "sub-folder\\file";

  // create the expected directory path
  char current_dir[PATH_MAX + 1];
  getcwd(current_dir, PATH_MAX);
  std::string dir(current_dir);
  if (dir[dir.size() - 1] == '/')
    dir = dir + "sub-folder/file";
  else
    dir = dir + "/sub-folder/file";

  EXPECT_EQ(dir, filesystem.GetAbsolutePathName(path));
}

TEST(FileSystem, GetAbsolutePathName_Accuracy7) {
  FileSystem filesystem;
  char path[] = "\\";
  EXPECT_EQ("/", filesystem.GetAbsolutePathName(path));
}

// test with NULL argument
// empty string expected
TEST(FileSystem, GetAbsolutePathName_Failure1) {
  FileSystem filesystem;
  EXPECT_EQ("", filesystem.GetAbsolutePathName(NULL));
}

// test with empty string argument
// empty string expected
TEST(FileSystem, GetAbsolutePathName_Failure2) {
  FileSystem filesystem;
  EXPECT_EQ("", filesystem.GetAbsolutePathName(""));
}

// test method GetTempName
TEST(FileSystem, GetTempName) {
  FileSystem filesystem;

  std::string temp = filesystem.GetTempName();

  EXPECT_GT(temp.size(), 0U);
  LOG("Temp file name: %s", temp.c_str());
}

// test method DriveExists
TEST(FileSystem, DriveExists) {
  FileSystem filesystem;
  EXPECT_FALSE(filesystem.DriveExists("/"));
  EXPECT_FALSE(filesystem.DriveExists(kTestDir));
  EXPECT_FALSE(filesystem.DriveExists("NULL"));
  EXPECT_FALSE(filesystem.DriveExists(""));
}

// test method FileExists
TEST(FileSystem, FileExists_Accuracy1) {
  FileSystem filesystem;
  system("rm -R /tmp/GGL_FileSystem_Test");
  system("mkdir /tmp/GGL_FileSystem_Test");
  system("touch /tmp/GGL_FileSystem_Test/file.cc");
  EXPECT_TRUE(filesystem.FileExists("/tmp/GGL_FileSystem_Test/file.cc"));
  EXPECT_TRUE(filesystem.FileExists("\\tmp\\GGL_FileSystem_Test\\file.cc"));
  EXPECT_FALSE(filesystem.FileExists("/tmp/GGL_FileSystem_Test/invalid.cc"));
  system("rm -R /tmp/GGL_FileSystem_Test");
}

// if the target is not a file, it should return false
TEST(FileSystem, FileExists_Accuracy2) {
  FileSystem filesystem;
  system("rm -R /tmp/GGL_FileSystem_Test");
  system("mkdir /tmp/GGL_FileSystem_Test");
  EXPECT_FALSE(filesystem.FileExists(kTestDir));
  system("rm -R /tmp/GGL_FileSystem_Test");
}

// tests with NULL argument
// empty string expected
TEST(FileSystem, FileExists_Failure1) {
  FileSystem filesystem;
  EXPECT_FALSE(filesystem.FileExists(NULL));
}

// tests with empty string argument
// empty string expected
TEST(FileSystem, FileExists_Failure2) {
  FileSystem filesystem;
  EXPECT_FALSE(filesystem.FileExists(""));
}

// test method FolderExists
TEST(FileSystem, FolderExists_Accuracy1) {
  FileSystem filesystem;
  system("mkdir /tmp/GGL_FileSystem_Test");
  EXPECT_TRUE(filesystem.FolderExists(kTestDir));
  EXPECT_FALSE(filesystem.FolderExists("/tmp/jfsj213132dlksf"));
  system("rm -R /tmp/GGL_FileSystem_Test");
}

// if the target is not a folder, it should return false
TEST(FileSystem, FolderExists_Accuracy2) {
  FileSystem filesystem;
  system("rm -R /tmp/GGL_FileSystem_Test");
  system("touch /tmp/GGL_FileSystem_Test");
  EXPECT_FALSE(filesystem.FolderExists(kTestDir));
  system("rm /tmp/GGL_FileSystem_Test");
}

// tests with NULL argument
// empty string expected
TEST(FileSystem, FolderExists_Failure1) {
  FileSystem filesystem;
  EXPECT_FALSE(filesystem.FolderExists(NULL));
}

// tests with empty string argument
// empty string expected
TEST(FileSystem, FolderExists_Failure2) {
  FileSystem filesystem;
  EXPECT_FALSE(filesystem.FolderExists(""));
}

// test method GetDrive
TEST(FileSystem, GetDrive) {
  FileSystem filesystem;
  EXPECT_TRUE(filesystem.GetDrive("/") == NULL);
  EXPECT_TRUE(filesystem.GetDrive(kTestDir) == NULL);
}

// test method GetFile with existing file
TEST(FileSystem, GetFile_Accuracy1) {
  FileSystem filesystem;
  EXPECT_TRUE(filesystem.GetFile("/") == NULL);

  system("touch /tmp/GGL_FileSystem_Test");
  EXPECT_TRUE(filesystem.GetFile(kTestDir) != NULL);
  EXPECT_TRUE(filesystem.GetFile("\\tmp\\GGL_FileSystem_Test") != NULL);
  system("rm /tmp/GGL_FileSystem_Test");
}

// test method GetFile with non-existing file
TEST(FileSystem, GetFile_Accuracy2) {
  FileSystem filesystem;

  system("rm /tmp/no_exist_file");
  EXPECT_TRUE(filesystem.GetFile("/tmp/no_exist_file") == NULL);
  EXPECT_TRUE(filesystem.GetFile("\\tmp\\no_exist_file") == NULL);
}

// test method GetFile with existing folder
TEST(FileSystem, GetFile_Accuracy3) {
  FileSystem filesystem;

  system("mkdir /tmp/GGL_FileSystem_Test");
  EXPECT_TRUE(filesystem.GetFile(kTestDir) == NULL);
  EXPECT_TRUE(filesystem.GetFile("\\tmp\\GGL_FileSystem_Test") == NULL);
  system("rm -r /tmp/GGL_FileSystem_Test");
}

// test whether the FileInterface contains correct information
TEST(FileSystem, GetFile_Accuracy4) {
  FileSystem filesystem;

  system("touch /tmp/GGL_FileSystem_Test");
  EXPECT_TRUE(filesystem.GetFile(kTestDir) != NULL);

  FileInterface *file = filesystem.GetFile(kTestDir);
  EXPECT_TRUE(file->GetPath() == kTestDir);

  system("rm /tmp/GGL_FileSystem_Test");
}

// tests with NULL argument
// false expected
TEST(FileSystem, GetFile_Failure1) {
  FileSystem filesystem;
  EXPECT_TRUE(filesystem.GetFile(NULL) == NULL);
}

// tests with empty string argument
// false expected
TEST(FileSystem, GetFile_Failure2) {
  FileSystem filesystem;
  EXPECT_TRUE(filesystem.GetFile("") == NULL);
}

// test method GetFolder with existing folder
TEST(FileSystem, GetFolder_Accuracy1) {
  FileSystem filesystem;
  EXPECT_TRUE(filesystem.GetFolder("/") != NULL);

  system("mkdir /tmp/GGL_FileSystem_Test");
  EXPECT_TRUE(filesystem.GetFolder(kTestDir) != NULL);
  EXPECT_TRUE(filesystem.GetFolder("\\tmp\\GGL_FileSystem_Test") != NULL);
  system("rm -r /tmp/GGL_FileSystem_Test");
}

// test method GetFolder with non-existing file
TEST(FileSystem, GetFolder_Accuracy2) {
  FileSystem filesystem;

  system("rm -r /tmp/no_exist_folder");
  EXPECT_TRUE(filesystem.GetFolder("/tmp/no_exist_folder") == NULL);
  EXPECT_TRUE(filesystem.GetFolder("\\tmp\\no_exist_folder") == NULL);
}

// test method GetFolder with existing file
TEST(FileSystem, GetFolder_Accuracy3) {
  FileSystem filesystem;

  system("touch /tmp/GGL_FileSystem_Test");
  EXPECT_TRUE(filesystem.GetFolder(kTestDir) == NULL);
  EXPECT_TRUE(filesystem.GetFolder("\\tmp\\GGL_FileSystem_Test") == NULL);
  system("rm /tmp/GGL_FileSystem_Test");
}

// test whether the FolderInterface contains correct information
TEST(FileSystem, GetFolder_Accuracy4) {
  FileSystem filesystem;

  system("mkdir /tmp/GGL_FileSystem_Test");
  EXPECT_TRUE(filesystem.GetFolder(kTestDir) != NULL);

  FolderInterface *folder = filesystem.GetFolder(kTestDir);
  EXPECT_TRUE(folder->GetPath() == kTestDir);

  system("rm -r /tmp/GGL_FileSystem_Test");
}

// tests with NULL argument
// false expected
TEST(FileSystem, GetFolder_Failure1) {
  FileSystem filesystem;
  EXPECT_TRUE(filesystem.GetFolder(NULL) == NULL);
}

// tests with empty string argument
// false expected
TEST(FileSystem, GetFolder_Failure2) {
  FileSystem filesystem;
  EXPECT_TRUE(filesystem.GetFolder("") == NULL);
}

// tests method GetSpecialFolder
TEST(FileSystem, GetSpecialFolder) {
  FileSystem filesystem;
  EXPECT_TRUE(filesystem.GetSpecialFolder(SPECIAL_FOLDER_WINDOWS) == NULL);
  EXPECT_TRUE(filesystem.GetSpecialFolder(SPECIAL_FOLDER_SYSTEM) == NULL);
  EXPECT_TRUE(filesystem.GetSpecialFolder(SPECIAL_FOLDER_TEMPORARY) == NULL);
}

// test method DeleteFile with existing file
TEST(FileSystem, DeleteFile_Accuracy_ExistingFile_1) {
  FileSystem filesystem;
  system("rm -R /tmp/GGL_FileSystem_Test");
  system("touch /tmp/GGL_FileSystem_Test");
  EXPECT_TRUE(filesystem.FileExists(kTestDir));

  EXPECT_TRUE(filesystem.DeleteFile(kTestDir, true));

  EXPECT_FALSE(filesystem.FileExists(kTestDir));
  system("rm /tmp/GGL_FileSystem_Test");
}

// test method DeleteFile with existing file
TEST(FileSystem, DeleteFile_Accuracy_ExistingFile_2) {
  FileSystem filesystem;
  system("rm -R /tmp/GGL_FileSystem_Test");
  system("touch /tmp/GGL_FileSystem_Test");
  EXPECT_TRUE(filesystem.FileExists(kTestDir));

  EXPECT_TRUE(filesystem.DeleteFile("\\tmp\\GGL_FileSystem_Test", true));

  EXPECT_FALSE(filesystem.FileExists(kTestDir));
  system("rm /tmp/GGL_FileSystem_Test");
}

// test method DeleteFile with non-existing file
TEST(FileSystem, DeleteFile_Accuracy_NonExistingFile_1) {
  FileSystem filesystem;
  system("rm -R /tmp/GGL_FileSystem_Test");
  system("rm /tmp/GGL_FileSystem_Test");
  EXPECT_FALSE(filesystem.FileExists(kTestDir));
  EXPECT_FALSE(filesystem.DeleteFile(kTestDir, true));
}

// test method DeleteFile with non-existing file
TEST(FileSystem, DeleteFile_Accuracy_NonExistingFile_2) {
  FileSystem filesystem;
  system("rm -R /tmp/GGL_FileSystem_Test");
  system("rm /tmp/GGL_FileSystem_Test");
  EXPECT_FALSE(filesystem.FileExists(kTestDir));
  EXPECT_FALSE(filesystem.DeleteFile("\\tmp\\GGL_FileSystem_Test", true));
}

// test method DeleteFile with existing folder
TEST(FileSystem, DeleteFile_Accuracy_ExistingFolder_1) {
  FileSystem filesystem;
  system("mkdir /tmp/GGL_FileSystem_Test");
  EXPECT_FALSE(filesystem.FileExists(kTestDir));
  EXPECT_FALSE(filesystem.DeleteFile(kTestDir, true));
  system("rm -r /tmp/GGL_FileSystem_Test");
}

// test method DeleteFile with existing folder
TEST(FileSystem, DeleteFile_Accuracy_ExistingFolder_2) {
  FileSystem filesystem;
  system("mkdir /tmp/GGL_FileSystem_Test");
  EXPECT_FALSE(filesystem.FileExists(kTestDir));
  EXPECT_FALSE(filesystem.DeleteFile("\\tmp\\GGL_FileSystem_Test", true));
  system("rm -r /tmp/GGL_FileSystem_Test");
}

// test with NULL argument
TEST(FileSystem, DeleteFile_Failure1) {
  FileSystem filesystem;
  EXPECT_FALSE(filesystem.DeleteFile(NULL, true));
  EXPECT_FALSE(filesystem.DeleteFile(NULL, false));
}

// test with empty string argument
TEST(FileSystem, DeleteFile_Failure2) {
  FileSystem filesystem;
  EXPECT_FALSE(filesystem.DeleteFile("", true));
  EXPECT_FALSE(filesystem.DeleteFile("", false));
}

// test method DeleteFolder with existing folder
TEST(FileSystem, DeleteFolder_Accuracy_ExistingFolder_1) {
  FileSystem filesystem;
  system("rm /tmp/GGL_FileSystem_Test");
  system("mkdir /tmp/GGL_FileSystem_Test");
  EXPECT_TRUE(filesystem.FolderExists(kTestDir));

  EXPECT_TRUE(filesystem.DeleteFolder(kTestDir, true));

  EXPECT_FALSE(filesystem.FolderExists(kTestDir));
  system("rm /tmp/GGL_FileSystem_Test");
}

// test method DeleteFolder with existing folder
TEST(FileSystem, DeleteFolder_Accuracy_ExistingFolder_2) {
  FileSystem filesystem;
  system("rm /tmp/GGL_FileSystem_Test");
  system("mkdir /tmp/GGL_FileSystem_Test");
  EXPECT_TRUE(filesystem.FolderExists(kTestDir));

  EXPECT_TRUE(filesystem.DeleteFolder("\\tmp\\GGL_FileSystem_Test", true));

  EXPECT_FALSE(filesystem.FolderExists(kTestDir));
  system("rm /tmp/GGL_FileSystem_Test");
}

// test method DeleteFolder with non-existing file
TEST(FileSystem, DeleteFolder_Accuracy_NonExistingFile_1) {
  FileSystem filesystem;
  system("rm -R /tmp/GGL_FileSystem_Test");
  system("rm /tmp/GGL_FileSystem_Test");
  EXPECT_FALSE(filesystem.FolderExists(kTestDir));
  EXPECT_FALSE(filesystem.DeleteFolder(kTestDir, true));
}

// test method DeleteFolder with non-existing file
TEST(FileSystem, DeleteFolder_Accuracy_NonExistingFile_2) {
  FileSystem filesystem;
  system("rm -R /tmp/GGL_FileSystem_Test");
  system("rm /tmp/GGL_FileSystem_Test");
  EXPECT_FALSE(filesystem.FolderExists(kTestDir));
  EXPECT_FALSE(filesystem.DeleteFolder("\\tmp\\GGL_FileSystem_Test", true));
}

// test method DeleteFolder with existing file
TEST(FileSystem, DeleteFolder_Accuracy_ExistingFile_1) {
  FileSystem filesystem;
  system("rm -R /tmp/GGL_FileSystem_Test");
  system("touch /tmp/GGL_FileSystem_Test");
  EXPECT_TRUE(filesystem.FileExists(kTestDir));
  EXPECT_FALSE(filesystem.DeleteFolder(kTestDir, true));
  system("rm /tmp/GGL_FileSystem_Test");
}

// test method DeleteFolder with existing file
TEST(FileSystem, DeleteFolder_Accuracy_ExistingFile_2) {
  FileSystem filesystem;
  system("rm -R /tmp/GGL_FileSystem_Test");
  system("touch /tmp/GGL_FileSystem_Test");
  EXPECT_TRUE(filesystem.FileExists(kTestDir));
  EXPECT_FALSE(filesystem.DeleteFolder("\\tmp\\GGL_FileSystem_Test", true));
  system("rm /tmp/GGL_FileSystem_Test");
}

// test with NULL argument
TEST(FileSystem, DeleteFolder_Failure1) {
  FileSystem filesystem;
  EXPECT_FALSE(filesystem.DeleteFolder(NULL, true));
  EXPECT_FALSE(filesystem.DeleteFolder(NULL, false));
}

// test with empty string argument
TEST(FileSystem, DeleteFolder_Failure2) {
  FileSystem filesystem;
  EXPECT_FALSE(filesystem.DeleteFolder("", true));
  EXPECT_FALSE(filesystem.DeleteFolder("", false));
}


// test method MoveFile with existing file
TEST(FileSystem, MoveFile_Accuracy_ExistingFile_1) {
  FileSystem filesystem;
  system("rm -R /tmp/GGL_FileSystem_Test");
  system("mkdir /tmp/GGL_FileSystem_Test");
  system("touch /tmp/GGL_FileSystem_Test/file1");
  system("echo -n \"sample content\" > /tmp/GGL_FileSystem_Test/file1");
  system("mkdir /tmp/GGL_FileSystem_Test/subfolder");
  EXPECT_TRUE(filesystem.FileExists("/tmp/GGL_FileSystem_Test/file1"));
  EXPECT_TRUE(filesystem.FolderExists("/tmp/GGL_FileSystem_Test/subfolder"));

  bool result = filesystem.MoveFile("/tmp/GGL_FileSystem_Test/file1",
                                    "/tmp/GGL_FileSystem_Test/subfolder/");

  EXPECT_TRUE(result);
  EXPECT_FALSE(filesystem.FileExists("/tmp/GGL_FileSystem_Test/file1"));
  EXPECT_TRUE(
    filesystem.FileExists("/tmp/GGL_FileSystem_Test/subfolder/file1"));

  system("rm -R /tmp/GGL_FileSystem_Test");
}

// test method MoveFile with existing file
TEST(FileSystem, MoveFile_Accuracy_ExistingFile_2) {
  FileSystem filesystem;
  system("rm -R /tmp/GGL_FileSystem_Test");
  system("mkdir /tmp/GGL_FileSystem_Test");
  system("touch /tmp/GGL_FileSystem_Test/file1");
  system("echo -n \"sample content\" > /tmp/GGL_FileSystem_Test/file1");
  system("mkdir /tmp/GGL_FileSystem_Test/subfolder");
  EXPECT_TRUE(filesystem.FileExists("/tmp/GGL_FileSystem_Test/file1"));
  EXPECT_TRUE(filesystem.FolderExists("/tmp/GGL_FileSystem_Test/subfolder"));

  bool result = filesystem.MoveFile("\\tmp\\GGL_FileSystem_Test\\file1",
                                    "\\tmp\\GGL_FileSystem_Test\\subfolder");

  EXPECT_TRUE(result);
  EXPECT_FALSE(filesystem.FileExists("/tmp/GGL_FileSystem_Test/file1"));
  EXPECT_TRUE(
    filesystem.FileExists("/tmp/GGL_FileSystem_Test/subfolder/file1"));

  system("rm -R /tmp/GGL_FileSystem_Test");
}


// test method MoveFile with existing file
TEST(FileSystem, MoveFile_Accuracy_ExistingFile_3) {
  FileSystem filesystem;
  system("rm -R /tmp/GGL_FileSystem_Test");
  system("mkdir /tmp/GGL_FileSystem_Test");
  system("touch /tmp/GGL_FileSystem_Test/file1");
  system("echo -n \"sample content\" > /tmp/GGL_FileSystem_Test/file1");
  system("mkdir /tmp/GGL_FileSystem_Test/subfolder");
  EXPECT_TRUE(filesystem.FileExists("/tmp/GGL_FileSystem_Test/file1"));
  EXPECT_TRUE(filesystem.FolderExists("/tmp/GGL_FileSystem_Test/subfolder"));

  bool result = filesystem.MoveFile("/tmp/GGL_FileSystem_Test/file1",
                                    "/tmp/");

  EXPECT_TRUE(result);
  EXPECT_FALSE(filesystem.FileExists("/tmp/GGL_FileSystem_Test/file1"));
  EXPECT_TRUE(
    filesystem.FileExists("/tmp/file1"));

  system("rm -R /tmp/file1");
  system("rm -R /tmp/GGL_FileSystem_Test");
}

// test method MoveFile with existing file
TEST(FileSystem, MoveFile_Accuracy_ExistingFile_4) {
  FileSystem filesystem;
  system("rm -R /tmp/GGL_FileSystem_Test");
  system("mkdir /tmp/GGL_FileSystem_Test");
  system("touch /tmp/GGL_FileSystem_Test/file1");
  system("echo -n \"sample content\" > /tmp/GGL_FileSystem_Test/file1");
  system("mkdir /tmp/GGL_FileSystem_Test/subfolder");
  EXPECT_TRUE(filesystem.FileExists("/tmp/GGL_FileSystem_Test/file1"));
  EXPECT_TRUE(filesystem.FolderExists("/tmp/GGL_FileSystem_Test/subfolder"));

  bool result = filesystem.MoveFile("\\tmp\\GGL_FileSystem_Test\\file1",
                                    "\\tmp");

  EXPECT_TRUE(result);
  EXPECT_FALSE(filesystem.FileExists("/tmp/GGL_FileSystem_Test/file1"));
  EXPECT_TRUE(
    filesystem.FileExists("/tmp/file1"));

  system("rm -R /tmp/file1");
  system("rm -R /tmp/GGL_FileSystem_Test");
}

// test method MoveFile with non-existing file
TEST(FileSystem, MoveFile_Accuracy_NonExistingFile_1) {
  FileSystem filesystem;
  system("rm -R /tmp/GGL_FileSystem_Test");
  system("mkdir /tmp/GGL_FileSystem_Test");
  system("mkdir /tmp/GGL_FileSystem_Test/subfolder");
  EXPECT_FALSE(filesystem.FileExists("/tmp/GGL_FileSystem_Test/file1"));
  EXPECT_TRUE(filesystem.FolderExists("/tmp/GGL_FileSystem_Test/subfolder"));

  bool result = filesystem.MoveFile("/tmp/GGL_FileSystem_Test/file1",
                                    "/tmp/");

  EXPECT_FALSE(result);

  system("rm -R /tmp/GGL_FileSystem_Test");
}

// test method MoveFile with non-existing file
TEST(FileSystem, MoveFile_Accuracy_NonExistingFile_2) {
  FileSystem filesystem;
  system("rm -R /tmp/GGL_FileSystem_Test");
  system("mkdir /tmp/GGL_FileSystem_Test");
  system("mkdir /tmp/GGL_FileSystem_Test/subfolder");
  EXPECT_FALSE(filesystem.FileExists("/tmp/GGL_FileSystem_Test/file1"));
  EXPECT_TRUE(filesystem.FolderExists("/tmp/GGL_FileSystem_Test/subfolder"));

  bool result = filesystem.MoveFile("\\tmp\\GGL_FileSystem_Test\\file1",
                                    "\\tmp");

  EXPECT_FALSE(result);

  system("rm -R /tmp/GGL_FileSystem_Test");
}

// test method MoveFile with non-existing folder
TEST(FileSystem, MoveFile_Accuracy_NonExistingFolder_1) {
  FileSystem filesystem;
  system("rm -R /tmp/GGL_FileSystem_Test");
  system("mkdir /tmp/GGL_FileSystem_Test");
  system("mkdir /tmp/GGL_FileSystem_Test/subfolder");
  EXPECT_TRUE(filesystem.FolderExists("/tmp/GGL_FileSystem_Test/subfolder"));

  bool result = filesystem.MoveFile("/tmp/GGL_FileSystem_Test/subfolder",
                                    "/tmp/GGL_FileSystem_Test");

  EXPECT_FALSE(result);

  system("rm -R /tmp/GGL_FileSystem_Test");
}

// test method MoveFile with non-existing folder
TEST(FileSystem, MoveFile_Accuracy_NonExistingFolder_2) {
  FileSystem filesystem;
  system("rm -R /tmp/GGL_FileSystem_Test");
  system("mkdir /tmp/GGL_FileSystem_Test");
  system("mkdir /tmp/GGL_FileSystem_Test/subfolder");
  EXPECT_TRUE(filesystem.FolderExists("/tmp/GGL_FileSystem_Test/subfolder"));

  bool result = filesystem.MoveFile("\\tmp/GGL_FileSystem_Test\\subfolder",
                                    "\\tmp/GGL_FileSystem_Test");

  EXPECT_FALSE(result);

  system("rm -R /tmp/GGL_FileSystem_Test");
}

// test with NULL argument
TEST(FileSystem, MoveFile_Failure1) {
  FileSystem filesystem;
  EXPECT_FALSE(filesystem.MoveFile(NULL, kTestDir));
  EXPECT_FALSE(filesystem.MoveFile(kTestDir, NULL));
}

// test with EMPTY string argument
TEST(FileSystem, MoveFile_Failure2) {
  FileSystem filesystem;
  EXPECT_FALSE(filesystem.MoveFile("", kTestDir));
  EXPECT_FALSE(filesystem.MoveFile(kTestDir, ""));
}


// test method MoveFolder with existing file
TEST(FileSystem, MoveFolder_Accuracy_ExistingFile_1) {
  FileSystem filesystem;
  system("rm -R /tmp/GGL_FileSystem_Test");
  system("mkdir /tmp/GGL_FileSystem_Test");
  system("touch /tmp/GGL_FileSystem_Test/file1");
  system("echo -n \"sample content\" > /tmp/GGL_FileSystem_Test/file1");
  system("mkdir /tmp/GGL_FileSystem_Test/subfolder");
  EXPECT_TRUE(filesystem.FileExists("/tmp/GGL_FileSystem_Test/file1"));
  EXPECT_TRUE(filesystem.FolderExists("/tmp/GGL_FileSystem_Test/subfolder"));

  bool result = filesystem.MoveFolder("/tmp/GGL_FileSystem_Test/file1",
                                    "/tmp/GGL_FileSystem_Test/subfolder/");

  EXPECT_FALSE(result);
  EXPECT_TRUE(filesystem.FileExists("/tmp/GGL_FileSystem_Test/file1"));

  system("rm -R /tmp/GGL_FileSystem_Test");
}

// test method MoveFolder with existing file
TEST(FileSystem, MoveFolder_Accuracy_ExistingFile_2) {
  FileSystem filesystem;
  system("rm -R /tmp/GGL_FileSystem_Test");
  system("mkdir /tmp/GGL_FileSystem_Test");
  system("touch /tmp/GGL_FileSystem_Test/file1");
  system("echo -n \"sample content\" > /tmp/GGL_FileSystem_Test/file1");
  system("mkdir /tmp/GGL_FileSystem_Test/subfolder");
  EXPECT_TRUE(filesystem.FileExists("/tmp/GGL_FileSystem_Test/file1"));
  EXPECT_TRUE(filesystem.FolderExists("/tmp/GGL_FileSystem_Test/subfolder"));

  bool result = filesystem.MoveFolder("\\tmp\\GGL_FileSystem_Test\\file1",
                                    "\\tmp\\GGL_FileSystem_Test\\subfolder");

  EXPECT_FALSE(result);
  EXPECT_TRUE(filesystem.FileExists("/tmp/GGL_FileSystem_Test/file1"));

  system("rm -R /tmp/GGL_FileSystem_Test");
}

// test method MoveFolder with existing file
TEST(FileSystem, MoveFolder_Accuracy_ExistingFile_3) {
  FileSystem filesystem;
  system("rm -R /tmp/GGL_FileSystem_Test");
  system("mkdir /tmp/GGL_FileSystem_Test");
  system("touch /tmp/GGL_FileSystem_Test/file1");
  system("echo -n \"sample content\" > /tmp/GGL_FileSystem_Test/file1");
  system("mkdir /tmp/GGL_FileSystem_Test/subfolder");
  EXPECT_TRUE(filesystem.FileExists("/tmp/GGL_FileSystem_Test/file1"));
  EXPECT_TRUE(filesystem.FolderExists("/tmp/GGL_FileSystem_Test/subfolder"));

  bool result = filesystem.MoveFolder("/tmp/GGL_FileSystem_Test/file1",
                                    "/tmp/");

  EXPECT_FALSE(result);
  EXPECT_TRUE(filesystem.FileExists("/tmp/GGL_FileSystem_Test/file1"));

  system("rm -R /tmp/file1");
  system("rm -R /tmp/GGL_FileSystem_Test");
}

// test method MoveFolder with existing folder
TEST(FileSystem, MoveFolder_Accuracy_ExistingFile_4) {
  FileSystem filesystem;
  system("rm -R /tmp/GGL_FileSystem_Test");
  system("mkdir /tmp/GGL_FileSystem_Test");
  system("touch /tmp/GGL_FileSystem_Test/file1");
  system("echo -n \"sample content\" > /tmp/GGL_FileSystem_Test/file1");
  system("mkdir /tmp/GGL_FileSystem_Test/subfolder");
  EXPECT_TRUE(filesystem.FileExists("/tmp/GGL_FileSystem_Test/file1"));
  EXPECT_TRUE(filesystem.FolderExists("/tmp/GGL_FileSystem_Test/subfolder"));

  bool result = filesystem.MoveFolder("\\tmp\\GGL_FileSystem_Test\\file1",
                                    "\\tmp");

  EXPECT_FALSE(result);
  EXPECT_TRUE(filesystem.FileExists("/tmp/GGL_FileSystem_Test/file1"));

  system("rm -R /tmp/file1");
  system("rm -R /tmp/GGL_FileSystem_Test");
}

// test method MoveFolder with existing folder
TEST(FileSystem, MoveFolder_Accuracy_ExistingFolder_1) {
  FileSystem filesystem;
  system("rm -R /tmp/GGL_FileSystem_Test");
  system("mkdir /tmp/GGL_FileSystem_Test");
  system("mkdir /tmp/GGL_FileSystem_Test/folder1");
  system("mkdir /tmp/GGL_FileSystem_Test/subfolder");
  EXPECT_TRUE(filesystem.FolderExists("/tmp/GGL_FileSystem_Test/folder1"));
  EXPECT_TRUE(filesystem.FolderExists("/tmp/GGL_FileSystem_Test/subfolder"));

  bool result = filesystem.MoveFolder("/tmp/GGL_FileSystem_Test/folder1",
                                    "/tmp/GGL_FileSystem_Test/subfolder");

  EXPECT_TRUE(result);
  EXPECT_FALSE(filesystem.FolderExists("/tmp/GGL_FileSystem_Test/folder1"));
  EXPECT_TRUE(filesystem.FolderExists(
                "/tmp/GGL_FileSystem_Test/subfolder/folder1"));

  system("rm -R /tmp/GGL_FileSystem_Test");
}

// test method MoveFolder with existing folder
TEST(FileSystem, MoveFolder_Accuracy_ExistingFolder_2) {
  FileSystem filesystem;
  system("rm -R /tmp/GGL_FileSystem_Test");
  system("mkdir /tmp/GGL_FileSystem_Test");
  system("mkdir /tmp/GGL_FileSystem_Test/folder1");
  system("mkdir /tmp/GGL_FileSystem_Test/subfolder");
  EXPECT_TRUE(filesystem.FolderExists("/tmp/GGL_FileSystem_Test/folder1"));
  EXPECT_TRUE(filesystem.FolderExists("/tmp/GGL_FileSystem_Test/subfolder"));

  bool result = filesystem.MoveFolder("\\tmp/GGL_FileSystem_Test\\folder1",
                                    "/tmp\\GGL_FileSystem_Test\\subfolder");

  EXPECT_TRUE(result);
  EXPECT_FALSE(filesystem.FolderExists("/tmp/GGL_FileSystem_Test/folder1"));
  EXPECT_TRUE(filesystem.FolderExists(
                "/tmp/GGL_FileSystem_Test/subfolder/folder1"));

  system("rm -R /tmp/GGL_FileSystem_Test");
}

// test method MoveFolder with non-existing folder
TEST(FileSystem, MoveFolder_Accuracy_NonExistingFolder_1) {
  FileSystem filesystem;
  system("rm -R /tmp/GGL_FileSystem_Test");
  system("mkdir /tmp/GGL_FileSystem_Test");
  EXPECT_FALSE(filesystem.FolderExists("/tmp/GGL_FileSystem_Test/subfolder"));

  bool result = filesystem.MoveFolder("/tmp/GGL_FileSystem_Test/subfolder",
                                    "/tmp/GGL_FileSystem_Test");

  EXPECT_FALSE(result);

  system("rm -R /tmp/GGL_FileSystem_Test");
}

// test method MoveFolder with non-existing folder
TEST(FileSystem, MoveFolder_Accuracy_NonExistingFolder_2) {
  FileSystem filesystem;
  system("rm -R /tmp/GGL_FileSystem_Test");
  system("mkdir /tmp/GGL_FileSystem_Test");
  EXPECT_FALSE(filesystem.FolderExists("/tmp/GGL_FileSystem_Test/subfolder"));

  bool result = filesystem.MoveFolder("/tmp\\GGL_FileSystem_Test\\subfolder",
                                    "\\tmp\\GGL_FileSystem_Test");

  EXPECT_FALSE(result);

  system("rm -R /tmp/GGL_FileSystem_Test");
}

// test with NULL argument
TEST(FileSystem, MoveFolder_Failure1) {
  FileSystem filesystem;
  EXPECT_FALSE(filesystem.MoveFolder(NULL, kTestDir));
  EXPECT_FALSE(filesystem.MoveFolder(kTestDir, NULL));
}

// test with EMPTY string argument
TEST(FileSystem, MoveFolder_Failure2) {
  FileSystem filesystem;
  EXPECT_FALSE(filesystem.MoveFolder("", kTestDir));
  EXPECT_FALSE(filesystem.MoveFolder(kTestDir, ""));
}


// test method CopyFile with existing file
TEST(FileSystem, CopyFile_Accuracy_ExistingFile_1) {
  FileSystem filesystem;
  system("rm -R /tmp/GGL_FileSystem_Test");
  system("mkdir /tmp/GGL_FileSystem_Test");
  system("touch /tmp/GGL_FileSystem_Test/file1");
  system("echo -n \"sample content\" > /tmp/GGL_FileSystem_Test/file1");
  system("mkdir /tmp/GGL_FileSystem_Test/subfolder");
  EXPECT_TRUE(filesystem.FileExists("/tmp/GGL_FileSystem_Test/file1"));
  EXPECT_TRUE(filesystem.FolderExists("/tmp/GGL_FileSystem_Test/subfolder"));

  bool result = filesystem.CopyFile("/tmp/GGL_FileSystem_Test/file1",
                                    "/tmp/GGL_FileSystem_Test/subfolder/",
                                    false);

  EXPECT_TRUE(result);
  EXPECT_TRUE(filesystem.FileExists("/tmp/GGL_FileSystem_Test/file1"));
  EXPECT_TRUE(
    filesystem.FileExists("/tmp/GGL_FileSystem_Test/subfolder/file1"));

  system("rm -R /tmp/GGL_FileSystem_Test");
}

// test method CopyFile with existing file
TEST(FileSystem, CopyFile_Accuracy_ExistingFile_2) {
  FileSystem filesystem;
  system("rm -R /tmp/GGL_FileSystem_Test");
  system("mkdir /tmp/GGL_FileSystem_Test");
  system("touch /tmp/GGL_FileSystem_Test/file1");
  system("echo -n \"sample content\" > /tmp/GGL_FileSystem_Test/file1");
  system("mkdir /tmp/GGL_FileSystem_Test/subfolder");
  EXPECT_TRUE(filesystem.FileExists("/tmp/GGL_FileSystem_Test/file1"));
  EXPECT_TRUE(filesystem.FolderExists("/tmp/GGL_FileSystem_Test/subfolder"));

  bool result = filesystem.CopyFile("\\tmp\\GGL_FileSystem_Test\\file1",
                                    "\\tmp\\GGL_FileSystem_Test\\subfolder",
                                    false);

  EXPECT_TRUE(result);
  EXPECT_TRUE(filesystem.FileExists("/tmp/GGL_FileSystem_Test/file1"));
  EXPECT_TRUE(
    filesystem.FileExists("/tmp/GGL_FileSystem_Test/subfolder/file1"));

  system("rm -R /tmp/GGL_FileSystem_Test");
}


// test method CopyFile with existing file
TEST(FileSystem, CopyFile_Accuracy_ExistingFile_3) {
  FileSystem filesystem;
  system("rm -R /tmp/GGL_FileSystem_Test");
  system("mkdir /tmp/GGL_FileSystem_Test");
  system("touch /tmp/GGL_FileSystem_Test/file1");
  system("echo -n \"sample content\" > /tmp/GGL_FileSystem_Test/file1");
  system("mkdir /tmp/GGL_FileSystem_Test/subfolder");
  EXPECT_TRUE(filesystem.FileExists("/tmp/GGL_FileSystem_Test/file1"));
  EXPECT_TRUE(filesystem.FolderExists("/tmp/GGL_FileSystem_Test/subfolder"));

  bool result = filesystem.CopyFile("/tmp/GGL_FileSystem_Test/file1",
                                    "/tmp/",
                                    false);

  EXPECT_TRUE(result);
  EXPECT_TRUE(filesystem.FileExists("/tmp/GGL_FileSystem_Test/file1"));
  EXPECT_TRUE(
    filesystem.FileExists("/tmp/file1"));

  system("rm -R /tmp/file1");
  system("rm -R /tmp/GGL_FileSystem_Test");
}

// test method CopyFile with existing file
TEST(FileSystem, CopyFile_Accuracy_ExistingFile_4) {
  FileSystem filesystem;
  system("rm -R /tmp/GGL_FileSystem_Test");
  system("mkdir /tmp/GGL_FileSystem_Test");
  system("touch /tmp/GGL_FileSystem_Test/file1");
  system("echo -n \"sample content\" > /tmp/GGL_FileSystem_Test/file1");
  system("mkdir /tmp/GGL_FileSystem_Test/subfolder");
  EXPECT_TRUE(filesystem.FileExists("/tmp/GGL_FileSystem_Test/file1"));
  EXPECT_TRUE(filesystem.FolderExists("/tmp/GGL_FileSystem_Test/subfolder"));

  bool result = filesystem.CopyFile("\\tmp\\GGL_FileSystem_Test\\file1",
                                    "\\tmp",
                                    false);

  EXPECT_TRUE(result);
  EXPECT_TRUE(filesystem.FileExists("/tmp/GGL_FileSystem_Test/file1"));
  EXPECT_TRUE(
    filesystem.FileExists("/tmp/file1"));

  system("rm -R /tmp/file1");
  system("rm -R /tmp/GGL_FileSystem_Test");
}

// test method CopyFile with overwritting file
TEST(FileSystem, CopyFile_Accuracy_ExistingFile_5) {
  FileSystem filesystem;
  system("rm -R /tmp/GGL_FileSystem_Test");
  system("mkdir /tmp/GGL_FileSystem_Test");
  system("touch /tmp/GGL_FileSystem_Test/file1");
  system("echo -n \"sample content\" > /tmp/GGL_FileSystem_Test/file1");
  system("mkdir /tmp/GGL_FileSystem_Test/subfolder");
  system("touch /tmp/GGL_FileSystem_Test/subfolder/file1");
  EXPECT_TRUE(filesystem.FileExists("/tmp/GGL_FileSystem_Test/file1"));
  EXPECT_TRUE(filesystem.FolderExists("/tmp/GGL_FileSystem_Test/subfolder"));

  bool result = filesystem.CopyFile("/tmp/GGL_FileSystem_Test/file1",
                                    "/tmp/GGL_FileSystem_Test/subfolder",
                                    false);

  EXPECT_FALSE(result);

  system("rm -R /tmp/GGL_FileSystem_Test");
}

// test method CopyFile with overwritting file
TEST(FileSystem, CopyFile_Accuracy_ExistingFile_6) {
  FileSystem filesystem;
  system("rm -R /tmp/GGL_FileSystem_Test");
  system("mkdir /tmp/GGL_FileSystem_Test");
  system("touch /tmp/GGL_FileSystem_Test/file1");
  system("echo -n \"sample content\" > /tmp/GGL_FileSystem_Test/file1");
  system("mkdir /tmp/GGL_FileSystem_Test/subfolder");
  system("touch /tmp/GGL_FileSystem_Test/subfolder/file1");
  EXPECT_TRUE(filesystem.FileExists("/tmp/GGL_FileSystem_Test/file1"));
  EXPECT_TRUE(filesystem.FolderExists("/tmp/GGL_FileSystem_Test/subfolder"));

  bool result = filesystem.CopyFile("/tmp/GGL_FileSystem_Test/file1",
                                    "/tmp/GGL_FileSystem_Test/subfolder",
                                    true);

  EXPECT_TRUE(result);

  TextStreamInterface *text =
    filesystem.CreateTextFile("/tmp/GGL_FileSystem_Test/subfolder/file1",
                              false,
                              false);

  EXPECT_TRUE(text != NULL);
  EXPECT_EQ("sample content", text->ReadAll());

  if (text) {
    text->Destroy();
    text = NULL;
  }

  system("rm -R /tmp/GGL_FileSystem_Test");
}

// test method CopyFile with non-existing file
TEST(FileSystem, CopyFile_Accuracy_NonExistingFile_1) {
  FileSystem filesystem;
  system("rm -R /tmp/GGL_FileSystem_Test");
  system("mkdir /tmp/GGL_FileSystem_Test");
  system("mkdir /tmp/GGL_FileSystem_Test/subfolder");
  EXPECT_FALSE(filesystem.FileExists("/tmp/GGL_FileSystem_Test/file1"));
  EXPECT_TRUE(filesystem.FolderExists("/tmp/GGL_FileSystem_Test/subfolder"));

  bool result = filesystem.CopyFile("/tmp/GGL_FileSystem_Test/file1",
                                    "/tmp/",
                                    false);

  EXPECT_FALSE(result);

  system("rm -R /tmp/GGL_FileSystem_Test");
}

// test method CopyFile with non-existing file
TEST(FileSystem, CopyFile_Accuracy_NonExistingFile_2) {
  FileSystem filesystem;
  system("rm -R /tmp/GGL_FileSystem_Test");
  system("mkdir /tmp/GGL_FileSystem_Test");
  system("mkdir /tmp/GGL_FileSystem_Test/subfolder");
  EXPECT_FALSE(filesystem.FileExists("/tmp/GGL_FileSystem_Test/file1"));
  EXPECT_TRUE(filesystem.FolderExists("/tmp/GGL_FileSystem_Test/subfolder"));

  bool result = filesystem.CopyFile("\\tmp\\GGL_FileSystem_Test\\file1",
                                    "\\tmp",
                                    false);

  EXPECT_FALSE(result);

  system("rm -R /tmp/GGL_FileSystem_Test");
}

// test method CopyFile with existing folder
TEST(FileSystem, CopyFile_Accuracy_ExistingFolder_1) {
  FileSystem filesystem;
  system("rm -R /tmp/GGL_FileSystem_Test");
  system("mkdir /tmp/GGL_FileSystem_Test");
  system("mkdir /tmp/GGL_FileSystem_Test/subfolder");
  EXPECT_TRUE(filesystem.FolderExists("/tmp/GGL_FileSystem_Test/subfolder"));

  bool result = filesystem.CopyFile("/tmp/GGL_FileSystem_Test/subfolder",
                                    "/tmp/GGL_FileSystem_Test",
                                    false);

  EXPECT_FALSE(result);

  system("rm -R /tmp/GGL_FileSystem_Test");
}

// test method CopyFile with existing folder
TEST(FileSystem, CopyFile_Accuracy_ExistingFolder_2) {
  FileSystem filesystem;
  system("rm -R /tmp/GGL_FileSystem_Test");
  system("mkdir /tmp/GGL_FileSystem_Test");
  system("mkdir /tmp/GGL_FileSystem_Test/subfolder");
  EXPECT_TRUE(filesystem.FolderExists("/tmp/GGL_FileSystem_Test/subfolder"));

  bool result = filesystem.CopyFile("\\tmp/GGL_FileSystem_Test\\subfolder",
                                    "\\tmp/GGL_FileSystem_Test",
                                    false);

  EXPECT_FALSE(result);

  system("rm -R /tmp/GGL_FileSystem_Test");
}

// test with NULL argument
TEST(FileSystem, CopyFile_Failure1) {
  FileSystem filesystem;
  EXPECT_FALSE(filesystem.CopyFile(NULL, kTestDir, false));
  EXPECT_FALSE(filesystem.CopyFile(kTestDir, NULL, false));
  EXPECT_FALSE(filesystem.CopyFile(NULL, kTestDir, true));
  EXPECT_FALSE(filesystem.CopyFile(kTestDir, NULL, true));
}

// test with EMPTY string argument
TEST(FileSystem, CopyFile_Failure2) {
  FileSystem filesystem;
  EXPECT_FALSE(filesystem.CopyFile("", kTestDir, false));
  EXPECT_FALSE(filesystem.CopyFile(kTestDir, "", false));
  EXPECT_FALSE(filesystem.CopyFile("", kTestDir, true));
  EXPECT_FALSE(filesystem.CopyFile(kTestDir, "", true));
}


// test method CopyFolder with existing file
TEST(FileSystem, CopyFolder_Accuracy_ExistingFile_1) {
  FileSystem filesystem;
  system("rm -R /tmp/GGL_FileSystem_Test");
  system("mkdir /tmp/GGL_FileSystem_Test");
  system("touch /tmp/GGL_FileSystem_Test/file1");
  system("echo -n \"sample content\" > /tmp/GGL_FileSystem_Test/file1");
  system("mkdir /tmp/GGL_FileSystem_Test/subfolder");
  EXPECT_TRUE(filesystem.FileExists("/tmp/GGL_FileSystem_Test/file1"));
  EXPECT_TRUE(filesystem.FolderExists("/tmp/GGL_FileSystem_Test/subfolder"));

  bool result = filesystem.CopyFolder("/tmp/GGL_FileSystem_Test/file1",
                                    "/tmp/GGL_FileSystem_Test/subfolder/",
                                    false);

  EXPECT_FALSE(result);
  EXPECT_TRUE(filesystem.FileExists("/tmp/GGL_FileSystem_Test/file1"));

  system("rm -R /tmp/GGL_FileSystem_Test");
}

// test method CopyFolder with existing file
TEST(FileSystem, CopyFolder_Accuracy_ExistingFile_2) {
  FileSystem filesystem;
  system("rm -R /tmp/GGL_FileSystem_Test");
  system("mkdir /tmp/GGL_FileSystem_Test");
  system("touch /tmp/GGL_FileSystem_Test/file1");
  system("echo -n \"sample content\" > /tmp/GGL_FileSystem_Test/file1");
  system("mkdir /tmp/GGL_FileSystem_Test/subfolder");
  EXPECT_TRUE(filesystem.FileExists("/tmp/GGL_FileSystem_Test/file1"));
  EXPECT_TRUE(filesystem.FolderExists("/tmp/GGL_FileSystem_Test/subfolder"));

  bool result = filesystem.CopyFolder("\\tmp\\GGL_FileSystem_Test\\file1",
                                    "\\tmp\\GGL_FileSystem_Test\\subfolder",
                                    false);

  EXPECT_FALSE(result);
  EXPECT_TRUE(filesystem.FileExists("/tmp/GGL_FileSystem_Test/file1"));

  system("rm -R /tmp/GGL_FileSystem_Test");
}

// test method CopyFolder with existing file
TEST(FileSystem, CopyFolder_Accuracy_ExistingFile_3) {
  FileSystem filesystem;
  system("rm -R /tmp/GGL_FileSystem_Test");
  system("mkdir /tmp/GGL_FileSystem_Test");
  system("touch /tmp/GGL_FileSystem_Test/file1");
  system("echo -n \"sample content\" > /tmp/GGL_FileSystem_Test/file1");
  system("mkdir /tmp/GGL_FileSystem_Test/subfolder");
  EXPECT_TRUE(filesystem.FileExists("/tmp/GGL_FileSystem_Test/file1"));
  EXPECT_TRUE(filesystem.FolderExists("/tmp/GGL_FileSystem_Test/subfolder"));

  bool result = filesystem.CopyFolder("/tmp/GGL_FileSystem_Test/file1",
                                      "/tmp/",
                                      false);

  EXPECT_FALSE(result);
  EXPECT_TRUE(filesystem.FileExists("/tmp/GGL_FileSystem_Test/file1"));

  system("rm -R /tmp/file1");
  system("rm -R /tmp/GGL_FileSystem_Test");
}

// test method CopyFolder with existing folder
TEST(FileSystem, CopyFolder_Accuracy_ExistingFile_4) {
  FileSystem filesystem;
  system("rm -R /tmp/GGL_FileSystem_Test");
  system("mkdir /tmp/GGL_FileSystem_Test");
  system("touch /tmp/GGL_FileSystem_Test/file1");
  system("echo -n \"sample content\" > /tmp/GGL_FileSystem_Test/file1");
  system("mkdir /tmp/GGL_FileSystem_Test/subfolder");
  EXPECT_TRUE(filesystem.FileExists("/tmp/GGL_FileSystem_Test/file1"));
  EXPECT_TRUE(filesystem.FolderExists("/tmp/GGL_FileSystem_Test/subfolder"));

  bool result = filesystem.CopyFolder("\\tmp\\GGL_FileSystem_Test\\file1",
                                      "\\tmp",
                                      false);

  EXPECT_FALSE(result);
  EXPECT_TRUE(filesystem.FileExists("/tmp/GGL_FileSystem_Test/file1"));

  system("rm -R /tmp/file1");
  system("rm -R /tmp/GGL_FileSystem_Test");
}

// test method CopyFolder with existing folder
TEST(FileSystem, CopyFolder_Accuracy_ExistingFolder_1) {
  FileSystem filesystem;
  system("rm -R /tmp/GGL_FileSystem_Test");
  system("mkdir /tmp/GGL_FileSystem_Test");
  system("mkdir /tmp/GGL_FileSystem_Test/folder1");
  system("mkdir /tmp/GGL_FileSystem_Test/subfolder");
  EXPECT_TRUE(filesystem.FolderExists("/tmp/GGL_FileSystem_Test/folder1"));
  EXPECT_TRUE(filesystem.FolderExists("/tmp/GGL_FileSystem_Test/subfolder"));

  bool result = filesystem.CopyFolder("/tmp/GGL_FileSystem_Test/folder1",
                                      "/tmp/GGL_FileSystem_Test/subfolder",
                                      false);

  EXPECT_TRUE(result);
  EXPECT_TRUE(filesystem.FolderExists("/tmp/GGL_FileSystem_Test/folder1"));
  EXPECT_TRUE(filesystem.FolderExists(
                "/tmp/GGL_FileSystem_Test/subfolder/folder1"));

  system("rm -R /tmp/GGL_FileSystem_Test");
}

// test method CopyFolder with existing folder
TEST(FileSystem, CopyFolder_Accuracy_ExistingFolder_2) {
  FileSystem filesystem;
  system("rm -R /tmp/GGL_FileSystem_Test");
  system("mkdir /tmp/GGL_FileSystem_Test");
  system("mkdir /tmp/GGL_FileSystem_Test/folder1");
  system("mkdir /tmp/GGL_FileSystem_Test/subfolder");
  EXPECT_TRUE(filesystem.FolderExists("/tmp/GGL_FileSystem_Test/folder1"));
  EXPECT_TRUE(filesystem.FolderExists("/tmp/GGL_FileSystem_Test/subfolder"));

  bool result = filesystem.CopyFolder("\\tmp/GGL_FileSystem_Test\\folder1",
                                      "/tmp\\GGL_FileSystem_Test\\subfolder",
                                      false);

  EXPECT_TRUE(result);
  EXPECT_TRUE(filesystem.FolderExists("/tmp/GGL_FileSystem_Test/folder1"));
  EXPECT_TRUE(filesystem.FolderExists(
                "/tmp/GGL_FileSystem_Test/subfolder/folder1"));

  system("rm -R /tmp/GGL_FileSystem_Test");
}

// test method CopyFolder with overwritting folder
TEST(FileSystem, CopyFolder_Accuracy_ExistingFolder_3) {
  FileSystem filesystem;
  system("rm -R /tmp/GGL_FileSystem_Test");
  system("mkdir /tmp/GGL_FileSystem_Test");
  system("mkdir /tmp/GGL_FileSystem_Test/folder1");
  system("mkdir /tmp/GGL_FileSystem_Test/subfolder");
  system("mkdir /tmp/GGL_FileSystem_Test/subfolder/folder1");
  EXPECT_TRUE(filesystem.FolderExists("/tmp/GGL_FileSystem_Test/subfolder"));

  bool result = filesystem.CopyFolder("/tmp/GGL_FileSystem_Test/folder1",
                                    "/tmp/GGL_FileSystem_Test/subfolder",
                                    false);

  EXPECT_FALSE(result);

  system("rm -R /tmp/GGL_FileSystem_Test");
}

// test method CopyFolder with overwritting folder
TEST(FileSystem, CopyFolder_Accuracy_ExistingFolder_4) {
  FileSystem filesystem;
  system("rm -R /tmp/GGL_FileSystem_Test");
  system("mkdir /tmp/GGL_FileSystem_Test");
  system("mkdir /tmp/GGL_FileSystem_Test/folder1");
  system("mkdir /tmp/GGL_FileSystem_Test/subfolder");
  system("mkdir /tmp/GGL_FileSystem_Test/subfolder/folder1");
  EXPECT_TRUE(filesystem.FolderExists("/tmp/GGL_FileSystem_Test/subfolder"));

  bool result = filesystem.CopyFolder("/tmp/GGL_FileSystem_Test/folder1",
                                    "/tmp/GGL_FileSystem_Test/subfolder",
                                    true);

  EXPECT_TRUE(result);

  system("rm -R /tmp/GGL_FileSystem_Test");
}

// test method CopyFolder with non-existing folder
TEST(FileSystem, CopyFolder_Accuracy_NonExistingFolder_1) {
  FileSystem filesystem;
  system("rm -R /tmp/GGL_FileSystem_Test");
  system("mkdir /tmp/GGL_FileSystem_Test");
  EXPECT_FALSE(filesystem.FolderExists("/tmp/GGL_FileSystem_Test/subfolder"));

  bool result = filesystem.CopyFolder("/tmp/GGL_FileSystem_Test/subfolder",
                                      "/tmp/GGL_FileSystem_Test",
                                      false);

  EXPECT_FALSE(result);

  system("rm -R /tmp/GGL_FileSystem_Test");
}

// test method CopyFolder with non-existing folder
TEST(FileSystem, CopyFolder_Accuracy_NonExistingFolder_2) {
  FileSystem filesystem;
  system("rm -R /tmp/GGL_FileSystem_Test");
  system("mkdir /tmp/GGL_FileSystem_Test");
  EXPECT_FALSE(filesystem.FolderExists("/tmp/GGL_FileSystem_Test/subfolder"));

  bool result = filesystem.CopyFolder("/tmp\\GGL_FileSystem_Test\\subfolder",
                                      "\\tmp\\GGL_FileSystem_Test",
                                      false);

  EXPECT_FALSE(result);

  system("rm -R /tmp/GGL_FileSystem_Test");
}

// test with NULL argument
TEST(FileSystem, CopyFolder_Failure1) {
  FileSystem filesystem;
  EXPECT_FALSE(filesystem.CopyFolder(NULL, kTestDir, false));
  EXPECT_FALSE(filesystem.CopyFolder(kTestDir, NULL, false));
  EXPECT_FALSE(filesystem.CopyFolder(NULL, kTestDir, true));
  EXPECT_FALSE(filesystem.CopyFolder(kTestDir, NULL, true));
}

// test with EMPTY string argument
TEST(FileSystem, CopyFolder_Failure2) {
  FileSystem filesystem;
  EXPECT_FALSE(filesystem.CopyFolder("", kTestDir, false));
  EXPECT_FALSE(filesystem.CopyFolder(kTestDir, "", false));
  EXPECT_FALSE(filesystem.CopyFolder("", kTestDir, true));
  EXPECT_FALSE(filesystem.CopyFolder(kTestDir, "", true));
}


// test method CreateFolder with non-existing folder
TEST(FileSystem, CreateFolder_Accuracy_NonExistingFolder_1) {
  FileSystem filesystem;
  system("rm -R /tmp/GGL_FileSystem_Test");
  EXPECT_FALSE(filesystem.FolderExists(kTestDir));
  FolderInterface *folder = filesystem.CreateFolder(kTestDir);
  EXPECT_TRUE(folder != NULL);
  EXPECT_EQ(kTestDir, folder->GetPath());
  EXPECT_TRUE(filesystem.FolderExists(kTestDir));
  system("rm -R /tmp/GGL_FileSystem_Test");
}

// test method CreateFolder with non-existing folder
TEST(FileSystem, CreateFolder_Accuracy_NonExistingFolder_2) {
  FileSystem filesystem;
  system("rm -R /tmp/GGL_FileSystem_Test");
  EXPECT_FALSE(filesystem.FolderExists(kTestDir));
  FolderInterface
      *folder = filesystem.CreateFolder("\\tmp\\GGL_FileSystem_Test");
  EXPECT_TRUE(folder != NULL);
  EXPECT_EQ(kTestDir, folder->GetPath());
  EXPECT_TRUE(filesystem.FolderExists(kTestDir));
  system("rm -R /tmp/GGL_FileSystem_Test");
}

// test method CreateFolder with existing folder
TEST(FileSystem, CreateFolder_ExistingFolder_1) {
  FileSystem filesystem;
  system("mkdir /tmp/GGL_FileSystem_Test");
  EXPECT_TRUE(filesystem.FolderExists(kTestDir));
  FolderInterface *folder = filesystem.CreateFolder(kTestDir);
  EXPECT_TRUE(folder != NULL);
  EXPECT_EQ(kTestDir, folder->GetPath());
  EXPECT_TRUE(filesystem.FolderExists(kTestDir));
  system("rm -R /tmp/GGL_FileSystem_Test");
}

// test method CreateFolder with existing folder
TEST(FileSystem, CreateFolder_ExistingFolder_2) {
  FileSystem filesystem;
  system("mkdir /tmp/GGL_FileSystem_Test");
  EXPECT_TRUE(filesystem.FolderExists(kTestDir));
  FolderInterface*folder =filesystem.CreateFolder("\\tmp\\GGL_FileSystem_Test");
  EXPECT_TRUE(folder != NULL);
  EXPECT_EQ(kTestDir, folder->GetPath());
  EXPECT_TRUE(filesystem.FolderExists(kTestDir));
  system("rm -R /tmp/GGL_FileSystem_Test");
}

// test method CreateFolder with existing file
TEST(FileSystem, CreateFolder_Accuracy_ExistingFile_1) {
  FileSystem filesystem;
  system("rm -R /tmp/GGL_FileSystem_Test");
  system("touch /tmp/GGL_FileSystem_Test");
  EXPECT_FALSE(filesystem.FolderExists(kTestDir));
  EXPECT_TRUE(filesystem.FileExists(kTestDir));
  FolderInterface *folder = filesystem.CreateFolder(kTestDir);
  EXPECT_TRUE(folder == NULL);
  EXPECT_FALSE(filesystem.FolderExists(kTestDir));
  EXPECT_TRUE(filesystem.FileExists(kTestDir));
  system("rm /tmp/GGL_FileSystem_Test");
}

// test method CreateFolder with existing file
TEST(FileSystem, CreateFolder_Accuracy_ExistingFile_2) {
  FileSystem filesystem;
  system("rm -R /tmp/GGL_FileSystem_Test");
  system("touch /tmp/GGL_FileSystem_Test");
  EXPECT_FALSE(filesystem.FolderExists(kTestDir));
  EXPECT_TRUE(filesystem.FileExists(kTestDir));
  FolderInterface
      *folder = filesystem.CreateFolder("\\tmp\\GGL_FileSystem_Test");
  EXPECT_TRUE(folder == NULL);
  EXPECT_FALSE(filesystem.FolderExists(kTestDir));
  EXPECT_TRUE(filesystem.FileExists(kTestDir));
  system("rm /tmp/GGL_FileSystem_Test");
}

// test with NULL argument
TEST(FileSystem, CreateFolder_Failure1) {
  FileSystem filesystem;
  EXPECT_FALSE(filesystem.CreateFolder(NULL));
}

// test with empty string argument
TEST(FileSystem, CreateFolder_Failure2) {
  FileSystem filesystem;
  EXPECT_FALSE(filesystem.CreateFolder(""));
}

// test method CreateTextFile with non-existing file
TEST(FileSystem, CreateTextFile_Accuracy_NonExistingFile_1) {
  FileSystem filesystem;
  system("rm -R /tmp/GGL_FileSystem_Test");
  EXPECT_FALSE(filesystem.FileExists(kTestDir));
  EXPECT_FALSE(filesystem.FolderExists(kTestDir));
  TextStreamInterface
      *text = filesystem.CreateTextFile(kTestDir, false,
                                        false);
  EXPECT_TRUE(text != NULL);
  EXPECT_EQ("", text->ReadAll());
  text->Close();
  text->Destroy();
  text = NULL;
  EXPECT_TRUE(filesystem.FileExists(kTestDir));
  EXPECT_FALSE(filesystem.FolderExists(kTestDir));
  system("rm -R /tmp/GGL_FileSystem_Test");
}

// test method CreateTextFile with non-existing file
TEST(FileSystem, CreateTextFile_Accuracy_NonExistingFile_2) {
  FileSystem filesystem;
  system("rm -R /tmp/GGL_FileSystem_Test");
  EXPECT_FALSE(filesystem.FileExists(kTestDir));
  EXPECT_FALSE(filesystem.FolderExists(kTestDir));
  TextStreamInterface
      *text = filesystem.CreateTextFile("\\tmp\\GGL_FileSystem_Test", false,
                                        false);
  EXPECT_TRUE(text != NULL);
  EXPECT_EQ("", text->ReadAll());
  text->Close();
  text->Destroy();
  text = NULL;
  EXPECT_TRUE(filesystem.FileExists(kTestDir));
  EXPECT_FALSE(filesystem.FolderExists(kTestDir));
  system("rm -R /tmp/GGL_FileSystem_Test");
}

// test method CreateTextFile with existing file
TEST(FileSystem, CreateTextFile_Accuracy_ExistingFile_1) {
  FileSystem filesystem;
  system("rm -R /tmp/GGL_FileSystem_Test");
  FILE *fp = fopen(kTestDir, "wb");
  if (!fp)
    return;
  fputs("Test for create text file!\n", fp);
  fclose(fp);
  EXPECT_TRUE(filesystem.FileExists(kTestDir));
  TextStreamInterface
      *text = filesystem.CreateTextFile(kTestDir, false,
                                        false);
  EXPECT_TRUE(text != NULL);
  EXPECT_EQ("Test for create text file!\n", text->ReadAll());
  EXPECT_TRUE(filesystem.FileExists(kTestDir));
  system("rm -R /tmp/GGL_FileSystem_Test");
}

// test method CreateTextFile with existing file
TEST(FileSystem, CreateTextFile_Accuracy_ExistingFile_2) {
  FileSystem filesystem;
  system("rm -R /tmp/GGL_FileSystem_Test");
  FILE *fp = fopen(kTestDir, "wb");
  if (!fp)
    return;
  fputs("Test for create text file!", fp);
  fclose(fp);
  EXPECT_TRUE(filesystem.FileExists(kTestDir));
  TextStreamInterface
      *text = filesystem.CreateTextFile("\\tmp\\GGL_FileSystem_Test", false,
                                        false);
  EXPECT_TRUE(text != NULL);
  EXPECT_EQ("Test for create text file!", text->ReadAll());
  EXPECT_TRUE(filesystem.FileExists(kTestDir));
  system("rm -R /tmp/GGL_FileSystem_Test");
}

// test method CreateTextFile with existing folder
TEST(FileSystem, CreateTextFile_Accuracy_ExistingFolder_1) {
  FileSystem filesystem;
  system("rm -R /tmp/GGL_FileSystem_Test");
  system("mkdir /tmp/GGL_FileSystem_Test");
  EXPECT_TRUE(filesystem.FolderExists(kTestDir));
  EXPECT_FALSE(filesystem.FileExists(kTestDir));
  TextStreamInterface
      *text = filesystem.CreateTextFile(kTestDir, false,
                                        false);
  EXPECT_TRUE(text == NULL);
  EXPECT_TRUE(filesystem.FolderExists(kTestDir));
  EXPECT_FALSE(filesystem.FileExists(kTestDir));
  system("rm -R /tmp/GGL_FileSystem_Test");
}

// test method CreateTextFile with existing folder
TEST(FileSystem, CreateTextFile_Accuracy_ExistingFolder_2) {
  FileSystem filesystem;
  system("rm -R /tmp/GGL_FileSystem_Test");
  system("mkdir /tmp/GGL_FileSystem_Test");
  EXPECT_TRUE(filesystem.FolderExists(kTestDir));
  EXPECT_FALSE(filesystem.FileExists(kTestDir));
  TextStreamInterface
      *text = filesystem.CreateTextFile("\\tmp\\GGL_FileSystem_Test", false,
                                        false);
  EXPECT_TRUE(text == NULL);
  EXPECT_TRUE(filesystem.FolderExists(kTestDir));
  EXPECT_FALSE(filesystem.FileExists(kTestDir));
  system("rm -R /tmp/GGL_FileSystem_Test");
}

// test with NULL argument
TEST(FileSystem, CreateTextFile_Failure1) {
  FileSystem filesystem;
  EXPECT_TRUE(filesystem.CreateTextFile(NULL, false, false) == NULL);
}

// test with empty string argument
TEST(FileSystem, CreateTextFile_Failure2) {
  FileSystem filesystem;
  EXPECT_TRUE(filesystem.CreateTextFile("", false, false) == NULL);
}

// test method OpenTextFile with non-existing file
TEST(FileSystem, OpenTextFile_Accuracy_NonExistingFile_1) {
  FileSystem filesystem;
  system("rm -R /tmp/GGL_FileSystem_Test");
  EXPECT_FALSE(filesystem.FileExists(kTestDir));
  EXPECT_FALSE(filesystem.FolderExists(kTestDir));
  TextStreamInterface
      *text = filesystem.OpenTextFile(kTestDir,
                                      IO_MODE_READING, true, TRISTATE_TRUE);
  EXPECT_TRUE(text != NULL);
  EXPECT_EQ("", text->ReadAll());
  text->Close();
  text->Destroy();
  text = NULL;
  EXPECT_TRUE(filesystem.FileExists(kTestDir));
  EXPECT_FALSE(filesystem.FolderExists(kTestDir));
  system("rm -R /tmp/GGL_FileSystem_Test");
}

// test method OpenTextFile with non-existing file
TEST(FileSystem, OpenTextFile_Accuracy_NonExistingFile_2) {
  FileSystem filesystem;
  system("rm -R /tmp/GGL_FileSystem_Test");
  EXPECT_FALSE(filesystem.FileExists(kTestDir));
  EXPECT_FALSE(filesystem.FolderExists(kTestDir));
  TextStreamInterface
      *text = filesystem.OpenTextFile("\\tmp\\GGL_FileSystem_Test",
                                      IO_MODE_READING, true, TRISTATE_TRUE);
  EXPECT_TRUE(text != NULL);
  EXPECT_EQ("", text->ReadAll());
  text->Close();
  text->Destroy();
  text = NULL;
  EXPECT_TRUE(filesystem.FileExists(kTestDir));
  EXPECT_FALSE(filesystem.FolderExists(kTestDir));
  system("rm -R /tmp/GGL_FileSystem_Test");
}

// test method OpenTextFile with non-existing file with create == false
TEST(FileSystem, OpenTextFile_Accuracy_NonExistingFile_3) {
  FileSystem filesystem;
  system("rm -R /tmp/GGL_FileSystem_Test");
  EXPECT_FALSE(filesystem.FileExists(kTestDir));
  EXPECT_FALSE(filesystem.FolderExists(kTestDir));
  TextStreamInterface
      *text = filesystem.OpenTextFile(kTestDir,
                                      IO_MODE_READING, false, TRISTATE_TRUE);
  EXPECT_TRUE(text == NULL);
  EXPECT_FALSE(filesystem.FileExists(kTestDir));
  EXPECT_FALSE(filesystem.FolderExists(kTestDir));
  system("rm -R /tmp/GGL_FileSystem_Test");
}

// test method OpenTextFile with non-existing file with create == false
TEST(FileSystem, OpenTextFile_Accuracy_NonExistingFile_4) {
  FileSystem filesystem;
  system("rm -R /tmp/GGL_FileSystem_Test");
  EXPECT_FALSE(filesystem.FileExists(kTestDir));
  EXPECT_FALSE(filesystem.FolderExists(kTestDir));
  TextStreamInterface
      *text = filesystem.OpenTextFile("\\tmp\\GGL_FileSystem_Test",
                                      IO_MODE_READING, false, TRISTATE_TRUE);
  EXPECT_TRUE(text == NULL);
  EXPECT_FALSE(filesystem.FileExists(kTestDir));
  EXPECT_FALSE(filesystem.FolderExists(kTestDir));
  system("rm -R /tmp/GGL_FileSystem_Test");
}

// test method OpenTextFile with existing file
TEST(FileSystem, OpenTextFile_Accuracy_ExistingFile_1) {
  FileSystem filesystem;
  system("rm -R /tmp/GGL_FileSystem_Test");
  FILE *fp = fopen(kTestDir, "wb");
  if (!fp)
    return;
  fputs("Test for create text file!\n", fp);
  fclose(fp);
  EXPECT_TRUE(filesystem.FileExists(kTestDir));
  TextStreamInterface
      *text = filesystem.OpenTextFile(kTestDir,
                                      IO_MODE_READING, false, TRISTATE_TRUE);
  EXPECT_TRUE(text != NULL);
  EXPECT_EQ("Test for create text file!\n", text->ReadAll());
  EXPECT_TRUE(filesystem.FileExists(kTestDir));
  system("rm -R /tmp/GGL_FileSystem_Test");
}

// test method OpenTextFile with existing file
TEST(FileSystem, OpenTextFile_Accuracy_ExistingFile_2) {
  FileSystem filesystem;
  system("rm -R /tmp/GGL_FileSystem_Test");
  FILE *fp = fopen(kTestDir, "wb");
  if (!fp)
    return;
  fputs("Test for create text file!", fp);
  fclose(fp);
  EXPECT_TRUE(filesystem.FileExists(kTestDir));
  TextStreamInterface
      *text = filesystem.OpenTextFile("\\tmp\\GGL_FileSystem_Test",
                                      IO_MODE_READING, false, TRISTATE_TRUE);
  EXPECT_TRUE(text != NULL);
  EXPECT_EQ("Test for create text file!", text->ReadAll());
  EXPECT_TRUE(filesystem.FileExists(kTestDir));
  system("rm -R /tmp/GGL_FileSystem_Test");
}

// test method OpenTextFile with existing folder
TEST(FileSystem, OpenTextFile_Accuracy_ExistingFolder_1) {
  FileSystem filesystem;
  system("rm -R /tmp/GGL_FileSystem_Test");
  system("mkdir /tmp/GGL_FileSystem_Test");
  EXPECT_TRUE(filesystem.FolderExists(kTestDir));
  EXPECT_FALSE(filesystem.FileExists(kTestDir));
  TextStreamInterface
      *text = filesystem.OpenTextFile(kTestDir,
                                      IO_MODE_READING, false, TRISTATE_TRUE);
  EXPECT_TRUE(text == NULL);
  EXPECT_TRUE(filesystem.FolderExists(kTestDir));
  EXPECT_FALSE(filesystem.FileExists(kTestDir));
  system("rm -R /tmp/GGL_FileSystem_Test");
}

// test method OpenTextFile with existing folder
TEST(FileSystem, OpenTextFile_Accuracy_ExistingFolder_2) {
  FileSystem filesystem;
  system("rm -R /tmp/GGL_FileSystem_Test");
  system("mkdir /tmp/GGL_FileSystem_Test");
  EXPECT_TRUE(filesystem.FolderExists(kTestDir));
  EXPECT_FALSE(filesystem.FileExists(kTestDir));
  TextStreamInterface
      *text = filesystem.OpenTextFile("\\tmp\\GGL_FileSystem_Test",
                                      IO_MODE_READING, false, TRISTATE_TRUE);
  EXPECT_TRUE(text == NULL);
  EXPECT_TRUE(filesystem.FolderExists(kTestDir));
  EXPECT_FALSE(filesystem.FileExists(kTestDir));
  system("rm -R /tmp/GGL_FileSystem_Test");
}

// test with NULL argument
TEST(FileSystem, OpenTextFile_Failure1) {
  FileSystem filesystem;
  EXPECT_TRUE(filesystem.OpenTextFile(NULL, IO_MODE_READING, false,
                                      TRISTATE_TRUE) == NULL);
}

// test with empty string argument
TEST(FileSystem, OpenTextFile_Failure2) {
  FileSystem filesystem;
  EXPECT_TRUE(filesystem.OpenTextFile("", IO_MODE_READING, false, TRISTATE_TRUE)
      == NULL);
}

// test method GetStandardStream
TEST(FileSystem, GetStandardStream_Accuracy) {
  FileSystem filesystem;

  TextStreamInterface *text_in = filesystem.GetStandardStream(STD_STREAM_OUT,
                                                              false);
  EXPECT_TRUE(text_in != NULL);

  TextStreamInterface *text_out = filesystem.GetStandardStream(STD_STREAM_OUT,
                                                               false);
  EXPECT_TRUE(text_out != NULL);

  TextStreamInterface *text_err = filesystem.GetStandardStream(STD_STREAM_OUT,
                                                               false);
  EXPECT_TRUE(text_err != NULL);

  text_out->WriteLine("Test for standard output!");
}

// test with invalid argument
TEST(FileSystem, GetStandardStream_Failure1) {
  FileSystem filesystem;
  EXPECT_TRUE(filesystem.GetStandardStream((StandardStreamType) -1, false)
      == NULL);
}

// test with invalid argument
TEST(FileSystem, GetStandardStream_Failure2) {
  FileSystem filesystem;
  EXPECT_TRUE(filesystem.GetStandardStream((StandardStreamType) 100, false)
      == NULL);
}

// test method GetFileVersion
TEST(FileSystem, GetFileVersion_Accuracy) {
  FileSystem filesystem;
  EXPECT_EQ("", filesystem.GetFileVersion(kTestDir));
  EXPECT_EQ("", filesystem.GetFileVersion("/tmp"));
  EXPECT_EQ("", filesystem.GetFileVersion("/"));
  EXPECT_EQ("", filesystem.GetFileVersion(NULL));
  EXPECT_EQ("", filesystem.GetFileVersion(""));
}

int main(int argc, char **argv) {
  testing::ParseGTestFlags(&argc, argv);

  return RUN_ALL_TESTS();
}
