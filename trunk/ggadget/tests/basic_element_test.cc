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
#include "mocked_element.h"
#include "mocked_gadget_host.h"
#include "mocked_view_host.h"

ggadget::ElementFactory *gFactory = NULL;
using ggadget::down_cast;

class BasicElementTest : public testing::Test {
 protected:
  virtual void SetUp() {
  }

  virtual void TearDown() {
  }
};

TEST_F(BasicElementTest, TestCreate) {
  MockedViewHost vh(gFactory);
  Muffin m(NULL, vh.GetViewInternal(), NULL);
  Pie p(NULL, vh.GetViewInternal(), NULL);
}

TEST_F(BasicElementTest, TestChildren) {
  MockedViewHost vh(gFactory);
  Muffin m(NULL, vh.GetViewInternal(), NULL);
  ggadget::BasicElement *c1 = 
      m.GetChildren()->AppendElement("muffin", NULL);
  ggadget::BasicElement *c2 =
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
  MockedViewHost vh(gFactory);
  Muffin m(NULL, vh.GetViewInternal(), NULL);
  ASSERT_TRUE(m.GetCursor() == ggadget::ViewHostInterface::CURSOR_ARROW);
  m.SetCursor(ggadget::ViewHostInterface::CURSOR_BUSY);
  ASSERT_TRUE(m.GetCursor() == ggadget::ViewHostInterface::CURSOR_BUSY);
}

TEST_F(BasicElementTest, TestDropTarget) {
  MockedViewHost vh(gFactory);
  Muffin m(NULL, vh.GetViewInternal(), NULL);
  ASSERT_FALSE(m.IsDropTarget());
  m.SetDropTarget(true);
  ASSERT_TRUE(m.IsDropTarget());
}

TEST_F(BasicElementTest, TestEnabled) {
  MockedViewHost vh(gFactory);
  Muffin m(NULL, vh.GetViewInternal(), NULL);
  ASSERT_FALSE(m.IsEnabled());
  m.SetEnabled(true);
  ASSERT_TRUE(m.IsEnabled());
}

TEST_F(BasicElementTest, TestPixelHeight) {
  MockedViewHost vh(gFactory);
  ASSERT_FALSE(vh.GetQueuedDraw());
  Muffin m(NULL, vh.GetViewInternal(), NULL);
  ASSERT_FLOAT_EQ(0.0, m.GetPixelHeight());
  m.SetPixelHeight(100.0);
  ASSERT_TRUE(vh.GetQueuedDraw());
  ASSERT_FALSE(vh.GetQueuedDraw());
  ASSERT_FLOAT_EQ(100.0, m.GetPixelHeight());
  // Setting the height as negative value will make no effect.
  m.SetPixelHeight(-100.0);
  ASSERT_FALSE(vh.GetQueuedDraw());
  ASSERT_FLOAT_EQ(100.0, m.GetPixelHeight());
  ggadget::BasicElement *c = m.GetChildren()->AppendElement("pie", NULL);
  c->SetPixelHeight(50.0);
  ASSERT_TRUE(vh.GetQueuedDraw());
  // Modifying the height of the parent will not effect the child.
  m.SetPixelHeight(150.0);
  ASSERT_TRUE(vh.GetQueuedDraw());
  ASSERT_FLOAT_EQ(50.0, c->GetPixelHeight());
}

