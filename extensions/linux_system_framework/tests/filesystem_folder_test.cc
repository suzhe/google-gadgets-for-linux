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

#include <stdio.h>
#include "ggadget/common.h"
#include "ggadget/logger.h"
#include "unittest/gtest.h"

#include "../file_system.h"

using namespace ggadget;
using namespace ggadget::framework;
using namespace ggadget::framework::linux_system;

static const char *kTestDir = "/tmp/GGL_FileSystem_Test";

class FolderTest : public testing::Test {
 protected:
  virtual void SetUp() {
    std::system("rm -R /tmp/GGL_FileSystem_Test");
    folder_ = filesystem_.CreateFolder(kTestDir);
  }

  virtual void TearDown() {
    if (folder_) {
      folder_->Destroy();
      folder_ = NULL;
    }
    std::system("rm -R /tmp/GGL_FileSystem_Test");
  }

  FileSystem filesystem_;
  FolderInterface *folder_;
};

// GGL_FileSystem_Test constructor of class Folder
TEST_F(FolderTest, Folder_1) {
  EXPECT_TRUE(folder_ != NULL);
  EXPECT_EQ(kTestDir, folder_->GetPath());
  EXPECT_EQ("GGL_FileSystem_Test", folder_->GetName());
  EXPECT_TRUE(filesystem_.FolderExists(kTestDir));
}

// GGL_FileSystem_Test constructor of class Folder
TEST_F(FolderTest, Folder_2) {
  FolderInterface *folder = filesystem_.CreateFolder("\\tmp\\GGL_FileSystem_Test");
  EXPECT_TRUE(folder != NULL);
  EXPECT_EQ(kTestDir, folder_->GetPath());
  EXPECT_EQ("GGL_FileSystem_Test", folder_->GetName());
  EXPECT_TRUE(filesystem_.FolderExists(kTestDir));
  folder->Destroy();
}

// GGL_FileSystem_Test method GetPath
TEST_F(FolderTest, GetPath) {
  EXPECT_TRUE(folder_ != NULL);
  EXPECT_EQ(kTestDir, folder_->GetPath());
}

// GGL_FileSystem_Test method GetName
TEST_F(FolderTest, GetName) {
  EXPECT_TRUE(folder_ != NULL);
  EXPECT_EQ("GGL_FileSystem_Test", folder_->GetName());
}

// GGL_FileSystem_Test method SetName
TEST_F(FolderTest, SetName_Accuracy) {
  std::system("rm -R /tmp/new_folder");
  EXPECT_TRUE(folder_ != NULL);
  EXPECT_EQ("GGL_FileSystem_Test", folder_->GetName());
  EXPECT_TRUE(folder_->SetName("new_folder"));
  EXPECT_FALSE(filesystem_.FolderExists(kTestDir));
  EXPECT_TRUE(filesystem_.FolderExists("/tmp/new_folder"));
  EXPECT_EQ("new_folder", folder_->GetName());
  std::system("rm -R /tmp/new_folder");
}

// GGL_FileSystem_Test method SetName with same name
TEST_F(FolderTest, SetName_Accuracy_SameName) {
  EXPECT_TRUE(folder_ != NULL);
  EXPECT_EQ("GGL_FileSystem_Test", folder_->GetName());
  EXPECT_TRUE(folder_->SetName("GGL_FileSystem_Test"));
  EXPECT_TRUE(filesystem_.FolderExists(kTestDir));
  EXPECT_EQ("GGL_FileSystem_Test", folder_->GetName());
}

// GGL_FileSystem_Test method SetName with NULL argument
TEST_F(FolderTest, SetName_Failure_NULL) {
  EXPECT_TRUE(folder_ != NULL);
  EXPECT_FALSE(folder_->SetName(NULL));
  EXPECT_TRUE(filesystem_.FolderExists(kTestDir));
}

// GGL_FileSystem_Test method SetName with empty string argument
TEST_F(FolderTest, SetName_Failure_EmptyString) {
  EXPECT_TRUE(folder_ != NULL);
  EXPECT_FALSE(folder_->SetName(""));
  EXPECT_TRUE(filesystem_.FolderExists(kTestDir));
}


