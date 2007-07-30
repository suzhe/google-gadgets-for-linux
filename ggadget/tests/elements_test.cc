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

// The total count of elements.
int count = 0;

class Muffin : public ggadget::ElementInterface {
 public:
  Muffin(ggadget::ElementInterface *parent) {
    ++count;
  }

  ~Muffin() {
    --count;
  }

 public:
  virtual const char *type() const {
    return "muffin";
  }

  virtual void Release() {
    delete this;
  }
};

class Pie : public ggadget::ElementInterface {
 public:
  Pie(ggadget::ElementInterface *parent) {
    ++count;
  }

  ~Pie() {
    --count;
  }

 public:
  virtual const char *type() const {
    return "pie";
  }

  virtual void Release() {
    delete this;
  }
};

class MockedElementFactory : public ggadget::ElementFactoryInterface {
  virtual ggadget::ElementInterface *CreateElement(
      const char *type, ggadget::ElementInterface *parent) {
    if (strcmp(type, "muffin") == 0)
      return new Muffin(parent);
    if (strcmp(type, "pie") == 0)
      return new Pie(parent);
    return NULL;
  }

  virtual bool RegisterElementClass(const char *type,
                                    ggadget::ElementInterface *(*creator)()) {
    return true;
  }
};

class ElementsImplTest : public testing::Test {
 protected:
  virtual void SetUp() {
    factory = new MockedElementFactory();
    elements = new ggadget::Elements(factory, NULL);
    elements->Init();
  }

  virtual void TearDown() {
    delete elements;
    delete factory;
    ASSERT_EQ(0, count);
  }

  MockedElementFactory *factory;
  ggadget::Elements *elements;
};

TEST_F(ElementsImplTest, TestCreate) {
  ggadget::ElementInterface *e1 = elements->AppendElement("muffin");
  ASSERT_TRUE(e1 != NULL);
  ggadget::ElementInterface *e2 = elements->AppendElement("pie");
  ASSERT_TRUE(e2 != NULL);
  ggadget::ElementInterface *e3 = elements->AppendElement("bread");
  ASSERT_TRUE(e3 == NULL);
}

TEST_F(ElementsImplTest, TestOrder) {
  ggadget::ElementInterface *e1 = elements->AppendElement("muffin");
  ggadget::ElementInterface *e2 = elements->AppendElement("pie");
  ggadget::ElementInterface *e3 = elements->AppendElement("pie");
  ASSERT_EQ(3, elements->GetCount());
  ASSERT_TRUE(e1 == elements->GetItem(0));
  ASSERT_TRUE(e2 == elements->GetItem(1));
  ASSERT_TRUE(e3 == elements->GetItem(2));
  ASSERT_TRUE(NULL == elements->GetItem(3));
}

TEST_F(ElementsImplTest, TestInsert) {
  ggadget::ElementInterface *e1 = elements->InsertElement("muffin", NULL);
  ggadget::ElementInterface *e2 = elements->InsertElement("pie", e1);
  ggadget::ElementInterface *e3 = elements->InsertElement("pie", e2);
  ASSERT_EQ(3, elements->GetCount());
  ASSERT_TRUE(e1 == elements->GetItem(2));
  ASSERT_TRUE(e2 == elements->GetItem(1));
  ASSERT_TRUE(e3 == elements->GetItem(0));
}

TEST_F(ElementsImplTest, TestRemove) {
  ggadget::ElementInterface *e1 = elements->AppendElement("muffin");
  ggadget::ElementInterface *e2 = elements->AppendElement("pie");
  ggadget::ElementInterface *e3 = elements->AppendElement("pie");
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
