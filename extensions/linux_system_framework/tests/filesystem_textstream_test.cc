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
#include <ggadget/common.h>
#include <ggadget/logger.h>
#include <unittest/gtest.h>
#include "../file_system.h"

using namespace ggadget;
using namespace ggadget::framework;
using namespace ggadget::framework::linux_system;

static const char *kTestDir = "/tmp/GGL_FileSystem_Test";

class TextStreamTest : public testing::Test {
 protected:
  virtual void SetUp() {
    std::system("rm -R /tmp/GGL_FileSystem_Test");
    std::system("touch /tmp/GGL_FileSystem_Test");
    std::system("echo -n \"line1\nline2\nline3\n\" > /tmp/GGL_FileSystem_Test");
    text_ = filesystem_.CreateTextFile(kTestDir, false, false);
  }

  virtual void TearDown() {
    if (text_) {
      text_->Close();
      text_->Destroy();
      text_ = NULL;
    }
    std::system("rm /tmp/GGL_FileSystem_Test");
  }

  FileSystem filesystem_;
  TextStreamInterface *text_;
};

// GGL_FileSystem_Test constructor of class File
TEST_F(TextStreamTest, TextStream_1) {
  EXPECT_TRUE(text_ != NULL);
  EXPECT_EQ("line1\nline2\nline3\n", text_->ReadAll());
}

// GGL_FileSystem_Test method GetPath
TEST_F(TextStreamTest, GetLine1) {
  EXPECT_TRUE(text_ != NULL);
  // GGL_FileSystem_Test init line number
  EXPECT_EQ(1, text_->GetLine());
}

// GGL_FileSystem_Test method GetPath
TEST_F(TextStreamTest, GetLine2) {
  EXPECT_TRUE(text_ != NULL);

  text_->SkipLine();
  EXPECT_EQ(2, text_->GetLine());

  text_->SkipLine();
  text_->SkipLine();
  EXPECT_EQ(4, text_->GetLine());
}

// GGL_FileSystem_Test method GetColumn
TEST_F(TextStreamTest, GetColumn1) {
  EXPECT_TRUE(text_ != NULL);
  // GGL_FileSystem_Test init column number
  EXPECT_EQ(1, text_->GetColumn());
}

// GGL_FileSystem_Test method GetColumn
TEST_F(TextStreamTest, GetColumn2) {
  EXPECT_TRUE(text_ != NULL);

  text_->Skip(1);
  EXPECT_EQ(2, text_->GetColumn());

  text_->Skip(2);
  EXPECT_EQ(4, text_->GetColumn());
}


// GGL_FileSystem_Test method IsAtEndOfStream
TEST_F(TextStreamTest, IsAtEndOfStream1) {
  EXPECT_TRUE(text_ != NULL);
  EXPECT_FALSE(text_->IsAtEndOfStream());
}

// GGL_FileSystem_Test method IsAtEndOfStream
TEST_F(TextStreamTest, IsAtEndOfStream2) {
  EXPECT_TRUE(text_ != NULL);
  text_->Skip(3);
  EXPECT_FALSE(text_->IsAtEndOfStream());
  text_->SkipLine();
  EXPECT_FALSE(text_->IsAtEndOfStream());
}

// GGL_FileSystem_Test method IsAtEndOfStream
TEST_F(TextStreamTest, IsAtEndOfStream3) {
  EXPECT_TRUE(text_ != NULL);

  text_->ReadAll();
  EXPECT_TRUE(text_->IsAtEndOfStream());
}


// GGL_FileSystem_Test method IsAtEndOfLine
TEST_F(TextStreamTest, IsAtEndOfLine1) {
  EXPECT_TRUE(text_ != NULL);
  EXPECT_FALSE(text_->IsAtEndOfLine());
}

// GGL_FileSystem_Test method IsAtEndOfLine
TEST_F(TextStreamTest, IsAtEndOfLine2) {
  EXPECT_TRUE(text_ != NULL);
  text_->Skip(3);
  EXPECT_FALSE(text_->IsAtEndOfLine());
  text_->SkipLine();
  EXPECT_FALSE(text_->IsAtEndOfLine());
}

