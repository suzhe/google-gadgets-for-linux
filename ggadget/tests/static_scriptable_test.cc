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

#include "testing/gunit.h"

#include "scriptables.h"

using namespace ggadget;

// Structure of expected property information.
struct PropertyInfo {
  const char *name;
  int id;
  bool is_method;
  Variant prototype;
};

static void CheckProperty(int i, ScriptableInterface *scriptable,
                          const PropertyInfo &info) {
  int id;
  Variant prototype;
  bool is_method;
  printf("CheckProperty: %d %s\n", i, info.name);
  ASSERT_TRUE(scriptable->GetPropertyInfoByName(info.name, &id,
                                                &prototype, &is_method));
  ASSERT_EQ(info.id, id);
  ASSERT_EQ(info.is_method, is_method);
  ASSERT_EQ(info.prototype, prototype);

  ASSERT_TRUE(scriptable->GetPropertyInfoById(id, &prototype, &is_method));
  ASSERT_EQ(info.id, id);
  ASSERT_EQ(info.is_method, is_method);
  ASSERT_EQ(info.prototype, prototype);
}

TEST(static_scriptable, TestPropertyInfo) {
  TestScriptable1 *scriptable = new TestScriptable1();
  ASSERT_STREQ("", g_buffer.c_str());

  // Expected property information for TestScriptable1.
  PropertyInfo property_info[] = {
    { "TestMethodVoid0", -1, true,
      // Here the result of NewSlot leaks.  Ignore it.
      Variant(NewSlot(scriptable, &TestScriptable1::TestMethodVoid0)) },
    { "TestMethodDouble2", -2, true,
      // Here the result of NewSlot leaks.  Ignore it.
      Variant(NewSlot(scriptable, &TestScriptable1::TestMethodDouble2)) },
    { "DoubleProperty", -3, false, Variant(Variant::TYPE_DOUBLE) },
    { "BufferReadOnly", -4, false, Variant(Variant::TYPE_STRING) },
    { "Buffer", -5, false, Variant(Variant::TYPE_STRING) },
    { kOnDeleteSignal, -6, false,
      // Here the result of new SignalSlot leaks.  Ignore it.
      Variant(new SignalSlot(&scriptable->ondelete_signal_)) },
    { "IntSimple", -7, false, Variant(Variant::TYPE_INT64) },
    { "VariantProperty", -8, false, Variant(Variant::TYPE_VARIANT) },
  };

  for (int i = 0; i < static_cast<int>(arraysize(property_info)); i++) {
    CheckProperty(i, scriptable, property_info[i]);
  }

  delete scriptable;
  EXPECT_STREQ("Destruct\n", g_buffer.c_str());
}

void TestOnDelete() {
  AppendBuffer("TestOnDelete\n");
}

void TestOnDeleteAsEventSink() {
  AppendBuffer("TestOnDeleteAsEventSink\n");
}

TEST(static_scriptable, TestOnDelete) {
  TestScriptable1 *scriptable = new TestScriptable1();
  ASSERT_STREQ("", g_buffer.c_str());
  ASSERT_TRUE(scriptable->ConnectToOnDeleteSignal(NewSlot(TestOnDelete)));
  scriptable->SetProperty(-6, Variant(NewSlot(TestOnDeleteAsEventSink)));
  delete scriptable;
  EXPECT_STREQ("TestOnDeleteAsEventSink\nTestOnDelete\nDestruct\n",
               g_buffer.c_str());
}

TEST(static_scriptable, TestPropertyAndMethod) {
  TestScriptable1 *scriptable = new TestScriptable1();
  ASSERT_STREQ("", g_buffer.c_str());
  // -4: the "BufferReadOnly" property.
  ASSERT_EQ(Variant(""), scriptable->GetProperty(-4));
  AppendBuffer("TestBuffer\n");
  // "BufferReadOnly" is a readonly property.
  ASSERT_FALSE(scriptable->SetProperty(-4, Variant("Buffer\n")));
  ASSERT_EQ(Variant("TestBuffer\n"), scriptable->GetProperty(-4));
  g_buffer.clear();

  // -3: the "DoubleProperty" property.
  ASSERT_EQ(Variant(0.0), scriptable->GetProperty(-3));
  ASSERT_STREQ("GetDoubleProperty()=0.000\n", g_buffer.c_str());
  g_buffer.clear();
  ASSERT_TRUE(scriptable->SetProperty(-3, Variant(3.25)));
  ASSERT_STREQ("SetDoubleProperty(3.250)\n", g_buffer.c_str());
  g_buffer.clear();
  ASSERT_EQ(Variant(3.25), scriptable->GetProperty(-3));
  ASSERT_STREQ("GetDoubleProperty()=3.250\n", g_buffer.c_str());

  // -1: the "TestMethodVoid0" method.
  Variant result1(scriptable->GetProperty(-1));
  ASSERT_EQ(result1.type, Variant::TYPE_SLOT);
  ASSERT_EQ(Variant(), result1.v.slot_value->Call(0, NULL));
  ASSERT_STREQ("", g_buffer.c_str());

  // -7: the "SimpleInt" property.
  ASSERT_EQ(Variant(0), scriptable->GetProperty(-7));
  ASSERT_TRUE(scriptable->SetProperty(-7, Variant(54321)));
  ASSERT_EQ(Variant(54321), scriptable->GetProperty(-7));

  // -8: the "VariantProperty" property.
  ASSERT_EQ(Variant(0), scriptable->GetProperty(-8));
  ASSERT_TRUE(scriptable->SetProperty(-8, Variant(1234)));
  ASSERT_EQ(Variant(1234), scriptable->GetProperty(-8));
  delete scriptable;
}

