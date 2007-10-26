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
#include "ggadget/basic_element.h"
#include "ggadget/element_factory.h"
#include "ggadget/elements.h"
#include "ggadget/xml_utils.h"
#include "mocked_view.h"

ggadget::ElementFactory *gFactory = NULL;

class Muffin : public ggadget::BasicElement {
 public:
  Muffin(ggadget::ElementInterface *parent,
         ggadget::ViewInterface *view,
         const char *name)
      : ggadget::BasicElement(parent, view, "muffin", name, true) {
  }

  virtual ~Muffin() {
  }

  virtual void DoDraw(ggadget::CanvasInterface *canvas,
                      const ggadget::CanvasInterface *children_canvas) { }

 public:
  DEFINE_CLASS_ID(0x6c0dee0e5bbe11dc, ggadget::BasicElement)

 public:
  static ggadget::ElementInterface *CreateInstance(
      ggadget::ElementInterface *parent,
      ggadget::ViewInterface *view,
      const char *name) {
    return new Muffin(parent, view, name);
  }
};

class Pie : public ggadget::BasicElement {
 public:
  Pie(ggadget::ElementInterface *parent,
      ggadget::ViewInterface *view,
      const char *name)
      : ggadget::BasicElement(parent, view, "pie", name, true) {
  }

  virtual ~Pie() {
  }

  virtual void DoDraw(ggadget::CanvasInterface *canvas,
                      const ggadget::CanvasInterface *children_canvas) { }

 public:
  DEFINE_CLASS_ID(0x829defac5bbe11dc, ggadget::BasicElement)

 public:
  static ggadget::ElementInterface *CreateInstance(
      ggadget::ElementInterface *parent,
      ggadget::ViewInterface *view,
      const char *name) {
    return new Pie(parent, view, name);
  }
};

class BasicElementTest : public testing::Test {
 protected:
  virtual void SetUp() {
  }

  virtual void TearDown() {
  }
};

TEST_F(BasicElementTest, TestCreate) {
  MockedView v(gFactory);
  Muffin m(NULL, &v, NULL);
  Pie p(NULL, &v, NULL);
}

TEST_F(BasicElementTest, TestChildren) {
  MockedView v(gFactory);
  Muffin m(NULL, &v, NULL);
  ggadget::ElementInterface *c1 = 
      m.GetChildren()->AppendElement("muffin", NULL);
  ggadget::ElementInterface *c2 =
      m.GetChildren()->InsertElement("pie", c1, "First");
  ASSERT_EQ(2, m.GetChildren()->GetCount());
  ASSERT_TRUE(m.GetChildren()->GetItemByIndex(0) == c2);
  ASSERT_TRUE(m.GetChildren()->GetItemByIndex(1) == c1);
  ASSERT_TRUE(m.GetChildren()->GetItemByName("First") == c2);
  m.GetChildren()->RemoveElement(c2);
  ASSERT_EQ(1, m.GetChildren()->GetCount());
  ASSERT_TRUE(m.GetChildren()->GetItemByIndex(0) == c1);
  m.GetChildren()->RemoveAllElements();
  ASSERT_EQ(0, m.GetChildren()->GetCount());
}

TEST_F(BasicElementTest, TestCursor) {
  MockedView v(gFactory);
  Muffin m(NULL, &v, NULL);
  ASSERT_TRUE(m.GetCursor() == ggadget::ElementInterface::CURSOR_ARROW);
  m.SetCursor(ggadget::ElementInterface::CURSOR_BUSY);
  ASSERT_TRUE(m.GetCursor() == ggadget::ElementInterface::CURSOR_BUSY);
}

TEST_F(BasicElementTest, TestDropTarget) {
  MockedView v(gFactory);
  Muffin m(NULL, &v, NULL);
  ASSERT_FALSE(m.IsDropTarget());
  m.SetDropTarget(true);
  ASSERT_TRUE(m.IsDropTarget());
}

TEST_F(BasicElementTest, TestEnabled) {
  MockedView v(gFactory);
  Muffin m(NULL, &v, NULL);
  ASSERT_FALSE(m.IsEnabled());
  m.SetEnabled(true);
  ASSERT_TRUE(m.IsEnabled());
}

