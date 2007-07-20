// Copyright 2007 Google Inc. All Rights Reserved.
// Author: wangxianzhu@google.com (Xianzhu Wang)

#include <stdio.h>
#include "ggadget/signal.h"
#include "testing/gunit.h"

using namespace ggadget;

// Hold the result of test functions/methods.
char result[1024];

void TestVoidFunction0() {
  strcpy(result, "TestVoidFunction0");
}

void TestVoidFunction9(int p1, bool p2, const char *p3, const std::string &p4,
                       std::string p5, char p6, unsigned char p7,
                       short p8, unsigned short p9) {
  sprintf(result, "TestVoidFunction9: %d %d %s %s %s %c %c %d %d",
          p1, p2, p3, p4.c_str(), p5.c_str(), p6, p7, p8, p9);
}

bool TestBoolFunction0() {
  strcpy(result, "TestBoolFunction0");
  return false;
}

bool TestBoolFunction9(int p1, bool p2, const char *p3, const std::string &p4,
                       std::string p5, char p6, unsigned char p7,
                       short p8, unsigned short p9) {
  sprintf(result, "TestBoolFunction9: %d %d %s %s %s %c %c %d %d",
          p1, p2, p3, p4.c_str(), p5.c_str(), p6, p7, p8, p9);
  return true;
}

class TestClass0 {
 public:
  virtual void TestVoidMethod2(char p1, unsigned long p2) = 0;
};

class TestClass : public TestClass0 {
 public:
  void TestVoidMethod0() {
    strcpy(result, "TestVoidMethod0");
  }
  bool TestBoolMethod0() {
    strcpy(result, "TestBoolMethod0");
    return true;
  }
  virtual void TestVoidMethod2(char p1, unsigned long p2) {
    sprintf(result, "TestVoidMethod2: %c %lx", p1, p2);
  }
  double TestDoubleMethod2(int p1, double p2) {
    sprintf(result, "TestDoubleMethod2: %d %.3lf", p1, p2);
    return 2;
  }
  void TestVoidMethod9(int p1, bool p2, const char *p3, const std::string &p4,
                       std::string p5, char p6, unsigned char p7,
                       short p8, unsigned short p9) {
    sprintf(result, "TestVoidMethod9: %d %d %s %s %s %c %c %d %d",
            p1, p2, p3, p4.c_str(), p5.c_str(), p6, p7, p8, p9);
  }
  bool TestBoolMethod9(int p1, bool p2, const char *p3, const std::string &p4,
                       std::string p5, char p6, unsigned char p7,
                       short p8, unsigned short p9) {
    sprintf(result, "TestBoolMethod9: %d %d %s %s %s %c %c %d %d",
            p1, p2, p3, p4.c_str(), p5.c_str(), p6, p7, p8, p9);
    return false;
  }

  Slot *TestSlotMethod(int i) {
    switch (i) {
      case 0: return NewSlot(TestVoidFunction0);
      case 1: return NewSlot(TestVoidFunction9);
      case 2: return NewSlot(TestBoolFunction0);
      case 3: return NewSlot(TestBoolFunction9);
      case 4: return NewSlot(this, &TestClass::TestVoidMethod0);
      case 5: return NewSlot(this, &TestClass::TestBoolMethod0);
      case 6: return NewSlot(this, &TestClass::TestVoidMethod2);
      case 7: return NewSlot(this, &TestClass::TestDoubleMethod2);
      case 8: return NewSlot(this, &TestClass::TestVoidMethod9);
      case 9: return NewSlot(this, &TestClass::TestBoolMethod9);
      case 10: return NewSlot((TestClass0 *)this, &TestClass0::TestVoidMethod2);
      default: return NULL;
    }
  }
};

std::string str_b("bbb");
std::string str_c("ccc");
std::string str_e("eee");
std::string str_f("fff");