// GGL_FileSystem_Test method IsAtEndOfLine
TEST_F(TextStreamTest, IsAtEndOfLine3) {
  EXPECT_TRUE(text_ != NULL);

  text_->ReadAll();
  EXPECT_TRUE(text_->IsAtEndOfLine());
}

// GGL_FileSystem_Test method IsAtEndOfLine
TEST_F(TextStreamTest, IsAtEndOfLine4) {
  EXPECT_TRUE(text_ != NULL);

  text_->Skip(5);
  EXPECT_TRUE(text_->IsAtEndOfLine());
}


// GGL_FileSystem_Test method Read
TEST_F(TextStreamTest, Read1) {
  EXPECT_TRUE(text_ != NULL);
  EXPECT_EQ("l", text_->Read(1));
  EXPECT_EQ("ine1", text_->Read(4));
  EXPECT_EQ("\n", text_->Read(1));
}

// GGL_FileSystem_Test method Read
TEST_F(TextStreamTest, Read2) {
  EXPECT_TRUE(text_ != NULL);
  EXPECT_EQ("line1\nline2\nline3\n", text_->Read(100));
}

// GGL_FileSystem_Test method Read with invalid input argument
TEST_F(TextStreamTest, Read_Failure_1) {
  EXPECT_TRUE(text_ != NULL);
  EXPECT_EQ("", text_->Read(-1));
}

// GGL_FileSystem_Test method Read with invalid input argument
TEST_F(TextStreamTest, Read_Failure_2) {
  EXPECT_TRUE(text_ != NULL);
  EXPECT_EQ("", text_->Read(0));
}


// GGL_FileSystem_Test method ReadLine
TEST_F(TextStreamTest, ReadLine1) {
  EXPECT_TRUE(text_ != NULL);
  EXPECT_EQ("line1\n", text_->ReadLine());
  EXPECT_EQ("line2\n", text_->ReadLine());
  EXPECT_EQ("line3\n", text_->ReadLine());
}

// GGL_FileSystem_Test method ReadLine
TEST_F(TextStreamTest, ReadLine2) {
  EXPECT_TRUE(text_ != NULL);
  EXPECT_EQ("line1\n", text_->ReadLine());
  EXPECT_EQ("line2\n", text_->ReadLine());
  EXPECT_EQ("line3\n", text_->ReadLine());
  EXPECT_EQ("", text_->ReadLine());
  EXPECT_EQ("", text_->ReadLine());
}


// GGL_FileSystem_Test method ReadAll
TEST_F(TextStreamTest, ReadAll1) {
  EXPECT_TRUE(text_ != NULL);
  EXPECT_EQ("line1\nline2\nline3\n", text_->ReadAll());
}

// GGL_FileSystem_Test method ReadAll
TEST_F(TextStreamTest, ReadAll2) {
  TextStreamInterface *text =
    filesystem_.CreateTextFile("/tmp/no_existing_file", false, false);
  EXPECT_TRUE(text != NULL);
  EXPECT_EQ("", text->ReadAll());
  text->Close();
  text->Destroy();
  std::system("rm /tmp/no_existing_file");
}


// GGL_FileSystem_Test method Write
TEST_F(TextStreamTest, Write1) {
  std::system("rm -R /tmp/file.cc");
  TextStreamInterface *text =
      filesystem_.CreateTextFile("/tmp/file.cc", false, false);
  EXPECT_TRUE(text != NULL);
  text->Write("new content");
  text->Close();
  text->Destroy();

  text = filesystem_.CreateTextFile("/tmp/file.cc", false, false);
  EXPECT_TRUE(text != NULL);
  EXPECT_EQ("new content", text->ReadAll());
  text->Close();
  text->Destroy();
  std::system("rm -R /tmp/file.cc");
}

// GGL_FileSystem_Test method Write
TEST_F(TextStreamTest, Write2) {
  EXPECT_TRUE(text_ != NULL);
  text_->Write("new content");
  text_->Close();

  TextStreamInterface *text =
    filesystem_.CreateTextFile(kTestDir, false, false);
  ASSERT_TRUE(text != NULL);
  EXPECT_EQ("new", text->Read(3));
  text->Close();
  text->Destroy();
}

