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

// This file is to be included by unittest files

#include <stdarg.h>
#include <stdio.h>
#include "ggadget/scriptable_interface.h"
#include "ggadget/signal.h"
#include "ggadget/signal_consts.h"
#include "ggadget/slot.h"
#include "ggadget/static_scriptable.h"

namespace ggadget {

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

// A normal scriptable class.
class TestScriptable1 : public ScriptableInterface {
 public:
  TestScriptable1()
      : static_scriptable_(new StaticScriptable()),
        double_property_(0),
        int_property_(0) {
    g_buffer.clear();
    static_scriptable_->RegisterMethod("TestMethodVoid0",
        NewSlot(this, &TestScriptable1::TestMethodVoid0));
    static_scriptable_->RegisterMethod("TestMethodDouble2",
        NewSlot(this, &TestScriptable1::TestMethodDouble2));
    static_scriptable_->RegisterProperty("DoubleProperty",
        NewSlot(this, &TestScriptable1::GetDoubleProperty),
        NewSlot(this, &TestScriptable1::SetDoubleProperty));
    static_scriptable_->RegisterProperty("BufferReadOnly",
        NewSlot(this, &TestScriptable1::GetBuffer), NULL);
    static_scriptable_->RegisterProperty("Buffer",
        NewSlot(this, &TestScriptable1::GetBuffer),
        NewSlot(this, &TestScriptable1::SetBuffer));
    static_scriptable_->RegisterSignal(kOnDeleteSignal, &ondelete_signal_);
    static_scriptable_->RegisterProperty("IntSimple",
        NewSimpleGetterSlot(&int_property_),
        NewSimpleSetterSlot(&int_property_));
    static_scriptable_->RegisterProperty("Fixed",
        NewFixedGetterSlot(123456789), NULL);
  }

  virtual ~TestScriptable1() {
    ondelete_signal_();
    AppendBuffer("Destruct\n");
  }

  virtual Connection *ConnectToOnDeleteSignal(Slot0<void> *slot) {
    return static_scriptable_->ConnectToOnDeleteSignal(slot);
  }
  virtual bool GetPropertyInfoByName(const char *name,
                                     int *id, Variant *prototype,
                                     bool *is_method) {
    return static_scriptable_->GetPropertyInfoByName(name, id,
                                                     prototype, is_method);
  }
  virtual bool GetPropertyInfoById(int id, Variant *prototype,
                                   bool *is_method) {
    return static_scriptable_->GetPropertyInfoById(id, prototype, is_method);
  }
  virtual Variant GetProperty(int id) {
    return static_scriptable_->GetProperty(id);
  }
  virtual bool SetProperty(int id, Variant value) {
    return static_scriptable_->SetProperty(id, value);
  }

  void TestMethodVoid0() {
    g_buffer.clear();
  }
  double TestMethodDouble2(bool p1, long p2) {
    AppendBuffer("TestMethodDouble2(%d, %ld)\n", p1, p2);
    return p1 ? p2 : -p2;
  }
  void SetDoubleProperty(double double_property) {
    double_property_ = double_property;
    AppendBuffer("SetDoubleProperty(%.3lf)\n", double_property_);
  }
  double GetDoubleProperty() const {
    AppendBuffer("GetDoubleProperty()=%.3lf\n", double_property_);
    return double_property_;
  }

  std::string GetBuffer() const {
    return g_buffer;
  }
  void SetBuffer(const std::string& buffer) {
    g_buffer = buffer;
  }

  OnDeleteSignal ondelete_signal_;

 private:
  StaticScriptable *static_scriptable_;
  double double_property_;
  int int_property_;
};

// A scriptable class with some dynamic properties, supporting array indexes,
// and some property/methods with arguments or return types of Scriptable.
class TestScriptable2 : public TestScriptable1 {
 public:
  TestScriptable2() {
    // Register...
  }
 protected:
  virtual ~TestScriptable2() {
  }
};

} // namespace ggadget