TEST_F(BasicElementTest, TestPixelHeight) {
  MockedView v(gFactory);
  ASSERT_FALSE(v.GetQueuedDraw());
  Muffin m(NULL, &v, NULL);
  ASSERT_FLOAT_EQ(0.0, m.GetPixelHeight());
  m.SetPixelHeight(100.0);
  ASSERT_TRUE(v.GetQueuedDraw());
  ASSERT_FALSE(v.GetQueuedDraw());
  ASSERT_FLOAT_EQ(100.0, m.GetPixelHeight());
  // Setting the height as negative value will make no effect.
  m.SetPixelHeight(-100.0);
  ASSERT_FALSE(v.GetQueuedDraw());
  ASSERT_FLOAT_EQ(100.0, m.GetPixelHeight());
  ggadget::ElementInterface *c = m.GetChildren()->AppendElement("pie", NULL);
  c->SetPixelHeight(50.0);
  ASSERT_TRUE(v.GetQueuedDraw());
  // Modifying the height of the parent will not effect the child.
  m.SetPixelHeight(150.0);
  ASSERT_TRUE(v.GetQueuedDraw());
  ASSERT_FLOAT_EQ(50.0, c->GetPixelHeight());
}

TEST_F(BasicElementTest, TestRelativeHeight) {
  MockedView v(gFactory);
  Muffin m(NULL, &v, NULL);
  m.SetRelativeHeight(0.50);
  ASSERT_TRUE(v.GetQueuedDraw());
  ASSERT_FLOAT_EQ(0.50, m.GetRelativeHeight());
  ASSERT_FLOAT_EQ(150.0, m.GetPixelHeight());
  ggadget::ElementInterface *c = m.GetChildren()->AppendElement("pie", NULL);
  c->SetRelativeHeight(0.50);
  ASSERT_TRUE(v.GetQueuedDraw());
  ASSERT_FLOAT_EQ(0.50, c->GetRelativeHeight());
  ASSERT_FLOAT_EQ(75.0, c->GetPixelHeight());
  // Setting the height as negative value will make no effect.
  c->SetRelativeHeight(-0.50);
  ASSERT_FALSE(v.GetQueuedDraw());
  ASSERT_FLOAT_EQ(0.50, c->GetRelativeHeight());
  ASSERT_FLOAT_EQ(75.0, c->GetPixelHeight());
  // Modifying the height of the parent will effect the child.
  m.SetRelativeHeight(1.0);
  ASSERT_TRUE(v.GetQueuedDraw());
  ASSERT_FLOAT_EQ(0.50, c->GetRelativeHeight());
  ASSERT_FLOAT_EQ(150.0, c->GetPixelHeight());
  // Modifying the height of the parent will effect the child.
  m.SetPixelHeight(100.0);
  ASSERT_TRUE(v.GetQueuedDraw());
  ASSERT_FLOAT_EQ(50.0, c->GetPixelHeight());
}

TEST_F(BasicElementTest, TestHitTest) {
  MockedView v(gFactory);
  Muffin m(NULL, &v, NULL);
  ASSERT_TRUE(m.GetHitTest() == ggadget::ElementInterface::HT_DEFAULT);
  m.SetHitTest(ggadget::ElementInterface::HT_CLIENT);
  ASSERT_TRUE(m.GetHitTest() == ggadget::ElementInterface::HT_CLIENT);
}

TEST_F(BasicElementTest, TestMask) {
  MockedView v(gFactory);
  Muffin m(NULL, &v, NULL);
  ASSERT_STREQ("", m.GetMask());
  m.SetMask("mymask.png");
  ASSERT_STREQ("mymask.png", m.GetMask());
  m.SetMask(NULL);
  ASSERT_STREQ("", m.GetMask());
}

TEST_F(BasicElementTest, TestName) {
  MockedView view(gFactory);
  Muffin m(NULL, &view, "mymuffin");
  ASSERT_STREQ("mymuffin", m.GetName());
}

TEST_F(BasicElementTest, TestConst) {
  MockedView v(gFactory);
  Muffin m(NULL, &v, NULL);
  ggadget::ElementInterface *c = m.GetChildren()->AppendElement("pie", NULL);
  const ggadget::ElementInterface *cc = c;
  ASSERT_TRUE(cc->GetView() == &v);
  ASSERT_TRUE(cc->GetParentElement() == &m);
}

