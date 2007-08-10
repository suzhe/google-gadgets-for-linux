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
#include "ggadget/element_factory_impl.h"
#include "ggadget/element_factory.h"
#include "mocked_element.h"

class Muffin : public MockedElement {
 public:
  Muffin(ggadget::ElementInterface *parent,
         ggadget::ViewInterface *view,
         const char *name) : MockedElement(parent, view, name) {
  }

  virtual ~Muffin() {
  }

 public:
  virtual const char *GetTagName() const {
    return "muffin";
  }

 public:
  static ggadget::ElementInterface *CreateInstance(
      ggadget::ElementInterface *parent,
      ggadget::ViewInterface *view,
      const char *name) {
    return new Muffin(parent, view, name);
  }
};

class Pie : public MockedElement {
 public:
  Pie(ggadget::ElementInterface *parent,
      ggadget::ViewInterface *view,
      const char *name) : MockedElement(parent, view, name) {
  }

  virtual ~Pie() {
  }

 public:
  virtual const char *GetTagName() const {
    return "pie";
  }

 public:
  static ggadget::ElementInterface *CreateInstance(
      ggadget::ElementInterface *parent,
      ggadget::ViewInterface *view,
      const char *name) {
    return new Pie(parent, view, name);
  }
};

class ElementFactoryTest : public testing::Test {
 protected:
  virtual void SetUp() {
  }

  virtual void TearDown() {
  }
};

TEST_F(ElementFactoryTest, TestSingleton) {
  ggadget::ElementFactoryInterface *inter1 =
      ggadget::ElementFactory::GetInstance();
  ggadget::ElementFactoryInterface *inter2 =
      ggadget::ElementFactory::GetInstance();
  ASSERT_TRUE(inter1 == inter2);
}

TEST_F(ElementFactoryTest, TestRegister) {
  ggadget::internal::ElementFactoryImpl impl;
  ASSERT_TRUE(impl.RegisterElementClass("muffin", Muffin::CreateInstance));
  ASSERT_FALSE(impl.RegisterElementClass("muffin", Muffin::CreateInstance));
  ASSERT_TRUE(impl.RegisterElementClass("pie", Pie::CreateInstance));
  ASSERT_FALSE(impl.RegisterElementClass("pie", Pie::CreateInstance));
}

TEST_F(ElementFactoryTest, TestCreate) {
  ggadget::ElementFactoryInterface *factory =
      ggadget::ElementFactory::GetInstance();
  factory->RegisterElementClass("muffin", Muffin::CreateInstance);
  factory->RegisterElementClass("pie", Pie::CreateInstance);

  ggadget::ElementInterface *e1 = factory->CreateElement("muffin",
                                                         NULL,
                                                         NULL,
                                                         NULL);
  ASSERT_TRUE(e1 != NULL);
  ASSERT_STREQ(e1->GetTagName(), "muffin");

  ggadget::ElementInterface *e2 = factory->CreateElement("pie",
                                                         e1,
                                                         NULL,
                                                         NULL);
  ASSERT_TRUE(e2 != NULL);
  ASSERT_STREQ(e2->GetTagName(), "pie");

  ggadget::ElementInterface *e3 = factory->CreateElement("bread",
                                                         e2,
                                                         NULL,
                                                         NULL);
  ASSERT_TRUE(e3 == NULL);

  e1->Destroy();
  e2->Destroy();
}

int main(int argc, char *argv[]) {
  testing::ParseGUnitFlags(&argc, argv);
  return RUN_ALL_TESTS();
}
