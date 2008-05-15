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

#include "ggadget/logger.h"
#include "scriptables.h"

using namespace ggadget;

// Store testing status to be checked in unit test code.
std::string g_buffer;
const char *enum_type_names[] = { "VALUE_0", "VALUE_1", "VALUE_2" };

void AppendBuffer(const char *format, ...) {
  char buffer[1024];
  va_list ap;
  va_start(ap, format);
  vsnprintf(buffer, sizeof(buffer), format, ap);
  va_end(ap);
  g_buffer.append(buffer);
  printf("AppendBuffer: %s\n", buffer);
}

TestScriptable1::TestScriptable1(bool native_owned)
    : native_owned_(native_owned),
      double_property_(0),
      enum_property_(VALUE_0),
      variant_property_(0) {
  if (native_owned) Ref();
  g_buffer.clear();
  RegisterMethod("ClearBuffer",
                 NewSlot(this, &TestScriptable1::ClearBuffer));
  RegisterMethod("TestMethodDouble2",
                 NewSlot(this, &TestScriptable1::TestMethodDouble2));
  RegisterProperty("DoubleProperty",
                   NewSlot(this, &TestScriptable1::GetDoubleProperty),
                   NewSlot(this, &TestScriptable1::SetDoubleProperty));
  RegisterProperty("BufferReadOnly",
                   NewSlot(this, &TestScriptable1::GetBuffer), NULL);
  RegisterProperty("Buffer",
                   NewSlot(this, &TestScriptable1::GetBuffer),
                   NewSlot(this, &TestScriptable1::SetBuffer));
  RegisterProperty("JSON",
                   NewSlot(this, &TestScriptable1::GetJSON),
                   NewSlot(this, &TestScriptable1::SetJSON));
  // This signal is only for test, no relation to ConnectOnReferenceChange.
  RegisterSignal("my_ondelete", &my_ondelete_signal_);
  RegisterSimpleProperty("EnumSimple", &enum_property_);
  RegisterStringEnumProperty("EnumString",
                             NewSimpleGetterSlot(&enum_property_),
                             NewSimpleSetterSlot(&enum_property_),
                             enum_type_names, arraysize(enum_type_names));
  RegisterConstant("Fixed", 123456789);
  RegisterSimpleProperty("VariantProperty", &variant_property_);
  RegisterConstants(arraysize(enum_type_names), enum_type_names, NULL);

  // Register 10 integer constants.
  static char names_arr[20][20]; // Ugly...
  char *names[10];
  for (int i = 0; i < 10; i++) {
    names[i] = names_arr[i];
    sprintf(names[i], "ICONSTANT%d", i);
  }
  RegisterConstants(10, names, NULL);

  // Register 10 string constants.
  Variant const_values[10];
  for (int i = 0; i < 10; i++) {
    names[i] = names_arr[i + 10];
    sprintf(names[i], "SCONSTANT%d", i);
    const_values[i] = Variant(names[i]);
  }
  RegisterConstants(10, names, const_values);
}

TestScriptable1::~TestScriptable1() {
  LOG("TestScriptable1 Destruct: this=%p", this);
  my_ondelete_signal_();
  AppendBuffer("Destruct\n");
  LOG("TestScriptable1 Destruct End: this=%p", this);
  if (native_owned_) Unref(true);
  // Then ScriptableHelper::~ScriptableHelper will be called, and in turn
  // the "official" ondelete signal will be emitted.
}

TestPrototype *TestPrototype::instance_ = NULL;

const Variant kNewObjectDefaultArgs[] = { Variant(true), Variant(true) };
const Variant kReleaseObjectDefaultArgs[] =
   { Variant(static_cast<ScriptableInterface *>(NULL)) };

TestPrototype::TestPrototype() {
  RegisterMethod("PrototypeMethod", NewSlot(this, &TestPrototype::Method));
  RegisterProperty("PrototypeSelf", NewSlot(this, &TestPrototype::GetSelf),
                   NULL);
  RegisterSignal("ontest", &ontest_signal_);
  RegisterConstant("Const", 987654321);
  RegisterProperty("OverrideSelf",
                   NewSlot(this, &TestPrototype::GetSelf), NULL);
}

TestScriptable2::TestScriptable2(bool native_owned, bool strict)
    : TestScriptable1(native_owned),
      strict_(strict), callback_(NULL) {
  RegisterMethod("TestMethod", NewSlot(this, &TestScriptable2::TestMethod));
  RegisterSignal("onlunch", &onlunch_signal_);
  RegisterSignal("onsupper", &onsupper_signal_);
  RegisterProperty("time",
      NewSimpleGetterSlot(&time_), NewSlot(this, &TestScriptable2::SetTime));
  RegisterProperty("OverrideSelf",
                   NewSlot(this, &TestScriptable2::GetSelf), NULL);
  RegisterConstant("length", kArraySize);
  RegisterReadonlySimpleProperty("SignalResult", &signal_result_);

  RegisterMethod("NewObject",
      NewSlotWithDefaultArgs(NewSlot(this, &TestScriptable2::NewObject),
                             kNewObjectDefaultArgs));
  RegisterMethod("ReleaseObject",
      NewSlotWithDefaultArgs(NewSlot(this, &TestScriptable2::ReleaseObject),
                             kReleaseObjectDefaultArgs));
  RegisterProperty("NativeOwned",
                   NewSlot(implicit_cast<TestScriptable1 *>(this),
                           &TestScriptable1::IsNativeOwned), NULL);
  RegisterMethod("ConcatArray", NewSlot(this, &TestScriptable2::ConcatArray));
  RegisterMethod("SetCallback", NewSlot(this, &TestScriptable2::SetCallback));
  RegisterMethod("CallCallback", NewSlot(this, &TestScriptable2::CallCallback));
  SetInheritsFrom(TestPrototype::GetInstance());
  SetArrayHandler(NewSlot(this, &TestScriptable2::GetArray),
                  NewSlot(this, &TestScriptable2::SetArray));
  SetDynamicPropertyHandler(
      NewSlot(this, &TestScriptable2::GetDynamicProperty),
      NewSlot(this, &TestScriptable2::SetDynamicProperty));
}

TestScriptable2::~TestScriptable2() {
  delete callback_;
  LOG("TestScriptable2 Destruct: this=%p", this);
}

ScriptableArray *TestScriptable2::ConcatArray(ScriptableInterface *array1,
                                              ScriptableInterface *array2) {
  if (array1 == NULL || array2 == NULL)
    return NULL;
  int id;
  Variant prototype;
  bool is_method;
  array1->GetPropertyInfoByName("length", &id, &prototype, &is_method);
  size_t count1 = VariantValue<size_t>()(array1->GetProperty(id));
  LOG("id=%d count1=%zd", id, count1);
  array2->GetPropertyInfoByName("length", &id, &prototype, &is_method);
  size_t count2 = VariantValue<size_t>()(array2->GetProperty(id));
  LOG("id=%d count2=%zd", id, count2);

  Variant *new_array = new Variant[count1 + count2];
  for (size_t i = 0; i < count1; i++)
    new_array[i] = array1->GetProperty(i);
  for (size_t i = 0; i < count2; i++)
    new_array[i + count1] = array2->GetProperty(i);
  return ScriptableArray::Create(new_array, count1 + count2);
}

void TestScriptable2::SetCallback(Slot *callback) {
  delete callback_;
  callback_ = callback;
}

std::string TestScriptable2::CallCallback(int x) {
  if (callback_) {
    Variant vx(x);
    Variant result = callback_->Call(1, &vx);
    return result.Print();
  }
  return "NO CALLBACK";
}