TEST_F(BasicElementTest, TestOpacity) {
  MockedView v(gFactory);
  Muffin m(NULL, &v, NULL);
  ASSERT_FLOAT_EQ(1.0, m.GetOpacity());
  m.SetOpacity(0.5);
  ASSERT_TRUE(v.GetQueuedDraw());
  ASSERT_FLOAT_EQ(0.5, m.GetOpacity());
  // Setting the value greater than 1 will make no effect.
  m.SetOpacity(1.5);
  ASSERT_FALSE(v.GetQueuedDraw());
  ASSERT_FLOAT_EQ(0.5, m.GetOpacity());
  // Setting the value less than 0 will make no effect.
  m.SetOpacity(-0.5);
  ASSERT_FALSE(v.GetQueuedDraw());
  ASSERT_FLOAT_EQ(0.5, m.GetOpacity());
}

TEST_F(BasicElementTest, TestPixelPinX) {
  MockedView v(gFactory);
  Muffin m(NULL, &v, NULL);
  ASSERT_FLOAT_EQ(0.0, m.GetPixelPinX());
  m.SetPixelPinX(100.5);
  ASSERT_TRUE(v.GetQueuedDraw());
  ASSERT_FLOAT_EQ(100.5, m.GetPixelPinX());
  // Modifying the width of the parent will not effect the pin x.
  m.SetPixelWidth(150.5);
  ASSERT_TRUE(v.GetQueuedDraw());
  ASSERT_FLOAT_EQ(100.5, m.GetPixelPinX());
  ASSERT_FALSE(m.PinXIsRelative());
  m.SetPixelPinX(-50.5);
  ASSERT_TRUE(v.GetQueuedDraw());
  ASSERT_FLOAT_EQ(-50.5, m.GetPixelPinX());
}

TEST_F(BasicElementTest, TestRelativePinX) {
  MockedView v(gFactory);
  Muffin m(NULL, &v, NULL);
  m.SetPixelWidth(200.0);
  ASSERT_TRUE(v.GetQueuedDraw());
  m.SetRelativePinX(0.5);
  ASSERT_TRUE(v.GetQueuedDraw());
  ASSERT_FLOAT_EQ(100.0, m.GetPixelPinX());
  // Modifying the width will effect the pin x.
  m.SetPixelWidth(400.0);
  ASSERT_FLOAT_EQ(200.0, m.GetPixelPinX());
  ASSERT_TRUE(m.PinXIsRelative());
  m.SetRelativePinX(-0.25);
  ASSERT_TRUE(v.GetQueuedDraw());
  ASSERT_FLOAT_EQ(-100.0, m.GetPixelPinX());
}

TEST_F(BasicElementTest, TestPixelPinY) {
  MockedView v(gFactory);
  Muffin m(NULL, &v, NULL);
  m.SetPixelHeight(150.5);
  ASSERT_TRUE(v.GetQueuedDraw());
  m.SetPixelPinY(100.5);
  ASSERT_TRUE(v.GetQueuedDraw());
  ASSERT_FLOAT_EQ(100.5, m.GetPixelPinY());
  // Modifying the width will not effect the pin y.
  m.SetPixelHeight(300.5);
  ASSERT_TRUE(v.GetQueuedDraw());
  ASSERT_FLOAT_EQ(100.5, m.GetPixelPinY());
  ASSERT_FALSE(m.PinYIsRelative());
  m.SetPixelPinY(-50.5);
  ASSERT_TRUE(v.GetQueuedDraw());
  ASSERT_FLOAT_EQ(-50.5, m.GetPixelPinY());
}

TEST_F(BasicElementTest, TestRelativePinY) {
  MockedView v(gFactory);
  Muffin m(NULL, &v, NULL);
  m.SetPixelHeight(150.0);
  ASSERT_TRUE(v.GetQueuedDraw());
  m.SetRelativePinY(0.5);
  ASSERT_TRUE(v.GetQueuedDraw());
  ASSERT_FLOAT_EQ(75.0, m.GetPixelPinY());
  // Modifying the width of the parent will not effect the pin y.
  m.SetPixelHeight(300.0);
  ASSERT_TRUE(v.GetQueuedDraw());
  ASSERT_FLOAT_EQ(150.0, m.GetPixelPinY());
  ASSERT_TRUE(m.PinYIsRelative());
  m.SetRelativePinY(-0.25);
  ASSERT_TRUE(v.GetQueuedDraw());
  ASSERT_FLOAT_EQ(-75.0, m.GetPixelPinY());
}

