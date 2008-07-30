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
#include "scriptables.h"
#include "ggadget/scriptable_enumerator.h"

class MyItem {
 public:
  MyItem(char data) : data_(data) {
  }

  char GetValue() {
    return data_;
  }

 private:
  char data_;
};

class MyEnumeratable {
 public:
  MyEnumeratable(const char *str,
                 bool *flag) : start_(str), str_(str), flag_(flag) {
    ASSERT(str);
  }

  ~MyEnumeratable() {
    if (flag_)
      *flag_ = true;
  }

  bool AtEnd() {
    return *str_ == '\0';
  }

  MyItem GetItem() {
    return MyItem(*str_);
  }

  bool MoveFirst() {
    str_ = start_;
    return true;
  }

  bool MoveNext() {
    if (*str_) {
      return *(++str_) != 0;
    } else {
      return false;
    }
  }

 private:
  const char *start_;
  const char *str_;
  bool *flag_;
};

class MyItemWrapper : public ScriptableHelperDefault {
 public:
  DEFINE_CLASS_ID(0x33dff5245c8811dd, ScriptableInterface);

  MyItemWrapper(MyItem item) : data_(item.GetValue()) {
  }

  Variant GetValue() {
    return data_;
  }

 protected:
  virtual void DoClassRegister() {
    RegisterMethod("value", NewSlot(&MyItemWrapper::GetValue));
  }

 private:
  Variant data_;
};

TEST(ScriptableEnumeratorTest, CreateAndDestroy) {
  bool removed = false;
  BaseScriptable *base = new BaseScriptable(false, true);
  base->Ref();
  ggadget::ScriptableEnumerator<MyEnumeratable,
                                MyItemWrapper,
                                UINT64_C(0x09129e0a5c6011dd)> *enumerator =
                                    new ggadget::ScriptableEnumerator<
                                        MyEnumeratable,
                                        MyItemWrapper,
                                        UINT64_C(0x09129e0a5c6011dd)>(
                                            base,
                                            new MyEnumeratable(
                                                "test",
                                                &removed));
  enumerator->Ref();

  enumerator->Unref();
  base->Unref();

  EXPECT_TRUE(removed);
}

char GetItem(ggadget::ScriptableEnumerator<MyEnumeratable,
                                           MyItemWrapper,
                                           UINT64_C(0x09129e0a5c6011dd)> *e) {
  Variant get_item;
  EXPECT_TRUE(e->GetPropertyInfo("item", &get_item) ==
                  ggadget::ScriptableInterface::PROPERTY_METHOD);
  EXPECT_TRUE(get_item.type() == Variant::TYPE_SLOT);
  // Calls the method "item".
  ResultVariant resulti =
      VariantValue<Slot *>()(get_item)->Call(e, 0, NULL);

  // Retrieves the wrapper.
  MyItemWrapper *wrapper = VariantValue<MyItemWrapper *>()(resulti.v());
  Variant get_value;
  EXPECT_TRUE(wrapper->GetPropertyInfo("value", &get_value) ==
                  ggadget::ScriptableInterface::PROPERTY_METHOD);
  EXPECT_TRUE(get_value.type() == Variant::TYPE_SLOT);
  // Calls the method "value".
  ResultVariant resultv =
      VariantValue<Slot *>()(get_value)->Call(wrapper, 0, NULL);
  // Retrieves the data.
  char data = VariantValue<char>()(resultv.v());
  return data;
}

bool MoveFirst(ggadget::ScriptableEnumerator<MyEnumeratable,
                                             MyItemWrapper,
                                             UINT64_C(0x09129e0a5c6011dd)> *e) {
  Variant move_first;
  EXPECT_TRUE(e->GetPropertyInfo("moveFirst", &move_first) ==
                  ggadget::ScriptableInterface::PROPERTY_METHOD);
  EXPECT_TRUE(move_first.type() == Variant::TYPE_SLOT);
  // Calls the method.
  ResultVariant result =
      VariantValue<Slot *>()(move_first)->Call(e, 0, NULL);
  // Retrieves the wrapper.
  bool data = VariantValue<bool>()(result.v());
  return data;
}

bool MoveNext(ggadget::ScriptableEnumerator<MyEnumeratable,
                                            MyItemWrapper,
                                            UINT64_C(0x09129e0a5c6011dd)> *e) {
  Variant move_next;
  EXPECT_TRUE(e->GetPropertyInfo("moveNext", &move_next) ==
                  ggadget::ScriptableInterface::PROPERTY_METHOD);
  EXPECT_TRUE(move_next.type() == Variant::TYPE_SLOT);
  // Calls the method.
  ResultVariant result =
      VariantValue<Slot *>()(move_next)->Call(e, 0, NULL);
  // Retrieves the wrapper.
  bool data = VariantValue<bool>()(result.v());
  return data;
}

bool AtEnd(ggadget::ScriptableEnumerator<MyEnumeratable,
                                         MyItemWrapper,
                                         UINT64_C(0x09129e0a5c6011dd)> *e) {
  Variant at_end;
  EXPECT_TRUE(e->GetPropertyInfo("atEnd", &at_end) ==
                  ggadget::ScriptableInterface::PROPERTY_METHOD);
  EXPECT_TRUE(at_end.type() == Variant::TYPE_SLOT);
  // Calls the method.
  ResultVariant result =
      VariantValue<Slot *>()(at_end)->Call(e, 0, NULL);
  // Retrieves the wrapper.
  bool data = VariantValue<bool>()(result.v());
  return data;
}

TEST(ScriptableEnumeratorTest, Enumerate) {
  BaseScriptable *base = new BaseScriptable(false, true);
  base->Ref();
  ggadget::ScriptableEnumerator<MyEnumeratable,
                                MyItemWrapper,
                                UINT64_C(0x09129e0a5c6011dd)> *enumerator =
                                    new ggadget::ScriptableEnumerator<
                                        MyEnumeratable,
                                        MyItemWrapper,
                                        UINT64_C(0x09129e0a5c6011dd)>(
                                            base,
                                            new MyEnumeratable(
                                                "test",
                                                NULL));
  enumerator->Ref();

  EXPECT_FALSE(AtEnd(enumerator));
  EXPECT_EQ('t', GetItem(enumerator));

  EXPECT_TRUE(MoveNext(enumerator));
  EXPECT_FALSE(AtEnd(enumerator));
  EXPECT_EQ('e', GetItem(enumerator));

  EXPECT_TRUE(MoveNext(enumerator));
  EXPECT_FALSE(AtEnd(enumerator));
  EXPECT_EQ('s', GetItem(enumerator));

  EXPECT_TRUE(MoveNext(enumerator));
  EXPECT_FALSE(AtEnd(enumerator));
  EXPECT_EQ('t', GetItem(enumerator));

  EXPECT_FALSE(MoveNext(enumerator));
  EXPECT_TRUE(AtEnd(enumerator));

  EXPECT_FALSE(MoveNext(enumerator));

  EXPECT_TRUE(MoveFirst(enumerator));
  EXPECT_FALSE(AtEnd(enumerator));
  EXPECT_EQ('t', GetItem(enumerator));

  enumerator->Unref();
  base->Unref();
}

int main(int argc, char *argv[]) {
  testing::ParseGTestFlags(&argc, argv);
  int result = RUN_ALL_TESTS();
  return result;
}
