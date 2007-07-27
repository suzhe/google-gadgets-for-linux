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

function TestScriptableBasics(scriptable) {
  scriptable.Buffer = "TestBuffer";
  ASSERT(EQ("TestBuffer", scriptable.Buffer));
  scriptable.TestMethodVoid0();  // It will clear the buffer.
  ASSERT(STRICT_EQ("", scriptable.Buffer));

  ASSERT(STRICT_EQ(0, scriptable.DoubleProperty));
  ASSERT(EQ("GetDoubleProperty()=0.000\n", scriptable.Buffer));
  scriptable.Buffer = "";
  scriptable.DoubleProperty = 3.25;
  ASSERT(EQ("SetDoubleProperty(3.250)\n", scriptable.Buffer));
  scriptable.Buffer = "";
  ASSERT(STRICT_EQ(3.25, scriptable.DoubleProperty));
  ASSERT(EQ("GetDoubleProperty()=3.250\n", scriptable.Buffer));

  ASSERT(STRICT_EQ(0, scriptable.IntSimple));
  scriptable.IntSimple = 12345;
  ASSERT(EQ(12345, scriptable.IntSimple));
  scriptable.IntSimple = "100.2";
  ASSERT(EQ(100, scriptable.IntSimple));

  ASSERT(EQ(123456789, scriptable.Fixed));

  ASSERT(STRICT_EQ(0, scriptable.IntOrStringProperty));
  scriptable.IntOrStringProperty = 1234;
  ASSERT(EQ(1234, scriptable.IntOrStringProperty));
  scriptable.IntOrStringProperty = "100.2";
  ASSERT(EQ(100, scriptable.IntOrStringProperty));
  scriptable.IntOrStringProperty = "80%";
  ASSERT(EQ("80%", scriptable.IntOrStringProperty));

  ASSERT(UNDEFINED(scriptable.NotDefinedProperty));
  scriptable.NotDefinedProperty = "TestValue";
  ASSERT(EQ("TestValue", scriptable.NotDefinedProperty));
}

TEST("Test property & method basics", function() {
  // "scriptable" is registered as a global property in wrapper_test_shell.cc.
  TestScriptableBasics(scriptable);
});

TEST("Test readonly property", function() {
  // Buffer and BufferReadOnly are backed with single C++ object.
  scriptable.Buffer = "Buffer";
  // Assignment to a readonly property has no effect. 
  scriptable.BufferReadOnly = "TestBuffer";
  ASSERT(EQ("Buffer", scriptable.BufferReadOnly));
});

DEATH_TEST("Test integer property with non-number", function() {
  // The following assignment should cause an error.
  scriptable.IntSimple = "80%";
  ASSERT(DEATH());
});

TEST("Test constants", function() {
  ASSERT(EQ(123456789, scriptable.Fixed));
  for (var i = 0; i < 10; i++) {
    ASSERT(STRICT_EQ(i, scriptable["ICONSTANT" + i]));
    ASSERT(EQ("SCONSTANT" + i, scriptable["SCONSTANT" + i]));
  }
});

TEST("Test constant assignment", function() {
  // Assignment to constant has no effect.
  scriptable.Fixed = 100;
  ASSERT(EQ(123456789, scriptable.Fixed));
});

TEST("Test inheritance & prototype", function() {
  ASSERT(NE(scriptable, scriptable2));
  TestScriptableBasics(scriptable2);
  // Const is defined in scriptable2's prototype.
  ASSERT(EQ(987654321, scriptable2.Const));
});

TEST("Test array", function() {
  ASSERT(EQ(20, scriptable2.length));
  for (var i = 0; i < scriptable2.length; i++)
    scriptable2[i] = i * 2;
  for (i = 0; i < scriptable2.length; i++) {
    ASSERT(STRICT_EQ(i * 2, scriptable2[i]));
    ASSERT(STRICT_EQ(i * 2, scriptable2["" + i]));
  }
  ASSERT(UNDEFINED(scriptable2[scriptable2.length]));
  ASSERT(UNDEFINED(scriptable2[scriptable2.length + 1]));
});

/*
TEST("Test signals", function() {
  // Defined in scriptable1.
  ASSERT(NULL(scriptable2.ondelete));
  // Defined in prototype.
  ASSERT(NULL(scriptable2.ontest));
  ASSERT(NULL(scriptable2.onlunch));
  ASSERT(NULL(scriptable2.onsupper));
  print("********After NULL test");
  var scriptable2_in_closure = scriptable2;

  var onlunch_triggered = false;
  var onsupper_triggered = false;
  print("********Before onlunch_function definition");
  onlunch_function1 = function(s) {
    ASSERT(EQ(scriptable2, scriptable2_in_closure));
    ASSERT(EQ("Have lunch", s));
    // A long lunch taking a whole afternoon :).
    scriptable2.time = "supper";   // Will cause recursive onsupper.
    ASSERT(TRUE(onsupper_triggered));
    ASSERT(EQ("Supper finished", scriptable2.SignalResult));
    // Lunch is finished after finishing supper :).
    onlunch_triggered = true;
    return "Lunch finished";
  };
  
  onsupper_function1 = function(s) {
    ASSERT(EQ(scriptable2, scriptable2_in_closure));
    ASSERT(EQ("Have supper", s));
    onsupper_triggered = true;
    return "Supper finished";
  };
  print("********After onsupper_function definition");

  var onlunch_function = function(s) {
    return onlunch_function1(s);
  };
  var onsupper_function = function(s) {
    return onsupper_function1(s);
  };
  scriptable2.onlunch = onlunch_function1;
  scriptable2.onsupper = onsupper_function1;

  onsupper_function("Have supper");

  // Trigger onlunch.
  scriptable2.time = "lunch";
  ASSERT(EQ("Lunch finished", scriptable2.SignalResult));
  ASSERT(TRUE(onsupper_triggered));
  ASSERT(TRUE(onlunch_triggered));

  onlunch_triggered = false;
  onsupper_triggered = false;
  // Now we have supper when the others are having lunch.
  scriptable2.onlunch = onsupper_function;
  ASSERT(STRICT_EQ(scriptable2.onlunch, scriptable2.onsupper));
  scriptable2.time = "lunch";
  ASSERT(EQ("Supper finished", scriptable2.SignalResult));
  ASSERT(FALSE(onsupper_triggered));
  ASSERT(TRUE(onlunch_triggered));

  // Test disconnect.
  scriptable2.onlunch = null;
  ASSERT(NULL(scriptable2.onlunch));
});
*/
RUN_ALL_TESTS();