TEST_F(BasicElementTest, TestRelativeHeight) {
  MockedViewHost vh(gFactory);
  vh.GetView()->SetSize(400, 300);
  ggadget::BasicElement *m =
      vh.GetViewInternal()->GetChildren()->AppendElement("muffin", NULL);
  m->SetRelativeHeight(0.50);
  ASSERT_TRUE(vh.GetQueuedDraw());
  ASSERT_FLOAT_EQ(0.50, m->GetRelativeHeight());
  ASSERT_FLOAT_EQ(150.0, m->GetPixelHeight());
  ggadget::BasicElement *c = m->GetChildren()->AppendElement("pie", NULL);
  c->SetRelativeHeight(0.50);
  ASSERT_TRUE(vh.GetQueuedDraw());
  ASSERT_FLOAT_EQ(0.50, c->GetRelativeHeight());
  ASSERT_FLOAT_EQ(75.0, c->GetPixelHeight());
  // Setting the height as negative value will make no effect.
  c->SetRelativeHeight(-0.50);
  ASSERT_FALSE(vh.GetQueuedDraw());
  ASSERT_FLOAT_EQ(0.50, c->GetRelativeHeight());
  ASSERT_FLOAT_EQ(75.0, c->GetPixelHeight());
  // Modifying the height of the parent will effect the child.
  m->SetRelativeHeight(1.0);
  ASSERT_TRUE(vh.GetQueuedDraw());
  ASSERT_FLOAT_EQ(0.50, c->GetRelativeHeight());
  ASSERT_FLOAT_EQ(150.0, c->GetPixelHeight());
  // Modifying the height of the parent will effect the child.
  m->SetPixelHeight(100.0);
  ASSERT_TRUE(vh.GetQueuedDraw());
  ASSERT_FLOAT_EQ(50.0, c->GetPixelHeight());
}

TEST_F(BasicElementTest, TestHitTest) {
  MockedViewHost vh(gFactory);
  Muffin m(NULL, vh.GetViewInternal(), NULL);
  ASSERT_TRUE(m.GetHitTest() == ggadget::BasicElement::HT_DEFAULT);
  m.SetHitTest(ggadget::BasicElement::HT_CLIENT);
  ASSERT_TRUE(m.GetHitTest() == ggadget::BasicElement::HT_CLIENT);
}

/*TEST_F(BasicElementTest, TestMask) {
  MockedViewHost vh(gFactory);
  Muffin m(NULL, vh.GetViewInternal(), NULL);
  ASSERT_STREQ("", m.GetMask().c_str());
  m.SetMask("mymask.png");
  ASSERT_STREQ("mymask.png", m.GetMask().c_str());
  m.SetMask(NULL);
  ASSERT_STREQ("", m.GetMask().c_str());
}*/

TEST_F(BasicElementTest, TestName) {
  MockedViewHost vh(gFactory);
  Muffin m(NULL, vh.GetViewInternal(), "mymuffin");
  ASSERT_STREQ("mymuffin", m.GetName().c_str());
}

TEST_F(BasicElementTest, TestConst) {
  MockedViewHost vh(gFactory);
  Muffin m(NULL, vh.GetViewInternal(), NULL);
  ggadget::BasicElement *c = m.GetChildren()->AppendElement("pie", NULL);
  const ggadget::BasicElement *cc = c;
  ASSERT_TRUE(cc->GetView() == vh.GetViewInternal());
  ASSERT_TRUE(cc->GetParentElement() == &m);
}

TEST_F(BasicElementTest, TestOpacity) {
  MockedViewHost vh(gFactory);
  Muffin m(NULL, vh.GetViewInternal(), NULL);
  ASSERT_FLOAT_EQ(1.0, m.GetOpacity());
  m.SetOpacity(0.5);
  ASSERT_TRUE(vh.GetQueuedDraw());
  ASSERT_FLOAT_EQ(0.5, m.GetOpacity());
  // Setting the value greater than 1 will make no effect.
  m.SetOpacity(1.5);
  ASSERT_FALSE(vh.GetQueuedDraw());
  ASSERT_FLOAT_EQ(0.5, m.GetOpacity());
  // Setting the value less than 0 will make no effect.
  m.SetOpacity(-0.5);
  ASSERT_FALSE(vh.GetQueuedDraw());
  ASSERT_FLOAT_EQ(0.5, m.GetOpacity());
}

TEST_F(BasicElementTest, TestPixelPinX) {
  MockedViewHost vh(gFactory);
  vh.GetView()->SetSize(400, 300);
  Muffin m(NULL, vh.GetViewInternal(), NULL);
  ASSERT_FLOAT_EQ(0.0, m.GetPixelPinX());
  m.SetPixelPinX(100.5);
  ASSERT_TRUE(vh.GetQueuedDraw());
  ASSERT_FLOAT_EQ(100.5, m.GetPixelPinX());
  // Modifying the width of the parent will not effect the pin x.
  m.SetPixelWidth(150.5);
  ASSERT_TRUE(vh.GetQueuedDraw());
  ASSERT_FLOAT_EQ(100.5, m.GetPixelPinX());
  ASSERT_FALSE(m.PinXIsRelative());
  m.SetPixelPinX(-50.5);
  ASSERT_TRUE(vh.GetQueuedDraw());
  ASSERT_FLOAT_EQ(-50.5, m.GetPixelPinX());
}

