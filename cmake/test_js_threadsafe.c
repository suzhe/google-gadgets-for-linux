// This file is used to test if JS_THREADSAFE should be defined to use
// SpiderMonkey library.
// It will fail to link if JS_THREADSAFE macro is JS_THREADSAFE is defined
// but the library was not compiled with the flag.

#include <jsapi.h>

#if JS_VERSION < MIN_JS_VERSION
#error "SpiderMonkey version is too low."
#endif

int main() {
#ifdef JS_THREADSAFE
  // The following call will cause link error if the library was not compiled
  // with the JS_THREADSAFE flag.
  JS_BeginRequest(0);
  // Make sure the JS_THREADSAFE macro is effective.
  JS_GetClass((JSContext *)NULL, (JSObject *)NULL);
#else
  // Make sure the JS_THREADSAFE macro is not effective.
  JS_GetClass((JSObject *)NULL);
#endif
  return 0;
}
