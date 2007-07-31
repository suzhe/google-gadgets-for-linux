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
#include "testing/gunit.h"
#include "ggadget/elements_impl.h"
#include "ggadget/elements.h"
#include "ggadget/element_interface.h"
#include "ggadget/element_factory_interface.h"
#include "mocked_element.h"

// The total count of elements.
int count = 0;

class Muffin : public MockedElement {
 public:
  Muffin(ggadget::ElementInterface *parent) : MockedElement(parent) {
    ++count;
  }

  virtual ~Muffin() {
    --count;
  }

 public:
  virtual const char *type() const {
    return "muffin";
  }
};

class Pie : public MockedElement {
 public:
  Pie(ggadget::ElementInterface *parent) : MockedElement(parent) {
    ++count;
  }

  virtual ~Pie() {
    --count;
  }

 public:
  virtual const char *type() const {
    return "pie";
  }
};

class MockedElementFactory : public ggadget::ElementFactoryInterface {
  virtual ggadget::ElementInterface *CreateElement(
      const char *tag_name,
      ggadget::ElementInterface *parent,
      const char *name) {
    if (strcmp(tag_name, "muffin") == 0)
      return new Muffin(parent);
    if (strcmp(tag_name, "pie") == 0)
      return new Pie(parent);
    return NULL;
  }

  virtual bool RegisterElementClass(
      const char *tag_name,
      ggadget::ElementInterface *(*creator)(ggadget::ElementInterface *,
                                            const char *)) {
    return true;
  }
};

class ElementsTest : public testing::Test {
 protected:
  virtual void SetUp() {
    factory = new MockedElementFactory();
    elements = new ggadget::Elements(factory, NULL);
  }

  virtual void TearDown() {
    delete elements;
    delete factory;
    ASSERT_EQ(0, count);
  }

  MockedElementFactory *factory;
  ggadget::Elements *elements;
};

TEST_F(ElementsTest, TestCreate) {
  ggadget::ElementInterface *e1 = elements->AppendElement("muffin", NULL);
  ASSERT_TRUE(e1 != NULL);
  ggadget::ElementInterface *e2 = elements->AppendElement("pie", NULL);
  ASSERT_TRUE(e2 != NULL);
  ggadget::ElementInterface *e3 = elements->AppendElement("bread", NULL);
  ASSERT_TRUE(e3 == NULL);
}

TEST_F(ElementsTest, TestOrder) {
  ggadget::ElementInterface *e1 = elements->AppendElement("muffin", NULL);
  ggadget::ElementInterface *e2 = elements->AppendElement("pie", NULL);
  ggadget::ElementInterface *e3 = elements->AppendElement("pie", NULL);
  ASSERT_EQ(3, elements->GetCount());
  ASSERT_TRUE(e1 == elements->GetItem(0));
  ASSERT_TRUE(e2 == elements->GetItem(1));
  ASSERT_TRUE(e3 == elements->GetItem(2));
  ASSERT_TRUE(NULL == elements->GetItem(3));
}

TEST_F(ElementsTest, TestInsert) {
  ggadget::ElementInterface *e1 = elements->InsertElement("muffin", NULL, NULL);
  ggadget::ElementInterface *e2 = elements->InsertElement("pie", e1, NULL);
  ggadget::ElementInterface *e3 = elements->InsertElement("pie", e2, NULL);
  ASSERT_EQ(3, elements->GetCount());
  ASSERT_TRUE(e1 == elements->GetItem(2));
  ASSERT_TRUE(e2 == elements->GetItem(1));
  ASSERT_TRUE(e3 == elements->GetItem(0));
}

TEST_F(ElementsTest, TestRemove) {
  ggadget::ElementInterface *e1 = elements->AppendElement("muffin", NULL);
  ggadget::ElementInterface *e2 = elements->AppendElement("pie", NULL);
  ggadget::ElementInterface *e3 = elements->AppendElement("pie", NULL);
  ASSERT_EQ(3, elements->GetCount());
  ASSERT_TRUE(elements->RemoveElement(e2));
  ASSERT_EQ(2, elements->GetCount());
  ASSERT_TRUE(elements->GetItem(0) == e1);
  ASSERT_TRUE(elements->GetItem(1) == e3);
  ASSERT_TRUE(elements->RemoveElement(e1));
  ASSERT_FALSE(elements->RemoveElement(e1));
  ASSERT_TRUE(elements->GetItem(0) == e3);
}

int main(int argc, char *argv[]) {
  testing::ParseGUnitFlags(&argc, argv);
  return RUN_ALL_TESTS();
}
