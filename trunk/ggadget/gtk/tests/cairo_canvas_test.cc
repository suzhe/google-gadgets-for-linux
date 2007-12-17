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
#include <strings.h>

#include "ggadget/common.h"
#include "ggadget/canvas_interface.h"
#include "ggadget/gtk/cairo_canvas.h"
#include "unittest/gunit.h"

using namespace ggadget;
using namespace ggadget::gtk;

const double kPi = 3.141592653589793;
bool g_savepng = false;

// fixture for creating the CairoCanvas object
class CairoCanvasTest : public testing::Test {
 protected:
  CanvasInterface *gfx_;
  cairo_surface_t *surface_;

  CairoCanvasTest() {
    surface_ = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 300, 150);
    cairo_t *cr = cairo_create(surface_);
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
    cairo_set_source_rgba(cr, 0., 0., 0., 0.);
    cairo_paint(cr);
    gfx_ = new CairoCanvas(cr, 300, 150, false);
    cairo_destroy(cr);
    cr = NULL;
  }

  ~CairoCanvasTest() {
    gfx_->Destroy();
    gfx_ = NULL;

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

TEST_F(CairoCanvasTest, PushPopStateReturnValues) {
  EXPECT_FALSE(gfx_->PopState());

  // push 1x, pop 1x
  EXPECT_TRUE(gfx_->PushState());
  EXPECT_TRUE(gfx_->PopState());
  EXPECT_FALSE(gfx_->PopState());

  // push 3x, pop 3x
  EXPECT_TRUE(gfx_->PushState());
  EXPECT_TRUE(gfx_->PushState());
  EXPECT_TRUE(gfx_->PushState());
  EXPECT_TRUE(gfx_->PopState());
  EXPECT_TRUE(gfx_->PopState());
  EXPECT_TRUE(gfx_->PopState());
  EXPECT_FALSE(gfx_->PopState());

  EXPECT_FALSE(gfx_->PopState());
}

TEST_F(CairoCanvasTest, OpacityReturnValues) {
  EXPECT_FALSE(gfx_->MultiplyOpacity(1.7));
  EXPECT_TRUE(gfx_->MultiplyOpacity(.5));
  EXPECT_FALSE(gfx_->MultiplyOpacity(-.7));
  EXPECT_TRUE(gfx_->MultiplyOpacity(.7));
  EXPECT_FALSE(gfx_->MultiplyOpacity(1000.));
  EXPECT_TRUE(gfx_->MultiplyOpacity(.2));
}

TEST_F(CairoCanvasTest, DrawLines) {
  EXPECT_FALSE(gfx_->DrawLine(10., 10., 200., 20., -1., Color(1., 0., 0.)));
  EXPECT_TRUE(gfx_->DrawLine(10., 10., 200., 20., 1., Color(1., 0., 0.)));
  EXPECT_TRUE(gfx_->DrawLine(10., 30., 200., 30., 2., Color(0., 1., 0.)));
  EXPECT_TRUE(gfx_->DrawLine(10., 40., 200., 40., 1.5, Color(0., 0., 1.)));
  EXPECT_TRUE(gfx_->DrawLine(10., 50., 200., 50., 1., Color(0., 0., 0.)));
  EXPECT_TRUE(gfx_->DrawLine(10., 60., 200., 60., 4., Color(1., 1., 1.)));
}

TEST_F(CairoCanvasTest, DrawRectReturnValues) {
  EXPECT_FALSE(gfx_->DrawFilledRect(5., 6., -1., 5., Color(0., 0., 0.)));
  EXPECT_TRUE(gfx_->DrawFilledRect(5., 6., 1., 5., Color(0., 0., 0.)));
  EXPECT_FALSE(gfx_->DrawFilledRect(5., 6., 1., -5., Color(0., 0., 0.)));
}

TEST_F(CairoCanvasTest, ClipRectReturnValues) {
  EXPECT_FALSE(gfx_->IntersectRectClipRegion(5., 6., -1., 5.));
  EXPECT_TRUE(gfx_->IntersectRectClipRegion(5., 6., 1., 5.));
  EXPECT_FALSE(gfx_->IntersectRectClipRegion(5., 6., 1., -5.));
}

// this test is meaningful only with -savepng
TEST_F(CairoCanvasTest, PushPopStateLines) {
  // should show up as 1.0
  EXPECT_TRUE(gfx_->DrawLine(10., 10., 200., 10., 10., Color(1., 0., 0.)));
  EXPECT_TRUE(gfx_->MultiplyOpacity(1.0));
  // should show up as 1.0
  EXPECT_TRUE(gfx_->DrawLine(10., 30., 200., 30., 10., Color(1., 0., 0.)));
  EXPECT_TRUE(gfx_->PushState());
  EXPECT_TRUE(gfx_->MultiplyOpacity(.5));
  // should show up as .5
  EXPECT_TRUE(gfx_->DrawLine(10., 50., 200., 50., 10., Color(1., 0., 0.)));
  EXPECT_TRUE(gfx_->PopState());
  // should show up as 1.0
  EXPECT_TRUE(gfx_->DrawLine(10., 70., 200., 70., 10., Color(1., 0., 0.)));
  EXPECT_TRUE(gfx_->MultiplyOpacity(.5));
  // should show up as .5
  EXPECT_TRUE(gfx_->DrawLine(10., 90., 200., 90., 10., Color(1., 0., 0.)));
  EXPECT_TRUE(gfx_->MultiplyOpacity(.5));
  // should show up as .25
  EXPECT_TRUE(gfx_->DrawLine(10., 110., 200., 110., 10., Color(1., 0., 0.)));
}

// this test is meaningful only with -savepng
TEST_F(CairoCanvasTest, Transformations) {
  // rotation
  EXPECT_TRUE(gfx_->DrawLine(10., 10., 200., 10., 10., Color(0., 1., 0.)));
  EXPECT_TRUE(gfx_->PushState());
  gfx_->RotateCoordinates(kPi/6);
  EXPECT_TRUE(gfx_->DrawLine(10., 10., 200., 10., 10., Color(0., 1., 0.)));
  EXPECT_TRUE(gfx_->PopState());

  EXPECT_TRUE(gfx_->MultiplyOpacity(.5));
  EXPECT_TRUE(gfx_->PushState());

  // scale
  EXPECT_TRUE(gfx_->DrawLine(10., 50., 200., 50., 10., Color(1., 0., 0.)));
  gfx_->ScaleCoordinates(1.3, 1.5);
  EXPECT_TRUE(gfx_->DrawLine(10., 50., 200., 50., 10., Color(1., 0., 0.)));
  EXPECT_TRUE(gfx_->PopState());

  // translation
  EXPECT_TRUE(gfx_->DrawLine(10., 110., 200., 110., 10., Color(0., 0., 1.)));
  gfx_->TranslateCoordinates(20., 25.);
  EXPECT_TRUE(gfx_->DrawLine(10., 110., 200., 110., 10., Color(0., 0., 1.)));
}

// this test is meaningful only with -savepng
TEST_F(CairoCanvasTest, FillRectAndClipping) {
  EXPECT_TRUE(gfx_->MultiplyOpacity(.5));
  EXPECT_TRUE(gfx_->PushState());
  EXPECT_TRUE(gfx_->DrawFilledRect(10., 10., 280., 130., Color(1., 0., 0.)));
  EXPECT_TRUE(gfx_->IntersectRectClipRegion(30., 30., 100., 100.));
  EXPECT_TRUE(gfx_->IntersectRectClipRegion(70., 40., 100., 70.));
  EXPECT_TRUE(gfx_->DrawFilledRect(20., 20., 260., 110., Color(0., 1., 0.)));
  EXPECT_TRUE(gfx_->PopState());
  EXPECT_TRUE(gfx_->DrawFilledRect(110., 40., 90., 70., Color(0., 0., 1.)));
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