TEST_F(BasicElementTest, TestRelativePinX) {
  MockedViewHost vh(gFactory);
  vh.GetView()->SetSize(400, 300);
  ggadget::BasicElement *m =
      vh.GetViewInternal()->GetChildren()->AppendElement("muffin", NULL);
  m->SetPixelWidth(200.0);
  ASSERT_TRUE(vh.GetQueuedDraw());
  m->SetRelativePinX(0.5);
  ASSERT_TRUE(vh.GetQueuedDraw());
  ASSERT_FLOAT_EQ(100.0, m->GetPixelPinX());
  // Modifying the width will effect the pin x.
  m->SetPixelWidth(400.0);
  ASSERT_TRUE(vh.GetQueuedDraw());
  ASSERT_FLOAT_EQ(200.0, m->GetPixelPinX());
  ASSERT_TRUE(m->PinXIsRelative());
  m->SetRelativePinX(-0.25);
  ASSERT_TRUE(vh.GetQueuedDraw());
  ASSERT_FLOAT_EQ(-100.0, m->GetPixelPinX());
}

TEST_F(BasicElementTest, TestPixelPinY) {
  MockedViewHost vh(gFactory);
  vh.GetView()->SetSize(400, 300);
  Muffin m(NULL, vh.GetViewInternal(), NULL);
  m.SetPixelHeight(150.5);
  ASSERT_TRUE(vh.GetQueuedDraw());
  m.SetPixelPinY(100.5);
  ASSERT_TRUE(vh.GetQueuedDraw());
  ASSERT_FLOAT_EQ(100.5, m.GetPixelPinY());
  // Modifying the width will not effect the pin y.
  m.SetPixelHeight(300.5);
  ASSERT_TRUE(vh.GetQueuedDraw());
  ASSERT_FLOAT_EQ(100.5, m.GetPixelPinY());
  ASSERT_FALSE(m.PinYIsRelative());
  m.SetPixelPinY(-50.5);
  ASSERT_TRUE(vh.GetQueuedDraw());
  ASSERT_FLOAT_EQ(-50.5, m.GetPixelPinY());
}

TEST_F(BasicElementTest, TestRelativePinY) {
  MockedViewHost vh(gFactory);
  vh.GetView()->SetSize(400, 300);
  ggadget::BasicElement *m =
      vh.GetViewInternal()->GetChildren()->AppendElement("muffin", NULL);
  m->SetPixelHeight(150.0);
  ASSERT_TRUE(vh.GetQueuedDraw());
  m->SetRelativePinY(0.5);
  ASSERT_TRUE(vh.GetQueuedDraw());
  ASSERT_FLOAT_EQ(75.0, m->GetPixelPinY());
  // Modifying the width of the parent will not effect the pin y.
  m->SetPixelHeight(300.0);
  ASSERT_TRUE(vh.GetQueuedDraw());
  ASSERT_FLOAT_EQ(150.0, m->GetPixelPinY());
  ASSERT_TRUE(m->PinYIsRelative());
  m->SetRelativePinY(-0.25);
  ASSERT_TRUE(vh.GetQueuedDraw());
  ASSERT_FLOAT_EQ(-75.0, m->GetPixelPinY());
}

TEST_F(BasicElementTest, TestRotation) {
  MockedViewHost vh(gFactory);
  Muffin m(NULL, vh.GetViewInternal(), NULL);
  ASSERT_FLOAT_EQ(0.0, m.GetRotation());
  m.SetRotation(0.5);
  ASSERT_TRUE(vh.GetQueuedDraw());
  ASSERT_FLOAT_EQ(0.5, m.GetRotation());
}

TEST_F(BasicElementTest, TestTooltip) {
  MockedViewHost vh(gFactory);
  Muffin m(NULL, vh.GetViewInternal(), NULL);
  ASSERT_STREQ("", m.GetTooltip().c_str());
  m.SetTooltip("mytooltip");
  ASSERT_STREQ("mytooltip", m.GetTooltip().c_str());
  m.SetTooltip(NULL);
  ASSERT_STREQ("", m.GetTooltip().c_str());
}

