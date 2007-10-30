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

#include <cstdio>
#include <cairo.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <strings.h>

#include "ggadget/common.h"
#include "ggadget/graphics/cairo_canvas.h"
#include "ggadget/graphics/cairo_graphics.h"
#include "unittest/gunit.h"

using namespace ggadget;

const double kPi = 3.141592653589793; 
bool g_savepng = false;

// fixture for creating the CairoCanvas object
class CairoGfxTest : public testing::Test {
 protected:
  GraphicsInterface *gfx_;
  CanvasInterface *target_;
  cairo_surface_t *surface_;

  CairoGfxTest() {
    // create a target canvas for tests
    surface_ = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 300, 150);
    //surface_ = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 600, 300);    
    cairo_t *cr = cairo_create(surface_);
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
    cairo_set_source_rgba(cr, 0, 0, 0, 0);
    cairo_paint(cr);
    //cairo_scale(cr, 2, 2);
    target_ = new CairoCanvas(cr, 300, 150, false);
    cairo_destroy(cr);    
    cr = NULL;

    gfx_ = new CairoGraphics(2.0);
  }

  ~CairoGfxTest() {
    delete gfx_;
    gfx_ = NULL;

    target_->Destroy();
    target_ = NULL;

    if (g_savepng) {
      const testing::TestInfo *const test_info = 
        testing::UnitTest::GetInstance()->current_test_info();
      char file[100]; 
      snprintf(file, arraysize(file), "%s.png", test_info->name());
      cairo_surface_write_to_png(surface_, file);      
    }

    cairo_surface_destroy(surface_);
    surface_ = NULL;
  }  
};

// this test is meaningful only with -savepng
TEST_F(CairoGfxTest, NewCanvas) {
  EXPECT_TRUE(target_->DrawFilledRect(150, 0, 150, 150, Color(1, 1, 1)));

  CanvasInterface *c = gfx_->NewCanvas(100, 100);
  ASSERT_TRUE(c != NULL);
  EXPECT_TRUE(c->DrawFilledRect(0, 0, 100, 100, Color(1, 0, 0)));

  EXPECT_TRUE(target_->DrawCanvas(50, 50, c));

  c->Destroy();
  c = NULL;
}

TEST_F(CairoGfxTest, LoadImage) {
  char *buffer = NULL;

  int fd = open("120day.png", O_RDONLY);
  ASSERT_NE(-1, fd);

  struct stat statvalue;
  ASSERT_EQ(0, fstat(fd, &statvalue));

  size_t filelen = statvalue.st_size;
  ASSERT_NE((size_t)0, filelen);

  buffer = (char*)mmap(NULL, filelen, PROT_READ, MAP_PRIVATE, fd, 0);
  ASSERT_NE(MAP_FAILED, buffer);

  CanvasInterface *img = gfx_->NewImage(buffer, filelen);
  ASSERT_FALSE(NULL == img);

  EXPECT_TRUE(NULL == gfx_->NewImage(buffer, 0));
  EXPECT_TRUE(NULL == gfx_->NewImage(NULL, 500));

  EXPECT_EQ((size_t)450, img->GetWidth());
  EXPECT_EQ((size_t)310, img->GetHeight());
  EXPECT_FALSE(img->IsMask());

  img->Destroy();
  img = NULL;

  munmap(buffer, filelen);
}

