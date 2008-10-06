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
#include "unittest/gtest.h"
#include "ggadget/basic_element.h"
#include "ggadget/elements.h"
#include "ggadget/view.h"
#include "ggadget/element_factory.h"
#include "ggadget/slot.h"
#include "mocked_element.h"
#include "mocked_timer_main_loop.h"
#include "mocked_view_host.h"

MockedTimerMainLoop main_loop(0);

// The total count of elements_.
int count = 0;

class MockedElementFactory : public ggadget::ElementFactory {
 public:
  MockedElementFactory() {
    RegisterElementClass("muffin", Muffin::CreateInstance);
    RegisterElementClass("pie", Pie::CreateInstance);
  }
};

class ElementsTest : public testing::Test {
 protected:
  virtual void SetUp() {
    factory_ = new MockedElementFactory();
    view_ = new ggadget::View(
        new MockedViewHost(ggadget::ViewHostInterface::VIEW_HOST_MAIN),
        NULL, factory_, NULL);
    view_elements_ = view_->GetChildren();
    muffin_ = new Muffin(view_, NULL);
    elements_ = muffin_->GetChildren();

    another_muffin_ = new Muffin(view_, NULL);
    another_elements_ = another_muffin_->GetChildren();
  }

  virtual void TearDown() {
    delete another_muffin_;
    delete muffin_;
    delete factory_;
    delete view_;
    ASSERT_EQ(0, count);
  }

  MockedElementFactory *factory_;
  ggadget::View *view_;
  ggadget::Elements *view_elements_;
  ggadget::Elements *elements_;
  Muffin *muffin_;

  ggadget::Elements *another_elements_;
  Muffin *another_muffin_;
};

#define ASSERT_INDEX(index, elements, element) \
  ASSERT_TRUE((element) == (elements)->GetItemByIndex(index)); \
  ASSERT_EQ((index), (element)->GetIndex());

TEST_F(ElementsTest, TestCreate) {
  ggadget::BasicElement *e1 = elements_->AppendElement("muffin", NULL);
  ASSERT_TRUE(e1 != NULL);
  ggadget::BasicElement *e2 = elements_->AppendElement("pie", NULL);
  ASSERT_TRUE(e2 != NULL);
  ggadget::BasicElement *e3 = elements_->AppendElement("bread", NULL);
  ASSERT_TRUE(e3 == NULL);
}

TEST_F(ElementsTest, TestCreateInView) {
  ggadget::BasicElement *e1 = view_elements_->AppendElement("muffin", NULL);
  ASSERT_TRUE(e1 != NULL);
  ggadget::BasicElement *e2 = view_elements_->AppendElement("pie", NULL);
  ASSERT_TRUE(e2 != NULL);
  ggadget::BasicElement *e3 = view_elements_->AppendElement("bread", NULL);
  ASSERT_TRUE(e3 == NULL);
}

TEST_F(ElementsTest, TestOrder) {
  ggadget::BasicElement *e1 = elements_->AppendElement("muffin", NULL);
  ggadget::BasicElement *e2 = elements_->AppendElement("pie", NULL);
  ggadget::BasicElement *e3 = elements_->AppendElement("pie", NULL);
  ASSERT_EQ(3U, elements_->GetCount());
  ASSERT_INDEX(0U, elements_, e1);
  ASSERT_INDEX(1U, elements_, e2);
  ASSERT_INDEX(2U, elements_, e3);
  ASSERT_TRUE(NULL == elements_->GetItemByIndex(3));

  ASSERT_TRUE(elements_->InsertElement(e1, e2));
  ASSERT_INDEX(0U, elements_, e1);
  ASSERT_INDEX(1U, elements_, e2);
  ASSERT_INDEX(2U, elements_, e3);

  ASSERT_TRUE(elements_->InsertElement(e3, NULL));
  ASSERT_INDEX(0U, elements_, e1);
  ASSERT_INDEX(1U, elements_, e2);
  ASSERT_INDEX(2U, elements_, e3);

  ASSERT_TRUE(elements_->InsertElement(e1, NULL));
  ASSERT_INDEX(0U, elements_, e2);
  ASSERT_INDEX(1U, elements_, e3);
  ASSERT_INDEX(2U, elements_, e1);

  ASSERT_TRUE(elements_->InsertElement(e1, e3));
  ASSERT_INDEX(0U, elements_, e2);
  ASSERT_INDEX(1U, elements_, e1);
  ASSERT_INDEX(2U, elements_, e3);
}

