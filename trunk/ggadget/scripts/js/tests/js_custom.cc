
// Hook from js.c to initialize test objects.

#include "scriptables.cc"

#include <jsapi.h>
JSBool InitCustomObjects(JSContext *cx, JSObject *obj) {
  TestScriptable1 *test_scriptable1 = new TestScriptable1();
  test_scriptable1->AddRef();
  JSObject *obj1 = NativeJSWrapper::Wrap(cx, test_scriptable1);
  ASSERT(obj1);
  JS_SetProperty(cx, obj, "obj1", OBJECT_TO_JSVAL(obj1));
  return JS_TRUE;
}