// GGL_FileSystem_Test method GetShortPath
TEST_F(FolderTest, GetShortPath_Accuracy_1) {
  EXPECT_TRUE(folder_ != NULL);
  FolderInterface *folder =
    filesystem_.CreateFolder("/tmp/I_love_you_MengMeng");
  EXPECT_TRUE(folder != NULL);
  EXPECT_EQ("/tmp/I_love_you_MengMeng", folder->GetPath());
  EXPECT_EQ("/tmp/I_LOVE~1", folder->GetShortPath());
  folder->Destroy();
  std::system("rm -R /tmp/I_love_you_MengMeng");
}

// GGL_FileSystem_Test method GetShortPath
TEST_F(FolderTest, GetShortPath_Accuracy_2) {
  EXPECT_TRUE(folder_ != NULL);
  FolderInterface *folder =
    filesystem_.CreateFolder("/tmp/TestCase");
  EXPECT_TRUE(folder != NULL);
  EXPECT_EQ("/tmp/TestCase", folder->GetPath());
  EXPECT_EQ("/tmp/TESTCASE", folder->GetShortPath());
  folder->Destroy();
  std::system("rm -R /tmp/TestCase");
}


// GGL_FileSystem_Test method GetShortName
TEST_F(FolderTest, GetShortName_Accuracy_1) {
  FolderInterface *folder =
    filesystem_.CreateFolder("/tmp/I_love_you_MengMeng");
  EXPECT_TRUE(folder != NULL);
  EXPECT_EQ("I_love_you_MengMeng", folder->GetName());
  EXPECT_EQ("I_LOVE~1", folder->GetShortName());
  folder->Destroy();
  std::system("rm -R /tmp/I_love_you_MengMeng");
}

// GGL_FileSystem_Test method GetShortName
TEST_F(FolderTest, GetShortName_Accuracy_2) {
  EXPECT_TRUE(folder_ != NULL);
  EXPECT_EQ("GGL_FileSystem_Test", folder_->GetName());
  EXPECT_EQ("GGL_FI~1", folder_->GetShortName());
}


// GGL_FileSystem_Test method GetDrive
TEST_F(FolderTest, GetDrive_Accuracy_1) {
  EXPECT_TRUE(folder_ != NULL);
  EXPECT_TRUE(folder_->GetDrive() == NULL);
}


// GGL_FileSystem_Test method GetParentFolder
TEST_F(FolderTest, GetParentFolder_Accuracy_1) {
  EXPECT_TRUE(folder_ != NULL);
  FolderInterface *parent = folder_->GetParentFolder();
  EXPECT_TRUE(parent != NULL);
  EXPECT_EQ("/tmp", parent->GetPath());
  parent->Destroy();
}

// GGL_FileSystem_Test method GetParentFolder
TEST_F(FolderTest, GetParentFolder_Accuracy_2) {
  FolderInterface *folder = filesystem_.CreateFolder("/tmp");
  FolderInterface *parent = folder->GetParentFolder();
  EXPECT_TRUE(parent != NULL);
  EXPECT_EQ("/", parent->GetPath());
  parent->Destroy();
  folder->Destroy();
}


// GGL_FileSystem_Test method GetAttributes
TEST_F(FolderTest, GetAttributes_Accuracy_1) {
  EXPECT_TRUE(folder_ != NULL);
  EXPECT_EQ(FILE_ATTR_DIRECTORY, folder_->GetAttributes());
}

// GGL_FileSystem_Test method GetAttributes
TEST_F(FolderTest, GetAttributes_Accuracy_2) {
  std::system("rm -R /tmp/.GGL_FileSystem_Test");
  FolderInterface *folder = filesystem_.CreateFolder("/tmp/.GGL_FileSystem_Test");
  EXPECT_NE(0, folder->GetAttributes() & FILE_ATTR_DIRECTORY);
  EXPECT_NE(0, folder->GetAttributes() & FILE_ATTR_HIDDEN);
  EXPECT_EQ(0, folder->GetAttributes() & FILE_ATTR_READONLY);
  folder->Destroy();
  std::system("rm -R /tmp/.GGL_FileSystem_Test");
}

// GGL_FileSystem_Test method GetAttributes
TEST_F(FolderTest, GetAttributes_Accuracy_3) {
  std::system("chmod -w /tmp/GGL_FileSystem_Test");
  EXPECT_TRUE(folder_ != NULL);
  EXPECT_NE(0, folder_->GetAttributes() & FILE_ATTR_DIRECTORY);
  EXPECT_EQ(0, folder_->GetAttributes() & FILE_ATTR_HIDDEN);
  EXPECT_NE(0, folder_->GetAttributes() & FILE_ATTR_READONLY);
  std::system("chmod +w /tmp/GGL_FileSystem_Test");
  std::system("rm -R /tmp/GGL_FileSystem_Test");
}