TEST_F(ElementsTest, TestOrderAfter) {
  ggadget::BasicElement *e1 = elements_->AppendElement("muffin", NULL);
  ggadget::BasicElement *e2 = elements_->AppendElement("pie", NULL);
  ggadget::BasicElement *e3 = elements_->AppendElement("pie", NULL);
  ASSERT_EQ(3U, elements_->GetCount());
  ASSERT_INDEX(0U, elements_, e1);
  ASSERT_INDEX(1U, elements_, e2);
  ASSERT_INDEX(2U, elements_, e3);
  ASSERT_TRUE(NULL == elements_->GetItemByIndex(3));

  ASSERT_TRUE(elements_->InsertElementAfter(e3, e2));
  ASSERT_INDEX(0U, elements_, e1);
  ASSERT_INDEX(1U, elements_, e2);
  ASSERT_INDEX(2U, elements_, e3);

  ASSERT_TRUE(elements_->InsertElementAfter(e1, NULL));
  ASSERT_INDEX(0U, elements_, e1);
  ASSERT_INDEX(1U, elements_, e2);
  ASSERT_INDEX(2U, elements_, e3);

  ASSERT_TRUE(elements_->InsertElementAfter(e3, NULL));
  ASSERT_INDEX(0U, elements_, e3);
  ASSERT_INDEX(1U, elements_, e1);
  ASSERT_INDEX(2U, elements_, e2);

  ASSERT_TRUE(elements_->InsertElementAfter(e2, e3));
  ASSERT_INDEX(0U, elements_, e3);
  ASSERT_INDEX(1U, elements_, e2);
  ASSERT_INDEX(2U, elements_, e1);
}

TEST_F(ElementsTest, TestOrderInView) {
  ggadget::BasicElement *e1 = view_elements_->AppendElement("muffin", NULL);
  ggadget::BasicElement *e2 = view_elements_->AppendElement("pie", NULL);
  ggadget::BasicElement *e3 = view_elements_->AppendElement("pie", NULL);
  ASSERT_EQ(3U, view_elements_->GetCount());
  ASSERT_INDEX(0U, view_elements_, e1);
  ASSERT_INDEX(1U, view_elements_, e2);
  ASSERT_INDEX(2U, view_elements_, e3);
  ASSERT_TRUE(NULL == view_elements_->GetItemByIndex(3));

  ASSERT_TRUE(view_elements_->InsertElement(e1, NULL));
  ASSERT_INDEX(0U, view_elements_, e2);
  ASSERT_INDEX(1U, view_elements_, e3);
  ASSERT_INDEX(2U, view_elements_, e1);

  ASSERT_TRUE(view_elements_->InsertElement(e1, e3));
  ASSERT_INDEX(0U, view_elements_, e2);
  ASSERT_INDEX(1U, view_elements_, e1);
  ASSERT_INDEX(2U, view_elements_, e3);
}

TEST_F(ElementsTest, TestReparent) {
  ggadget::BasicElement *e1 = elements_->AppendElement("muffin", NULL);
  ggadget::BasicElement *e2 = elements_->AppendElement("pie", NULL);
  ggadget::BasicElement *e3 = elements_->AppendElement("pie", NULL);
  ASSERT_EQ(3U, elements_->GetCount());
  ASSERT_INDEX(0U, elements_, e1);
  ASSERT_INDEX(1U, elements_, e2);
  ASSERT_INDEX(2U, elements_, e3);
  ASSERT_TRUE(NULL == elements_->GetItemByIndex(3));

  ASSERT_TRUE(another_elements_->AppendElement(e1));
  ASSERT_EQ(2U, elements_->GetCount());
  ASSERT_EQ(1U, another_elements_->GetCount());
  ASSERT_INDEX(0U, another_elements_, e1);
  ASSERT_INDEX(0U, elements_, e2);
  ASSERT_INDEX(1U, elements_, e3);

  ASSERT_TRUE(another_elements_->InsertElement(e2, e1));
  ASSERT_EQ(1U, elements_->GetCount());
  ASSERT_EQ(2U, another_elements_->GetCount());
  ASSERT_INDEX(1U, another_elements_, e1);
  ASSERT_INDEX(0U, another_elements_, e2);
  ASSERT_INDEX(0U, elements_, e3);
}

