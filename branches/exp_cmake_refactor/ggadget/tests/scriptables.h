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

#include <string>
#include <map>
#include <stdarg.h>
#include <stdio.h>
#include "ggadget/scriptable_interface.h"
#include "ggadget/scriptable_helper.h"
#include "ggadget/signals.h"
#include "ggadget/slot.h"

using namespace ggadget;

// Store testing status to be checked in unit test code.
extern std::string g_buffer;

void AppendBuffer(const char *format, ...);

// A normal scriptable class.
class TestScriptable1 : public ScriptableHelper<ScriptableInterface> {
 public:
  DEFINE_CLASS_ID(0xdb06ba021f1b4c05, ScriptableInterface);

  TestScriptable1();
  virtual ~TestScriptable1();

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
    g_buffer = "Buffer:" + buffer;
  }

  JSONString GetJSON() const {
    return JSONString(g_buffer);
  }
  void SetJSON(const JSONString json) {
    g_buffer = json.value;
  }

  // This signal is only for test, no relation to ConnectToOndeleteSignal.
  // Place this signal declaration here for testing.
  Signal0<void> my_ondelete_signal_;

  enum EnumType { VALUE_0, VALUE_1, VALUE_2 };

 private:

  double double_property_;
  EnumType enum_property_;
  Variant variant_property_;
};

class TestPrototype : public ScriptableHelper<ScriptableInterface> {
 public:
  DEFINE_CLASS_ID(0xbb7f8eddc2e94353, ScriptableInterface);
  static TestPrototype *GetInstance() {
    return instance_ ? instance_ : (instance_ = new TestPrototype());
  }

  virtual OwnershipPolicy Attach() {
    return NATIVE_PERMANENT;
  }

  // Place this signal declaration here for testing.
  // In production code, it should be palced in private section. 
  Signal0<void> ontest_signal_;

  ScriptableInterface *Method(const ScriptableInterface *s) { 
    return const_cast<ScriptableInterface *>(s);
  }
  TestPrototype *GetSelf() { return this; }

 private:
  TestPrototype();

  static TestPrototype *instance_;
};

// A scriptable class with some dynamic properties, supporting array indexes,
// and some property/methods with arguments or return types of Scriptable.
class TestScriptable2 : public TestScriptable1 {
 public:
  DEFINE_CLASS_ID(0xa88ea50b8b884e, TestScriptable1);
  TestScriptable2(bool script_owned_ = false);
  virtual ~TestScriptable2();

  virtual OwnershipPolicy Attach();
  virtual bool Detach();
  virtual bool IsStrict() const { return false; }

  static const int kArraySize = 20;

  Variant GetArray(int index) {
    if (index >= 0 && index < kArraySize)
      return Variant(array_[index]);
    return Variant();  // void
  }

  bool SetArray(int index, int value) {
    if (index >= 0 && index < kArraySize) {
      // Distinguish from JavaScript builtin logic.
      array_[index] = value + 10000;
      return true;
    }
    return false;
  }

  Variant GetDynamicProperty(const char *name) {
    if (name[0] == 'd')
      return Variant(dynamic_properties_[name]);
    return Variant();
  }

  bool SetDynamicProperty(const char *name, const char *value) {
    if (name[0] == 'd') {
      // Distinguish from JavaScript builtin logic.
      dynamic_properties_[name] = std::string("Value:") + value;
      return true;
    }
    return false;
  }

  void SetTime(const std::string &time) {
    time_ = time;
    if (time == "lunch")
      signal_result_ = onlunch_signal_(std::string("Have lunch"));
    else if (time == "supper")
      signal_result_= onsupper_signal_(std::string("Have supper"), this);
  }

  TestScriptable2 *GetSelf() { return this; }
  TestScriptable2 *TestMethod(const TestScriptable2 *t) {
    return const_cast<TestScriptable2 *>(t);
  }
  TestScriptable2 *NewObject(bool script_owned) {
    return new TestScriptable2(script_owned);
  }
  void DeleteObject(TestScriptable2 *obj) { delete obj; }

  // Place signal declarations here for testing.
  // In production code, they should be palced in private section.
  typedef Signal1<std::string, const std::string &> OnLunchSignal;
  typedef Signal2<std::string, const std::string &,
                  TestScriptable2 *> OnSupperSignal;

  OnLunchSignal onlunch_signal_;
  OnSupperSignal onsupper_signal_;

 private:
  bool script_owned_;
  int array_[kArraySize];
  std::string time_;
  std::string signal_result_;
  std::map<std::string, std::string> dynamic_properties_;
};

#endif // GGADGET_TESTS_SCRIPTABLES_H__