// GGL_FileSystem_Test method GetAttributes
TEST_F(FolderTest, GetAttributes_Accuracy_4) {
  std::system("rm -R /tmp/.GGL_FileSystem_Test");
  FolderInterface *folder = filesystem_.CreateFolder("/tmp/.GGL_FileSystem_Test");
  std::system("chmod -w /tmp/.GGL_FileSystem_Test");
  EXPECT_TRUE(folder != NULL);
  EXPECT_NE(0, folder->GetAttributes() & FILE_ATTR_DIRECTORY);
  EXPECT_NE(0, folder->GetAttributes() & FILE_ATTR_HIDDEN);
  EXPECT_NE(0, folder->GetAttributes() & FILE_ATTR_READONLY);
  folder->Destroy();
  std::system("chmod +w /tmp/.GGL_FileSystem_Test");
  std::system("rm -R /tmp/.GGL_FileSystem_Test");
}


// GGL_FileSystem_Test method SetAttributes
TEST_F(FolderTest, SetAttributes_Accuracy_1) {
  EXPECT_TRUE(folder_ != NULL);
  EXPECT_NE(0, folder_->GetAttributes() & FILE_ATTR_DIRECTORY);
  EXPECT_EQ(0, folder_->GetAttributes() & FILE_ATTR_HIDDEN);
  EXPECT_EQ(0, folder_->GetAttributes() & FILE_ATTR_READONLY);
  EXPECT_TRUE(folder_->SetAttributes(FILE_ATTR_READONLY));
  EXPECT_NE(0, folder_->GetAttributes() & FILE_ATTR_DIRECTORY);
  EXPECT_EQ(0, folder_->GetAttributes() & FILE_ATTR_HIDDEN);
  EXPECT_NE(0, folder_->GetAttributes() & FILE_ATTR_READONLY);
  std::system("chmod a+w /tmp/GGL_FileSystem_Test");
}

// GGL_FileSystem_Test method SetAttributes
TEST_F(FolderTest, SetAttributes_Accuracy_2) {
  std::system("chmod a+w /tmp/.GGL_FileSystem_Test");
  std::system("rm -R /tmp/.GGL_FileSystem_Test");
  EXPECT_TRUE(folder_ != NULL);
  EXPECT_NE(0, folder_->GetAttributes() & FILE_ATTR_DIRECTORY);
  EXPECT_EQ(0, folder_->GetAttributes() & FILE_ATTR_HIDDEN);
  EXPECT_EQ(0, folder_->GetAttributes() & FILE_ATTR_READONLY);
  EXPECT_TRUE(folder_->SetAttributes(FILE_ATTR_HIDDEN));
  EXPECT_NE(0, folder_->GetAttributes() & FILE_ATTR_DIRECTORY);
  EXPECT_NE(0, folder_->GetAttributes() & FILE_ATTR_HIDDEN);
  EXPECT_EQ(0, folder_->GetAttributes() & FILE_ATTR_READONLY);
  EXPECT_EQ(".GGL_FileSystem_Test", folder_->GetName());
  std::system("rm -R /tmp/.GGL_FileSystem_Test");
}

// GGL_FileSystem_Test method SetAttributes
TEST_F(FolderTest, SetAttributes_Accuracy_3) {
  std::system("chmod a+w /tmp/.GGL_FileSystem_Test");
  std::system("rm -R /tmp/.GGL_FileSystem_Test");
  EXPECT_TRUE(folder_ != NULL);
  EXPECT_NE(0, folder_->GetAttributes() & FILE_ATTR_DIRECTORY);
  EXPECT_EQ(0, folder_->GetAttributes() & FILE_ATTR_HIDDEN);
  EXPECT_EQ(0, folder_->GetAttributes() & FILE_ATTR_READONLY);
  EXPECT_TRUE(folder_->SetAttributes((FileAttribute)
                                     (FILE_ATTR_HIDDEN | FILE_ATTR_READONLY)));
  EXPECT_NE(0, folder_->GetAttributes() & FILE_ATTR_DIRECTORY);
  EXPECT_NE(0, folder_->GetAttributes() & FILE_ATTR_HIDDEN);
  EXPECT_NE(0, folder_->GetAttributes() & FILE_ATTR_READONLY);
  EXPECT_EQ(".GGL_FileSystem_Test", folder_->GetName());
  std::system("chmod a+w /tmp/.GGL_FileSystem_Test");
  std::system("rm -R /tmp/.GGL_FileSystem_Test");
}

