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
#include "ggadget/common.h"
#include "ggadget/logger.h"
#include "unittest/gtest.h"
#include "../file_system.h"

using namespace ggadget;
using namespace ggadget::framework;
using namespace ggadget::framework::linux_system;

static const char *kTestDir = "/tmp/GGL_FileSystem_Test";

class FileTest : public testing::Test {
 protected:
  virtual void SetUp() {
    system("rm -R /tmp/GGL_FileSystem_Test");
    system("touch /tmp/GGL_FileSystem_Test");
    system("echo -n \"test content: 12345\" > /tmp/GGL_FileSystem_Test");
    file_ = filesystem_.GetFile(kTestDir);
  }

  virtual void TearDown() {
    if (file_) {
      file_->Destroy();
      file_ = NULL;
    }
    system("rm /tmp/GGL_FileSystem_Test");
  }

  FileSystem filesystem_;
  FileInterface *file_;
};

// GGL_FileSystem_Test constructor of class File
TEST_F(FileTest, File_1) {
  EXPECT_TRUE(file_ != NULL);
  EXPECT_EQ(kTestDir, file_->GetPath());
  EXPECT_EQ("GGL_FileSystem_Test", file_->GetName());
  EXPECT_TRUE(filesystem_.FileExists(kTestDir));
}

// GGL_FileSystem_Test constructor of class File
TEST_F(FileTest, File_2) {
  system("rm -R /tmp/GGL_FileSystem_Test");
  system("touch /tmp/GGL_FileSystem_Test");
  FileInterface *file = filesystem_.GetFile("\\tmp\\GGL_FileSystem_Test");
  EXPECT_TRUE(file != NULL);
  EXPECT_EQ(kTestDir, file->GetPath());
  EXPECT_EQ("GGL_FileSystem_Test", file->GetName());
  EXPECT_TRUE(filesystem_.FileExists(kTestDir));
  file->Destroy();
  system("rm -R /tmp/GGL_FileSystem_Test");
}

// GGL_FileSystem_Test method GetPath
TEST_F(FileTest, GetPath) {
  EXPECT_TRUE(file_ != NULL);
  EXPECT_EQ(kTestDir, file_->GetPath());
}

// GGL_FileSystem_Test method GetName
TEST_F(FileTest, GetName) {
  EXPECT_TRUE(file_ != NULL);
  EXPECT_EQ("GGL_FileSystem_Test", file_->GetName());
}

// GGL_FileSystem_Test method SetName
TEST_F(FileTest, SetName_Accuracy) {
  system("rm -R /tmp/new_folder");
  EXPECT_TRUE(file_ != NULL);
  EXPECT_EQ("GGL_FileSystem_Test", file_->GetName());
  EXPECT_TRUE(file_->SetName("new_name"));
  EXPECT_FALSE(filesystem_.FileExists(kTestDir));
  EXPECT_TRUE(filesystem_.FileExists("/tmp/new_name"));
  EXPECT_EQ("new_name", file_->GetName());
  system("rm -R /tmp/new_name");
}

// GGL_FileSystem_Test method SetName with same name
TEST_F(FileTest, SetName_Accuracy_SameName) {
  EXPECT_TRUE(file_ != NULL);
  EXPECT_EQ("GGL_FileSystem_Test", file_->GetName());
  EXPECT_TRUE(file_->SetName("GGL_FileSystem_Test"));
  EXPECT_TRUE(filesystem_.FileExists(kTestDir));
  EXPECT_EQ("GGL_FileSystem_Test", file_->GetName());
}

// GGL_FileSystem_Test method SetName with NULL argument
TEST_F(FileTest, SetName_Failure_NULL) {
  EXPECT_TRUE(file_ != NULL);
  EXPECT_FALSE(file_->SetName(NULL));
  EXPECT_TRUE(filesystem_.FileExists(kTestDir));
}

// GGL_FileSystem_Test method SetName with empty string argument
TEST_F(FileTest, SetName_Failure_EmptyString) {
  EXPECT_TRUE(file_ != NULL);
  EXPECT_FALSE(file_->SetName(""));
  EXPECT_TRUE(filesystem_.FileExists(kTestDir));
}


// GGL_FileSystem_Test method GetShortPath
TEST_F(FileTest, GetShortPath_Accuracy_1) {
  EXPECT_TRUE(file_ != NULL);
  system("touch /tmp/I_love_you_MengMeng");
  FileInterface *file =
    filesystem_.GetFile("/tmp/I_love_you_MengMeng");
  EXPECT_TRUE(file != NULL);
  EXPECT_EQ("/tmp/I_love_you_MengMeng", file->GetPath());
  EXPECT_EQ("/tmp/I_LOVE~1", file->GetShortPath());
  file->Destroy();
  system("rm -R /tmp/I_love_you_MengMeng");
}