// GGL_FileSystem_Test method Write with NULL argument
TEST_F(TextStreamTest, Write_Failure_1) {
  EXPECT_TRUE(text_ != NULL);
  text_->Write(NULL);
  text_->Close();

  TextStreamInterface *text =
    filesystem_.CreateTextFile(kTestDir, false, false);
  ASSERT_TRUE(text != NULL);
  EXPECT_EQ("line1\nline2\nline3\n", text->ReadAll());
  text->Close();
  text->Destroy();
}


// GGL_FileSystem_Test method WriteLine
TEST_F(TextStreamTest, WriteLine1) {
  std::system("rm -R /tmp/file.cc");
  TextStreamInterface *text =
      filesystem_.CreateTextFile("/tmp/file.cc", false, false);
  EXPECT_TRUE(text != NULL);
  text->WriteLine("new content");
  text->Close();
  text->Destroy();

  text = filesystem_.CreateTextFile("/tmp/file.cc", false, false);
  EXPECT_TRUE(text != NULL);
  EXPECT_EQ("new content\n", text->ReadAll());
  text->Close();
  text->Destroy();
  std::system("rm -R /tmp/file.cc");
}

// GGL_FileSystem_Test method WriteLine
TEST_F(TextStreamTest, WriteLine2) {
  EXPECT_TRUE(text_ != NULL);
  text_->WriteLine("new\n");
  text_->Close();

  TextStreamInterface *text =
    filesystem_.CreateTextFile(kTestDir, false, false);
  ASSERT_TRUE(text != NULL);
  EXPECT_EQ("new\n", text->Read(4));
  text->Close();
  text->Destroy();
}

// GGL_FileSystem_Test method WriteLine with NULL argument
TEST_F(TextStreamTest, WriteLine_Failure_1) {
  EXPECT_TRUE(text_ != NULL);
  text_->WriteLine(NULL);
  text_->Close();

  TextStreamInterface *text =
    filesystem_.CreateTextFile(kTestDir, false, false);
  ASSERT_TRUE(text != NULL);
  EXPECT_EQ("line1\nline2\nline3\n", text->ReadAll());
  text->Close();
  text->Destroy();
}

// GGL_FileSystem_Test method WriteLine with empty string argument
TEST_F(TextStreamTest, WriteLine_Failure_2) {
  EXPECT_TRUE(text_ != NULL);
  text_->WriteLine(NULL);
  text_->Close();

  TextStreamInterface *text =
    filesystem_.CreateTextFile(kTestDir, false, false);
  ASSERT_TRUE(text != NULL);
  EXPECT_EQ("line1\nline2\nline3\n", text->ReadAll());
  text->Close();
  text->Destroy();
}


// GGL_FileSystem_Test method WriteBlankLines
TEST_F(TextStreamTest, WriteBlankLines1) {
  std::system("rm -R /tmp/file.cc");
  TextStreamInterface *text =
      filesystem_.CreateTextFile("/tmp/file.cc", false, false);
  EXPECT_TRUE(text != NULL);
  text->WriteBlankLines(1);
  text->Close();
  text->Destroy();

  text = filesystem_.CreateTextFile("/tmp/file.cc", false, false);
  EXPECT_TRUE(text != NULL);
  EXPECT_EQ("\n", text->ReadAll());
  text->Close();
  text->Destroy();
  std::system("rm -R /tmp/file.cc");
}

TEST_F(TextStreamTest, WriteBlankLines1_) {
  std::system("rm -R /tmp/file.cc");
  TextStreamInterface *text =
      filesystem_.CreateTextFile("/tmp/file.cc", false, false);
  EXPECT_TRUE(text != NULL);
  text->WriteBlankLines(3);
  text->Close();
  text->Destroy();

  text = filesystem_.CreateTextFile("/tmp/file.cc", false, false);
  EXPECT_TRUE(text != NULL);
  EXPECT_EQ("\n\n\n", text->ReadAll());
  text->Close();
  text->Destroy();
  std::system("rm -R /tmp/file.cc");
}