struct TestData {
  int argc;
  Variant::Type return_type;
  Variant::Type arg_types[10];
  Variant args[10];
  Variant return_value;
  const char *result;
} testdata[] = {
  { 0, Variant::TYPE_VOID, { }, { }, Variant(), "TestVoidFunction0" },
  { 9, Variant::TYPE_VOID, { Variant::TYPE_INT64,
                             Variant::TYPE_BOOL,
                             Variant::TYPE_STRING,
                             Variant::TYPE_STRING,
                             Variant::TYPE_STRING,
                             Variant::TYPE_INT64,
                             Variant::TYPE_INT64,
                             Variant::TYPE_INT64,
                             Variant::TYPE_INT64,
                           },
                           { Variant(1),
                             Variant(true),
                             Variant("a"),
                             Variant(str_b),
                             Variant(str_c),
                             Variant('x'),
                             Variant('y'),
                             Variant(100),
                             Variant(200),
                           },
    Variant(), "TestVoidFunction9: 1 1 a bbb ccc x y 100 200" },
  { 0, Variant::TYPE_BOOL, { }, { }, Variant(false), "TestBoolFunction0" },
  { 9, Variant::TYPE_BOOL, { Variant::TYPE_INT64,
                             Variant::TYPE_BOOL,
                             Variant::TYPE_STRING,
                             Variant::TYPE_STRING,
                             Variant::TYPE_STRING,
                             Variant::TYPE_INT64,
                             Variant::TYPE_INT64,
                             Variant::TYPE_INT64,
                             Variant::TYPE_INT64,
                           },
                           { Variant(100),
                             Variant(false),
                             Variant("d"),
                             Variant(str_e),
                             Variant(str_f),
                             Variant('X'),
                             Variant('Y'),
                             Variant(-222),
                             Variant(111),
                           },
    Variant(true), "TestBoolFunction9: 100 0 d eee fff X Y -222 111" },
  { 0, Variant::TYPE_VOID, { }, { }, Variant(), "TestVoidMethod0" },
  { 0, Variant::TYPE_BOOL, { }, { }, Variant(true), "TestBoolMethod0" },
  { 2, Variant::TYPE_VOID, { Variant::TYPE_INT64, Variant::TYPE_INT64 },
                           { Variant('a'), Variant(0xffffffffUL) },
    Variant(), "TestVoidMethod2: a ffffffff" },
  { 2, Variant::TYPE_DOUBLE, { Variant::TYPE_INT64, Variant::TYPE_DOUBLE },
                             { Variant(-999), Variant(-3.14) },
    Variant(2.0), "TestDoubleMethod2: -999 -3.140" },
  { 9, Variant::TYPE_VOID, { Variant::TYPE_INT64,
                             Variant::TYPE_BOOL,
                             Variant::TYPE_STRING,
                             Variant::TYPE_STRING,
                             Variant::TYPE_STRING,
                             Variant::TYPE_INT64,
                             Variant::TYPE_INT64,
                             Variant::TYPE_INT64,
                             Variant::TYPE_INT64,
                           },
                           { Variant(100),
                             Variant(false),
                             Variant("a"),
                             Variant(str_b),
                             Variant(str_c),
                             Variant('x'),
                             Variant('y'),
                             Variant(999),
                             Variant(888),
                           },
    Variant(), "TestVoidMethod9: 100 0 a bbb ccc x y 999 888" },
  { 9, Variant::TYPE_BOOL, { Variant::TYPE_INT64,
                             Variant::TYPE_BOOL,
                             Variant::TYPE_STRING,
                             Variant::TYPE_STRING,
                             Variant::TYPE_STRING,
                             Variant::TYPE_INT64,
                             Variant::TYPE_INT64,
                             Variant::TYPE_INT64,
                             Variant::TYPE_INT64,
                           },
                           { Variant(100),
                             Variant(false),
                             Variant("d"),
                             Variant(str_e),
                             Variant(str_f),
                             Variant('X'),
                             Variant('Y'),
                             Variant(222),
                             Variant(333),
                           },
    Variant(false), "TestBoolMethod9: 100 0 d eee fff X Y 222 333" },
  { 2, Variant::TYPE_VOID, { Variant::TYPE_INT64, Variant::TYPE_INT64 },
                           { Variant('a'), Variant(0xffffffffUL) },
    Variant(), "TestVoidMethod2: a ffffffff" },
};

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

const int kNumTestData = arraysize(testdata); 

static void CheckSlot(int i, Slot *slot) {
  ASSERT_TRUE(slot->HasMetadata());
  ASSERT_EQ(testdata[i].argc, slot->GetArgCount());
  ASSERT_EQ(testdata[i].return_type, slot->GetReturnType());
  for (int j = 0; j < testdata[i].argc; j++)
    ASSERT_EQ(testdata[i].arg_types[j], slot->GetArgTypes()[j]);
  Variant call_result = slot->Call(testdata[i].argc, testdata[i].args);
  ASSERT_TRUE(testdata[i].return_value == call_result);
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
  ASSERT_EQ(1, meta_signal.GetArgCount() == 1);
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
}

TEST(signal, ConnectGeneral_and_SignalSlot) {
  TestClass obj;
  MetaSignal meta_signal;
  meta_signal.Connect(NewSlot(&obj, &TestClass::TestSlotMethod));

  Signal0Void signal0, signal4;
  Signal0Bool signal2, signal5;
  Signal9Void signal1, signal8;
  Signal9Bool signal3, signal9;
  Signal2Void signal6, signal10;
  Signal2Double signal7;
  
  Signal *signals[] = { &signal0, &signal1, &signal2, &signal3, &signal4,
                        &signal5, &signal6, &signal7, &signal8, &signal9,
                        &signal10 };

  // Initially unblocked.
  for (int i = 0; i < kNumTestData; i++) {
    signals[i]->ConnectGeneral(meta_signal(i));
    Slot *slot = new SignalSlot(signals[i]);
    // SignalSlot -> Signal -> Slot.
    CheckSlot(i, slot);
    delete slot;
  }
}

TEST(signal, SignalSlotCompatibility) {
  TestClass obj;
  MetaSignal meta_signal;
  meta_signal.Connect(NewSlot(&obj, &TestClass::TestSlotMethod));

  Signal0Void signal0, signal4;
  Signal0Bool signal2, signal5;
  Signal9Void signal1, signal8;
  Signal9Bool signal3, signal9;
  Signal2Void signal6, signal10;
  Signal2Double signal7;
  Signal9VoidCompatible signal9_compatible;

  Signal *signals[] = { &signal0, &signal1, &signal2, &signal3, &signal4,
                        &signal5, &signal6, &signal7, &signal8, &signal9,
                        &signal10 };

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
