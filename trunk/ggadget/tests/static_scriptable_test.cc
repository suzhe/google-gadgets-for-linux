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

#include "scriptables.cc"

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
    { "Buffer", -4, false, Variant(Variant::TYPE_STRING) },
    { kOnDeleteSignal, -5, false,
      // Here the result of new SignalSlot leaks.  Ignore it.
      Variant(new SignalSlot(&scriptable->ondelete_signal_)) },
  };

  for (int i = 0; i < arraysize(property_info); i++) {
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
  scriptable->SetProperty(-5, Variant(NewSlot(TestOnDeleteAsEventSink)));
  delete scriptable;
  EXPECT_STREQ("TestOnDeleteAsEventSink\nTestOnDelete\nDestruct\n",
               g_buffer.c_str());
}

TEST(static_scriptable, TestPropertyAndMethod) {
  TestScriptable1 *scriptable = new TestScriptable1();
  ASSERT_STREQ("", g_buffer.c_str());
  // -4: the "Buffer" property.
  ASSERT_EQ(Variant(""), scriptable->GetProperty(-4));
  AppendBuffer("TestBuffer\n");
  // "Buffer" is a readonly property.
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

  delete scriptable;
}

int main(int argc, char **argv) {
  testing::ParseGUnitFlags(&argc, argv);
  return RUN_ALL_TESTS();
}