// GGL_FileSystem_Test method SetAttributes with invalid input argument
TEST_F(FolderTest, SetAttributes_Failure) {
  EXPECT_TRUE(folder_ != NULL);
//  EXPECT_FALSE(folder_->SetAttributes(FileAttribute(-1)));
//  EXPECT_FALSE(folder_->SetAttributes(FileAttribute(1000000)));
}


// GGL_FileSystem_Test method GetDateCreated
TEST_F(FolderTest, GetDateCreated) {
  EXPECT_TRUE(folder_ != NULL);
  EXPECT_TRUE(folder_->GetDateCreated() == Date(0));
}


// GGL_FileSystem_Test method GetDateLastModified
TEST_F(FolderTest, GetDateLastModified) {
  EXPECT_TRUE(folder_ != NULL);
  EXPECT_FALSE(folder_->GetDateLastModified() == Date(0));
}


// GGL_FileSystem_Test method GetDateLastAccessed
TEST_F(FolderTest, GetDateLastAccessed) {
  EXPECT_TRUE(folder_ != NULL);
  EXPECT_FALSE(folder_->GetDateLastAccessed() == Date(0));
}


// GGL_FileSystem_Test method GetType
TEST_F(FolderTest, GetType) {
  EXPECT_TRUE(folder_ != NULL);
  EXPECT_EQ("FOLDER", folder_->GetType());
}


// GGL_FileSystem_Test method Delete
TEST_F(FolderTest, Delete) {
  EXPECT_TRUE(folder_ != NULL);
  EXPECT_TRUE(filesystem_.FolderExists(folder_->GetPath().c_str()));
  EXPECT_TRUE(folder_->Delete(true));
  EXPECT_FALSE(filesystem_.FolderExists(folder_->GetPath().c_str()));
}


// GGL_FileSystem_Test method IsRootFolder
TEST_F(FolderTest, IsRootFolder1) {
  EXPECT_TRUE(folder_ != NULL);
  EXPECT_FALSE(folder_->IsRootFolder());
}

// GGL_FileSystem_Test method IsRootFolder
TEST_F(FolderTest, IsRootFolder2) {
  FolderInterface *folder = filesystem_.CreateFolder("/");
  EXPECT_TRUE(folder != NULL);
  EXPECT_TRUE(folder->IsRootFolder());
  folder->Destroy();
}


// GGL_FileSystem_Test method GetSize
TEST_F(FolderTest, GetSize1) {
  EXPECT_TRUE(folder_ != NULL);
  std::system("touch /tmp/GGL_FileSystem_Test/file.cc");
  std::system("echo -n \"12345\" > /tmp/GGL_FileSystem_Test/file.cc");
  EXPECT_EQ(4096 + 5, folder_->GetSize());
}

// GGL_FileSystem_Test method GetSize
TEST_F(FolderTest, GetSize2) {
  EXPECT_TRUE(folder_ != NULL);
  std::system("mkdir /tmp/GGL_FileSystem_Test/subfolder");
  EXPECT_EQ(4096 * 2, folder_->GetSize());
}

// GGL_FileSystem_Test method GetSize
TEST_F(FolderTest, GetSize3) {
  EXPECT_TRUE(folder_ != NULL);
  std::system("mkdir /tmp/GGL_FileSystem_Test/subfolder");
  std::system("touch /tmp/GGL_FileSystem_Test/file.cc");
  std::system("echo -n \"12345\" > /tmp/GGL_FileSystem_Test/file.cc");
  std::system("touch /tmp/GGL_FileSystem_Test/subfolder/subfile");
  EXPECT_EQ(4096 * 2 + 5 + 0, folder_->GetSize());
}

// GGL_FileSystem_Test method GetSize with empty folder
TEST_F(FolderTest, GetSize4) {
  EXPECT_TRUE(folder_ != NULL);
  EXPECT_EQ(4096, folder_->GetSize());
}