TEST_F(BasicElementTest, TestPixelWidth) {
  MockedViewHost vh(gFactory);
  vh.GetView()->SetSize(400, 300);
  Muffin m(NULL, vh.GetViewInternal(), NULL);
  ASSERT_FLOAT_EQ(0.0, m.GetPixelWidth());
  m.SetPixelWidth(100.0);
  ASSERT_TRUE(vh.GetQueuedDraw());
  ASSERT_FLOAT_EQ(100.0, m.GetPixelWidth());
  // Setting the width as negative value will make no effect.
  m.SetPixelWidth(-100.0);
  ASSERT_FALSE(vh.GetQueuedDraw());
  ASSERT_FLOAT_EQ(100.0, m.GetPixelWidth());
  ggadget::BasicElement *c = m.GetChildren()->AppendElement("pie", NULL);
  c->SetPixelWidth(50.0);
  // Modifying the width of the parent will not effect the child.
  m.SetPixelWidth(200.0);
  ASSERT_TRUE(vh.GetQueuedDraw());
  ASSERT_FLOAT_EQ(50.0, c->GetPixelWidth());
}

TEST_F(BasicElementTest, TestRelativeWidth) {
  MockedViewHost vh(gFactory);
  vh.GetView()->SetSize(400, 300);
  ggadget::BasicElement *m =
      vh.GetViewInternal()->GetChildren()->AppendElement("muffin", NULL);
  m->SetRelativeWidth(0.50);
  ASSERT_TRUE(vh.GetQueuedDraw());
  ASSERT_FLOAT_EQ(0.50, m->GetRelativeWidth());
  ASSERT_FLOAT_EQ(200.0, m->GetPixelWidth());
  ggadget::BasicElement *c = m->GetChildren()->AppendElement("pie", NULL);
  c->SetRelativeWidth(0.50);
  ASSERT_TRUE(vh.GetQueuedDraw());
  ASSERT_FLOAT_EQ(0.50, c->GetRelativeWidth());
  ASSERT_FLOAT_EQ(100.0, c->GetPixelWidth());
  // Setting the width as negative value will make no effect.
  c->SetRelativeWidth(-0.50);
  ASSERT_FALSE(vh.GetQueuedDraw());
  ASSERT_FLOAT_EQ(0.50, c->GetRelativeWidth());
  ASSERT_FLOAT_EQ(100.0, c->GetPixelWidth());
  // Modifying the width of the parent will effect the child.
  m->SetRelativeWidth(1.0);
  ASSERT_TRUE(vh.GetQueuedDraw());
  ASSERT_FLOAT_EQ(0.50, c->GetRelativeWidth());
  ASSERT_FLOAT_EQ(200.0, c->GetPixelWidth());
  // Modifying the width of the parent will effect the child.
  m->SetPixelWidth(150.0);
  ASSERT_TRUE(vh.GetQueuedDraw());
  ASSERT_FLOAT_EQ(75.0, c->GetPixelWidth());
}

TEST_F(BasicElementTest, TestVisible) {
  MockedViewHost vh(gFactory);
  Muffin m(NULL, vh.GetViewInternal(), NULL);
  ASSERT_TRUE(m.IsVisible());
  m.SetVisible(false);
  ASSERT_FALSE(m.IsVisible());
}

TEST_F(BasicElementTest, TestPixelX) {
  MockedViewHost vh(gFactory);
  vh.GetView()->SetSize(400, 300);
  Muffin m(NULL, vh.GetViewInternal(), NULL);
  ASSERT_FLOAT_EQ(0.0, m.GetPixelX());
  m.SetPixelX(100.0);
  ASSERT_TRUE(vh.GetQueuedDraw());
  ASSERT_FLOAT_EQ(100.0, m.GetPixelX());
  ggadget::BasicElement *c = m.GetChildren()->AppendElement("pie", NULL);
  c->SetPixelX(50.0);
  ASSERT_TRUE(vh.GetQueuedDraw());
  // Modifying the width of the parent will not effect the child.
  m.SetPixelWidth(150.0);
  ASSERT_TRUE(vh.GetQueuedDraw());
  ASSERT_FLOAT_EQ(50.0, c->GetPixelX());
  m.SetPixelX(-50.5);
  ASSERT_TRUE(vh.GetQueuedDraw());
  ASSERT_FLOAT_EQ(-50.5, m.GetPixelX());
}

