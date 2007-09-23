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
#include "ggadget/xml_utils.h"
#include "ggadget/graphics/cairo_graphics.h"
#include "ggadget/graphics/cairo_canvas.h"
#include "ggadget/tests/mocked_view.h"

using namespace ggadget;

ElementFactory *gFactory = NULL;
bool g_savepng = false;

class ViewWithGraphics : public MockedView {
 public:
   ViewWithGraphics() : MockedView(gFactory), gfx_(new CairoGraphics(1.0)) {
   }
   
   ~ViewWithGraphics() {
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
  Muffin(ElementInterface *parent,
         ViewInterface *view,
         const char *name)
      : BasicElement(parent, view, name, true) {
  }

  virtual ~Muffin() {
  }

  virtual const char *GetTagName() const {
    return "muffin";
  }
  
  virtual const CanvasInterface *Draw(bool *changed) {
    *changed = true;
    SetUpCanvas();

    CanvasInterface *canvas = GetCanvas();

    canvas->MultiplyOpacity(GetOpacity());
    canvas->DrawFilledRect(0., 0., GetPixelWidth(), GetPixelHeight(), 
                           Color(1., 0., 0.));
    
    bool c;
    const CanvasInterface *child_canvas = GetChildren()->Draw(&c);
    if (child_canvas) {
      canvas->DrawCanvas(0., 0., child_canvas);
    }
    
    DrawBoundingBox();
    
    return canvas;
  }
  
  DEFINE_CLASS_ID(0x6c0dee0e5bbe11dc, BasicElement)
  
  static ElementInterface *CreateInstance(
      ElementInterface *parent,
      ViewInterface *view,
      const char *name) {
    return new Muffin(parent, view, name);
  }
};

class Pie : public BasicElement {
 public:
  Pie(ElementInterface *parent,
      ViewInterface *view,
      const char *name)
      : BasicElement(parent, view, name, false), color_(0., 0., 0.) {
  }

  virtual ~Pie() {
  }

  virtual const char *GetTagName() const {
    return "pie";
  }
  
  virtual void SetColor(const Color &c) {
    color_ = c;
  }
  
  virtual const CanvasInterface *Draw(bool *changed) {
    *changed = true;    
    SetUpCanvas();
    
    CanvasInterface *canvas = GetCanvas();
    canvas->MultiplyOpacity(GetOpacity());
    canvas->DrawFilledRect(0., 0., GetPixelWidth(), GetPixelHeight(), color_);
    DrawBoundingBox();

    return canvas;
  }
   
  DEFINE_CLASS_ID(0x829defac5bbe11dc, BasicElement)

  static ElementInterface *CreateInstance(
      ElementInterface *parent,
      ViewInterface *view,
      const char *name) {
    return new Pie(parent, view, name);
  }
  
 private:
  Color color_;
};

class BasicElementTest : public testing::Test {
 protected:
  CanvasInterface *target_;
  cairo_surface_t *surface_;
  ViewInterface *view_;
   
  BasicElementTest() {
    // create a target canvas for tests
    surface_ = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 300, 150);    
    cairo_t *cr = cairo_create(surface_);
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
    cairo_set_source_rgba(cr, 0, 0, 0, 0);
    cairo_paint(cr);
    target_ = new CairoCanvas(cr, 300, 150, false);
    cairo_destroy(cr);    
    cr = NULL;
    
    view_ = new ViewWithGraphics();
  }
   
  ~BasicElementTest() {
    delete view_;
    view_ = NULL;
    
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
  
  virtual void SetUp() {
  }

  virtual void TearDown() {
  }
};

// This test is meaningful only with -savepng
TEST_F(BasicElementTest, ElementsDraw) {
  Muffin m(NULL, view_, NULL);
  Pie *p = NULL;
  
  m.SetPixelWidth(200.);
  m.SetPixelHeight(100.);
  
  m.GetChildren()->AppendElement("pie", NULL);    
  p = down_cast<Pie*>(m.GetChildren()->GetItemByIndex(0));
  p->SetColor(Color(1., 1., 1.));
  p->SetPixelWidth(100.);
  p->SetPixelHeight(50.);
  p->SetPixelX(50.);
  p->SetPixelY(25.);
  p->SetOpacity(.8);
  p->SetPixelPinX(50.);
  p->SetPixelPinY(25.);
  
  m.GetChildren()->AppendElement("pie", NULL);    
  p = down_cast<Pie*>(m.GetChildren()->GetItemByIndex(1));
  p->SetColor(Color(0., 1., 0.));
  p->SetPixelWidth(100.);
  p->SetPixelHeight(50.);
  p->SetPixelX(50.);
  p->SetPixelY(25.);
  p->SetOpacity(.5);
  p->SetRotation(90.);
  p->SetPixelPinX(50.);
  p->SetPixelPinY(25.);
  
  m.GetChildren()->AppendElement("pie", NULL);    
  p = down_cast<Pie*>(m.GetChildren()->GetItemByIndex(2));
  p->SetColor(Color(0., 0., 1.));
  p->SetPixelWidth(100.);
  p->SetPixelHeight(50.);
  p->SetPixelX(50.);
  p->SetPixelY(25.);
  p->SetOpacity(.5);
  p->SetRotation(60.);
  p->SetPixelPinX(50.);
  p->SetPixelPinY(25.);
  
  m.GetChildren()->AppendElement("pie", NULL);    
  p = down_cast<Pie*>(m.GetChildren()->GetItemByIndex(3));
  p->SetColor(Color(0., 1., 1.));
  p->SetPixelWidth(100.);
  p->SetPixelHeight(50.);
  p->SetPixelX(50.);
  p->SetPixelY(25.);
  p->SetOpacity(.5);
  p->SetRotation(30.);
  p->SetPixelPinX(50.);
  p->SetPixelPinY(25.);
  
  bool changed = false;
  const CanvasInterface *canvas = m.Draw(&changed);
  ASSERT_TRUE(canvas != NULL);
  EXPECT_TRUE(changed);
  
  EXPECT_TRUE(target_->DrawCanvas(10, 10, canvas));
}
 
int main(int argc, char *argv[]) {
  testing::ParseGUnitFlags(&argc, argv);  
  for (int i = 0; i < argc; i++) {
    if (0 == strcasecmp(argv[i], "-savepng")) {
      g_savepng = true;
      break;
    }
  }
  
  gFactory = new ElementFactory();
  gFactory->RegisterElementClass("muffin", Muffin::CreateInstance);
  gFactory->RegisterElementClass("pie", Pie::CreateInstance);
  int result = RUN_ALL_TESTS();
  delete gFactory;
  return result;
}