TEST_F(BasicElementTest, TestRotation) {
  MockedView v(gFactory);
  Muffin m(NULL, &v, NULL);
  ASSERT_FLOAT_EQ(0.0, m.GetRotation());
  m.SetRotation(0.5);
  ASSERT_TRUE(v.GetQueuedDraw());
  ASSERT_FLOAT_EQ(0.5, m.GetRotation());
}

TEST_F(BasicElementTest, TestTooltip) {
  MockedView v(gFactory);
  Muffin m(NULL, &v, NULL);
  ASSERT_STREQ("", m.GetTooltip());
  m.SetTooltip("mytooltip");
  ASSERT_STREQ("mytooltip", m.GetTooltip());
  m.SetTooltip(NULL);
  ASSERT_STREQ("", m.GetTooltip());
}

TEST_F(BasicElementTest, TestPixelWidth) {
  MockedView v(gFactory);
  Muffin m(NULL, &v, NULL);
  ASSERT_FLOAT_EQ(0.0, m.GetPixelWidth());
  m.SetPixelWidth(100.0);
  ASSERT_TRUE(v.GetQueuedDraw());
  ASSERT_FLOAT_EQ(100.0, m.GetPixelWidth());
  // Setting the width as negative value will make no effect.
  m.SetPixelWidth(-100.0);
  ASSERT_FALSE(v.GetQueuedDraw());
  ASSERT_FLOAT_EQ(100.0, m.GetPixelWidth());
  ggadget::ElementInterface *c = m.GetChildren()->AppendElement("pie", NULL);
  c->SetPixelWidth(50.0);
  // Modifying the width of the parent will not effect the child.
  m.SetPixelWidth(200.0);
  ASSERT_TRUE(v.GetQueuedDraw());
  ASSERT_FLOAT_EQ(50.0, c->GetPixelWidth());
}

TEST_F(BasicElementTest, TestRelativeWidth) {
  MockedView v(gFactory);
  Muffin m(NULL, &v, NULL);
  m.SetRelativeWidth(0.50);
  ASSERT_TRUE(v.GetQueuedDraw());
  ASSERT_FLOAT_EQ(0.50, m.GetRelativeWidth());
  ASSERT_FLOAT_EQ(200.0, m.GetPixelWidth());
  ggadget::ElementInterface *c = m.GetChildren()->AppendElement("pie", NULL);
  c->SetRelativeWidth(0.50);
  ASSERT_TRUE(v.GetQueuedDraw());
  ASSERT_FLOAT_EQ(0.50, c->GetRelativeWidth());
  ASSERT_FLOAT_EQ(100.0, c->GetPixelWidth());
  // Setting the width as negative value will make no effect.
  c->SetRelativeWidth(-0.50);
  ASSERT_FALSE(v.GetQueuedDraw());
  ASSERT_FLOAT_EQ(0.50, c->GetRelativeWidth());
  ASSERT_FLOAT_EQ(100.0, c->GetPixelWidth());
  // Modifying the width of the parent will effect the child.
  m.SetRelativeWidth(1.0);
  ASSERT_TRUE(v.GetQueuedDraw());
  ASSERT_FLOAT_EQ(0.50, c->GetRelativeWidth());
  ASSERT_FLOAT_EQ(200.0, c->GetPixelWidth());
  // Modifying the width of the parent will effect the child.
  m.SetPixelWidth(150.0);
  ASSERT_TRUE(v.GetQueuedDraw());
  ASSERT_FLOAT_EQ(75.0, c->GetPixelWidth());
}

TEST_F(BasicElementTest, TestVisible) {
  MockedView v(gFactory);
  Muffin m(NULL, &v, NULL);
  ASSERT_TRUE(m.IsVisible());
  m.SetVisible(false);
  ASSERT_FALSE(m.IsVisible());
}

TEST_F(BasicElementTest, TestPixelX) {
  MockedView v(gFactory);
  Muffin m(NULL, &v, NULL);
  ASSERT_FLOAT_EQ(0.0, m.GetPixelX());
  m.SetPixelX(100.0);
  ASSERT_TRUE(v.GetQueuedDraw());
  ASSERT_FLOAT_EQ(100.0, m.GetPixelX());
  ggadget::ElementInterface *c = m.GetChildren()->AppendElement("pie", NULL);
  c->SetPixelX(50.0);
  ASSERT_TRUE(v.GetQueuedDraw());
  // Modifying the width of the parent will not effect the child.
  m.SetPixelWidth(150.0);
  ASSERT_TRUE(v.GetQueuedDraw());
  ASSERT_FLOAT_EQ(50.0, c->GetPixelX());
  m.SetPixelX(-50.5);
  ASSERT_TRUE(v.GetQueuedDraw());
  ASSERT_FLOAT_EQ(-50.5, m.GetPixelX());
}

