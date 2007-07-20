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

#include "ggadget/scriptable_interface.h"
#include "ggadget/slot.h"
#include "ggadget/static_scriptable.h"

using namespace ggadget;

// Store testing status to be checked in unit test code.
std::string g_buffer;

void AppendBuffer(const char *format, ...) {
  char buffer[1024];
  va_list ap;
  va_start(ap, format);
  vsnprintf(space, sizeof(space), format, ap);
  va_end(ap);
  g_buffer.append(buffer);
  printf("AppendBuffer: %s\n", buffer);
}

// A normal scriptable class.
class TestScriptable1 : public ScriptableInterface {
 public:
  TestScriptable1() 
      : static_scriptable_(new StaticScriptable()),
        double_property(0) {
    static_scriptable_->RegisterMethod("AddRef",
        NewSlot(&TestScriptable1::AddRef));
    static_scriptable_->RegisterMethod("Release",
        NewSlot(&TestScriptable1::Release));
    static_scriptable_->RegisterMethod("TestMethodVoid0",
        NewSlot(&TestScriptable1::TestMethodVoid0));
    static_scriptable_->RegisterMethod("TestMethodDouble2",
        NewSlot(&TestScriptable1::TestMethodDouble2));
    static_scriptable_->RegisterProperty("DoubleProperty",
        NewSlot(&TestScriptable1::GetDoubleProperty),
        NewSlot(&TestScriptable1::SetDoubleProperty));
    static_scriptable_->RegisterProperty("DoubleProperty",
        NewSlot(&TestScriptable1::GetDoubleProperty),
        NewSlot(&TestScriptable1::SetDoubleProperty));
    static_scriptable_->RegisterProperty("Buffer",
        NewSlot(&TestScriptable1::GetBuffer), NULL);
  }

  virtual int AddRef() {
    return static_scriptable_->AddRef();
  }
  virtual int Release() {
    int result = static_scriptable_->Release();
    if (result <= 0) delete this;
    return result;
  }
  virtual bool GetPropertyInfoByName(const char *name,
                                     int *id, Variant *prototype,
                                     bool *is_method) {
    return static_scriptable_->GetPropertyByName(name, id,
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
    buffer.clear();
  }
  double TestMethodDouble2(bool p1, long p2) {
    AppendBuffer("TestMethodDouble2(%d, %ld)\n", p1, p2);
  }
  void SetDoubleProperty(double double_property) {
    double_property_ = double_property;
    AppendBuffer("SetDoubleProperty(%.3lf)\n", double_property);
  }
  double GetDoubleProperty() {
    AppendBuffer("GetDoubleProperty()=%.3lf\n", double_property_); 
    return double_property_;
  }

  const char *GetBuffer() {
    return g_buffer.c_str();
  }

 private:
  virtual ~TestScriptable1() {
    AppendBuffer("Destruct\n");
  }
  
  StaticScriptable *static_scriptable_;
  double double_property_; 
};

// A scriptable class with some dynamic properties, support array indexes,
// and some property/methods with arguments or return types of Scriptable.    
class TestScriptable2 : public TestScriptable1 {
 public:
  TestScriptable2() {
    // Register...
  }
};