// GGL_FileSystem_Test method GetSubFolders with empty folder
TEST_F(FolderTest, GetSubFolders_Empty) {
  EXPECT_TRUE(folder_ != NULL);
  FoldersInterface *subfolders = folder_->GetSubFolders();
  EXPECT_TRUE(subfolders != NULL);
  EXPECT_EQ(0, subfolders->GetCount());
  if (subfolders)
    subfolders->Destroy();
}

// GGL_FileSystem_Test method GetSubFolders
TEST_F(FolderTest, GetSubFolders1) {
  EXPECT_TRUE(folder_ != NULL);
  std::system("mkdir /tmp/GGL_FileSystem_Test/subfolder");
  std::system("touch /tmp/GGL_FileSystem_Test/file.cc");
  std::system("echo -n \"12345\" > /tmp/GGL_FileSystem_Test/file.cc");
  FoldersInterface *subfolders = folder_->GetSubFolders();
  EXPECT_TRUE(subfolders != NULL);
  EXPECT_EQ(1, subfolders->GetCount());
  FolderInterface *subfolder = subfolders->GetItem(0);
  EXPECT_TRUE(subfolder != NULL);
  EXPECT_EQ("/tmp/GGL_FileSystem_Test/subfolder", subfolder->GetPath());
  if (subfolder)
    subfolder->Destroy();
  if (subfolders)
    subfolders->Destroy();
}

// GGL_FileSystem_Test method GetSubFolders
// TODO: check whether count of subfolders is 2 under windows
TEST_F(FolderTest, GetSubFolders2) {
  EXPECT_TRUE(folder_ != NULL);
  std::system("mkdir /tmp/GGL_FileSystem_Test/subfolder");
  std::system("touch /tmp/GGL_FileSystem_Test/file.cc");
  std::system("echo -n \"12345\" > /tmp/GGL_FileSystem_Test/file.cc");
  std::system("mkdir /tmp/GGL_FileSystem_Test/subfolder/subsubfolder");
  std::system("touch /tmp/GGL_FileSystem_Test/subfolder/subsubfolder/subsubfile");
  FoldersInterface *subfolders = folder_->GetSubFolders();
  EXPECT_TRUE(subfolders != NULL);
  EXPECT_EQ(2, subfolders->GetCount());
  FolderInterface *subfolder = subfolders->GetItem(0);
  EXPECT_TRUE(subfolder != NULL);
  EXPECT_EQ("/tmp/GGL_FileSystem_Test/subfolder", subfolder->GetPath());
  FoldersInterface *subsubfolders = subfolder->GetSubFolders();
  EXPECT_TRUE(subsubfolders != NULL);
  EXPECT_EQ(1, subsubfolders->GetCount());
  if (subsubfolders)
    subsubfolders->Destroy();
  if (subfolder)
    subfolder->Destroy();
  if (subfolders)
    subfolders->Destroy();
}


// GGL_FileSystem_Test method GetFiles with empty folder
TEST_F(FolderTest, GetFiles_Empty) {
  EXPECT_TRUE(folder_ != NULL);
  FilesInterface *files = folder_->GetFiles();
  EXPECT_TRUE(files != NULL);
  EXPECT_EQ(0, files->GetCount());
  if (files)
    files->Destroy();
}

// GGL_FileSystem_Test method GetFiles
TEST_F(FolderTest, GetFiles1) {
  EXPECT_TRUE(folder_ != NULL);
  std::system("mkdir /tmp/GGL_FileSystem_Test/subfolder");
  std::system("touch /tmp/GGL_FileSystem_Test/file.cc");
  std::system("echo -n \"12345\" > /tmp/GGL_FileSystem_Test/file.cc");
  FilesInterface *files = folder_->GetFiles();
  EXPECT_TRUE(files != NULL);
  EXPECT_EQ(1, files->GetCount());
  FileInterface *subfile = files->GetItem(0);
  EXPECT_TRUE(subfile != NULL);
  EXPECT_EQ("/tmp/GGL_FileSystem_Test/file.cc", subfile->GetPath());
  EXPECT_EQ(5, subfile->GetSize());
  if (subfile)
    subfile->Destroy();
  if (files)
    files->Destroy();
}

