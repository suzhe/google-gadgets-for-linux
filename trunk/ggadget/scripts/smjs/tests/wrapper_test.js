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

TEST("Test property & method basics", function() {
  // "scriptable" is registered as a global property in wrapper_test_shell.cc.
  ASSERT(STRICT_EQ("", scriptable.Buffer));
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
  END_TEST();
});

DEATH_TEST("Test readonly property", function() {
  // The following assignment should cause an error.
  scriptable.BufferReadOnly = "TestBuffer";
  print(11);
  END_TEST();
});

RUN_ALL_TESTS();