// GGL_FileSystem_Test method WriteBlankLines
TEST_F(TextStreamTest, WriteBlankLines2) {
  EXPECT_TRUE(text_ != NULL);
  text_->WriteBlankLines(2);
  text_->Close();

  TextStreamInterface *text =
    filesystem_.CreateTextFile(kTestDir, false, false);
  ASSERT_TRUE(text != NULL);
  EXPECT_EQ("\n\n", text->Read(2));
  text->Close();
  text->Destroy();
}

// GGL_FileSystem_Test method WriteBlankLines with invalid argument
TEST_F(TextStreamTest, WriteBlankLines_Failure_1) {
  EXPECT_TRUE(text_ != NULL);
  text_->WriteBlankLines(-1);
  text_->Close();

  TextStreamInterface *text =
    filesystem_.CreateTextFile(kTestDir, false, false);
  ASSERT_TRUE(text != NULL);
  EXPECT_EQ("line1\nline2\nline3\n", text->ReadAll());
  text->Close();
  text->Destroy();
}

// GGL_FileSystem_Test method WriteBlankLines with empty string argument
TEST_F(TextStreamTest, WriteBlankLines_Failure_2) {
  EXPECT_TRUE(text_ != NULL);
  text_->WriteBlankLines(0);
  text_->Close();

  TextStreamInterface *text =
    filesystem_.CreateTextFile(kTestDir, false, false);
  ASSERT_TRUE(text != NULL);
  EXPECT_EQ("line1\nline2\nline3\n", text->ReadAll());
  text->Close();
  text->Destroy();
}


// GGL_FileSystem_Test method Skip
TEST_F(TextStreamTest, Skip1) {
  EXPECT_TRUE(text_ != NULL);
  text_->Skip(1);
  EXPECT_EQ("ine1", text_->Read(4));
  text_->Skip(2);
  EXPECT_EQ("ne2\n", text_->Read(4));
}

// GGL_FileSystem_Test method Skip
TEST_F(TextStreamTest, Skip1_) {
  EXPECT_TRUE(text_ != NULL);
  text_->Skip(3);
  EXPECT_EQ("e1\nline2\nline3\n", text_->ReadAll());
}

// GGL_FileSystem_Test method Skip
TEST_F(TextStreamTest, Skip2) {
  EXPECT_TRUE(text_ != NULL);
  text_->Skip(0);
  EXPECT_EQ("line1\nline2\nline3\n", text_->ReadAll());
}

// GGL_FileSystem_Test method Skip with invalid input argument
TEST_F(TextStreamTest, Skip_Failure_1) {
  EXPECT_TRUE(text_ != NULL);
  text_->Skip(-1);
  EXPECT_EQ("line1\nline2\nline3\n", text_->ReadAll());
}

// GGL_FileSystem_Test method Skip with invalid input argument
TEST_F(TextStreamTest, Skip_Failure_2) {
  EXPECT_TRUE(text_ != NULL);
  text_->Skip(-100);
  EXPECT_EQ("line1\nline2\nline3\n", text_->ReadAll());
}


// GGL_FileSystem_Test method SkipLine
TEST_F(TextStreamTest, SkipLine1) {
  EXPECT_TRUE(text_ != NULL);
  EXPECT_EQ("line1\n", text_->ReadLine());
  text_->SkipLine();
  EXPECT_EQ("line3\n", text_->ReadLine());
}

// GGL_FileSystem_Test method SkipLine
TEST_F(TextStreamTest, SkipLine2) {
  EXPECT_TRUE(text_ != NULL);
  text_->SkipLine();
  EXPECT_EQ("line2\nline3\n", text_->ReadAll());
}

// GGL_FileSystem_Test method SkipLine
TEST_F(TextStreamTest, SkipLine3) {
  EXPECT_TRUE(text_ != NULL);
  text_->SkipLine();
  text_->SkipLine();
  EXPECT_EQ("line3\n", text_->ReadAll());
}

// GGL_FileSystem_Test method SkipLine
TEST_F(TextStreamTest, SkipLine4) {
  EXPECT_TRUE(text_ != NULL);
  text_->SkipLine();
  text_->SkipLine();
  text_->SkipLine();
  text_->SkipLine();
  EXPECT_EQ("", text_->ReadAll());
}



int main(int argc, char **argv) {
  testing::ParseGTestFlags(&argc, argv);

  return RUN_ALL_TESTS();
}
