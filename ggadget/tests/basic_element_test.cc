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

#include "unittest/gtest.h"
#include "ggadget/basic_element.h"
#include "ggadget/element_factory.h"
#include "ggadget/elements.h"
#include "ggadget/xml_utils.h"
#include "ggadget/view.h"
#include "mocked_element.h"
#include "mocked_timer_main_loop.h"
#include "mocked_view_host.h"
#include "init_extensions.h"

MockedTimerMainLoop main_loop(0);

ggadget::ElementFactory *g_factory = NULL;
using ggadget::down_cast;
using ggadget::View;
using ggadget::ViewInterface;
using ggadget::ViewHostInterface;

class BasicElementTest : public testing::Test {
 protected:
  virtual void SetUp() {
  }

  virtual void TearDown() {
  }
};

TEST_F(BasicElementTest, TestCreate) {
  View view(new MockedViewHost(ViewHostInterface::VIEW_HOST_MAIN),
            NULL, g_factory, NULL);

  Muffin m(&view, NULL);
  Pie p(&view, NULL);
}

TEST_F(BasicElementTest, TestChildren) {
  View view(new MockedViewHost(ViewHostInterface::VIEW_HOST_MAIN),
            NULL, g_factory, NULL);

  Muffin m(&view, NULL);
  ggadget::BasicElement *c1 =
      m.GetChildren()->AppendElement("muffin", NULL);
  ggadget::BasicElement *c2 =
      m.GetChildren()->InsertElement("pie", c1, "First");
  ggadget::BasicElement *c3 =
      m.GetChildren()->AppendElement("pie", "Last");
  ASSERT_EQ(3U, m.GetChildren()->GetCount());
  ASSERT_TRUE(m.GetChildren()->GetItemByIndex(0) == c2);
  ASSERT_EQ(0U, c2->GetIndex());
  ASSERT_TRUE(m.GetChildren()->GetItemByIndex(1) == c1);
  ASSERT_EQ(1U, c1->GetIndex());
  ASSERT_TRUE(m.GetChildren()->GetItemByIndex(2) == c3);
  ASSERT_EQ(2U, c3->GetIndex());
  ASSERT_TRUE(m.GetChildren()->GetItemByName("First") == c2);
  ASSERT_TRUE(m.GetChildren()->GetItemByName("Last") == c3);
  m.GetChildren()->RemoveElement(c2);
  ASSERT_EQ(2U, m.GetChildren()->GetCount());
  ASSERT_TRUE(m.GetChildren()->GetItemByIndex(0) == c1);
  ASSERT_EQ(0U, c1->GetIndex());
  ASSERT_TRUE(m.GetChildren()->GetItemByIndex(1) == c3);
  ASSERT_EQ(1U, c3->GetIndex());
  m.GetChildren()->RemoveElement(c3);
  ASSERT_EQ(1U, m.GetChildren()->GetCount());
  ASSERT_TRUE(m.GetChildren()->GetItemByIndex(0) == c1);
  ASSERT_EQ(0U, c1->GetIndex());
  m.GetChildren()->RemoveAllElements();
  ASSERT_EQ(0U, m.GetChildren()->GetCount());
}

TEST_F(BasicElementTest, TestCursor) {
  View view(new MockedViewHost(ViewHostInterface::VIEW_HOST_MAIN),
            NULL, g_factory, NULL);

  Muffin m(&view, NULL);
  ASSERT_EQ(ViewInterface::CURSOR_DEFAULT, m.GetCursor());
  m.SetCursor(ViewInterface::CURSOR_BUSY);
  ASSERT_EQ(ViewInterface::CURSOR_BUSY, m.GetCursor());
}

TEST_F(BasicElementTest, TestDropTarget) {
  View view(new MockedViewHost(ViewHostInterface::VIEW_HOST_MAIN),
            NULL, g_factory, NULL);

  Muffin m(&view, NULL);
  ASSERT_FALSE(m.IsDropTarget());
  m.SetDropTarget(true);
  ASSERT_TRUE(m.IsDropTarget());
}