// this test is meaningful only with -savepng
TEST_F(CairoGfxTest, DrawCanvas) {
  char *buffer = NULL;
  struct stat statvalue;
  size_t filelen;
  CanvasInterface *img;
  double h, scale;

  // PNG
  int fd = open("base.png", O_RDONLY);
  ASSERT_NE(-1, fd);

  ASSERT_EQ(0, fstat(fd, &statvalue));  
  filelen = statvalue.st_size;
  ASSERT_NE((size_t)0, filelen);

  buffer = (char*)mmap(NULL, filelen, PROT_READ, MAP_PRIVATE, fd, 0);
  ASSERT_NE(MAP_FAILED, buffer);

  img = gfx_->NewImage(buffer, filelen);
  ASSERT_FALSE(NULL == img);

  h = img->GetHeight();
  scale = 150. / h;

  EXPECT_FALSE(target_->DrawCanvas(50., 0., NULL));

  EXPECT_TRUE(target_->PushState());
  target_->ScaleCoordinates(scale, scale);  
  EXPECT_TRUE(target_->MultiplyOpacity(.5));
  EXPECT_TRUE(target_->DrawCanvas(150., 0., img));
  EXPECT_TRUE(target_->PopState());

  img->Destroy();
  img = NULL;

  munmap(buffer, filelen);

  // JPG
  fd = open("kitty419.jpg", O_RDONLY);
  ASSERT_NE(-1, fd);

  ASSERT_EQ(0, fstat(fd, &statvalue));    
  filelen = statvalue.st_size;
  ASSERT_NE((size_t)0, filelen);

  buffer = (char*)mmap(NULL, filelen, PROT_READ, MAP_PRIVATE, fd, 0);
  ASSERT_NE(MAP_FAILED, buffer);

  img = gfx_->NewImage(buffer, filelen);
  ASSERT_FALSE(NULL == img);   

  h = img->GetHeight();
  scale = 150. / h;
  target_->ScaleCoordinates(scale, scale);  
  EXPECT_TRUE(target_->DrawCanvas(0., 0., img));

  img->Destroy();
  img = NULL;

  munmap(buffer, filelen);
}

// this test is meaningful only with -savepng
TEST_F(CairoGfxTest, DrawImageMask) {
  char *buffer = NULL;
  struct stat statvalue;
  size_t filelen;
  CanvasInterface *mask, *img;
  double h, scale;

  EXPECT_TRUE(NULL == gfx_->NewMask(buffer, 0));
  EXPECT_TRUE(NULL == gfx_->NewMask(NULL, 500));
  EXPECT_TRUE(NULL == gfx_->NewMask(buffer, filelen));

  int fd = open("testmask.png", O_RDONLY);
  ASSERT_NE(-1, fd);

  ASSERT_EQ(0, fstat(fd, &statvalue));  
  filelen = statvalue.st_size;
  ASSERT_NE((size_t)0, filelen);

  buffer = (char*)mmap(NULL, filelen, PROT_READ, MAP_PRIVATE, fd, 0);
  ASSERT_NE(MAP_FAILED, buffer);

  mask = gfx_->NewMask(buffer, filelen);
  ASSERT_FALSE(NULL == mask);
  img = gfx_->NewImage(buffer, filelen);
  ASSERT_FALSE(NULL == img);

  EXPECT_EQ((size_t)450, mask->GetWidth());
  EXPECT_EQ((size_t)310, mask->GetHeight());
  EXPECT_TRUE(mask->IsMask());

  h = mask->GetHeight();
  scale = 150. / h;

  EXPECT_TRUE(target_->DrawFilledRect(0, 0, 300, 150, Color(0, 0, 1)));
  EXPECT_TRUE(target_->DrawCanvasWithMask(0, 0, img, 0, 0, mask));

  CanvasInterface *c = gfx_->NewCanvas(100, 100);
  ASSERT_TRUE(c != NULL);
  EXPECT_TRUE(c->DrawFilledRect(0, 0, 100, 100, Color(0, 1, 0)));
  EXPECT_TRUE(target_->DrawCanvasWithMask(150, 0, c, 0, 0, mask));

  mask->Destroy();
  mask = NULL;

  img->Destroy();
  img = NULL;

  c->Destroy();
  c = NULL;

  munmap(buffer, filelen);  
}