TEST_F(BasicElementTest, TestRelativeX) {
  MockedViewHost vh(gFactory);
  vh.GetView()->SetSize(400, 300);
  ggadget::BasicElement *m =
      vh.GetViewInternal()->GetChildren()->AppendElement("muffin", NULL);
  m->SetRelativeWidth(0.5);
  ASSERT_TRUE(vh.GetQueuedDraw());
  m->SetRelativeX(0.5);
  ASSERT_TRUE(vh.GetQueuedDraw());
  ASSERT_FLOAT_EQ(200.0, m->GetPixelX());
  ggadget::BasicElement *c = m->GetChildren()->AppendElement("pie", NULL);
  c->SetRelativeX(0.50);
  ASSERT_TRUE(vh.GetQueuedDraw());
  ASSERT_FLOAT_EQ(100.0, c->GetPixelX());
  // Modifying the width of the parent will effect the child.
  m->SetPixelWidth(100.0);
  ASSERT_TRUE(vh.GetQueuedDraw());
  ASSERT_FLOAT_EQ(50.0, c->GetPixelX());
  m->SetRelativeX(-0.25);
  ASSERT_TRUE(vh.GetQueuedDraw());
  ASSERT_FLOAT_EQ(-100.0, m->GetPixelX());
}

TEST_F(BasicElementTest, TestPixelY) {
  MockedViewHost vh(gFactory);
  vh.GetView()->SetSize(400, 300);
  Muffin m(NULL, vh.GetViewInternal(), NULL);
  ASSERT_FLOAT_EQ(0.0, m.GetPixelY());
  m.SetPixelY(100.0);
  ASSERT_TRUE(vh.GetQueuedDraw());
  ASSERT_FLOAT_EQ(100.0, m.GetPixelY());
  ggadget::BasicElement *c = m.GetChildren()->AppendElement("pie", NULL);
  c->SetPixelY(50.0);
  ASSERT_TRUE(vh.GetQueuedDraw());
  // Modifying the height of the parent will not effect the child.
  m.SetPixelHeight(150.0);
  ASSERT_TRUE(vh.GetQueuedDraw());
  ASSERT_FLOAT_EQ(50.0, c->GetPixelY());
  m.SetPixelY(-150.5);
  ASSERT_TRUE(vh.GetQueuedDraw());
  ASSERT_FLOAT_EQ(-150.5, m.GetPixelY());
}

TEST_F(BasicElementTest, TestRelativeY) {
  MockedViewHost vh(gFactory);
  vh.GetView()->SetSize(400, 300);
  ggadget::BasicElement *m =
      vh.GetViewInternal()->GetChildren()->AppendElement("muffin", NULL);
  m->SetRelativeHeight(0.5);
  ASSERT_TRUE(vh.GetQueuedDraw());
  m->SetRelativeY(0.5);
  ASSERT_TRUE(vh.GetQueuedDraw());
  ASSERT_FLOAT_EQ(150.0, m->GetPixelY());
  ggadget::BasicElement *c = m->GetChildren()->AppendElement("pie", NULL);
  c->SetRelativeY(0.50);
  ASSERT_TRUE(vh.GetQueuedDraw());
  ASSERT_FLOAT_EQ(75.0, c->GetPixelY());
  // Modifying the height of the parent will effect the child.
  m->SetPixelHeight(150.0);
  ASSERT_TRUE(vh.GetQueuedDraw());
  ASSERT_FLOAT_EQ(75.0, c->GetPixelY());
  m->SetRelativeY(-0.125);
  ASSERT_TRUE(vh.GetQueuedDraw());
  ASSERT_FLOAT_EQ(-37.5, m->GetPixelY());
}

