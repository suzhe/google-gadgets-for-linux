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

#include "scriptables.h"

using namespace ggadget;

// Store testing status to be checked in unit test code.
std::string g_buffer;

void AppendBuffer(const char *format, ...) {
  char buffer[1024];
  va_list ap;
  va_start(ap, format);
  vsnprintf(buffer, sizeof(buffer), format, ap);
  va_end(ap);
  g_buffer.append(buffer);
  printf("AppendBuffer: %s\n", buffer);
}

TestScriptable1::TestScriptable1()
    : static_scriptable_(new StaticScriptable()),
      double_property_(0),
      int_property_(0),
      variant_property_(0) {
  g_buffer.clear();
  RegisterMethod("TestMethodVoid0",
                 NewSlot(this, &TestScriptable1::TestMethodVoid0));
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
  RegisterSignal(kOnDeleteSignal, &ondelete_signal_);
  RegisterSimpleProperty("IntSimple", &int_property_);
  RegisterConstant("Fixed", 123456789);
  RegisterSimpleProperty("VariantProperty", &variant_property_);

  // Register 10 integer constants.
  static char names_arr[20][20]; // Ugly...
  char *names[10];
  for (int i = 0; i < 10; i++) {
    names[i] = names_arr[i];
    sprintf(names[i], "ICONSTANT%d", i);
  }
  static_scriptable_->RegisterConstants(10, names, NULL);

  // Register 10 string constants.
  Variant const_values[10];
  for (int i = 0; i < 10; i++) {
    names[i] = names_arr[i + 10];
    sprintf(names[i], "SCONSTANT%d", i);
    const_values[i] = Variant(names[i]);
  }
  static_scriptable_->RegisterConstants(10, names, const_values);
}

TestScriptable1::~TestScriptable1() {
  ondelete_signal_();
  delete static_scriptable_;
  AppendBuffer("Destruct\n");
}

TestPrototype *TestPrototype::instance_ = NULL;

TestPrototype::TestPrototype()
    : static_scriptable_(new StaticScriptable()) {
  RegisterMethod("PrototypeMethod", NewSlot(this, &TestPrototype::Method));
  RegisterProperty("PrototypeSelf", NewSlot(this, &TestPrototype::GetSelf),
                   NULL);
  RegisterSignal("ontest", &ontest_signal_);
  RegisterConstant("Const", 987654321);
  RegisterProperty("OverrideSelf",
                   NewSlot(this, &TestPrototype::GetSelf), NULL);
}

TestScriptable2::TestScriptable2() {
  RegisterMethod("TestMethod", NewSlot(this, &TestScriptable2::TestMethod));
  RegisterSignal("onlunch", &onlunch_signal_);
  RegisterSignal("onsupper", &onsupper_signal_);
  RegisterProperty("time",
      NewSimpleGetterSlot(&time_), NewSlot(this, &TestScriptable2::SetTime));
  RegisterProperty("OverrideSelf",
                   NewSlot(this, &TestScriptable2::GetSelf), NULL);
  RegisterConstant("length", kArraySize);
  RegisterReadonlySimpleProperty("SignalResult", &signal_result_);
  RegisterMethod("NewObject", NewSlot(this, &TestScriptable2::NewObject));
  RegisterMethod("DeleteObject", NewSlot(this, &TestScriptable2::DeleteObject));
  SetPrototype(TestPrototype::GetInstance());
}