// this test is meaningful only with -savepng
TEST_F(CairoGfxTest, NewFontAndDrawText) {  
  FontInterface *font1 = gfx_->NewFont("Serif", 14, 
      FontInterface::STYLE_ITALIC, FontInterface::WEIGHT_BOLD);
  EXPECT_EQ(FontInterface::STYLE_ITALIC, font1->GetStyle());
  EXPECT_EQ(FontInterface::WEIGHT_BOLD, font1->GetWeight());
  EXPECT_EQ((size_t)14, font1->GetPointSize());

  EXPECT_FALSE(target_->DrawText(0, 0, 100, 30, NULL, font1, Color(1, 0, 0), 
              CanvasInterface::ALIGN_LEFT, CanvasInterface::VALIGN_TOP, 
              CanvasInterface::TRIMMING_NONE, 0));
  EXPECT_FALSE(target_->DrawText(0, 0, 100, 30, "abc", NULL, Color(1, 0, 0), 
              CanvasInterface::ALIGN_LEFT, CanvasInterface::VALIGN_TOP, 
              CanvasInterface::TRIMMING_NONE, 0));

  ASSERT_TRUE(font1 != NULL);
  EXPECT_TRUE(target_->DrawText(0, 0, 100, 30, "hello world", font1, 
              Color(1, 0, 0), CanvasInterface::ALIGN_LEFT, 
              CanvasInterface::VALIGN_TOP,
              CanvasInterface::TRIMMING_NONE, 0));

  FontInterface *font2 = gfx_->NewFont("Serif", 14, 
      FontInterface::STYLE_NORMAL, FontInterface::WEIGHT_NORMAL);
  ASSERT_TRUE(font2 != NULL);
  EXPECT_TRUE(target_->DrawText(0, 30, 100, 30, "hello world", font2, 
              Color(0, 1, 0), CanvasInterface::ALIGN_LEFT, 
              CanvasInterface::VALIGN_TOP, 
              CanvasInterface::TRIMMING_NONE, 0));

  FontInterface *font3 = gfx_->NewFont("Serif", 14, FontInterface::STYLE_NORMAL,
      FontInterface::WEIGHT_BOLD);
  ASSERT_TRUE(font3 != NULL);
  EXPECT_TRUE(target_->DrawText(0, 60, 100, 30, "hello world", font3,
              Color(0, 0, 1), CanvasInterface::ALIGN_LEFT, 
              CanvasInterface::VALIGN_TOP, 
              CanvasInterface::TRIMMING_NONE, 0));

  FontInterface *font4 = gfx_->NewFont("Serif", 14, 
      FontInterface::STYLE_ITALIC, FontInterface::WEIGHT_NORMAL);
  ASSERT_TRUE(font4 != NULL);
  EXPECT_TRUE(target_->DrawText(0, 90, 100, 30, "hello world", font4, 
              Color(0, 1, 1), CanvasInterface::ALIGN_LEFT, 
              CanvasInterface::VALIGN_TOP, 
              CanvasInterface::TRIMMING_NONE, 0));

  FontInterface *font5 = gfx_->NewFont("Sans Serif", 16, 
      FontInterface::STYLE_NORMAL, FontInterface::WEIGHT_NORMAL);
  ASSERT_TRUE(font5 != NULL);
  EXPECT_TRUE(target_->DrawText(0, 120, 100, 30, "hello world", font5, 
              Color(1, 1, 0), CanvasInterface::ALIGN_LEFT, 
              CanvasInterface::VALIGN_TOP, 
              CanvasInterface::TRIMMING_NONE, 0));

  font1->Destroy();
  font2->Destroy();
  font3->Destroy();
  font4->Destroy();
  font5->Destroy();
}

