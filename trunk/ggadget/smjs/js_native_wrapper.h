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

#ifndef GGADGET_SCRIPTABLE_OBJECT_H__
#define GGADGET_SCRIPTABLE_OBJECT_H__

#include <jsapi.h>
#include <ggadget/common.h>
#include <ggadget/scriptable_helper.h>

namespace ggadget {
namespace smjs {

/**
 * Wraps a JavaScript object into a native @c ScriptableInterface.
 */
class JSNativeWrapper : public ScriptableHelper<ScriptableInterface> {
 public:
  DEFINE_CLASS_ID(0x65f4d888b7b749ed, ScriptableInterface);

  JSNativeWrapper(JSContext *js_context, JSObject *js_object);
  virtual ~JSNativeWrapper();

  // TODO: More sophisticated ownership policy.
  virtual OwnershipPolicy Attach() { return OWNERSHIP_TRANSFERRABLE; }
  virtual bool Detach() { delete this; return true; }

  Variant GetProperty(const char *name);
  bool SetProperty(const char *name, const Variant &value);
  Variant GetElement(int index);
  bool SetElement(int index, const Variant &value);

 private:
  DISALLOW_EVIL_CONSTRUCTORS(JSNativeWrapper);
  JSContext *js_context_;
  JSObject *js_object_;
};

} // namespace smjs
} // namespace ggadget

#endif // GGADGET_SCRIPTABLE_OBJECT_H__