TEST_F(ElementsTest, TestReparentInView) {
  ASSERT_TRUE(view_elements_->AppendElement(muffin_));
  ASSERT_EQ(1U, view_elements_->GetCount());
  ASSERT_INDEX(0U, view_elements_, muffin_);
  ASSERT_TRUE(view_elements_->InsertElement(another_muffin_, muffin_));
  ASSERT_EQ(2U, view_elements_->GetCount());
  ASSERT_INDEX(1U, view_elements_, muffin_);
  ASSERT_INDEX(0U, view_elements_, another_muffin_);
  ASSERT_TRUE(elements_->AppendElement(another_muffin_));
  ASSERT_EQ(1U, view_elements_->GetCount());
  ASSERT_INDEX(0U, view_elements_, muffin_);
  ASSERT_EQ(1U, elements_->GetCount());
  ASSERT_INDEX(0U, elements_, another_muffin_);
  ASSERT_TRUE(view_elements_->AppendElement(another_muffin_));
  ASSERT_EQ(2U, view_elements_->GetCount());
  ASSERT_INDEX(0U, view_elements_, muffin_);
  ASSERT_INDEX(1U, view_elements_, another_muffin_);
  ASSERT_EQ(0U, elements_->GetCount());
  // To avoid duplicated delete.
  muffin_ = NULL;
  another_muffin_ = NULL;
}

TEST_F(ElementsTest, TestGetByName) {
  ggadget::BasicElement *e1 = elements_->AppendElement("muffin", "muffin1");
  ggadget::BasicElement *e2 = elements_->AppendElement("pie", "pie2");
  ggadget::BasicElement *e3 = elements_->AppendElement("pie", "pie3");
  ggadget::BasicElement *e4 = elements_->AppendElement("pie", "pie3");
  ASSERT_TRUE(e4 != e3);
  ASSERT_EQ(4U, elements_->GetCount());
  ASSERT_TRUE(e1 == elements_->GetItemByName("muffin1"));
  ASSERT_TRUE(e2 == elements_->GetItemByName("pie2"));
  ASSERT_TRUE(e3 == elements_->GetItemByName("pie3"));
  ASSERT_TRUE(NULL == elements_->GetItemByName("hungry"));
  ASSERT_TRUE(NULL == elements_->GetItemByName(""));
}

TEST_F(ElementsTest, TestInsert) {
  ggadget::BasicElement *e1 = elements_->InsertElement("muffin", NULL, NULL);
  ggadget::BasicElement *e2 = elements_->InsertElement("pie", e1, NULL);
  ggadget::BasicElement *e3 = elements_->InsertElement("pie", e2, NULL);
  ggadget::BasicElement *e4 = elements_->InsertElement("bread", e2, NULL);
  ASSERT_EQ(3U, elements_->GetCount());
  ASSERT_INDEX(2U, elements_, e1);
  ASSERT_INDEX(1U, elements_, e2);
  ASSERT_INDEX(0U, elements_, e3);
  ASSERT_TRUE(NULL == e4);
}

TEST_F(ElementsTest, TestInsertAfter) {
  ggadget::BasicElement *e1 = elements_->InsertElementAfter("muffin", NULL,
                                                            NULL);
  ggadget::BasicElement *e2 = elements_->InsertElementAfter("pie", e1, NULL);
  ggadget::BasicElement *e3 = elements_->InsertElementAfter("pie", e1, NULL);
  ggadget::BasicElement *e4 = elements_->InsertElementAfter("bread", e1, NULL);
  ASSERT_EQ(3U, elements_->GetCount());
  ASSERT_INDEX(0U, elements_, e1);
  ASSERT_INDEX(2U, elements_, e2);
  ASSERT_INDEX(1U, elements_, e3);
  ASSERT_TRUE(NULL == e4);
}

TEST_F(ElementsTest, TestInsertInView) {
  ggadget::BasicElement *e1 = view_elements_->InsertElement("muffin",
                                                            NULL, NULL);
  ggadget::BasicElement *e2 = view_elements_->InsertElement("pie", e1, NULL);
  ggadget::BasicElement *e3 = view_elements_->InsertElement("pie", e2, NULL);
  ggadget::BasicElement *e4 = view_elements_->InsertElement("bread", e2, NULL);
  ASSERT_EQ(3U, view_elements_->GetCount());
  ASSERT_INDEX(2U, view_elements_, e1);
  ASSERT_INDEX(1U, view_elements_, e2);
  ASSERT_INDEX(0U, view_elements_, e3);
  ASSERT_TRUE(NULL == e4);
}