// this test is meaningful only with -savepng
TEST_F(CairoGfxTest, DrawTextWithTexture) {  
  char *buffer = NULL;
  struct stat statvalue;
  size_t filelen;
  CanvasInterface *img;
  
  int fd = open("kitty419.jpg", O_RDONLY);
  ASSERT_NE(-1, fd);
     
  ASSERT_EQ(0, fstat(fd, &statvalue));    
  filelen = statvalue.st_size;
  ASSERT_NE((size_t)0, filelen);
     
  buffer = (char*)mmap(NULL, filelen, PROT_READ, MAP_PRIVATE, fd, 0);
  ASSERT_NE(MAP_FAILED, buffer);
      
  img = gfx_->NewImage(buffer, filelen);
  ASSERT_FALSE(NULL == img);   
  
  FontInterface *font = gfx_->NewFont("Sans Serif", 20, 
      FontInterface::STYLE_NORMAL, FontInterface::WEIGHT_BOLD);
 
  // test underline, strikeout and wrap
  EXPECT_TRUE(target_->DrawFilledRect(0, 0, 150, 90, Color(.7, 0, 0)));
  EXPECT_TRUE(target_->DrawTextWithTexture(0, 0, 150, 90, 
              "hello world, gooooooogle", 
              font, img, CanvasInterface::ALIGN_LEFT, 
              CanvasInterface::VALIGN_TOP, CanvasInterface::TRIMMING_NONE, 
              CanvasInterface::TEXT_FLAGS_UNDERLINE |
              CanvasInterface::TEXT_FLAGS_WORDWRAP));
  EXPECT_TRUE(target_->DrawFilledRect(0, 100, 150, 50, Color(.7, 0, 0)));  
  EXPECT_TRUE(target_->DrawTextWithTexture(0, 100, 150, 50, "hello world", 
              font, img, CanvasInterface::ALIGN_LEFT, 
              CanvasInterface::VALIGN_TOP, CanvasInterface::TRIMMING_NONE, 
              CanvasInterface::TEXT_FLAGS_UNDERLINE |
              CanvasInterface::TEXT_FLAGS_STRIKEOUT));
 
  // test alignment
  EXPECT_TRUE(target_->DrawFilledRect(180, 0, 120, 60, Color(.7, 0, 0)));
  EXPECT_TRUE(target_->DrawTextWithTexture(180, 0, 120, 60, "hello", font, 
              img, CanvasInterface::ALIGN_CENTER, 
              CanvasInterface::VALIGN_MIDDLE, 
              CanvasInterface::TRIMMING_NONE, 0));
  EXPECT_TRUE(target_->DrawFilledRect(180, 80, 120, 60, Color(.7, 0, 0)));
  EXPECT_TRUE(target_->DrawTextWithTexture(180, 80, 120, 60, "hello", font, 
              img, CanvasInterface::ALIGN_RIGHT, 
              CanvasInterface::VALIGN_BOTTOM, 
              CanvasInterface::TRIMMING_NONE, 0));
   
  img->Destroy();
  img = NULL;
 
  font->Destroy();
  font = NULL;
  
  munmap(buffer, filelen);  
}

// this test is meaningful only with -savepng
TEST_F(CairoGfxTest, TextAttributeAndAlignment) {  
  FontInterface *font5 = gfx_->NewFont("Sans Serif", 16, 
      FontInterface::STYLE_NORMAL, FontInterface::WEIGHT_NORMAL);

  // test underline, strikeout and wrap
  EXPECT_TRUE(target_->DrawFilledRect(0, 0, 100, 110, Color(.1, .1, 0)));
  EXPECT_TRUE(target_->DrawFilledRect(0, 120, 100, 30, Color(.3, .3, .1)));
  EXPECT_TRUE(target_->DrawText(0, 0, 100, 120, "hello world, gooooooogle", 
              font5, Color(1, 1, 0), CanvasInterface::ALIGN_LEFT, 
              CanvasInterface::VALIGN_TOP, CanvasInterface::TRIMMING_NONE, 
              CanvasInterface::TEXT_FLAGS_UNDERLINE |
              CanvasInterface::TEXT_FLAGS_WORDWRAP));

  EXPECT_TRUE(target_->DrawText(0, 120, 100, 30, "hello world", font5,
              Color(1, 1, 0), CanvasInterface::ALIGN_LEFT, 
              CanvasInterface::VALIGN_TOP, CanvasInterface::TRIMMING_NONE, 
              CanvasInterface::TEXT_FLAGS_UNDERLINE |
              CanvasInterface::TEXT_FLAGS_STRIKEOUT));

  // test alignment
  EXPECT_TRUE(target_->DrawFilledRect(200, 0, 100, 60, Color(.1, .1, 0)));
  EXPECT_TRUE(target_->DrawFilledRect(200, 80, 100, 60, Color(.3, .3, .1)));
  EXPECT_TRUE(target_->DrawText(200, 0, 100, 60, "hello", font5, 
              Color(1, 1, 1), CanvasInterface::ALIGN_CENTER, 
              CanvasInterface::VALIGN_MIDDLE, 
              CanvasInterface::TRIMMING_NONE, 0));
  EXPECT_TRUE(target_->DrawText(200, 80, 100, 60, "hello", font5, 
              Color(1, 1, 1), CanvasInterface::ALIGN_RIGHT, 
              CanvasInterface::VALIGN_BOTTOM, 
              CanvasInterface::TRIMMING_NONE, 0));

  font5->Destroy();
}