static void CheckConstant(const char *name,
                          ScriptableInterface *scriptable,
                          Variant value) {
  int id;
  Variant prototype;
  bool is_method;
  printf("CheckConstant: %s\n", name);
  ASSERT_TRUE(scriptable->GetPropertyInfoByName(name, &id,
                                                &prototype, &is_method));
  ASSERT_EQ(0, id);
  ASSERT_FALSE(is_method);
  ASSERT_EQ(value, prototype);
}

TEST(static_scriptable, TestConstants) {
  TestScriptable1 *scriptable = new TestScriptable1();
  CheckConstant("Fixed", scriptable, Variant(123456789));
  for (int i = 0; i < 10; i++) {
    char name[20];
    sprintf(name, "ICONSTANT%d", i);
    CheckConstant(name, scriptable, Variant(i));
    sprintf(name, "SCONSTANT%d", i);
    CheckConstant(name, scriptable, Variant(name));
  }
  delete scriptable;
}

TEST(static_scriptable, TestPropertyInfo2) {
  TestScriptable2 *scriptable = new TestScriptable2();
  TestScriptable1 *scriptable1 = scriptable;
  ASSERT_STREQ("", g_buffer.c_str());

  // Expected property information for TestScriptable1.
  PropertyInfo property_info[] = {
    // -1 ~ -8 are inherited from TestScriptable1.
    { "TestMethodVoid0", -1, true,
      // Here the result of NewSlot leaks.  Ignore it.
      Variant(NewSlot(scriptable1, &TestScriptable1::TestMethodVoid0)) },
    { "TestMethodDouble2", -2, true,
      // Here the result of NewSlot leaks.  Ignore it.
      Variant(NewSlot(scriptable1, &TestScriptable1::TestMethodDouble2)) },
    { "DoubleProperty", -3, false, Variant(Variant::TYPE_DOUBLE) },
    { "BufferReadOnly", -4, false, Variant(Variant::TYPE_STRING) },
    { "Buffer", -5, false, Variant(Variant::TYPE_STRING) },
    { kOnDeleteSignal, -6, false,
      // Here the result of new SignalSlot leaks.  Ignore it.
      Variant(new SignalSlot(&scriptable->ondelete_signal_)) },
    { "IntSimple", -7, false, Variant(Variant::TYPE_INT64) },
    { "VariantProperty", -8, false, Variant(Variant::TYPE_VARIANT) },

    // -9 ~ -14 are defined in TestScriptable2 itself.
    { "TestMethod", -9, true,
      Variant(NewSlot(scriptable, &TestScriptable2::TestMethod)) },
    { "onlunch", -10, false,
      Variant(new SignalSlot(&scriptable->onlunch_signal_)) },
    { "onsupper", -11, false,
      Variant(new SignalSlot(&scriptable->onsupper_signal_)) },
    { "time", -12, false, Variant(Variant::TYPE_STRING) },
    { "OverrideSelf", -13, false, Variant(Variant::TYPE_SCRIPTABLE) },
    { "SignalResult", -14, false, Variant(Variant::TYPE_STRING) },
    { "NewObject", -15, true,
      Variant(NewSlot(scriptable, &TestScriptable2::NewObject)) },
    { "DeleteObject", -16, true,
      Variant(NewSlot(scriptable, &TestScriptable2::DeleteObject)) },

    // The following are defined in the prototype.
    { "PrototypeMethod", -17, true,
      Variant(NewSlot(TestPrototype::GetInstance(), &TestPrototype::Method)) },
    { "PrototypeSelf", -18, false, Variant(Variant::TYPE_SCRIPTABLE) },
    { "ontest", -19, false,
      Variant(new SignalSlot(&TestPrototype::GetInstance()->ontest_signal_)) },
    // Prototype's OverrideSelf is overriden by TestScriptable2's OverrideSelf.
  };

  for (int i = 0; i < static_cast<int>(arraysize(property_info)); i++) {
    CheckProperty(i, scriptable, property_info[i]);
  }

  // Const is defined in prototype.
  CheckConstant("Const", scriptable, Variant(987654321));

  delete scriptable;
  EXPECT_STREQ("Destruct\n", g_buffer.c_str());
}

TEST(static_scriptable, TestArray) {
  TestScriptable2 *scriptable = new TestScriptable2();
  Variant prototype;
  bool is_method;
  for (int i = 0; i < TestScriptable2::kArraySize; i++) {
    ASSERT_TRUE(scriptable->GetPropertyInfoById(i, &prototype, &is_method));
    ASSERT_EQ(false, is_method);
    ASSERT_EQ(Variant(Variant::TYPE_INT64), prototype);
    ASSERT_TRUE(scriptable->SetProperty(i, Variant(i * 2)));
  }
  for (int i = 0; i < TestScriptable2::kArraySize; i++) {
    ASSERT_EQ(Variant(i * 2), scriptable->GetProperty(i));
  }

  int invalid_id = TestScriptable2::kArraySize;
  ASSERT_FALSE(scriptable->GetPropertyInfoById(invalid_id,
                                               &prototype, &is_method));
  ASSERT_FALSE(scriptable->SetProperty(invalid_id, Variant(100)));
  ASSERT_EQ(Variant(), scriptable->GetProperty(invalid_id));
}

int main(int argc, char **argv) {
  testing::ParseGUnitFlags(&argc, argv);
  return RUN_ALL_TESTS();
}
