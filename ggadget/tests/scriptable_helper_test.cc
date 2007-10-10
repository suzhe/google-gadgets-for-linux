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
  const char *name;
  printf("CheckProperty: %d %s\n", i, info.name);
  ASSERT_TRUE(scriptable->GetPropertyInfoByName(info.name, &id,
                                                &prototype, &is_method));
  ASSERT_EQ(info.id, id);
  ASSERT_EQ(info.is_method, is_method);
  ASSERT_EQ(info.prototype, prototype);

  ASSERT_TRUE(scriptable->GetPropertyInfoById(id, &prototype,
                                              &is_method, &name));
  ASSERT_EQ(info.id, id);
  ASSERT_EQ(info.is_method, is_method);
  ASSERT_EQ(info.prototype, prototype);
  ASSERT_STREQ(info.name, name);
}

static void CheckFalseProperty(ScriptableInterface *scriptable,
                               const char *name) {
  int id;
  Variant prototype;
  bool is_method;
  ASSERT_FALSE(scriptable->GetPropertyInfoByName(name, &id,
                                                 &prototype, &is_method));
}

TEST(scriptable_helper, TestPropertyInfo) {
  TestScriptable1 *scriptable = new TestScriptable1();
  ASSERT_STREQ("", g_buffer.c_str());

  // Expected property information for TestScriptable1.
  PropertyInfo property_info[] = {
    { "TestMethodVoid0", -1, true,
      Variant(NewSlot(scriptable, &TestScriptable1::TestMethodVoid0)) },
    { "TestMethodDouble2", -2, true,
      Variant(NewSlot(scriptable, &TestScriptable1::TestMethodDouble2)) },
    { "DoubleProperty", -3, false, Variant(Variant::TYPE_DOUBLE) },
    { "BufferReadOnly", -4, false, Variant(Variant::TYPE_STRING) },
    { "Buffer", -5, false, Variant(Variant::TYPE_STRING) },
    { "JSON", -6, false, Variant(Variant::TYPE_JSON) },
    { "my_ondelete", -7, false,
      Variant(new SignalSlot(&scriptable->my_ondelete_signal_)) },
    { "EnumSimple", -8, false, Variant(Variant::TYPE_INT64) },
    { "EnumString", -9, false, Variant(Variant::TYPE_STRING) },
    { "VariantProperty", -10, false, Variant(Variant::TYPE_VARIANT) },
  };

  for (int i = 0; i < static_cast<int>(arraysize(property_info)); i++) {
    CheckProperty(i, scriptable, property_info[i]);
  }
  CheckFalseProperty(scriptable, "not_exist");

  for (int i = 0; i < static_cast<int>(arraysize(property_info)); i++) {
    if (property_info[i].prototype.type() == Variant::TYPE_SLOT)
      delete VariantValue<Slot *>()(property_info[i].prototype);
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

TEST(scriptable_helper, TestOnDelete) {
  TestScriptable1 *scriptable = new TestScriptable1();
  ASSERT_STREQ("", g_buffer.c_str());
  ASSERT_TRUE(scriptable->ConnectToOnDeleteSignal(NewSlot(TestOnDelete)));
  scriptable->SetProperty(-7, Variant(NewSlot(TestOnDeleteAsEventSink)));
  delete scriptable;
  EXPECT_STREQ("TestOnDeleteAsEventSink\nDestruct\nTestOnDelete\n",
               g_buffer.c_str());
}

TEST(scriptable_helper, TestPropertyAndMethod) {
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
  ASSERT_EQ(result1.type(), Variant::TYPE_SLOT);
  ASSERT_EQ(Variant(), VariantValue<Slot *>()(result1)->Call(0, NULL));
  ASSERT_STREQ("", g_buffer.c_str());

  // -8: the "EnumSimple" property.
  ASSERT_EQ(Variant(TestScriptable1::VALUE_0), scriptable->GetProperty(-8));
  ASSERT_TRUE(scriptable->SetProperty(-8, Variant(TestScriptable1::VALUE_2)));
  ASSERT_EQ(Variant(TestScriptable1::VALUE_2), scriptable->GetProperty(-8));

  // -9: the "EnumString" property.
  ASSERT_EQ(Variant("VALUE_2"), scriptable->GetProperty(-9));
  ASSERT_TRUE(scriptable->SetProperty(-9, Variant("VALUE_0")));
  ASSERT_EQ(Variant(TestScriptable1::VALUE_0), scriptable->GetProperty(-8));
  ASSERT_EQ(Variant("VALUE_0"), scriptable->GetProperty(-9));
  ASSERT_TRUE(scriptable->SetProperty(-9, Variant("VALUE_INVALID")));
  ASSERT_EQ(Variant(TestScriptable1::VALUE_0), scriptable->GetProperty(-8));
  ASSERT_EQ(Variant("VALUE_0"), scriptable->GetProperty(-9));
  
  // -10: the "VariantProperty" property.
  ASSERT_EQ(Variant(0), scriptable->GetProperty(-10));
  ASSERT_TRUE(scriptable->SetProperty(-10, Variant(1234)));
  ASSERT_EQ(Variant(1234), scriptable->GetProperty(-10));
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

TEST(scriptable_helper, TestConstants) {
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

TEST(scriptable_helper, TestPropertyInfo2) {
  TestScriptable2 *scriptable = new TestScriptable2();
  TestScriptable1 *scriptable1 = scriptable;
  ASSERT_STREQ("", g_buffer.c_str());

  // Expected property information for TestScriptable1.
  PropertyInfo property_info[] = {
    // -1 ~ -8 are inherited from TestScriptable1.
    { "TestMethodVoid0", -1, true,
      Variant(NewSlot(scriptable1, &TestScriptable1::TestMethodVoid0)) },
    { "TestMethodDouble2", -2, true,
      Variant(NewSlot(scriptable1, &TestScriptable1::TestMethodDouble2)) },
    { "DoubleProperty", -3, false, Variant(Variant::TYPE_DOUBLE) },
    { "BufferReadOnly", -4, false, Variant(Variant::TYPE_STRING) },
    { "Buffer", -5, false, Variant(Variant::TYPE_STRING) },
    { "JSON", -6, false, Variant(Variant::TYPE_JSON) },
    { "my_ondelete", -7, false,
      Variant(new SignalSlot(&scriptable->my_ondelete_signal_)) },
    { "EnumSimple", -8, false, Variant(Variant::TYPE_INT64) },
    { "EnumString", -9, false, Variant(Variant::TYPE_STRING) },
    { "VariantProperty", -10, false, Variant(Variant::TYPE_VARIANT) },

    // -9 ~ -14 are defined in TestScriptable2 itself.
    { "TestMethod", -11, true,
      Variant(NewSlot(scriptable, &TestScriptable2::TestMethod)) },
    { "onlunch", -12, false,
      Variant(new SignalSlot(&scriptable->onlunch_signal_)) },
    { "onsupper", -13, false,
      Variant(new SignalSlot(&scriptable->onsupper_signal_)) },
    { "time", -14, false, Variant(Variant::TYPE_STRING) },
    { "OverrideSelf", -15, false, Variant(Variant::TYPE_SCRIPTABLE) },
    { "SignalResult", -16, false, Variant(Variant::TYPE_STRING) },
    { "NewObject", -17, true,
      Variant(NewSlot(scriptable, &TestScriptable2::NewObject)) },
    { "DeleteObject", -18, true,
      Variant(NewSlot(scriptable, &TestScriptable2::DeleteObject)) },

    // The following are defined in the prototype.
    { "PrototypeMethod", -19, true,
      Variant(NewSlot(TestPrototype::GetInstance(), &TestPrototype::Method)) },
    { "PrototypeSelf", -20, false, Variant(Variant::TYPE_SCRIPTABLE) },
    { "ontest", -21, false,
      Variant(new SignalSlot(&TestPrototype::GetInstance()->ontest_signal_)) },
    // Prototype's OverrideSelf is overriden by TestScriptable2's OverrideSelf.
  };

  for (int i = 0; i < static_cast<int>(arraysize(property_info)); i++) {
    CheckProperty(i, scriptable, property_info[i]);
  }

  // Const is defined in prototype.
  CheckConstant("Const", scriptable, Variant(987654321));

  for (int i = 0; i < static_cast<int>(arraysize(property_info)); i++) {
    if (property_info[i].prototype.type() == Variant::TYPE_SLOT)
      delete VariantValue<Slot *>()(property_info[i].prototype);
  }

  delete scriptable;
  EXPECT_STREQ("Destruct\n", g_buffer.c_str());
}

TEST(scriptable_helper, TestArray) {
  TestScriptable2 *scriptable = new TestScriptable2();
  for (int i = 0; i < TestScriptable2::kArraySize; i++)
    ASSERT_TRUE(scriptable->SetProperty(i, Variant(i * 2)));
  for (int i = 0; i < TestScriptable2::kArraySize; i++)
    ASSERT_EQ(Variant(i * 2 + 10000), scriptable->GetProperty(i));

  int invalid_id = TestScriptable2::kArraySize;
  ASSERT_FALSE(scriptable->SetProperty(invalid_id, Variant(100)));
  ASSERT_EQ(Variant(), scriptable->GetProperty(invalid_id));
  delete scriptable;
}

TEST(scriptable_helper, TestDynamicProperty) {
  TestScriptable2 *scriptable = new TestScriptable2();
  int id;
  Variant prototype;
  bool is_method;
  char name[20];
  char value[20];
  const int num_tests = 10;

  for (int i = 0; i < num_tests; i++) {
    snprintf(name, sizeof(name), "d%d", i);
    snprintf(value, sizeof(value), "v%dv", i * 2);
    ASSERT_TRUE(scriptable->GetPropertyInfoByName(name, &id,
                                                  &prototype, &is_method));
    ASSERT_EQ(id, ScriptableInterface::ID_DYNAMIC_PROPERTY);
    ASSERT_FALSE(is_method);
    ASSERT_TRUE(scriptable->SetProperty(id, Variant(value)));
  }
  for (int i = 0; i < num_tests; i++) {
    snprintf(name, sizeof(name), "d%d", i);
    snprintf(value, sizeof(value), "Value:v%dv", i * 2);
    ASSERT_TRUE(scriptable->GetPropertyInfoByName(name, &id,
                                                  &prototype, &is_method));
    ASSERT_EQ(id, ScriptableInterface::ID_DYNAMIC_PROPERTY);
    ASSERT_FALSE(is_method);
    ASSERT_EQ(Variant(value), scriptable->GetProperty(id));
  }

  ASSERT_FALSE(scriptable->GetPropertyInfoByName("not_supported", &id,
                                                 &prototype, &is_method));
  delete scriptable;
}

int main(int argc, char **argv) {
  testing::ParseGUnitFlags(&argc, argv);
  return RUN_ALL_TESTS();
}
