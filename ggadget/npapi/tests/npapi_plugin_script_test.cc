/*
   Copyright 2008 Google Inc.

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

#include <vector>

#include <ggadget/slot.h>
#include <ggadget/variant.h>
#include <ggadget/npapi/npapi_impl.h>
#include <ggadget/npapi/npapi_plugin_script.h>

#include <ggadget/tests/init_extensions.h>
#include <ggadget/tests/mocked_timer_main_loop.h>
#include "unittest/gtest.h"

namespace ggadget {
DECLARE_VARIANT_PTR_TYPE(NPObject);
namespace npapi {

// Mocked NPObject.
static NPClass mock_class;
static NPObject mock_npobj = {
  &mock_class,
  1,
};

// Definition of properties and methods for the mocked NPObject.
struct Property {
  Property(const char *name, NPVariant value) : name_(name), value_(value) { }
  const char *name_;
  NPVariant value_;
};
struct Method {
  Method(const char *name, Slot *slot) : name_(name), slot_(slot) { }
  const char *name_;
  Slot *slot_;
};
static std::vector<Property> Properties;
static std::vector<Method> Methods;

static bool TestBoolean(bool boolean) { return boolean; }
static const char *TestString(const char *string) { return string; }
static int TestInteger(int integer) { return integer; }
static NPObject *TestObject(NPObject *object) { return object; }

void InitPropertiesAndMethods() {
  NPVariant v;
  INT32_TO_NPVARIANT(10, v);
  Properties.push_back(Property("integer", v));
  BOOLEAN_TO_NPVARIANT(true, v);
  Properties.push_back(Property("boolean", v));
  STRINGZ_TO_NPVARIANT("test", v);
  Properties.push_back(Property("string", v));
  OBJECT_TO_NPVARIANT(&mock_npobj, v);
  Properties.push_back(Property("object", v));

  Methods.push_back(Method("TestBoolean", NewSlot(&TestBoolean)));
  Methods.push_back(Method("TestString", NewSlot(&TestString)));
  Methods.push_back(Method("TestInteger", NewSlot(&TestInteger)));
  Methods.push_back(Method("TestObject", NewSlot(&TestObject)));
}

// NPClass methods.
static bool HasMethod(NPObject *npobj, NPIdentifier id) {
  EXPECT_EQ(&mock_npobj, npobj);
  EXPECT_TRUE(id);
  if (NPAPIImpl::NPN_IdentifierIsString(id)) {
    char *name = NPAPIImpl::NPN_UTF8FromIdentifier(id);
    for (size_t i = 0; i < Methods.size(); i++) {
      if (strcmp(Methods[i].name_, name) == 0) {
        NPAPIImpl::NPN_MemFree(name);
        return true;
      }
    }
    NPAPIImpl::NPN_MemFree(name);
    return false;
  } else {
    unsigned int index = NPAPIImpl::NPN_IntFromIdentifier(id);
    return Methods.size() > index;
  }
}

static bool Invoke(NPObject *npobj, NPIdentifier id, const NPVariant *args,
                   uint32_t argCount, NPVariant *result) {
  EXPECT_EQ(&mock_npobj, npobj);
  EXPECT_TRUE(id);
  Variant argv[argCount];
  for (size_t i = 0; i < argCount; ++i) {
    // In our test, plugin treats NPObject as local type.
    if NPVARIANT_IS_OBJECT(args[i])
      argv[i] = Variant(NPVARIANT_TO_OBJECT(args[i]));
    else
      argv[i] = ConvertNPToLocal(NULL, &args[i]);
  }
  Slot *slot = NULL;
  if (NPAPIImpl::NPN_IdentifierIsString(id)) {
    char *name = NPAPIImpl::NPN_UTF8FromIdentifier(id);
    for (size_t i = 0; i < Methods.size(); ++i) {
      if (strncmp(Methods[i].name_, name, strlen(Methods[i].name_)) == 0)
        slot = Methods[i].slot_;
    }
    NPAPIImpl::NPN_MemFree(name);
  } else {
    unsigned int index = NPAPIImpl::NPN_IntFromIdentifier(id);
    if (index < Methods.size())
      slot = Methods[index].slot_;
  }
  if (slot) {
    Variant ret = slot->Call(NULL, argCount, argv).v();
    if (result) {
      if (ret.type() == Variant::TYPE_ANY)
        // In our test, it's an NPObject.
        OBJECT_TO_NPVARIANT(
            reinterpret_cast<NPObject*>(VariantValue<void *>()(ret)), *result);
      else
        ConvertLocalToNP(NULL, ret, result);
    }
    return true;
  }
  return false;
}

static bool HasProperty(NPObject *npobj, NPIdentifier id) {
  EXPECT_EQ(&mock_npobj, npobj);
  EXPECT_TRUE(id);
  if (NPAPIImpl::NPN_IdentifierIsString(id)) {
    char *name = NPAPIImpl::NPN_UTF8FromIdentifier(id);
    for (size_t i = 0; i < Properties.size(); i++) {
      if (strncmp(Properties[i].name_, name, strlen(Properties[i].name_)) == 0) {
        NPAPIImpl::NPN_MemFree(name);
        return true;
      }
    }
    NPAPIImpl::NPN_MemFree(name);
    return false;
  } else {
    unsigned int index = NPAPIImpl::NPN_IntFromIdentifier(id);
    return Properties.size() > index;
  }
}

static bool GetProperty(NPObject *npobj, NPIdentifier id, NPVariant *result) {
  EXPECT_EQ(&mock_npobj, npobj);
  EXPECT_TRUE(id);
  if (NPAPIImpl::NPN_IdentifierIsString(id)) {
    char *name = NPAPIImpl::NPN_UTF8FromIdentifier(id);
    for (size_t i = 0; i < Properties.size(); i++) {
      if (strncmp(Properties[i].name_, name, strlen(Properties[i].name_)) == 0) {
        if (result)
          // For NPString and NPObject, we should make a copy rather than just
          // pass the pointer, as the caller will release them. But for this
          // unittest, for simplicity's sake, we just pass the pointer for
          // these two types and our mocked caller won't free them.
          *result = Properties[i].value_;
        NPAPIImpl::NPN_MemFree(name);
        return true;
      }
    }
    NPAPIImpl::NPN_MemFree(name);
  } else {
    unsigned int index = NPAPIImpl::NPN_IntFromIdentifier(id);
    if (index < Properties.size()) {
      if (result)
        *result = Properties[index].value_;
      return true;
    }
  }
  return false;
}

static bool SetProperty(NPObject *npobj, NPIdentifier id,
                        const NPVariant *value) {
  EXPECT_EQ(&mock_npobj, npobj);
  EXPECT_TRUE(id);
  if (NPAPIImpl::NPN_IdentifierIsString(id)) {
    char *name = NPAPIImpl::NPN_UTF8FromIdentifier(id);
    for (size_t i = 0; i < Properties.size(); i++) {
      if (strncmp(Properties[i].name_, name, strlen(Properties[i].name_)) == 0) {
        if (value)
          // For simplicity's sake.
          Properties[i].value_ = *value;
        NPAPIImpl::NPN_MemFree(name);
        return true;
      }
    }
    NPAPIImpl::NPN_MemFree(name);
  } else {
    unsigned int index = NPAPIImpl::NPN_IntFromIdentifier(id);
    if (index < Properties.size()) {
      if (value)
        Properties[index].value_ = *value;
      return true;
    }
  }
  return false;
}

static void InitNPObject() {
  InitPropertiesAndMethods();
  mock_class.allocate = NULL;
  mock_class.deallocate = NULL;
  mock_class.invalidate = NULL;
  mock_class.invokeDefault = NULL;
  mock_class.removeProperty = NULL;
  mock_class.hasMethod = (NPHasMethodFunctionPtr)HasMethod;
  mock_class.invoke = (NPInvokeFunctionPtr)Invoke;
  mock_class.hasProperty = (NPHasPropertyFunctionPtr)HasProperty;
  mock_class.getProperty = (NPGetPropertyFunctionPtr)GetProperty;
  mock_class.setProperty = (NPSetPropertyFunctionPtr)SetProperty;
}

} // namespace npapi
} // namespace ggadget

using namespace ggadget;
using namespace ggadget::npapi;

TEST(NPAPIPluginScript, CallNPPluginObject) {
  // Mocked NPPluginObject.
  InitNPObject();
  NPPluginObject mock_npplugin_object((NPP)1, &mock_npobj);

  // Test hasProperty and getProperty.
  Variant prototype;
  EXPECT_EQ(mock_npplugin_object.GetPropertyInfo("integer", &prototype),
            ScriptableInterface::PROPERTY_DYNAMIC);
  Variant value = mock_npplugin_object.GetProperty("integer").v();
  EXPECT_EQ(value.type(), Variant::TYPE_INT64);
  EXPECT_EQ(VariantValue<int>()(value), 10);

  EXPECT_EQ(mock_npplugin_object.GetPropertyInfo("boolean", &prototype),
            ScriptableInterface::PROPERTY_DYNAMIC);
  value = mock_npplugin_object.GetProperty("boolean").v();
  EXPECT_EQ(value.type(), Variant::TYPE_BOOL);
  EXPECT_EQ(VariantValue<bool>()(value), true);

  EXPECT_EQ(mock_npplugin_object.GetPropertyInfo("string", &prototype),
            ScriptableInterface::PROPERTY_DYNAMIC);
  value = mock_npplugin_object.GetProperty("string").v();
  EXPECT_EQ(value.type(), Variant::TYPE_STRING);
  EXPECT_EQ(strncmp(VariantValue<const char *>()(value), "test", 4), 0);

  EXPECT_EQ(mock_npplugin_object.GetPropertyInfo("object", &prototype),
            ScriptableInterface::PROPERTY_DYNAMIC);
  ResultVariant result = mock_npplugin_object.GetProperty("object");
  value = result.v();
  EXPECT_EQ(value.type(), Variant::TYPE_SCRIPTABLE);
  EXPECT_EQ(VariantValue<NPPluginObject *>()(value)->UnWrap(), &mock_npobj);

  // Test setProperty.
  EXPECT_TRUE(mock_npplugin_object.SetProperty("integer", Variant(20)));
  value = mock_npplugin_object.GetProperty("integer").v();
  EXPECT_EQ(value.type(), Variant::TYPE_INT64);
  EXPECT_EQ(VariantValue<int>()(value), 20);

  // Test hasMethod and invoke.
  Slot *slot;
  result = mock_npplugin_object.GetProperty("TestBoolean");
  value = result.v();
  EXPECT_EQ(value.type(), Variant::TYPE_SCRIPTABLE);
  ScriptableInterface *script = VariantValue<ScriptableInterface *>()(value);
  value = script->GetProperty("").v();
  EXPECT_EQ(value.type(), Variant::TYPE_SLOT);
  slot = VariantValue<Slot *>()(value);
  EXPECT_TRUE(slot);
  Variant param = Variant(true);
  Variant ret = slot->Call(NULL, 1, &param).v();
  EXPECT_EQ(ret.type(), Variant::TYPE_BOOL);
  EXPECT_EQ(VariantValue<bool>()(ret), true);

  result = mock_npplugin_object.GetProperty("TestString");
  value = result.v();
  EXPECT_EQ(value.type(), Variant::TYPE_SCRIPTABLE);
  script = VariantValue<ScriptableInterface *>()(value);
  value = script->GetProperty("").v();
  EXPECT_EQ(value.type(), Variant::TYPE_SLOT);
  slot = VariantValue<Slot *>()(value);
  EXPECT_TRUE(slot);
  param = Variant("test");
  ret = slot->Call(NULL, 1, &param).v();
  EXPECT_EQ(ret.type(), Variant::TYPE_STRING);
  EXPECT_EQ(strncmp(VariantValue<const char *>()(ret), "test", 4), 0);

  result = mock_npplugin_object.GetProperty("TestInteger");
  value = result.v();
  EXPECT_EQ(value.type(), Variant::TYPE_SCRIPTABLE);
  script = VariantValue<ScriptableInterface *>()(value);
  value = script->GetProperty("").v();
  EXPECT_EQ(value.type(), Variant::TYPE_SLOT);
  slot = VariantValue<Slot *>()(value);
  EXPECT_TRUE(slot);
  param = Variant(50);
  ret = slot->Call(NULL, 1, &param).v();
  EXPECT_EQ(ret.type(), Variant::TYPE_INT64);
  EXPECT_EQ(VariantValue<int>()(ret), 50);

  result = mock_npplugin_object.GetProperty("TestObject");
  value = result.v();
  EXPECT_EQ(value.type(), Variant::TYPE_SCRIPTABLE);
  script = VariantValue<ScriptableInterface *>()(value);
  value = script->GetProperty("").v();
  EXPECT_EQ(value.type(), Variant::TYPE_SLOT);
  slot = VariantValue<Slot *>()(value);
  EXPECT_TRUE(slot);
  param = Variant(&mock_npplugin_object);
  result = slot->Call(NULL, 1, &param);
  ret = result.v();
  EXPECT_EQ(ret.type(), Variant::TYPE_SCRIPTABLE);
  EXPECT_EQ(VariantValue<NPPluginObject *>()(ret)->UnWrap(), &mock_npobj);
}

MockedTimerMainLoop main_loop(0);

int main(int argc, char **argv) {
  ggadget::SetGlobalMainLoop(&main_loop);
  testing::ParseGTestFlags(&argc, argv);
  int result = RUN_ALL_TESTS();
  return result;
}