// GGL_FileSystem_Test method GetShortPath
TEST_F(FileTest, GetShortPath_Accuracy_2) {
  EXPECT_TRUE(file_ != NULL);
  system("touch /tmp/TestCase");
  FileInterface *file =
    filesystem_.GetFile("/tmp/TestCase");
  EXPECT_TRUE(file != NULL);
  EXPECT_EQ("/tmp/TestCase", file->GetPath());
  EXPECT_EQ("/tmp/TESTCASE", file->GetShortPath());
  file->Destroy();
  system("rm -R /tmp/TestCase");
}

// GGL_FileSystem_Test method GetShortPath
TEST_F(FileTest, GetShortPath_Accuracy_3) {
  EXPECT_TRUE(file_ != NULL);
  system("touch /tmp/I_love_you.txt");
  FileInterface *file =
    filesystem_.GetFile("/tmp/I_love_you.txt");
  EXPECT_TRUE(file != NULL);
  EXPECT_EQ("/tmp/I_love_you.txt", file->GetPath());
  EXPECT_EQ("/tmp/I_LOVE~1.TXT", file->GetShortPath());
  file->Destroy();
  system("rm -R /tmp/I_love_you.txt");
}

// GGL_FileSystem_Test method GetShortPath
TEST_F(FileTest, GetShortPath_Accuracy_4) {
  EXPECT_TRUE(file_ != NULL);
  system("touch /tmp/I_love_you.txt1234");
  FileInterface *file =
    filesystem_.GetFile("/tmp/I_love_you.txt1234");
  EXPECT_TRUE(file != NULL);
  EXPECT_EQ("/tmp/I_love_you.txt1234", file->GetPath());
  EXPECT_EQ("/tmp/I_LOVE~1.TXT", file->GetShortPath());
  file->Destroy();
  system("rm -R /tmp/I_love_you.txt1234");
}



// GGL_FileSystem_Test method GetShortName
TEST_F(FileTest, GetShortName_Accuracy_1) {
  system("touch /tmp/I_love_you_MengMeng");
  FileInterface *file =
    filesystem_.GetFile("/tmp/I_love_you_MengMeng");
  EXPECT_TRUE(file != NULL);
  EXPECT_EQ("I_love_you_MengMeng", file->GetName());
  EXPECT_EQ("I_LOVE~1", file->GetShortName());
  file->Destroy();
  system("rm -R /tmp/I_love_you_MengMeng");
}

// GGL_FileSystem_Test method GetShortName
TEST_F(FileTest, GetShortName_Accuracy_2) {
  EXPECT_TRUE(file_ != NULL);
  EXPECT_EQ("GGL_FileSystem_Test", file_->GetName());
  EXPECT_EQ("GGL_FI~1", file_->GetShortName());
}

// GGL_FileSystem_Test method GetShortName
TEST_F(FileTest, GetShortName_Accuracy_3) {
  EXPECT_TRUE(file_ != NULL);
  system("touch /tmp/I_love_you.txt");
  FileInterface *file =
    filesystem_.GetFile("/tmp/I_love_you.txt");
  EXPECT_TRUE(file != NULL);
  EXPECT_EQ("I_love_you.txt", file->GetName());
  EXPECT_EQ("I_LOVE~1.TXT", file->GetShortName());
  file->Destroy();
  system("rm -R /tmp/I_love_you.txt");
}

// GGL_FileSystem_Test method GetShortName
TEST_F(FileTest, GetShortName_Accuracy_4) {
  EXPECT_TRUE(file_ != NULL);
  system("touch /tmp/I_love_you.txt1234");
  FileInterface *file =
    filesystem_.GetFile("/tmp/I_love_you.txt1234");
  EXPECT_TRUE(file != NULL);
  EXPECT_EQ("I_love_you.txt1234", file->GetName());
  EXPECT_EQ("I_LOVE~1.TXT", file->GetShortName());
  file->Destroy();
  system("rm -R /tmp/I_love_you.txt1234");
}


// GGL_FileSystem_Test method GetDrive
TEST_F(FileTest, GetDrive_Accuracy_1) {
  EXPECT_TRUE(file_ != NULL);
  EXPECT_TRUE(file_->GetDrive() == NULL);
}


// GGL_FileSystem_Test method GetParentFolder
TEST_F(FileTest, GetParentFile_Accuracy_1) {
  EXPECT_TRUE(file_ != NULL);
  FolderInterface *parent = file_->GetParentFolder();
  EXPECT_TRUE(parent != NULL);
  EXPECT_EQ("/tmp", parent->GetPath());
  parent->Destroy();
}


