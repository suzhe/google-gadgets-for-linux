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

#include <cstdio>
#include "ggadget/scriptable_interface.h"
#include "ggadget/variant.h"
#include "unittest/gunit.h"

using namespace ggadget;

template <typename T>
void CheckType(Variant::Type t0) {
  Variant::Type t1 = VariantType<T>::type;
  ASSERT_EQ(t0, t1);
}

TEST(Variant, TestVoid) {
  Variant v;
  ASSERT_EQ(Variant::TYPE_VOID, v.type());
  CheckType<void>(Variant::TYPE_VOID);
  VariantValue<void>()(v);
  Variant v1(v);
  ASSERT_EQ(Variant::TYPE_VOID, v.type());
  printf("%s\n", v.ToString().c_str());
}

template <typename T, Variant::Type Type>
void CheckVariant(T value) {
  CheckType<T>(Type);
  Variant v(value);
  ASSERT_EQ(Type, v.type());
  ASSERT_TRUE(value == VariantValue<T>()(v));
  Variant v1(v);
  ASSERT_EQ(Type, v1.type());
  ASSERT_TRUE(value == VariantValue<T>()(v1));
  Variant v2;
  v2 = v;
  ASSERT_EQ(Type, v2.type());
  ASSERT_TRUE(value == VariantValue<T>()(v2));
  printf("%s\n", v.ToString().c_str());
}

TEST(Variant, TestBool) {
  CheckVariant<bool, Variant::TYPE_BOOL>(true);
  CheckVariant<bool, Variant::TYPE_BOOL>(false);
}

template <typename T>
void CheckIntVariant(T value) {
  CheckVariant<T, Variant::TYPE_INT64>(value);
}

enum {
  NONAME_1,
  NONAME_2,
};
enum NamedEnum {
  NAMED_1,
  NAMED_2,
};

TEST(Variant, TestInt) {
  Variant ve0(NONAME_2);
  ASSERT_EQ(Variant::TYPE_INT64, ve0.type());
  ASSERT_EQ(1, VariantValue<int>()(ve0));

  CheckIntVariant<NamedEnum>(NAMED_2);
  CheckIntVariant<int>(1234);
  CheckIntVariant<unsigned int>(1234U);
  CheckIntVariant<char>('a');
  CheckIntVariant<unsigned char>(0x20);
  CheckIntVariant<short>(2345);
  CheckIntVariant<unsigned short>(3456);
  CheckIntVariant<long>(-4567890);
  CheckIntVariant<unsigned long>(5678901);
  CheckIntVariant<int64_t>(INT64_C(0x1234567887654321));
  CheckIntVariant<uint64_t>(UINT64_C(0x8765432112345678));
}

TEST(Variant, TestDouble) {
  CheckVariant<float, Variant::TYPE_DOUBLE>(12345.6789);
  CheckVariant<double, Variant::TYPE_DOUBLE>(2930423.34932);
}

template <typename T, typename VT, Variant::Type Type>
void CheckStringVariantBase(T value) {
  CheckType<T>(Type);
  Variant v(value);
  ASSERT_EQ(Type, v.type());
  ASSERT_TRUE(VT(value) == VT(VariantValue<T>()(v)));
  Variant v1(v);
  ASSERT_EQ(Type, v1.type());
  ASSERT_TRUE(VT(value) == VT(VariantValue<T>()(v1)));
  Variant v2;
  v2 = v;
  ASSERT_EQ(Type, v2.type());
  ASSERT_TRUE(VT(value) == VT(VariantValue<T>()(v2)));
  printf("%s\n", v.ToString().c_str());
  Variant v3("1234");
  v3 = v;
  ASSERT_EQ(Type, v3.type());
  ASSERT_TRUE(VT(value) == VT(VariantValue<T>()(v3)));
}

template <typename T>
void CheckStringVariant(T value) {
  CheckStringVariantBase<T, std::string, Variant::TYPE_STRING>(value);
}

TEST(Variant, TestString) {
  CheckStringVariant<const char *>("abcdefg");
  CheckStringVariant<std::string>("xyz");
  CheckStringVariant<const std::string &>("120394");
  Variant v(static_cast<const char *>(NULL));
  ASSERT_EQ(std::string(""), VariantValue<std::string>()(v));
  ASSERT_TRUE(NULL == VariantValue<const char *>()(v));
  Variant v1(v);
  ASSERT_TRUE(NULL == VariantValue<const char *>()(v1));
  Variant v2;
  v2 = v;
  ASSERT_TRUE(NULL == VariantValue<const char *>()(v2));
  Variant v3("xyz");
  v3 = v;
  ASSERT_TRUE(NULL == VariantValue<const char *>()(v3));
}

TEST(Variant, TestJSON) {
  CheckVariant<JSONString, Variant::TYPE_JSON>(JSONString(std::string("abc")));
  CheckVariant<const JSONString &, Variant::TYPE_JSON>(JSONString("def"));
}

template <typename T>
void CheckUTF16StringVariant(T value) {
  CheckStringVariantBase<T, UTF16String, Variant::TYPE_UTF16STRING>(value);
}

TEST(Variant, TestUTF16String) {
  UTF16Char p[] = { 100, 200, 300, 400, 500, 0 };
  CheckUTF16StringVariant<const UTF16Char *>(p);
  CheckUTF16StringVariant<UTF16String>(UTF16String(p));
  CheckUTF16StringVariant<const UTF16String &>(UTF16String(p));
}

class Scriptable1 : public ScriptableInterface { };

TEST(Variant, TestScriptableAndAny) {
  CheckVariant<ScriptableInterface *, Variant::TYPE_SCRIPTABLE>(NULL);
  CheckVariant<const ScriptableInterface *,
               Variant::TYPE_CONST_SCRIPTABLE>(NULL);
  CheckVariant<Scriptable1 *, Variant::TYPE_SCRIPTABLE>(NULL);
  CheckVariant<const Scriptable1 *, Variant::TYPE_CONST_SCRIPTABLE>(NULL);
  CheckVariant<void *, Variant::TYPE_ANY>(NULL);
  CheckVariant<const void *, Variant::TYPE_CONST_ANY>(NULL);
}

TEST(Variant, TestSlot) {
  CheckVariant<Slot *, Variant::TYPE_SLOT>(NULL);
}

int main(int argc, char **argv) {
  testing::ParseGUnitFlags(&argc, argv);
  return RUN_ALL_TESTS();
}