TEST_F(ElementsTest, TestRemove) {
  ggadget::BasicElement *e1 = elements_->AppendElement("muffin", NULL);
  ggadget::BasicElement *e2 = elements_->AppendElement("pie", NULL);
  ggadget::BasicElement *e3 = elements_->AppendElement("pie", NULL);
  ASSERT_EQ(3U, elements_->GetCount());
  ASSERT_TRUE(elements_->RemoveElement(e2));
  ASSERT_EQ(2U, elements_->GetCount());
  ASSERT_INDEX(0U, elements_, e1);
  ASSERT_INDEX(1U, elements_, e3);
  ASSERT_TRUE(elements_->RemoveElement(e1));
  ASSERT_FALSE(elements_->RemoveElement(e1));
  ASSERT_INDEX(0U, elements_, e3);
}

TEST_F(ElementsTest, TestRemoveInView) {
  ggadget::BasicElement *e1 = view_elements_->AppendElement("muffin", NULL);
  ggadget::BasicElement *e2 = view_elements_->AppendElement("pie", NULL);
  ggadget::BasicElement *e3 = view_elements_->AppendElement("pie", NULL);
  ASSERT_EQ(3U, view_elements_->GetCount());
  ASSERT_TRUE(view_elements_->RemoveElement(e2));
  ASSERT_EQ(2U, view_elements_->GetCount());
  ASSERT_INDEX(0U, view_elements_, e1);
  ASSERT_INDEX(1U, view_elements_, e3);
  ASSERT_TRUE(view_elements_->RemoveElement(e1));
  ASSERT_FALSE(view_elements_->RemoveElement(e1));
  ASSERT_INDEX(0U, view_elements_, e3);
}

TEST_F(ElementsTest, TestRemoveAll) {
  elements_->AppendElement("muffin", NULL);
  elements_->AppendElement("pie", NULL);
  elements_->AppendElement("pie", NULL);
  ASSERT_EQ(3U, elements_->GetCount());
  elements_->RemoveAllElements();
  ASSERT_EQ(0U, elements_->GetCount());
}

TEST_F(ElementsTest, TestInvalidInsert) {
  ggadget::BasicElement *e1 = elements_->AppendElement("muffin", NULL);
  ASSERT_TRUE(e1);
  // Should fail if insert element before a non-child.
  ASSERT_FALSE(elements_->InsertElement("muffin", another_muffin_, NULL));
  // Should fail if insert element before itself.
  ASSERT_FALSE(elements_->InsertElement("muffin", muffin_, NULL));
  // Should fail if insert itself before any child.
  ASSERT_FALSE(elements_->InsertElement(muffin_, e1));
  // Should fail if append itself.
  ASSERT_FALSE(elements_->AppendElement(muffin_));
  // Should fail if insert element before a non-child.
  ASSERT_FALSE(elements_->InsertElement(muffin_, another_muffin_));
  // Should fail if append ancester to itself.
  ASSERT_FALSE(e1->GetChildren()->AppendElement(muffin_));

  // Should fail if insert element before a non-child.
  ASSERT_FALSE(view_elements_->InsertElement("muffin", another_muffin_, NULL));
  // Should fail if insert element before a non-child.
  ASSERT_FALSE(view_elements_->InsertElement(muffin_, another_muffin_));

  ggadget::View *view1 = new ggadget::View(
      new MockedViewHost(ggadget::ViewHostInterface::VIEW_HOST_MAIN),
      NULL, factory_, NULL);
  ggadget::BasicElement *e_another_view = new Muffin(view1, NULL);
  ASSERT_FALSE(elements_->AppendElement(e_another_view));
  ASSERT_FALSE(view_elements_->AppendElement(e_another_view));
  delete e_another_view;
  delete view1;
}

int main(int argc, char *argv[]) {
  ggadget::SetGlobalMainLoop(&main_loop);
  testing::ParseGTestFlags(&argc, argv);
  return RUN_ALL_TESTS();
}
