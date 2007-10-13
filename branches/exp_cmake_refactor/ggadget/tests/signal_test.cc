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

#include <stdio.h>
#include "ggadget/signal.h"
#include "unittest/gunit.h"

#include "slots.h"

using namespace ggadget;

typedef Signal0<void> Signal0Void;
typedef Signal0<bool> Signal0Bool;
typedef Signal9<void, int, bool, const char *, const char *, std::string,
                char, unsigned char, short, unsigned short> Signal9Void;
typedef Signal9<bool, int, bool, const char *, const char *, std::string,
                char, unsigned char, short, unsigned short> Signal9Bool;
typedef Signal2<void, char, unsigned long> Signal2Void;
typedef Signal2<double, int, double> Signal2Double;
typedef Signal1<Slot *, int> MetaSignal;

typedef Signal9<void, long, bool, std::string, std::string, const char *,
                int, unsigned short, int, unsigned long> Signal9VoidCompatible;
typedef Signal1<Variant, Variant> SignalVariant;

static void CheckSlot(int i, Slot *slot) {
  ASSERT_TRUE(slot->HasMetadata());
  ASSERT_EQ(testdata[i].argc, slot->GetArgCount());
  ASSERT_EQ(testdata[i].return_type, slot->GetReturnType());
  for (int j = 0; j < testdata[i].argc; j++)
    ASSERT_EQ(testdata[i].arg_types[j], slot->GetArgTypes()[j]);
  Variant call_result = slot->Call(testdata[i].argc, testdata[i].args);
  ASSERT_EQ(testdata[i].return_value, call_result);
  printf("%d: '%s' '%s'\n", i, result, testdata[i].result);
  ASSERT_STREQ(testdata[i].result, result);
}

TEST(signal, SignalBasics) {
  TestClass obj;
  MetaSignal meta_signal;
  Connection *connection = meta_signal.Connect(
      NewSlot(&obj, &TestClass::TestSlotMethod));
  ASSERT_TRUE(connection != NULL);
  ASSERT_FALSE(connection->blocked());
  ASSERT_EQ(1, meta_signal.GetArgCount());
  ASSERT_EQ(Variant::TYPE_INT64, meta_signal.GetArgTypes()[0]);
  ASSERT_EQ(Variant::TYPE_SLOT, meta_signal.GetReturnType());

  // Initially unblocked.
  for (int i = 0; i < kNumTestData; i++) {
    Slot *temp_slot = meta_signal(i);
    CheckSlot(i, temp_slot);
    delete temp_slot;
  }

  // Block the connection.
  connection->Block();
  ASSERT_TRUE(connection->blocked());
  for (int i = 0; i < kNumTestData; i++) {
    Slot *temp_slot = meta_signal(i);
    ASSERT_TRUE(temp_slot == NULL);
  }

  // Unblock the connection.
  connection->Unblock();
  ASSERT_FALSE(connection->blocked());
  for (int i = 0; i < kNumTestData; i++) {
    Slot *temp_slot = meta_signal(i);
    CheckSlot(i, temp_slot);
    delete temp_slot;
  }

  // Disconnect the connection.
  connection->Disconnect();
  ASSERT_TRUE(connection->blocked());
  for (int i = 0; i < kNumTestData; i++) {
    Slot *temp_slot = meta_signal(i);
    ASSERT_TRUE(temp_slot == NULL);
  }

  // A disconnected connection will be always blocked.
  connection->Unblock();
  ASSERT_TRUE(connection->blocked());
  for (int i = 0; i < kNumTestData; i++) {
    Slot *temp_slot = meta_signal(i);
    ASSERT_TRUE(temp_slot == NULL);
  }

  connection->Reconnect(NewSlot(&obj, &TestClass::TestSlotMethod));
  ASSERT_FALSE(connection->blocked());
  for (int i = 0; i < kNumTestData; i++) {
    Slot *temp_slot = meta_signal(i);
    CheckSlot(i, temp_slot);
    delete temp_slot;
  }
}

TEST(signal, SignalConnectNullSlot) {
  TestClass obj;
  MetaSignal meta_signal;
  Connection *connection = meta_signal.Connect(NULL);
  ASSERT_TRUE(connection != NULL);
  ASSERT_TRUE(connection->blocked());
  ASSERT_TRUE(connection->slot() == NULL);

  connection->Reconnect(NewSlot(&obj, &TestClass::TestSlotMethod));
  ASSERT_FALSE(connection->blocked());
}

TEST(signal, SignalSlotCompatibility) {
  TestClass obj;
  MetaSignal meta_signal;
  meta_signal.Connect(NewSlot(&obj, &TestClass::TestSlotMethod));

  Signal0Void signal0, signal4, signal11;
  Signal0Bool signal2, signal5, signal13;
  Signal9Void signal1, signal8, signal12;
  Signal9Bool signal3, signal9, signal14;
  Signal2Void signal6, signal10;
  Signal2Double signal7;
  Signal9VoidCompatible signal9_compatible;
  SignalVariant signal15;

  Signal *signals[] = { &signal0, &signal1, &signal2, &signal3, &signal4,
                        &signal5, &signal6, &signal7, &signal8, &signal9,
                        &signal10, &signal11, &signal12, &signal13, &signal14,
                        &signal15 };

  for (int i = 0; i < kNumTestData; i++)
    ASSERT_TRUE(signals[i]->ConnectGeneral(meta_signal(i)) != NULL);

  // Compatible.
  ASSERT_TRUE(signal0.ConnectGeneral(meta_signal(0)) != NULL);
  ASSERT_TRUE(signal0.ConnectGeneral(meta_signal(4)) != NULL);
  // Signal returning void is compatible with slot returning any type.
  ASSERT_TRUE(signal0.ConnectGeneral(meta_signal(2)) != NULL);
  // Special compatible by variant type automatic conversion.
  ASSERT_TRUE(signal9_compatible.ConnectGeneral(meta_signal(1)) != NULL);
  ASSERT_TRUE(signal9_compatible.ConnectGeneral(meta_signal(8)) != NULL);
  ASSERT_TRUE(signal9_compatible.ConnectGeneral(meta_signal(3)) != NULL);
  ASSERT_TRUE(signal9_compatible.ConnectGeneral(meta_signal(9)) != NULL);

  // Incompatible.
  ASSERT_TRUE(signal0.ConnectGeneral(meta_signal(1)) == NULL);
  ASSERT_TRUE(signal0.ConnectGeneral(meta_signal(7)) == NULL);
  ASSERT_TRUE(signal0.ConnectGeneral(meta_signal(9)) == NULL);
  ASSERT_TRUE(signal2.ConnectGeneral(meta_signal(0)) == NULL);
  ASSERT_TRUE(signal9_compatible.ConnectGeneral(meta_signal(0)) == NULL);
  ASSERT_TRUE(signal9_compatible.ConnectGeneral(meta_signal(2)) == NULL);
  ASSERT_TRUE(signal9_compatible.ConnectGeneral(meta_signal(6)) == NULL);
  ASSERT_TRUE(signal9_compatible.ConnectGeneral(meta_signal(7)) == NULL);
  ASSERT_TRUE(signal9.ConnectGeneral(meta_signal(8)) == NULL);
}

int main(int argc, char **argv) {
  testing::ParseGUnitFlags(&argc, argv);
  return RUN_ALL_TESTS();
}