// This test is not merely for BasicElement, but mixed test for xml_utils
// and Elements.
TEST_F(BasicElementTest, TestFromXML) {
  MockedViewHost vh(gFactory);
  Muffin m(NULL, vh.GetViewInternal(), NULL);
  ggadget::Elements *children = down_cast<ggadget::Elements *>(m.GetChildren());
  ggadget::BasicElement *e1 = children->InsertElementFromXML(
      "<muffin/>", NULL);
  ggadget::BasicElement *e2 = children->InsertElementFromXML(
      "<pie/>", e1);
  ggadget::BasicElement *e3 = children->InsertElementFromXML(
      "<pie name=\"a-pie\"/>", e2);
  ggadget::BasicElement *e4 = children->AppendElementFromXML(
      "<bread/>");
  ggadget::BasicElement *e5 = children->InsertElementFromXML(
      "<bread/>", e2);
  ggadget::BasicElement *e6 = children->AppendElementFromXML(
      "<pie name=\"big-pie\"/>");
  ASSERT_EQ(4, children->GetCount());
  ASSERT_TRUE(e1 == children->GetItemByIndex(2));
  ASSERT_STREQ("muffin", e1->GetTagName().c_str());
  ASSERT_STREQ("", e1->GetName().c_str());
  ASSERT_TRUE(e2 == children->GetItemByIndex(1));
  ASSERT_STREQ("pie", e2->GetTagName().c_str());
  ASSERT_STREQ("", e2->GetName().c_str());
  ASSERT_TRUE(e3 == children->GetItemByIndex(0));
  ASSERT_TRUE(e3 == children->GetItemByName("a-pie"));
  ASSERT_STREQ("pie", e3->GetTagName().c_str());
  ASSERT_STREQ("a-pie", e3->GetName().c_str());
  ASSERT_TRUE(NULL == e4);
  ASSERT_TRUE(NULL == e5);
  ASSERT_TRUE(e6 == children->GetItemByIndex(3));
  ASSERT_TRUE(e6 == children->GetItemByName("big-pie"));
  ASSERT_STREQ("pie", e6->GetTagName().c_str());
  ASSERT_STREQ("big-pie", e6->GetName().c_str());
}

// This test is not merely for BasicElement, but mixed test for xml_utils
// and Elements.
TEST_F(BasicElementTest, XMLConstruction) {
  MockedViewHost vh(gFactory);
  Muffin m(NULL, vh.GetViewInternal(), NULL);

  const char *xml = 
    "<muffin n1=\"yy\" name=\"top\">\n"
    "  <pie tooltip=\"pie-tooltip\" x=\"50%\" y=\"100\">\n"
    "    <muffin tagName=\"haha\" name=\"muffin\"/>\n"
    "  </pie>\n"
    "  <pie name=\"pie1\"/>\n"
    "</muffin>\n";
  AppendElementFromXML(vh.GetViewInternal(), m.GetChildren(), xml);
  ASSERT_EQ(1, m.GetChildren()->GetCount());
  ggadget::BasicElement *e1 = m.GetChildren()->GetItemByIndex(0);
  ASSERT_TRUE(e1);
  ASSERT_TRUE(e1->IsInstanceOf(Muffin::CLASS_ID));
  ASSERT_FALSE(e1->IsInstanceOf(Pie::CLASS_ID));
  ASSERT_TRUE(e1->IsInstanceOf(ggadget::BasicElement::CLASS_ID));
  Muffin *m1 = down_cast<Muffin *>(e1);
  ASSERT_STREQ("top", m1->GetName().c_str());
  ASSERT_STREQ("muffin", m1->GetTagName().c_str());
  ASSERT_EQ(2, m1->GetChildren()->GetCount());
  ggadget::BasicElement *e2 = m1->GetChildren()->GetItemByIndex(0);
  ASSERT_TRUE(e2);
  ASSERT_TRUE(e2->IsInstanceOf(Pie::CLASS_ID));
  ASSERT_FALSE(e2->IsInstanceOf(Muffin::CLASS_ID));
  ASSERT_TRUE(e2->IsInstanceOf(ggadget::BasicElement::CLASS_ID));
  Pie *p1 = down_cast<Pie *>(e2);
  ASSERT_STREQ("", p1->GetName().c_str());
  ASSERT_STREQ("pie", p1->GetTagName().c_str());
  ASSERT_STREQ("pie-tooltip", p1->GetTooltip().c_str());
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