// GGL_FileSystem_Test method GetAttributes
TEST_F(FileTest, GetAttributes_Accuracy_1) {
  EXPECT_TRUE(file_ != NULL);
  EXPECT_EQ(FILE_ATTR_NORMAL, file_->GetAttributes());
}

// GGL_FileSystem_Test method GetAttributes
TEST_F(FileTest, GetAttributes_Accuracy_2) {
  system("rm -R /tmp/.GGL_FileSystem_Test");
  system("touch /tmp/.GGL_FileSystem_Test");
  FileInterface *file = filesystem_.GetFile("/tmp/.GGL_FileSystem_Test");
  EXPECT_EQ(0, file->GetAttributes() & FILE_ATTR_DIRECTORY);
  EXPECT_NE(0, file->GetAttributes() & FILE_ATTR_HIDDEN);
  EXPECT_EQ(0, file->GetAttributes() & FILE_ATTR_READONLY);
  file->Destroy();
  system("rm -R /tmp/.GGL_FileSystem_Test");
}

// GGL_FileSystem_Test method GetAttributes
TEST_F(FileTest, GetAttributes_Accuracy_3) {
  system("chmod -w /tmp/GGL_FileSystem_Test");
  EXPECT_TRUE(file_ != NULL);
  EXPECT_EQ(0, file_->GetAttributes() & FILE_ATTR_DIRECTORY);
  EXPECT_EQ(0, file_->GetAttributes() & FILE_ATTR_HIDDEN);
  EXPECT_NE(0, file_->GetAttributes() & FILE_ATTR_READONLY);
  system("chmod +w /tmp/GGL_FileSystem_Test");
  system("rm -R /tmp/GGL_FileSystem_Test");
}

// GGL_FileSystem_Test method GetAttributes
TEST_F(FileTest, GetAttributes_Accuracy_4) {
  system("rm -R /tmp/.GGL_FileSystem_Test");
  system("touch /tmp/.GGL_FileSystem_Test");
  FileInterface *file = filesystem_.GetFile("/tmp/.GGL_FileSystem_Test");
  system("chmod -w /tmp/.GGL_FileSystem_Test");
  EXPECT_TRUE(file != NULL);
  EXPECT_EQ(0, file->GetAttributes() & FILE_ATTR_DIRECTORY);
  EXPECT_NE(0, file->GetAttributes() & FILE_ATTR_HIDDEN);
  EXPECT_NE(0, file->GetAttributes() & FILE_ATTR_READONLY);
  file->Destroy();
  system("chmod +w /tmp/.GGL_FileSystem_Test");
  system("rm -R /tmp/.GGL_FileSystem_Test");
}


// GGL_FileSystem_Test method SetAttributes
TEST_F(FileTest, SetAttributes_Accuracy_1) {
  EXPECT_TRUE(file_ != NULL);
  EXPECT_EQ(0, file_->GetAttributes() & FILE_ATTR_DIRECTORY);
  EXPECT_EQ(0, file_->GetAttributes() & FILE_ATTR_HIDDEN);
  EXPECT_EQ(0, file_->GetAttributes() & FILE_ATTR_READONLY);
  EXPECT_TRUE(file_->SetAttributes(FILE_ATTR_READONLY));
  EXPECT_EQ(0, file_->GetAttributes() & FILE_ATTR_DIRECTORY);
  EXPECT_EQ(0, file_->GetAttributes() & FILE_ATTR_HIDDEN);
  EXPECT_NE(0, file_->GetAttributes() & FILE_ATTR_READONLY);
  system("chmod a+w /tmp/GGL_FileSystem_Test");
}

// GGL_FileSystem_Test method SetAttributes
TEST_F(FileTest, SetAttributes_Accuracy_2) {
  system("chmod a+w /tmp/.GGL_FileSystem_Test");
  system("rm -R /tmp/.GGL_FileSystem_Test");
  EXPECT_TRUE(file_ != NULL);
  EXPECT_EQ(0, file_->GetAttributes() & FILE_ATTR_DIRECTORY);
  EXPECT_EQ(0, file_->GetAttributes() & FILE_ATTR_HIDDEN);
  EXPECT_EQ(0, file_->GetAttributes() & FILE_ATTR_READONLY);
  EXPECT_TRUE(file_->SetAttributes(FILE_ATTR_HIDDEN));
  EXPECT_EQ(0, file_->GetAttributes() & FILE_ATTR_DIRECTORY);
  EXPECT_NE(0, file_->GetAttributes() & FILE_ATTR_HIDDEN);
  EXPECT_EQ(0, file_->GetAttributes() & FILE_ATTR_READONLY);
  EXPECT_EQ(".GGL_FileSystem_Test", file_->GetName());
  system("rm -R /tmp/.GGL_FileSystem_Test");
}

