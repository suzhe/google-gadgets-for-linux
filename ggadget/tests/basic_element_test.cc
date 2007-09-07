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
#include "ggadget/elements_interface.h"
#include "mocked_view.h"

class Muffin : public ggadget::BasicElement {
 public:
  Muffin(ggadget::ElementInterface *parent,
         ggadget::ViewInterface *view,
         const char *name)
      : ggadget::BasicElement(parent, view, name) {
  }

  virtual ~Muffin() {
  }

 public:
  virtual const char *GetTagName() const {
    return "muffin";
  }

 public:
  static const uint64_t CLASS_ID = UINT64_C(0x6c0dee0e5bbe11dc);
  virtual bool IsInstanceOf(uint64_t class_id) const {
    return class_id == CLASS_ID ||
        ggadget::BasicElement::IsInstanceOf(class_id);
  }

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
      : ggadget::BasicElement(parent, view, name) {
  }

  virtual ~Pie() {
  }

 public:
  virtual const char *GetTagName() const {
    return "pie";
  }

 public:
  static const uint64_t CLASS_ID = UINT64_C(0x829defac5bbe11dc);
  virtual bool IsInstanceOf(uint64_t class_id) const {
    return class_id == CLASS_ID ||
        ggadget::BasicElement::IsInstanceOf(class_id);
  }

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
  Muffin m(NULL, NULL, NULL);
  Pie p(NULL, NULL, NULL);
}

TEST_F(BasicElementTest, TestChildren) {
  Muffin m(NULL, NULL, NULL);
  ggadget::ElementInterface *c1 = m.AppendElement("muffin", NULL);
  ggadget::ElementInterface *c2 = m.InsertElement("pie", c1, "First");
  ASSERT_EQ(2, m.GetChildren()->GetCount());
  ASSERT_TRUE(m.GetChildren()->GetItemByIndex(0) == c2);
  ASSERT_TRUE(m.GetChildren()->GetItemByIndex(1) == c1);
  ASSERT_TRUE(m.GetChildren()->GetItemByName("First") == c2);
  m.RemoveAllElements();
  ASSERT_EQ(0, m.GetChildren()->GetCount());
}

TEST_F(BasicElementTest, TestCursor) {
  Muffin m(NULL, NULL, NULL);
  ASSERT_TRUE(m.GetCursor() == ggadget::ElementInterface::CURSOR_ARROW);
  m.SetCursor(ggadget::ElementInterface::CURSOR_BUSY);
  ASSERT_TRUE(m.GetCursor() == ggadget::ElementInterface::CURSOR_BUSY);
}

TEST_F(BasicElementTest, TestDropTarget) {
  Muffin m(NULL, NULL, NULL);
  ASSERT_FALSE(m.IsDropTarget());
  m.SetDropTarget(true);
  ASSERT_TRUE(m.IsDropTarget());
}

TEST_F(BasicElementTest, TestEnabled) {
  Muffin m(NULL, NULL, NULL);
  ASSERT_FALSE(m.IsEnabled());
  m.SetEnabled(true);
  ASSERT_TRUE(m.IsEnabled());
}

TEST_F(BasicElementTest, TestPixelHeight) {
  Muffin m(NULL, NULL, NULL);
  ASSERT_FLOAT_EQ(0.0, m.GetPixelHeight());
  m.SetPixelHeight(100.0);
  ASSERT_FLOAT_EQ(100.0, m.GetPixelHeight());
  // Setting the height as negative value will make no effect.
  m.SetPixelHeight(-100.0);
  ASSERT_FLOAT_EQ(100.0, m.GetPixelHeight());
  ggadget::ElementInterface *c = m.AppendElement("pie", NULL);
  c->SetPixelHeight(50.0);
  // Modifying the height of the parent will not effect the child.
  m.SetPixelHeight(150.0);
  ASSERT_FLOAT_EQ(50.0, c->GetPixelHeight());
}

TEST_F(BasicElementTest, TestRelativeHeight) {
  MockedView v;
  Muffin m(NULL, &v, NULL);
  m.SetRelativeHeight(0.50);
  ASSERT_FLOAT_EQ(0.50, m.GetRelativeHeight());
  ASSERT_FLOAT_EQ(150.0, m.GetPixelHeight());
  ggadget::ElementInterface *c = m.AppendElement("pie", NULL);
  c->SetRelativeHeight(0.50);
  ASSERT_FLOAT_EQ(0.50, c->GetRelativeHeight());
  ASSERT_FLOAT_EQ(75.0, c->GetPixelHeight());
  // Setting the height as negative value will make no effect.
  c->SetRelativeHeight(-0.50);
  ASSERT_FLOAT_EQ(0.50, c->GetRelativeHeight());
  ASSERT_FLOAT_EQ(75.0, c->GetPixelHeight());
  // Modifying the height of the parent will effect the child.
  m.SetRelativeHeight(1.0);
  ASSERT_FLOAT_EQ(0.50, c->GetRelativeHeight());
  ASSERT_FLOAT_EQ(150.0, c->GetPixelHeight());
  // Modifying the height of the parent will effect the child.
  m.SetPixelHeight(100.0);
  ASSERT_FLOAT_EQ(50.0, c->GetPixelHeight());
}

