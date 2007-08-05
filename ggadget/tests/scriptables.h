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

#ifndef GGADGET_TESTS_SCRIPTABLES_H__
#define GGADGET_TESTS_SCRIPTABLES_H__

// This file is to be included by unittest files

#include <stdarg.h>
#include <stdio.h>
#include "ggadget/scriptable_interface.h"
#include "ggadget/signal.h"
#include "ggadget/slot.h"
#include "ggadget/static_scriptable.h"

using namespace ggadget;

// Store testing status to be checked in unit test code.
extern std::string g_buffer;

void AppendBuffer(const char *format, ...);

// A normal scriptable class.
class TestScriptable1 : public ScriptableInterface {
 public:
  static const int CLASS_ID = -999;
  TestScriptable1();
  virtual ~TestScriptable1();

  virtual bool IsInstanceOf(int class_id) {
    return class_id == CLASS_ID || class_id == ScriptableInterface::CLASS_ID; 
  }

  DEFAULT_OWNERSHIP_POLICY
  DELEGATE_SCRIPTABLE_INTERFACE(static_scriptable_)
  DELEGATE_SCRIPTABLE_REGISTER(static_scriptable_)

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

  // This signal is only for test, no relation to ConnectToOndeleteSignal.
  // Place this signal declaration here for testing.
  Signal0<void> my_ondelete_signal_;

  enum EnumType { VALUE_0, VALUE_1, VALUE_2 };
  
 private:
  StaticScriptable static_scriptable_;

  double double_property_;
  EnumType enum_property_;
  Variant variant_property_;
};

typedef Signal1<std::string, const std::string &> OnLunchSignal;
typedef Signal1<std::string, const std::string &> OnSupperSignal;

class TestPrototype : public ScriptableInterface {
 public:
  static const int CLASS_ID = -888;
  static TestPrototype *GetInstance() {
    return instance_ ? instance_ : (instance_ = new TestPrototype());
  }

  // Should not be called.
  virtual bool IsInstanceOf(int class_id) {
    return class_id == ScriptableInterface::CLASS_ID || class_id == CLASS_ID;
  }

  DEFAULT_OWNERSHIP_POLICY
  DELEGATE_SCRIPTABLE_INTERFACE(static_scriptable_)
  DELEGATE_SCRIPTABLE_REGISTER(static_scriptable_)

  // Place this signal declaration here for testing.
  // In production code, it should be palced in private section. 
  Signal0<void> ontest_signal_;

  ScriptableInterface *Method(ScriptableInterface *s) { return s; }
  TestPrototype *GetSelf() { return this; }

 private:
  TestPrototype();

  static TestPrototype *instance_;
  StaticScriptable static_scriptable_;
};

// A scriptable class with some dynamic properties, supporting array indexes,
// and some property/methods with arguments or return types of Scriptable.
class TestScriptable2 : public TestScriptable1 {
 public:
  static const int CLASS_ID = -1000;
  TestScriptable2(bool script_owned_ = false);

  virtual void Detach() { if (script_owned_) delete this; }

  virtual bool IsInstanceOf(int class_id) {
    return class_id == CLASS_ID || TestScriptable1::IsInstanceOf(class_id); 
  }

  // A scriptable providing array indexed access should override the following
  // methods.
  virtual bool GetPropertyInfoById(int id, Variant *prototype,
                                   bool *is_method) {
    if (id >= 0 && id < kArraySize) {
      *prototype = Variant(Variant::TYPE_INT64);
      *is_method = false;
      return true;
    }
    return TestScriptable1::GetPropertyInfoById(id, prototype, is_method);
  }

  static const int kArraySize = 20;
  virtual Variant GetProperty(int id) {
    if (id >= 0 && id < kArraySize)
      return Variant(array_[id]);
    return TestScriptable1::GetProperty(id);
  }

  virtual bool SetProperty(int id, Variant value) {
    if (id >= 0 && id < kArraySize) {
      array_[id] = VariantValue<int>()(value);
      return true;
    }
    return TestScriptable1::SetProperty(id, value);
  }

  void SetTime(const std::string &time) {
    time_ = time;
    if (time == "lunch")
      signal_result_ = onlunch_signal_(std::string("Have lunch"));
    else if (time == "supper")
      signal_result_= onsupper_signal_(std::string("Have supper"));
  }

  TestScriptable2 *GetSelf() { return this; }
  TestScriptable2 *TestMethod(TestScriptable2 *t) { return t; }
  TestScriptable2 *NewObject(bool script_owned) {
    return new TestScriptable2(script_owned);
  }
  void DeleteObject(TestScriptable2 *obj) { delete obj; }

  // Place signal declarations here for testing.
  // In production code, they should be palced in private section.
  OnLunchSignal onlunch_signal_;
  OnSupperSignal onsupper_signal_;

 private:
  bool script_owned_;
  int array_[kArraySize];
  std::string time_;
  std::string signal_result_;
};

#endif // GGADGET_TESTS_SCRIPTABLES_H__
