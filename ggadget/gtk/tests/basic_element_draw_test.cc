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

#include "unittest/gunit.h"
#include "ggadget/common.h"
#include "ggadget/basic_element.h"
#include "ggadget/element_factory.h"
#include "ggadget/elements.h"
#include "ggadget/view.h"
#include "ggadget/xml_utils.h"
#include "ggadget/gtk/cairo_graphics.h"
#include "ggadget/gtk/cairo_canvas.h"
#include "ggadget/gtk/main_loop.h"
#include "ggadget/tests/mocked_view_host.h"

using namespace ggadget;
using namespace ggadget::gtk;

ElementFactory *g_factory = NULL;
bool g_savepng = false;
ggadget::gtk::MainLoop g_main_loop;

class ViewHostWithGraphics : public MockedViewHost {
 public:
  ViewHostWithGraphics(ViewHostInterface::Type type)
      : MockedViewHost(type), gfx_(new CairoGraphics(1.0)) {
  }

  virtual ~ViewHostWithGraphics() {
    delete gfx_;
  }

  virtual const GraphicsInterface *GetGraphics() const {
    return gfx_;
  }

 private:
  GraphicsInterface *gfx_;
};

class Muffin : public BasicElement {
 public:
  Muffin(BasicElement *parent, View *view, const char *name)
      : BasicElement(parent, view, "muffin", name, true) {
  }

  virtual ~Muffin() {
  }

  virtual void DoDraw(CanvasInterface *canvas) {
    canvas->DrawFilledRect(0., 0., GetPixelWidth(), GetPixelHeight(),
                           Color(1., 0., 0.));
    DrawChildren(canvas);
  }

  DEFINE_CLASS_ID(0x6c0dee0e5bbe11dc, BasicElement)

  static BasicElement *CreateInstance(BasicElement *parent, View *view,
                                      const char *name) {
    return new Muffin(parent, view, name);
  }
};

class Pie : public BasicElement {
 public:
  Pie(BasicElement *parent, View *view, const char *name)
      : BasicElement(parent, view, "pie", name, false), color_(0., 0., 0.) {
  }

  virtual ~Pie() {
  }

  virtual void SetColor(const Color &c) {
    color_ = c;
  }

  virtual void DoDraw(CanvasInterface *canvas) {
    canvas->DrawFilledRect(0., 0., GetPixelWidth(), GetPixelHeight(), color_);
  }

  DEFINE_CLASS_ID(0x829defac5bbe11dc, BasicElement)

  static BasicElement *CreateInstance(BasicElement *parent, View *view,
                                      const char *name) {
    return new Pie(parent, view, name);
  }

 private:
  Color color_;
};

class BasicElementTest : public testing::Test {
 protected:
  CairoCanvas *target_;
  ViewHostWithGraphics *view_host_;

  BasicElementTest() {
    view_host_ = new ViewHostWithGraphics(ViewHostInterface::VIEW_HOST_MAIN);
    target_ =
        down_cast<CairoCanvas*>(view_host_->GetGraphics()->NewCanvas(300, 150));
  }

  ~BasicElementTest() {
    if (g_savepng) {
      const testing::TestInfo *const test_info =
        testing::UnitTest::GetInstance()->current_test_info();
      char file[100];
      snprintf(file, arraysize(file), "%s.png", test_info->name());
      cairo_surface_write_to_png(target_->GetSurface(), file);
    }

    target_->Destroy();
    delete view_host_;
  }

  virtual void SetUp() {
  }

  virtual void TearDown() {
  }
};

// This test is meaningful only with -savepng
TEST_F(BasicElementTest, ElementsDraw) {
  View view(view_host_, NULL, g_factory, NULL);
  Muffin m(NULL, &view, NULL);
  Pie *p = NULL;

  m.SetPixelWidth(200.);
  m.SetPixelHeight(100.);

  m.GetChildren()->AppendElement("pie", NULL);
  p = down_cast<Pie*>(m.GetChildren()->GetItemByIndex(0));
  p->SetColor(Color(1., 1., 1.));
  p->SetPixelWidth(100.);
  p->SetPixelHeight(50.);
  p->SetPixelX(100.);
  p->SetPixelY(50.);
  p->SetOpacity(.8);
  p->SetPixelPinX(50.);
  p->SetPixelPinY(25.);

  m.GetChildren()->AppendElement("pie", NULL);
  p = down_cast<Pie*>(m.GetChildren()->GetItemByIndex(1));
  p->SetColor(Color(0., 1., 0.));
  p->SetPixelWidth(100.);
  p->SetPixelHeight(50.);
  p->SetPixelX(100.);
  p->SetPixelY(50.);
  p->SetOpacity(.5);
  p->SetRotation(90.);
  p->SetPixelPinX(50.);
  p->SetPixelPinY(25.);

  m.GetChildren()->AppendElement("pie", NULL);
  p = down_cast<Pie*>(m.GetChildren()->GetItemByIndex(2));
  p->SetColor(Color(0., 0., 1.));
  p->SetPixelWidth(100.);
  p->SetPixelHeight(50.);
  p->SetPixelX(100.);
  p->SetPixelY(50.);
  p->SetOpacity(.5);
  p->SetRotation(60.);
  p->SetPixelPinX(50.);
  p->SetPixelPinY(25.);

  m.GetChildren()->AppendElement("pie", NULL);
  p = down_cast<Pie*>(m.GetChildren()->GetItemByIndex(3));
  p->SetColor(Color(0., 1., 1.));
  p->SetPixelWidth(100.);
  p->SetPixelHeight(50.);
  p->SetPixelX(100.);
  p->SetPixelY(50.);
  p->SetOpacity(.5);
  p->SetRotation(30.);
  p->SetPixelPinX(50.);
  p->SetPixelPinY(25.);

  CanvasInterface *canvas =
    view_host_->GetGraphics()->NewCanvas(m.GetPixelWidth(),
                                         m.GetPixelHeight());
  ASSERT_TRUE(canvas != NULL);
  m.Draw(canvas);

  EXPECT_TRUE(target_->DrawCanvas(10, 10, canvas));

  canvas->Destroy();
  canvas = NULL;
}

int main(int argc, char *argv[]) {
  testing::ParseGUnitFlags(&argc, argv);
  for (int i = 0; i < argc; i++) {
    if (0 == strcasecmp(argv[i], "-savepng")) {
      g_savepng = true;
      break;
    }
  }

  SetGlobalMainLoop(&g_main_loop);

  g_factory = new ElementFactory();
  g_factory->RegisterElementClass("muffin", Muffin::CreateInstance);
  g_factory->RegisterElementClass("pie", Pie::CreateInstance);
  int result = RUN_ALL_TESTS();
  delete g_factory;
  return result;
}