// this test is meaningful only with -savepng
TEST_F(CairoGfxTest, SinglelineTrimming) {  
  FontInterface *font5 = gfx_->NewFont("Sans Serif", 16, 
      FontInterface::STYLE_NORMAL, FontInterface::WEIGHT_NORMAL);

  EXPECT_TRUE(target_->DrawFilledRect(0, 0, 100, 30, Color(.1, .1, 0)));
  EXPECT_TRUE(target_->DrawFilledRect(200, 0, 100, 30, Color(.1, .1, 0)));
  EXPECT_TRUE(target_->DrawFilledRect(0, 40, 100, 30, Color(.1, .1, 0)));
  EXPECT_TRUE(target_->DrawFilledRect(200, 40, 100, 30, Color(.1, .1, 0)));
  EXPECT_TRUE(target_->DrawFilledRect(0, 80, 100, 30, Color(.1, .1, 0)));
  EXPECT_TRUE(target_->DrawFilledRect(200, 80, 100, 30, Color(.1, .1, 0)));

  EXPECT_TRUE(target_->DrawText(0, 0, 100, 30, "hello world", font5, 
              Color(1, 1, 1), CanvasInterface::ALIGN_CENTER, 
              CanvasInterface::VALIGN_BOTTOM, 
              CanvasInterface::TRIMMING_NONE, 0));
  EXPECT_TRUE(target_->DrawText(0, 40, 100, 30, "hello world", font5, 
              Color(1, 1, 1), CanvasInterface::ALIGN_CENTER, 
              CanvasInterface::VALIGN_BOTTOM, 
              CanvasInterface::TRIMMING_CHARACTER, 0));
  EXPECT_TRUE(target_->DrawText(0, 80, 100, 30, "hello world", font5, 
              Color(1, 1, 1), CanvasInterface::ALIGN_CENTER, 
              CanvasInterface::VALIGN_BOTTOM, 
              CanvasInterface::TRIMMING_CHARACTER_ELLIPSIS, 0));
  EXPECT_TRUE(target_->DrawText(200, 0, 100, 30, "hello world", font5, 
              Color(1, 1, 1), CanvasInterface::ALIGN_CENTER, 
              CanvasInterface::VALIGN_BOTTOM, 
              CanvasInterface::TRIMMING_WORD, 0));
  EXPECT_TRUE(target_->DrawText(200, 40, 100, 30, "hello world", font5, 
              Color(1, 1, 1), CanvasInterface::ALIGN_CENTER, 
              CanvasInterface::VALIGN_BOTTOM, 
              CanvasInterface::TRIMMING_WORD_ELLIPSIS, 0));
  EXPECT_TRUE(target_->DrawText(200, 80, 100, 30, "hello world", font5,
              Color(1, 1, 1), CanvasInterface::ALIGN_CENTER, 
              CanvasInterface::VALIGN_BOTTOM, 
              CanvasInterface::TRIMMING_PATH_ELLIPSIS, 0));

  font5->Destroy();
}