TEST_F(BasicElementTest, TestRelativeX) {
  MockedView v(gFactory);
  Muffin m(NULL, &v, NULL);
  m.SetRelativeWidth(0.5);
  ASSERT_TRUE(v.GetQueuedDraw());
  m.SetRelativeX(0.5);
  ASSERT_TRUE(v.GetQueuedDraw());
  ASSERT_FLOAT_EQ(200.0, m.GetPixelX());
  ggadget::ElementInterface *c = m.GetChildren()->AppendElement("pie", NULL);
  c->SetRelativeX(0.50);
  ASSERT_TRUE(v.GetQueuedDraw());
  ASSERT_FLOAT_EQ(100.0, c->GetPixelX());
  // Modifying the width of the parent will effect the child.
  m.SetPixelWidth(100.0);
  ASSERT_TRUE(v.GetQueuedDraw());
  ASSERT_FLOAT_EQ(50.0, c->GetPixelX());
  m.SetRelativeX(-0.25);
  ASSERT_TRUE(v.GetQueuedDraw());
  ASSERT_FLOAT_EQ(-100.0, m.GetPixelX());
}

TEST_F(BasicElementTest, TestPixelY) {
  MockedView v(gFactory);
  Muffin m(NULL, &v, NULL);
  ASSERT_FLOAT_EQ(0.0, m.GetPixelY());
  m.SetPixelY(100.0);
  ASSERT_TRUE(v.GetQueuedDraw());
  ASSERT_FLOAT_EQ(100.0, m.GetPixelY());
  ggadget::ElementInterface *c = m.GetChildren()->AppendElement("pie", NULL);
  c->SetPixelY(50.0);
  ASSERT_TRUE(v.GetQueuedDraw());
  // Modifying the height of the parent will not effect the child.
  m.SetPixelHeight(150.0);
  ASSERT_TRUE(v.GetQueuedDraw());
  ASSERT_FLOAT_EQ(50.0, c->GetPixelY());
  m.SetPixelY(-150.5);
  ASSERT_TRUE(v.GetQueuedDraw());
  ASSERT_FLOAT_EQ(-150.5, m.GetPixelY());
}

TEST_F(BasicElementTest, TestRelativeY) {
  MockedView v(gFactory);
  Muffin m(NULL, &v, NULL);
  m.SetRelativeHeight(0.5);
  ASSERT_TRUE(v.GetQueuedDraw());
  m.SetRelativeY(0.5);
  ASSERT_TRUE(v.GetQueuedDraw());
  ASSERT_FLOAT_EQ(150.0, m.GetPixelY());
  ggadget::ElementInterface *c = m.GetChildren()->AppendElement("pie", NULL);
  c->SetRelativeY(0.50);
  ASSERT_TRUE(v.GetQueuedDraw());
  ASSERT_FLOAT_EQ(75.0, c->GetPixelY());
  // Modifying the height of the parent will effect the child.
  m.SetPixelHeight(150.0);
  ASSERT_TRUE(v.GetQueuedDraw());
  ASSERT_FLOAT_EQ(75.0, c->GetPixelY());
  m.SetRelativeY(-0.125);
  ASSERT_TRUE(v.GetQueuedDraw());
  ASSERT_FLOAT_EQ(-37.5, m.GetPixelY());
}