TEST_F(BasicElementTest, TestEnabled) {
  View view(new MockedViewHost(ViewHostInterface::VIEW_HOST_MAIN),
            NULL, g_factory, NULL);

  Muffin m(&view, NULL);
  ASSERT_FALSE(m.IsEnabled());
  m.SetEnabled(true);
  ASSERT_TRUE(m.IsEnabled());
}

TEST_F(BasicElementTest, TestPixelHeight) {
  MockedViewHost *host = new MockedViewHost(ViewHostInterface::VIEW_HOST_MAIN);
  View view(host, NULL, g_factory, NULL);

  ASSERT_FALSE(host->GetQueuedDraw());
  view.SetSize(100, 100);

  BasicElement *m = view.GetChildren()->AppendElement("muffin", NULL);
  ASSERT_DOUBLE_EQ(0.0, m->GetPixelHeight());
  m->SetPixelHeight(100.0);
  ASSERT_TRUE(host->GetQueuedDraw());
  ASSERT_FALSE(host->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(100.0, m->GetPixelHeight());
  // Setting the height as negative value will make no effect.
  m->SetPixelHeight(-100.0);
  ASSERT_FALSE(host->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(100.0, m->GetPixelHeight());
  ggadget::BasicElement *c = m->GetChildren()->AppendElement("pie", NULL);
  c->SetPixelHeight(50.0);
  ASSERT_TRUE(host->GetQueuedDraw());
  // Modifying the height of the parent will not effect the child.
  m->SetPixelHeight(150.0);
  ASSERT_TRUE(host->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(50.0, c->GetPixelHeight());
}

TEST_F(BasicElementTest, TestRelativeHeight) {
  MockedViewHost *host = new MockedViewHost(ViewHostInterface::VIEW_HOST_MAIN);
  View view(host, NULL, g_factory, NULL);

  view.SetSize(400, 300);
  ggadget::BasicElement *m = view.GetChildren()->AppendElement("muffin", NULL);
  m->SetPixelWidth(100);
  m->SetRelativeHeight(0.50);
  ASSERT_TRUE(host->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(0.50, m->GetRelativeHeight());
  ASSERT_DOUBLE_EQ(150.0, m->GetPixelHeight());
  ggadget::BasicElement *c = m->GetChildren()->AppendElement("pie", NULL);
  c->SetRelativeHeight(0.50);
  ASSERT_TRUE(host->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(0.50, c->GetRelativeHeight());
  ASSERT_DOUBLE_EQ(75.0, c->GetPixelHeight());
  // Setting the height as negative value will make no effect.
  c->SetRelativeHeight(-0.50);
  ASSERT_FALSE(host->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(0.50, c->GetRelativeHeight());
  ASSERT_DOUBLE_EQ(75.0, c->GetPixelHeight());
  // Modifying the height of the parent will effect the child.
  m->SetRelativeHeight(1.0);
  ASSERT_TRUE(host->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(0.50, c->GetRelativeHeight());
  ASSERT_DOUBLE_EQ(150.0, c->GetPixelHeight());
  // Modifying the height of the parent will effect the child.
  m->SetPixelHeight(100.0);
  ASSERT_TRUE(host->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(50.0, c->GetPixelHeight());
}

TEST_F(BasicElementTest, TestHitTest) {
  View view(new MockedViewHost(ViewHostInterface::VIEW_HOST_MAIN),
            NULL, g_factory, NULL);

  Muffin m(&view, NULL);
  m.SetPixelWidth(1);
  m.SetPixelHeight(1);
  ASSERT_TRUE(m.GetHitTest(0, 0) == ggadget::ViewInterface::HT_CLIENT);
  m.SetHitTest(ggadget::ViewInterface::HT_CAPTION);
  ASSERT_TRUE(m.GetHitTest(0, 0) == ggadget::ViewInterface::HT_CAPTION);
  ASSERT_TRUE(m.GetHitTest(1, 1) == ggadget::ViewInterface::HT_TRANSPARENT);
}

/*TEST_F(BasicElementTest, TestMask) {
  View view(new MockedViewHost(ViewHostInterface::VIEW_HOST_MAIN),
            NULL, g_factory, NULL);

  Muffin m(&view, NULL);
  ASSERT_STREQ("", m.GetMask().c_str());
  m.SetMask("mymask.png");
  ASSERT_STREQ("mymask.png", m.GetMask().c_str());
  m.SetMask(NULL);
  ASSERT_STREQ("", m.GetMask().c_str());
}*/

TEST_F(BasicElementTest, TestName) {
  View view(new MockedViewHost(ViewHostInterface::VIEW_HOST_MAIN),
            NULL, g_factory, NULL);

  Muffin m(&view, "mymuffin");
  ASSERT_STREQ("mymuffin", m.GetName().c_str());
}

TEST_F(BasicElementTest, TestConst) {
  View view(new MockedViewHost(ViewHostInterface::VIEW_HOST_MAIN),
            NULL, g_factory, NULL);

  Muffin m(&view, NULL);
  ggadget::BasicElement *c = m.GetChildren()->AppendElement("pie", NULL);
  const ggadget::BasicElement *cc = c;
  ASSERT_TRUE(cc->GetView() == &view);
  ASSERT_TRUE(cc->GetParentElement() == &m);
}

TEST_F(BasicElementTest, TestOpacity) {
  MockedViewHost *host = new MockedViewHost(ViewHostInterface::VIEW_HOST_MAIN);
  View view(host, NULL, g_factory, NULL);

  view.SetSize(100, 100);

  Muffin m(&view, NULL);
  ASSERT_DOUBLE_EQ(1.0, m.GetOpacity());
  m.SetOpacity(0.5);
  ASSERT_TRUE(host->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(0.5, m.GetOpacity());
  // Setting the value greater than 1 will make no effect.
  m.SetOpacity(1.5);
  ASSERT_FALSE(host->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(0.5, m.GetOpacity());
  // Setting the value less than 0 will make no effect.
  m.SetOpacity(-0.5);
  ASSERT_FALSE(host->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(0.5, m.GetOpacity());
}

TEST_F(BasicElementTest, TestPixelPinX) {
  MockedViewHost *host = new MockedViewHost(ViewHostInterface::VIEW_HOST_MAIN);
  View view(host, NULL, g_factory, NULL);

  view.SetSize(400, 300);
  BasicElement *m = view.GetChildren()->AppendElement("muffin", NULL);
  ASSERT_DOUBLE_EQ(0.0, m->GetPixelPinX());
  m->SetPixelWidth(100.0);
  m->SetPixelHeight(100.0);
  m->SetPixelPinX(100.5);
  ASSERT_TRUE(host->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(100.5, m->GetPixelPinX());
  // Modifying the width of the parent will not effect the pin x.
  m->SetPixelWidth(150.5);
  ASSERT_TRUE(host->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(100.5, m->GetPixelPinX());
  ASSERT_FALSE(m->PinXIsRelative());
  m->SetPixelPinX(-50.5);
  ASSERT_TRUE(host->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(-50.5, m->GetPixelPinX());
}

TEST_F(BasicElementTest, TestRelativePinX) {
  MockedViewHost *host = new MockedViewHost(ViewHostInterface::VIEW_HOST_MAIN);
  View view(host, NULL, g_factory, NULL);

  view.SetSize(400, 300);
  BasicElement *m = view.GetChildren()->AppendElement("muffin", NULL);
  m->SetPixelWidth(200.0);
  m->SetPixelHeight(100.0);
  ASSERT_TRUE(host->GetQueuedDraw());
  m->SetRelativePinX(0.5);
  ASSERT_TRUE(host->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(100.0, m->GetPixelPinX());
  // Modifying the width will effect the pin x.
  m->SetPixelWidth(400.0);
  ASSERT_TRUE(host->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(200.0, m->GetPixelPinX());
  ASSERT_TRUE(m->PinXIsRelative());
  m->SetRelativePinX(-0.25);
  ASSERT_TRUE(host->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(-100.0, m->GetPixelPinX());
}

TEST_F(BasicElementTest, TestPixelPinY) {
  MockedViewHost *host = new MockedViewHost(ViewHostInterface::VIEW_HOST_MAIN);
  View view(host, NULL, g_factory, NULL);

  view.SetSize(400, 300);
  BasicElement *m = view.GetChildren()->AppendElement("muffin", NULL);
  m->SetPixelHeight(150.5);
  m->SetPixelWidth(150.5);
  ASSERT_TRUE(host->GetQueuedDraw());
  m->SetPixelPinY(100.5);
  ASSERT_TRUE(host->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(100.5, m->GetPixelPinY());
  // Modifying the width will not effect the pin y.
  m->SetPixelHeight(300.5);
  ASSERT_TRUE(host->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(100.5, m->GetPixelPinY());
  ASSERT_FALSE(m->PinYIsRelative());
  m->SetPixelPinY(-50.5);
  ASSERT_TRUE(host->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(-50.5, m->GetPixelPinY());
}

TEST_F(BasicElementTest, TestRelativePinY) {
  MockedViewHost *host = new MockedViewHost(ViewHostInterface::VIEW_HOST_MAIN);
  View view(host, NULL, g_factory, NULL);

  view.SetSize(400, 300);
  BasicElement *m = view.GetChildren()->AppendElement("muffin", NULL);
  m->SetPixelWidth(150.0);
  m->SetPixelHeight(150.0);
  ASSERT_TRUE(host->GetQueuedDraw());
  m->SetRelativePinY(0.5);
  ASSERT_TRUE(host->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(75.0, m->GetPixelPinY());
  // Modifying the width of the parent will not effect the pin y.
  m->SetPixelHeight(300.0);
  ASSERT_TRUE(host->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(150.0, m->GetPixelPinY());
  ASSERT_TRUE(m->PinYIsRelative());
  m->SetRelativePinY(-0.25);
  ASSERT_TRUE(host->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(-75.0, m->GetPixelPinY());
}

TEST_F(BasicElementTest, TestRotation) {
  MockedViewHost *host = new MockedViewHost(ViewHostInterface::VIEW_HOST_MAIN);
  View view(host, NULL, g_factory, NULL);

  Muffin m(&view, NULL);
  m.SetPixelWidth(100.0);
  m.SetPixelHeight(100.0);
  ASSERT_DOUBLE_EQ(0.0, m.GetRotation());
  m.SetRotation(0.5);
  ASSERT_TRUE(host->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(0.5, m.GetRotation());
}

TEST_F(BasicElementTest, TestTooltip) {
  MockedViewHost *host = new MockedViewHost(ViewHostInterface::VIEW_HOST_MAIN);
  View view(host, NULL, g_factory, NULL);

  Muffin m(&view, NULL);
  ASSERT_STREQ("", m.GetTooltip().c_str());
  m.SetTooltip("mytooltip");
  ASSERT_STREQ("mytooltip", m.GetTooltip().c_str());
  m.SetTooltip(NULL);
  ASSERT_STREQ("", m.GetTooltip().c_str());
}

TEST_F(BasicElementTest, TestPixelWidth) {
  MockedViewHost *host = new MockedViewHost(ViewHostInterface::VIEW_HOST_MAIN);
  View view(host, NULL, g_factory, NULL);

  view.SetSize(400, 300);
  Muffin m(&view, NULL);
  ASSERT_DOUBLE_EQ(0.0, m.GetPixelWidth());
  m.SetPixelWidth(100.0);
  m.SetPixelHeight(100.0);
  ASSERT_TRUE(host->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(100.0, m.GetPixelWidth());
  // Setting the width as negative value will make no effect.
  m.SetPixelWidth(-100.0);
  ASSERT_FALSE(host->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(100.0, m.GetPixelWidth());
  ggadget::BasicElement *c = m.GetChildren()->AppendElement("pie", NULL);
  c->SetPixelWidth(50.0);
  // Modifying the width of the parent will not effect the child.
  m.SetPixelWidth(200.0);
  ASSERT_TRUE(host->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(50.0, c->GetPixelWidth());
}

TEST_F(BasicElementTest, TestRelativeWidth) {
  MockedViewHost *host = new MockedViewHost(ViewHostInterface::VIEW_HOST_MAIN);
  View view(host, NULL, g_factory, NULL);

  view.SetSize(400, 300);
  ggadget::BasicElement *m = view.GetChildren()->AppendElement("muffin", NULL);
  m->SetPixelHeight(100);
  m->SetRelativeWidth(0.50);
  ASSERT_TRUE(host->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(0.50, m->GetRelativeWidth());
  ASSERT_DOUBLE_EQ(200.0, m->GetPixelWidth());
  ggadget::BasicElement *c = m->GetChildren()->AppendElement("pie", NULL);
  c->SetRelativeWidth(0.50);
  ASSERT_TRUE(host->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(0.50, c->GetRelativeWidth());
  ASSERT_DOUBLE_EQ(100.0, c->GetPixelWidth());
  // Setting the width as negative value will make no effect.
  c->SetRelativeWidth(-0.50);
  ASSERT_FALSE(host->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(0.50, c->GetRelativeWidth());
  ASSERT_DOUBLE_EQ(100.0, c->GetPixelWidth());
  // Modifying the width of the parent will effect the child.
  m->SetRelativeWidth(1.0);
  ASSERT_TRUE(host->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(0.50, c->GetRelativeWidth());
  ASSERT_DOUBLE_EQ(200.0, c->GetPixelWidth());
  // Modifying the width of the parent will effect the child.
  m->SetPixelWidth(150.0);
  ASSERT_TRUE(host->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(75.0, c->GetPixelWidth());
}

TEST_F(BasicElementTest, TestVisible) {
  MockedViewHost *host = new MockedViewHost(ViewHostInterface::VIEW_HOST_MAIN);
  View view(host, NULL, g_factory, NULL);

  Muffin m(&view, NULL);
  ASSERT_TRUE(m.IsVisible());
  m.SetVisible(false);
  ASSERT_FALSE(m.IsVisible());
}

TEST_F(BasicElementTest, TestPixelX) {
  MockedViewHost *host = new MockedViewHost(ViewHostInterface::VIEW_HOST_MAIN);
  View view(host, NULL, g_factory, NULL);

  view.SetSize(400, 300);
  BasicElement *m = view.GetChildren()->AppendElement("muffin", NULL);
  ASSERT_DOUBLE_EQ(0.0, m->GetPixelX());
  m->SetPixelWidth(100.0);
  m->SetPixelHeight(100.0);
  m->SetPixelX(100.0);
  ASSERT_TRUE(host->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(100.0, m->GetPixelX());
  ggadget::BasicElement *c = m->GetChildren()->AppendElement("pie", NULL);
  c->SetPixelX(50.0);
  ASSERT_TRUE(host->GetQueuedDraw());
  // Modifying the width of the parent will not effect the child.
  m->SetPixelWidth(150.0);
  ASSERT_TRUE(host->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(50.0, c->GetPixelX());
  m->SetPixelX(-50.5);
  ASSERT_TRUE(host->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(-50.5, m->GetPixelX());
}

TEST_F(BasicElementTest, TestRelativeX) {
  MockedViewHost *host = new MockedViewHost(ViewHostInterface::VIEW_HOST_MAIN);
  View view(host, NULL, g_factory, NULL);

  view.SetSize(400, 300);
  ggadget::BasicElement *m = view.GetChildren()->AppendElement("muffin", NULL);
  m->SetPixelWidth(100.0);
  m->SetPixelHeight(100.0);
  m->SetRelativeWidth(0.5);
  ASSERT_TRUE(host->GetQueuedDraw());
  m->SetRelativeX(0.5);
  ASSERT_TRUE(host->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(200.0, m->GetPixelX());
  ggadget::BasicElement *c = m->GetChildren()->AppendElement("pie", NULL);
  c->SetRelativeX(0.50);
  ASSERT_TRUE(host->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(100.0, c->GetPixelX());
  // Modifying the width of the parent will effect the child.
  m->SetPixelWidth(100.0);
  ASSERT_TRUE(host->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(50.0, c->GetPixelX());
  m->SetRelativeX(-0.25);
  ASSERT_TRUE(host->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(-100.0, m->GetPixelX());
}

TEST_F(BasicElementTest, TestPixelY) {
  MockedViewHost *host = new MockedViewHost(ViewHostInterface::VIEW_HOST_MAIN);
  View view(host, NULL, g_factory, NULL);

  view.SetSize(400, 300);
  BasicElement *m = view.GetChildren()->AppendElement("muffin", NULL);
  ASSERT_DOUBLE_EQ(0.0, m->GetPixelY());
  m->SetPixelWidth(100.0);
  m->SetPixelHeight(100.0);
  m->SetPixelY(100.0);
  ASSERT_TRUE(host->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(100.0, m->GetPixelY());
  ggadget::BasicElement *c = m->GetChildren()->AppendElement("pie", NULL);
  c->SetPixelY(50.0);
  ASSERT_TRUE(host->GetQueuedDraw());
  // Modifying the height of the parent will not effect the child.
  m->SetPixelHeight(150.0);
  ASSERT_TRUE(host->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(50.0, c->GetPixelY());
  m->SetPixelY(-150.5);
  ASSERT_TRUE(host->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(-150.5, m->GetPixelY());
}

TEST_F(BasicElementTest, TestRelativeY) {
  MockedViewHost *host = new MockedViewHost(ViewHostInterface::VIEW_HOST_MAIN);
  View view(host, NULL, g_factory, NULL);

  view.SetSize(400, 300);
  ggadget::BasicElement *m =
      view.GetChildren()->AppendElement("muffin", NULL);
  m->SetPixelWidth(100.0);
  m->SetRelativeHeight(0.5);
  ASSERT_TRUE(host->GetQueuedDraw());
  m->SetRelativeY(0.5);
  ASSERT_TRUE(host->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(150.0, m->GetPixelY());
  ggadget::BasicElement *c = m->GetChildren()->AppendElement("pie", NULL);
  c->SetRelativeY(0.50);
  ASSERT_TRUE(host->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(75.0, c->GetPixelY());
  // Modifying the height of the parent will effect the child.
  m->SetPixelHeight(150.0);
  ASSERT_TRUE(host->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(75.0, c->GetPixelY());
  m->SetRelativeY(-0.125);
  ASSERT_TRUE(host->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(-37.5, m->GetPixelY());
}

// This test is not merely for BasicElement, but mixed test for xml_utils
// and Elements.
TEST_F(BasicElementTest, TestFromXML) {
  MockedViewHost *host = new MockedViewHost(ViewHostInterface::VIEW_HOST_MAIN);
  View view(host, NULL, g_factory, NULL);

  Muffin m(&view, NULL);
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
  ASSERT_EQ(4U, children->GetCount());
  ASSERT_TRUE(e1 == children->GetItemByIndex(2));
  ASSERT_EQ(2U, e1->GetIndex());
  ASSERT_STREQ("muffin", e1->GetTagName());
  ASSERT_STREQ("", e1->GetName().c_str());
  ASSERT_TRUE(e2 == children->GetItemByIndex(1));
  ASSERT_EQ(1U, e2->GetIndex());
  ASSERT_STREQ("pie", e2->GetTagName());
  ASSERT_STREQ("", e2->GetName().c_str());
  ASSERT_TRUE(e3 == children->GetItemByIndex(0));
  ASSERT_EQ(0U, e3->GetIndex());
  ASSERT_TRUE(e3 == children->GetItemByName("a-pie"));
  ASSERT_STREQ("pie", e3->GetTagName());
  ASSERT_STREQ("a-pie", e3->GetName().c_str());
  ASSERT_TRUE(NULL == e4);
  ASSERT_TRUE(NULL == e5);
  ASSERT_TRUE(e6 == children->GetItemByIndex(3));
  ASSERT_EQ(3U, e6->GetIndex());
  ASSERT_TRUE(e6 == children->GetItemByName("big-pie"));
  ASSERT_STREQ("pie", e6->GetTagName());
  ASSERT_STREQ("big-pie", e6->GetName().c_str());
}

// This test is not merely for BasicElement, but mixed test for xml_utils
// and Elements.
TEST_F(BasicElementTest, XMLConstruction) {
  MockedViewHost *host = new MockedViewHost(ViewHostInterface::VIEW_HOST_MAIN);
  View view(host, NULL, g_factory, NULL);

  Muffin m(&view, NULL);

  const char *xml =
    "<muffin n1=\"yy\" name=\"top\">\n"
    "  <pie tooltip=\"pie-tooltip\" x=\"50%\" y=\"100\">\n"
    "    <muffin tagName=\"haha\" name=\"muffin\"/>\n"
    "  </pie>\n"
    "  <pie name=\"pie1\"/>\n"
    "</muffin>\n";
  m.GetChildren()->InsertElementFromXML(xml, NULL);
  ASSERT_EQ(1U, m.GetChildren()->GetCount());
  ggadget::BasicElement *e1 = m.GetChildren()->GetItemByIndex(0);
  ASSERT_TRUE(e1);
  ASSERT_EQ(0U, e1->GetIndex());
  ASSERT_TRUE(e1->IsInstanceOf(Muffin::CLASS_ID));
  ASSERT_FALSE(e1->IsInstanceOf(Pie::CLASS_ID));
  ASSERT_TRUE(e1->IsInstanceOf(ggadget::BasicElement::CLASS_ID));
  Muffin *m1 = down_cast<Muffin *>(e1);
  ASSERT_STREQ("top", m1->GetName().c_str());
  ASSERT_STREQ("muffin", m1->GetTagName());
  ASSERT_EQ(2U, m1->GetChildren()->GetCount());
  ggadget::BasicElement *e2 = m1->GetChildren()->GetItemByIndex(0);
  ASSERT_EQ(0U, e2->GetIndex());
  ASSERT_TRUE(e2);
  ASSERT_TRUE(e2->IsInstanceOf(Pie::CLASS_ID));
  ASSERT_FALSE(e2->IsInstanceOf(Muffin::CLASS_ID));
  ASSERT_TRUE(e2->IsInstanceOf(ggadget::BasicElement::CLASS_ID));
  Pie *p1 = down_cast<Pie *>(e2);
  ASSERT_STREQ("", p1->GetName().c_str());
  ASSERT_STREQ("pie", p1->GetTagName());
  ASSERT_STREQ("pie-tooltip", p1->GetTooltip().c_str());
  ASSERT_TRUE(p1->XIsRelative());
  ASSERT_DOUBLE_EQ(0.5, p1->GetRelativeX());
  ASSERT_FALSE(p1->YIsRelative());
  ASSERT_DOUBLE_EQ(100, p1->GetPixelY());
  ASSERT_EQ(1U, p1->GetChildren()->GetCount());
}

int main(int argc, char *argv[]) {
  ggadget::SetGlobalMainLoop(&main_loop);
  testing::ParseGTestFlags(&argc, argv);

  static const char *kExtensions[] = {
    "libxml2_xml_parser/libxml2-xml-parser",
  };
  INIT_EXTENSIONS(argc, argv, kExtensions);

  g_factory = new ggadget::ElementFactory();
  g_factory->RegisterElementClass("muffin", Muffin::CreateInstance);
  g_factory->RegisterElementClass("pie", Pie::CreateInstance);
  int result = RUN_ALL_TESTS();
  delete g_factory;
  return result;
}