// this test is meaningful only with -savepng
TEST_F(CairoGfxTest, MultilineTrimming) {  
  FontInterface *font5 = gfx_->NewFont("Sans Serif", 16, 
      FontInterface::STYLE_NORMAL, FontInterface::WEIGHT_NORMAL);

  EXPECT_TRUE(target_->DrawFilledRect(0, 0, 100, 40, Color(.1, .1, 0)));
  EXPECT_TRUE(target_->DrawFilledRect(0, 50, 100, 40, Color(.1, .1, 0)));
  EXPECT_TRUE(target_->DrawFilledRect(0, 100, 100, 40, Color(.1, .1, 0)));
  EXPECT_TRUE(target_->DrawFilledRect(200, 0, 100, 40, Color(.1, .1, 0)));
  EXPECT_TRUE(target_->DrawFilledRect(200, 50, 100, 40, Color(.1, .1, 0)));
  EXPECT_TRUE(target_->DrawFilledRect(200, 100, 100, 40, Color(.1, .1, 0)));
  EXPECT_TRUE(target_->DrawText(0, 0, 100, 40, "Hello world, gooooogle", 
              font5, Color(1, 1, 1), CanvasInterface::ALIGN_CENTER, 
              CanvasInterface::VALIGN_MIDDLE, 
              CanvasInterface::TRIMMING_NONE, 
              CanvasInterface::TEXT_FLAGS_WORDWRAP));

  EXPECT_TRUE(target_->DrawText(0, 50, 100, 40, "Hello world, gooooogle", 
              font5, Color(1, 1, 1), CanvasInterface::ALIGN_CENTER, 
              CanvasInterface::VALIGN_MIDDLE, 
              CanvasInterface::TRIMMING_CHARACTER,
              CanvasInterface::TEXT_FLAGS_WORDWRAP));

  EXPECT_TRUE(target_->DrawText(0, 100, 100, 40, "Hello world, gooooogle", 
              font5, Color(1, 1, 1), CanvasInterface::ALIGN_CENTER, 
              CanvasInterface::VALIGN_MIDDLE, 
              CanvasInterface::TRIMMING_CHARACTER_ELLIPSIS, 
              CanvasInterface::TEXT_FLAGS_WORDWRAP));

  EXPECT_TRUE(target_->DrawText(200, 0, 100, 40, "Hello world, gooooogle", 
              font5, Color(1, 1, 1), CanvasInterface::ALIGN_CENTER, 
              CanvasInterface::VALIGN_MIDDLE, CanvasInterface::TRIMMING_WORD, 
              CanvasInterface::TEXT_FLAGS_WORDWRAP));

  EXPECT_TRUE(target_->DrawText(200, 50, 100, 40, "Hello world, gooooogle", 
              font5, Color(1, 1, 1), CanvasInterface::ALIGN_CENTER, 
              CanvasInterface::VALIGN_MIDDLE, 
              CanvasInterface::TRIMMING_WORD_ELLIPSIS, 
              CanvasInterface::TEXT_FLAGS_WORDWRAP));

  EXPECT_TRUE(target_->DrawText(200, 100, 100, 40, "Hello world, gooooogle", 
              font5, Color(1, 1, 1), CanvasInterface::ALIGN_CENTER, 
              CanvasInterface::VALIGN_MIDDLE, 
              CanvasInterface::TRIMMING_PATH_ELLIPSIS, 
              CanvasInterface::TEXT_FLAGS_WORDWRAP));

  font5->Destroy();
}

// this test is meaningful only with -savepng
TEST_F(CairoGfxTest, ChineseTrimming) {  
  FontInterface *font5 = gfx_->NewFont("Sans Serif", 16, 
      FontInterface::STYLE_NORMAL, FontInterface::WEIGHT_NORMAL);

  EXPECT_TRUE(target_->DrawFilledRect(0, 0, 105, 40, Color(.1, .1, 0)));
  EXPECT_TRUE(target_->DrawFilledRect(0, 50, 105, 40, Color(.1, .1, 0)));
  EXPECT_TRUE(target_->DrawFilledRect(0, 100, 105, 40, Color(.1, .1, 0)));
  EXPECT_TRUE(target_->DrawFilledRect(180, 0, 105, 40, Color(.1, .1, 0)));
  EXPECT_TRUE(target_->DrawFilledRect(180, 50, 105, 40, Color(.1, .1, 0)));
  EXPECT_TRUE(target_->DrawFilledRect(180, 100, 105, 40, Color(.1, .1, 0)));
  EXPECT_TRUE(target_->DrawText(0, 0, 105, 40, "你好，谷歌", 
              font5, Color(1, 1, 1), CanvasInterface::ALIGN_CENTER, 
              CanvasInterface::VALIGN_MIDDLE, 
              CanvasInterface::TRIMMING_NONE, 0));

  EXPECT_TRUE(target_->DrawText(0, 50, 105, 40, "你好，谷歌", 
              font5, Color(1, 1, 1), CanvasInterface::ALIGN_CENTER, 
              CanvasInterface::VALIGN_MIDDLE, 
              CanvasInterface::TRIMMING_CHARACTER,0));

  EXPECT_TRUE(target_->DrawText(0, 100, 105, 40, "你好，谷歌", 
              font5, Color(1, 1, 1), CanvasInterface::ALIGN_CENTER, 
              CanvasInterface::VALIGN_MIDDLE, 
              CanvasInterface::TRIMMING_CHARACTER_ELLIPSIS, 0));

  EXPECT_TRUE(target_->DrawText(180, 0, 105, 40, "你好，谷歌", 
              font5, Color(1, 1, 1), CanvasInterface::ALIGN_CENTER, 
              CanvasInterface::VALIGN_MIDDLE, 
              CanvasInterface::TRIMMING_WORD, 0));

  EXPECT_TRUE(target_->DrawText(180, 50, 105, 40, "你好，谷歌", 
              font5, Color(1, 1, 1), CanvasInterface::ALIGN_CENTER, 
              CanvasInterface::VALIGN_MIDDLE, 
              CanvasInterface::TRIMMING_WORD_ELLIPSIS, 0));

  EXPECT_TRUE(target_->DrawText(180, 100, 105, 40, "你好，谷歌", 
              font5, Color(1, 1, 1), CanvasInterface::ALIGN_CENTER, 
              CanvasInterface::VALIGN_MIDDLE, 
              CanvasInterface::TRIMMING_PATH_ELLIPSIS, 0));

  font5->Destroy();
}

