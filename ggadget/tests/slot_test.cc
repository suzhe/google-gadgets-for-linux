// Copyright 2007 Google Inc. All Rights Reserved.
// Author: wangxianzhu@google.com (Xianzhu Wang)

#include <stdio.h>
#include "ggadget/slot.h"
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

// These strings are used to test std::string parameters.
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

const int kNumTestData = arraysize(testdata); 

TEST(slot, Slot) {
  TestClass obj;
  Slot *meta_slot = NewSlot(&obj, &TestClass::TestSlotMethod);
  ASSERT_TRUE(meta_slot->HasMetadata());
  ASSERT_EQ(1, meta_slot->GetArgCount());
  ASSERT_EQ(Variant::TYPE_INT64, meta_slot->GetArgTypes()[0]);
  ASSERT_EQ(Variant::TYPE_SLOT, meta_slot->GetReturnType());
  for (int i = 0; i < kNumTestData; i++) {
    Variant param(i);
    Slot *slot = meta_slot->Call(1, &param).v.slot_value;
    ASSERT_TRUE(slot->HasMetadata());
    ASSERT_EQ(testdata[i].argc, slot->GetArgCount());
    ASSERT_EQ(testdata[i].return_type, slot->GetReturnType());
    for (int j = 0; j < testdata[i].argc; j++)
      ASSERT_EQ(testdata[i].arg_types[j], slot->GetArgTypes()[j]);
    Variant call_result = slot->Call(testdata[i].argc, testdata[i].args);
    ASSERT_TRUE(testdata[i].return_value == call_result);
    printf("%d: '%s' '%s'\n", i, result, testdata[i].result);
    ASSERT_STREQ(testdata[i].result, result);
    delete slot;
  }
  delete meta_slot;
}

int main(int argc, char **argv) {
  testing::ParseGUnitFlags(&argc, argv);
  return RUN_ALL_TESTS();
}