TEST_F(BasicElementTest, TestHitTest) {
  Muffin m(NULL, NULL, NULL);
  ASSERT_TRUE(m.GetHitTest() == ggadget::ElementInterface::HT_DEFAULT);
  m.SetHitTest(ggadget::ElementInterface::HT_CLIENT);
  ASSERT_TRUE(m.GetHitTest() == ggadget::ElementInterface::HT_CLIENT);
}

TEST_F(BasicElementTest, TestMask) {
  Muffin m(NULL, NULL, NULL);
  ASSERT_STREQ("", m.GetMask());
  m.SetMask("mymask.png");
  ASSERT_STREQ("mymask.png", m.GetMask());
}

TEST_F(BasicElementTest, TestName) {
  Muffin m(NULL, NULL, "mymuffin");
  ASSERT_STREQ("mymuffin", m.GetName());
}

TEST_F(BasicElementTest, TestOpacity) {
  Muffin m(NULL, NULL, NULL);
  ASSERT_FLOAT_EQ(1.0, m.GetOpacity());
  m.SetOpacity(0.5);
  ASSERT_FLOAT_EQ(0.5, m.GetOpacity());
  // Setting the value greater than 1 will make no effect.
  m.SetOpacity(1.5);
  ASSERT_FLOAT_EQ(0.5, m.GetOpacity());
  // Setting the value less than 0 will make no effect.
  m.SetOpacity(-0.5);
  ASSERT_FLOAT_EQ(0.5, m.GetOpacity());
}

TEST_F(BasicElementTest, TestPixelPinX) {
  Muffin m(NULL, NULL, NULL);
  ASSERT_FLOAT_EQ(0.0, m.GetPixelPinX());
  m.SetPixelPinX(100.0);
  ASSERT_FLOAT_EQ(100.0, m.GetPixelPinX());
  // Modifying the width of the parent will not effect the pin x.
  m.SetPixelWidth(150.0);
  ASSERT_FLOAT_EQ(100.0, m.GetPixelPinX());
}

TEST_F(BasicElementTest, TestRelativePinX) {
  Muffin m(NULL, NULL, NULL);
  m.SetPixelWidth(200.0);
  m.SetRelativePinX(0.5);
  ASSERT_FLOAT_EQ(100.0, m.GetPixelPinX());
  // Modifying the width of the parent will effect the pin x.
  m.SetPixelWidth(400.0);
  ASSERT_FLOAT_EQ(200.0, m.GetPixelPinX());
}

TEST_F(BasicElementTest, TestPixelPinY) {
  Muffin m(NULL, NULL, NULL);
  m.SetPixelHeight(150.0);
  m.SetRelativePinY(0.5);
  ASSERT_FLOAT_EQ(75.0, m.GetPixelPinY());
  // Modifying the width of the parent will not effect the pin y.
  m.SetPixelHeight(300.0);
  ASSERT_FLOAT_EQ(150.0, m.GetPixelPinY());
}

TEST_F(BasicElementTest, TestRotation) {
  Muffin m(NULL, NULL, NULL);
  ASSERT_FLOAT_EQ(0.0, m.GetRotation());
  m.SetRotation(0.5);
  ASSERT_FLOAT_EQ(0.5, m.GetRotation());
}

TEST_F(BasicElementTest, TestToolTip) {
  Muffin m(NULL, NULL, NULL);
  ASSERT_STREQ("", m.GetToolTip());
  m.SetToolTip("mytooltip");
  ASSERT_STREQ("mytooltip", m.GetToolTip());
}

TEST_F(BasicElementTest, TestPixelWidth) {
  Muffin m(NULL, NULL, NULL);
  ASSERT_FLOAT_EQ(0.0, m.GetPixelWidth());
  m.SetPixelWidth(100.0);
  ASSERT_FLOAT_EQ(100.0, m.GetPixelWidth());
  // Setting the width as negative value will make no effect.
  m.SetPixelWidth(-100.0);
  ASSERT_FLOAT_EQ(100.0, m.GetPixelWidth());
  ggadget::ElementInterface *c = m.AppendElement("pie", NULL);
  c->SetPixelWidth(50.0);
  // Modifying the width of the parent will not effect the child.
  m.SetPixelWidth(200.0);
  ASSERT_FLOAT_EQ(50.0, c->GetPixelWidth());
}