// GGL_FileSystem_Test method GetFiles
// TODO: check whether count of files is 2 under windows
TEST_F(FolderTest, GetFiles2) {
  EXPECT_TRUE(folder_ != NULL);
  std::system("mkdir /tmp/GGL_FileSystem_Test/subfolder");
  std::system("touch /tmp/GGL_FileSystem_Test/file.cc");
  std::system("echo -n \"12345\" > /tmp/GGL_FileSystem_Test/file.cc");
  std::system("mkdir /tmp/GGL_FileSystem_Test/subfolder/subsubfolder");
  std::system("touch /tmp/GGL_FileSystem_Test/subfolder/subsubfolder/subsubfile");
  FilesInterface *files = folder_->GetFiles();
  EXPECT_TRUE(files != NULL);
  EXPECT_EQ(2, files->GetCount());
  FileInterface *subfile = files->GetItem(0);
  EXPECT_TRUE(subfile != NULL);
  EXPECT_EQ("/tmp/GGL_FileSystem_Test/subfolder/subsubfolder/subsubfile", subfile->GetPath());
  if (subfile)
    subfile->Destroy();
  if (files)
    files->Destroy();
}


// GGL_FileSystem_Test method CreateTextFile with relative path
TEST_F(FolderTest, CreateTextFile_Accuracy1) {
  EXPECT_TRUE(folder_ != NULL);
  std::system("mkdir /tmp/GGL_FileSystem_Test/subfolder");
  std::system("touch /tmp/GGL_FileSystem_Test/file.cc");
  std::system("echo -n \"12345\" > /tmp/GGL_FileSystem_Test/file.cc");
  TextStreamInterface *text = folder_->CreateTextFile("file.cc", false, false);
  EXPECT_TRUE(text != NULL);
  EXPECT_EQ("12345", text->ReadAll());
  if (text)
    text->Destroy();
}

// GGL_FileSystem_Test method CreateTextFile with absolute path
TEST_F(FolderTest, CreateTextFile_Accuracy2) {
  EXPECT_TRUE(folder_ != NULL);
  std::system("mkdir /tmp/GGL_FileSystem_Test/subfolder");
  std::system("touch /tmp/GGL_FileSystem_Test/file.cc");
  std::system("echo -n \"12345\" > /tmp/GGL_FileSystem_Test/file.cc");
  TextStreamInterface *text = folder_->CreateTextFile("/tmp/GGL_FileSystem_Test/file.cc",
                                                      false,
                                                      false);
  EXPECT_TRUE(text != NULL);
  EXPECT_EQ("12345", text->ReadAll());
  if (text)
    text->Destroy();
}

// GGL_FileSystem_Test method CreateTextFile with empty file
TEST_F(FolderTest, CreateTextFile_Accuracy3) {
  EXPECT_TRUE(folder_ != NULL);
  std::system("mkdir /tmp/GGL_FileSystem_Test/subfolder");
  std::system("touch /tmp/GGL_FileSystem_Test/file.cc");
  TextStreamInterface *text = folder_->CreateTextFile("file.cc",
                                                      false,
                                                      false);
  EXPECT_TRUE(text != NULL);
  EXPECT_EQ("", text->ReadAll());
  if (text)
    text->Destroy();
}

// GGL_FileSystem_Test method CreateTextFile with absolute path
TEST_F(FolderTest, CreateTextFile_Accuracy4) {
  EXPECT_TRUE(folder_ != NULL);
  std::system("mkdir /tmp/GGL_FileSystem_Test/subfolder");
  std::system("touch /tmp/GGL_FileSystem_Test/file.cc");
  std::system("echo -n \"12345\" > /tmp/GGL_FileSystem_Test/file.cc");
  TextStreamInterface *text = folder_->CreateTextFile("\\tmp\\GGL_FileSystem_Test\\file.cc",
                                                      false,
                                                      false);
  EXPECT_TRUE(text != NULL);
  EXPECT_EQ("12345", text->ReadAll());
  if (text)
    text->Destroy();
}

// GGL_FileSystem_Test method CreateTextFile with NULL argument
TEST_F(FolderTest, CreateTextFile_Failure_NULL) {
  EXPECT_TRUE(folder_ != NULL);
  EXPECT_TRUE(folder_->CreateTextFile(NULL, false, false) == NULL);
}

// GGL_FileSystem_Test method CreateTextFile with empty string argument
TEST_F(FolderTest, CreateTextFile_Failure_EmptyString) {
  EXPECT_TRUE(folder_ != NULL);
  EXPECT_TRUE(folder_->CreateTextFile("", false, false) == NULL);
}

int main(int argc, char **argv) {
  testing::ParseGTestFlags(&argc, argv);

  return RUN_ALL_TESTS();
}