// This test is not merely for BasicElement, but mixed test for xml_utils
// and Elements.
TEST_F(BasicElementTest, TestFromXML) {
  MockedView v(gFactory);
  Muffin m(NULL, &v, NULL);
  ggadget::ElementInterface *e1 = m.GetChildren()->InsertElementFromXML(
      "<muffin/>", NULL);
  ggadget::ElementInterface *e2 = m.GetChildren()->InsertElementFromXML(
      "<pie/>", e1);
  ggadget::ElementInterface *e3 = m.GetChildren()->InsertElementFromXML(
      "<pie name=\"a-pie\"/>", e2);
  ggadget::ElementInterface *e4 = m.GetChildren()->AppendElementFromXML(
      "<bread/>");
  ggadget::ElementInterface *e5 = m.GetChildren()->InsertElementFromXML(
      "<bread/>", e2);
  ggadget::ElementInterface *e6 = m.GetChildren()->AppendElementFromXML(
      "<pie name=\"big-pie\"/>");
  ASSERT_EQ(4, m.GetChildren()->GetCount());
  ASSERT_TRUE(e1 == m.GetChildren()->GetItemByIndex(2));
  ASSERT_STREQ("muffin", e1->GetTagName());
  ASSERT_STREQ("", e1->GetName());
  ASSERT_TRUE(e2 == m.GetChildren()->GetItemByIndex(1));
  ASSERT_STREQ("pie", e2->GetTagName());
  ASSERT_STREQ("", e2->GetName());
  ASSERT_TRUE(e3 == m.GetChildren()->GetItemByIndex(0));
  ASSERT_TRUE(e3 == m.GetChildren()->GetItemByName("a-pie"));
  ASSERT_STREQ("pie", e3->GetTagName());
  ASSERT_STREQ("a-pie", e3->GetName());
  ASSERT_TRUE(NULL == e4);
  ASSERT_TRUE(NULL == e5);
  ASSERT_TRUE(e6 == m.GetChildren()->GetItemByIndex(3));
  ASSERT_TRUE(e6 == m.GetChildren()->GetItemByName("big-pie"));
  ASSERT_STREQ("pie", e6->GetTagName());
  ASSERT_STREQ("big-pie", e6->GetName());
}

// This test is not merely for BasicElement, but mixed test for xml_utils
// and Elements.
TEST_F(BasicElementTest, XMLConstruction) {
  MockedView v(gFactory);
  Muffin m(NULL, &v, NULL);

  const char *xml = 
    "<muffin n1=\"yy\" name=\"top\">\n"
    "  <pie tooltip=\"pie-tooltip\" x=\"50%\" y=\"100\">\n"
    "    <muffin tagName=\"haha\" name=\"muffin\"/>\n"
    "  </pie>\n"
    "  <pie name=\"pie1\"/>\n"
    "</muffin>\n";
  AppendElementFromXML(m.GetChildren(), xml);
  ASSERT_EQ(1, m.GetChildren()->GetCount());
  ggadget::ElementInterface *e1 = m.GetChildren()->GetItemByIndex(0);
  ASSERT_TRUE(e1);
  ASSERT_TRUE(e1->IsInstanceOf(Muffin::CLASS_ID));
  ASSERT_FALSE(e1->IsInstanceOf(Pie::CLASS_ID));
  ASSERT_TRUE(e1->IsInstanceOf(ggadget::ElementInterface::CLASS_ID));
  Muffin *m1 = ggadget::down_cast<Muffin *>(e1);
  ASSERT_STREQ("top", m1->GetName());
  ASSERT_STREQ("muffin", m1->GetTagName());
  ASSERT_EQ(2, m1->GetChildren()->GetCount());
  ggadget::ElementInterface *e2 = m1->GetChildren()->GetItemByIndex(0);
  ASSERT_TRUE(e2);
  ASSERT_TRUE(e2->IsInstanceOf(Pie::CLASS_ID));
  ASSERT_FALSE(e2->IsInstanceOf(Muffin::CLASS_ID));
  ASSERT_TRUE(e2->IsInstanceOf(ggadget::ElementInterface::CLASS_ID));
  Pie *p1 = ggadget::down_cast<Pie *>(e2);
  ASSERT_STREQ("", p1->GetName());
  ASSERT_STREQ("pie", p1->GetTagName());
  ASSERT_STREQ("pie-tooltip", p1->GetTooltip());
  ASSERT_TRUE(p1->XIsRelative());
  ASSERT_FLOAT_EQ(0.5, p1->GetRelativeX());
  ASSERT_FALSE(p1->YIsRelative());
  ASSERT_FLOAT_EQ(100, p1->GetPixelY());
  ASSERT_EQ(1, p1->GetChildren()->GetCount());
}

int main(int argc, char *argv[]) {
  testing::ParseGUnitFlags(&argc, argv);
  gFactory = new ggadget::ElementFactory();
  gFactory->RegisterElementClass("muffin", Muffin::CreateInstance);
  gFactory->RegisterElementClass("pie", Pie::CreateInstance);
  int result = RUN_ALL_TESTS();
  delete gFactory;
  return result;
}