TEST_F(BasicElementTest, TestRelativeWidth) {
  MockedView v;
  Muffin m(NULL, &v, NULL);
  m.SetRelativeWidth(0.50);
  ASSERT_FLOAT_EQ(0.50, m.GetRelativeWidth());
  ASSERT_FLOAT_EQ(200.0, m.GetPixelWidth());
  ggadget::ElementInterface *c = m.AppendElement("pie", NULL);
  c->SetRelativeWidth(0.50);
  ASSERT_FLOAT_EQ(0.50, c->GetRelativeWidth());
  ASSERT_FLOAT_EQ(100.0, c->GetPixelWidth());
  // Setting the width as negative value will make no effect.
  c->SetRelativeWidth(-0.50);
  ASSERT_FLOAT_EQ(0.50, c->GetRelativeWidth());
  ASSERT_FLOAT_EQ(100.0, c->GetPixelWidth());
  // Modifying the width of the parent will effect the child.
  m.SetRelativeWidth(1.0);
  ASSERT_FLOAT_EQ(0.50, c->GetRelativeWidth());
  ASSERT_FLOAT_EQ(200.0, c->GetPixelWidth());
  // Modifying the width of the parent will effect the child.
  m.SetPixelWidth(150.0);
  ASSERT_FLOAT_EQ(75.0, c->GetPixelWidth());
}

TEST_F(BasicElementTest, TestVisible) {
  Muffin m(NULL, NULL, NULL);
  ASSERT_TRUE(m.IsVisible());
  m.SetVisible(false);
  ASSERT_FALSE(m.IsVisible());
}

TEST_F(BasicElementTest, TestPixelX) {
  Muffin m(NULL, NULL, NULL);
  ASSERT_FLOAT_EQ(0.0, m.GetPixelX());
  m.SetPixelX(100.0);
  ASSERT_FLOAT_EQ(100.0, m.GetPixelX());
  ggadget::ElementInterface *c = m.AppendElement("pie", NULL);
  c->SetPixelX(50.0);
  // Modifying the width of the parent will not effect the child.
  m.SetPixelWidth(150.0);
  ASSERT_FLOAT_EQ(50.0, c->GetPixelX());
}

TEST_F(BasicElementTest, TestRelativeX) {
  MockedView v;
  Muffin m(NULL, &v, NULL);
  m.SetRelativeWidth(0.5);
  m.SetRelativeX(0.5);
  ASSERT_FLOAT_EQ(200.0, m.GetPixelX());
  ggadget::ElementInterface *c = m.AppendElement("pie", NULL);
  c->SetRelativeX(0.50);
  ASSERT_FLOAT_EQ(100.0, c->GetPixelX());
  // Modifying the width of the parent will effect the child.
  m.SetPixelWidth(100.0);
  ASSERT_FLOAT_EQ(50.0, c->GetPixelX());
}

TEST_F(BasicElementTest, TestPixelY) {
  Muffin m(NULL, NULL, NULL);
  ASSERT_FLOAT_EQ(0.0, m.GetPixelY());
  m.SetPixelY(100.0);
  ASSERT_FLOAT_EQ(100.0, m.GetPixelY());
  ggadget::ElementInterface *c = m.AppendElement("pie", NULL);
  c->SetPixelY(50.0);
  // Modifying the height of the parent will not effect the child.
  m.SetPixelHeight(150.0);
  ASSERT_FLOAT_EQ(50.0, c->GetPixelY());
}

TEST_F(BasicElementTest, TestRelativeY) {
  MockedView v;
  Muffin m(NULL, &v, NULL);
  m.SetRelativeHeight(0.5);
  m.SetRelativeY(0.5);
  ASSERT_FLOAT_EQ(150.0, m.GetPixelY());
  ggadget::ElementInterface *c = m.AppendElement("pie", NULL);
  c->SetRelativeY(0.50);
  ASSERT_FLOAT_EQ(75.0, c->GetPixelY());
  // Modifying the height of the parent will effect the child.
  m.SetPixelHeight(150.0);
  ASSERT_FLOAT_EQ(75.0, c->GetPixelY());
}

int main(int argc, char *argv[]) {
  testing::ParseGUnitFlags(&argc, argv);
  ggadget::ElementFactoryInterface *f = ggadget::ElementFactory::GetInstance();
  f->RegisterElementClass("muffin", Muffin::CreateInstance);
  f->RegisterElementClass("pie", Pie::CreateInstance);
  return RUN_ALL_TESTS();
}