// this test is meaningful only with -savepng
TEST_F(CairoGfxTest, RTLTrimming) {  
  FontInterface *font5 = gfx_->NewFont("Sans Serif", 16, 
      FontInterface::STYLE_NORMAL, FontInterface::WEIGHT_NORMAL);

  EXPECT_TRUE(target_->DrawFilledRect(0, 0, 100, 40, Color(.1, .1, 0)));
  EXPECT_TRUE(target_->DrawFilledRect(0, 50, 100, 40, Color(.1, .1, 0)));
  EXPECT_TRUE(target_->DrawFilledRect(0, 100, 100, 40, Color(.1, .1, 0)));
  EXPECT_TRUE(target_->DrawFilledRect(200, 0, 100, 40, Color(.1, .1, 0)));
  EXPECT_TRUE(target_->DrawFilledRect(200, 50, 100, 40, Color(.1, .1, 0)));
  EXPECT_TRUE(target_->DrawFilledRect(200, 100, 100, 40, Color(.1, .1, 0)));
  EXPECT_TRUE(target_->DrawText(0, 0, 100, 40,
              "سَدفهلكجشِلكَفهسدفلكجسدف", 
              font5, Color(1, 1, 1), CanvasInterface::ALIGN_CENTER, 
              CanvasInterface::VALIGN_MIDDLE, 
              CanvasInterface::TRIMMING_NONE, 0));

  EXPECT_TRUE(target_->DrawText(0, 50, 100, 40, 
              "سَدفهلكجشِلكَفهسدفلكجسدف", 
              font5, Color(1, 1, 1), CanvasInterface::ALIGN_CENTER, 
              CanvasInterface::VALIGN_MIDDLE, 
              CanvasInterface::TRIMMING_CHARACTER,0));

  EXPECT_TRUE(target_->DrawText(0, 100, 100, 40, 
              "سَدفهلكجشِلكَفهسدفلكجسدف", 
              font5, Color(1, 1, 1), CanvasInterface::ALIGN_CENTER, 
              CanvasInterface::VALIGN_MIDDLE, 
              CanvasInterface::TRIMMING_CHARACTER_ELLIPSIS, 0));

  EXPECT_TRUE(target_->DrawText(200, 0, 100, 40, 
              "سَدفهلكجشِلكَفهسدفلكجسدف", 
              font5, Color(1, 1, 1), CanvasInterface::ALIGN_CENTER, 
              CanvasInterface::VALIGN_MIDDLE, 
              CanvasInterface::TRIMMING_WORD, 0));

  EXPECT_TRUE(target_->DrawText(200, 50, 100, 40, 
              "سَدفهلكجشِلكَفهسدفلكجسدف", 
              font5, Color(1, 1, 1), CanvasInterface::ALIGN_CENTER, 
              CanvasInterface::VALIGN_MIDDLE, 
              CanvasInterface::TRIMMING_WORD_ELLIPSIS, 0));

  EXPECT_TRUE(target_->DrawText(200, 100, 100, 40, 
              "سَدفهلكجشِلكَفهسدفلكجسدف", 
              font5, Color(1, 1, 1), CanvasInterface::ALIGN_CENTER, 
              CanvasInterface::VALIGN_MIDDLE, 
              CanvasInterface::TRIMMING_PATH_ELLIPSIS, 0));

  font5->Destroy();
}

int main(int argc, char **argv) {
  testing::ParseGUnitFlags(&argc, argv);

  for (int i = 0; i < argc; i++) {
    if (0 == strcasecmp(argv[i], "-savepng")) {
      g_savepng = true;
      break;
    }
  }

  return RUN_ALL_TESTS();
}