// GGL_FileSystem_Test method SetAttributes
TEST_F(FileTest, SetAttributes_Accuracy_3) {
  system("chmod a+w /tmp/.GGL_FileSystem_Test");
  system("rm -R /tmp/.GGL_FileSystem_Test");
  EXPECT_TRUE(file_ != NULL);
  EXPECT_EQ(0, file_->GetAttributes() & FILE_ATTR_DIRECTORY);
  EXPECT_EQ(0, file_->GetAttributes() & FILE_ATTR_HIDDEN);
  EXPECT_EQ(0, file_->GetAttributes() & FILE_ATTR_READONLY);
  EXPECT_TRUE(file_->SetAttributes((FileAttribute)
                                     (FILE_ATTR_HIDDEN | FILE_ATTR_READONLY)));
  EXPECT_EQ(0, file_->GetAttributes() & FILE_ATTR_DIRECTORY);
  EXPECT_NE(0, file_->GetAttributes() & FILE_ATTR_HIDDEN);
  EXPECT_NE(0, file_->GetAttributes() & FILE_ATTR_READONLY);
  EXPECT_EQ(".GGL_FileSystem_Test", file_->GetName());
  system("chmod a+w /tmp/.GGL_FileSystem_Test");
  system("rm -R /tmp/.GGL_FileSystem_Test");
}

// GGL_FileSystem_Test method SetAttributes with invalid input argument
TEST_F(FileTest, SetAttributes_Failure) {
  EXPECT_TRUE(file_ != NULL);
//  EXPECT_FALSE(file_->SetAttributes(FileAttribute(-1)));
//  EXPECT_FALSE(file_->SetAttributes(FileAttribute(1000000)));
}


// GGL_FileSystem_Test method GetDateCreated
TEST_F(FileTest, GetDateCreated) {
  EXPECT_TRUE(file_ != NULL);
  EXPECT_TRUE(file_->GetDateCreated() == Date(0));
}


// GGL_FileSystem_Test method GetDateLastModified
TEST_F(FileTest, GetDateLastModified) {
  EXPECT_TRUE(file_ != NULL);
  EXPECT_FALSE(file_->GetDateLastModified() == Date(0));
}


// GGL_FileSystem_Test method GetDateLastAccessed
TEST_F(FileTest, GetDateLastAccessed) {
  EXPECT_TRUE(file_ != NULL);
  EXPECT_FALSE(file_->GetDateLastAccessed() == Date(0));
}


// GGL_FileSystem_Test method GetType
TEST_F(FileTest, GetType1) {
  EXPECT_TRUE(file_ != NULL);
  EXPECT_EQ("", file_->GetType());
}

// GGL_FileSystem_Test method GetType
TEST_F(FileTest, GetType2) {
  system("rm -R /tmp/file.cc");
  system("touch /tmp/file.cc");
  FileInterface *file = filesystem_.GetFile("/tmp/file.cc");
  EXPECT_TRUE(file != NULL);
  EXPECT_EQ("cc", file->GetType());
  file->Destroy();
  system("rm -R /tmp/file.cc");
}


// GGL_FileSystem_Test method Delete
TEST_F(FileTest, Delete) {
  EXPECT_TRUE(file_ != NULL);
  EXPECT_TRUE(filesystem_.FileExists(file_->GetPath().c_str()));
  EXPECT_TRUE(file_->Delete(true));
  EXPECT_FALSE(filesystem_.FileExists(file_->GetPath().c_str()));
}


// GGL_FileSystem_Test method GetSize
TEST_F(FileTest, GetSize1) {
  EXPECT_TRUE(file_ != NULL);
  EXPECT_EQ(19, file_->GetSize());
}

// GGL_FileSystem_Test method GetSize
TEST_F(FileTest, GetSize2) {
  system("rm -R /tmp/file.cc");
  system("touch /tmp/file.cc");
  FileInterface *file = filesystem_.GetFile("/tmp/file.cc");
  EXPECT_EQ(0, file->GetSize());
  file->Destroy();
  system("rm -R /tmp/file.cc");
}


// GGL_FileSystem_Test method OpenAsTextStream
TEST_F(FileTest, OpenAsTextStream) {
  EXPECT_TRUE(file_ != NULL);
  TextStreamInterface *text = file_->OpenAsTextStream(IO_MODE_READING,
                                                      TRISTATE_TRUE);
  EXPECT_EQ("test content: 12345", text->ReadAll());
  EXPECT_TRUE(text != NULL);
  text->Destroy();
}


int main(int argc, char **argv) {
  testing::ParseGTestFlags(&argc, argv);

  return RUN_ALL_TESTS();
}
