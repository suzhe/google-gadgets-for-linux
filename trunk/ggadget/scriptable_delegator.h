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

#ifndef GGADGET_SCRIPTABLE_DELEGATOR_H__
#define GGADGET_SCRIPTABLE_DELEGATOR_H__

#include <ggadget/scriptable_interface.h>

namespace ggadget {

/**
 * Wraps another @c ScriptableInterface instance and delegates all method
 * calls except @c IsStrict() to it.
 *
 * It's useful when registering different script objects backed with the same
 * C++ object in a script context.  For example, in a view's script context,
 * the view object is registered as the global object which is not strict,
 * while at the same time, as the global 'view' variable which is strict.
 * In this case, we register the non-strict view object as the global object,
 * while register a strict @c ScriptableDelegator of the view object as the
 * 'view' variable.
 *
 * @see ScriptableInterface::IsStrict()
 */
class ScriptableDelegator : public ScriptableInterface {
 public:
  ScriptableDelegator(ScriptableInterface *scriptable, bool strict)
      : scriptable_(scriptable),
    strict_(strict) { }
  virtual ~ScriptableDelegator() { }

	virtual uint64_t GetClassId() const { return scriptable_->GetClassId(); }
  virtual bool IsInstanceOf(uint64_t class_id) const {
    return scriptable_->IsInstanceOf(class_id);
  }
  virtual OwnershipPolicy Attach() { return NATIVE_OWNED; }
  virtual bool Detach() { return false; }
  virtual bool IsStrict() const { return strict_; }
  virtual Connection *ConnectToOnDeleteSignal(Slot0<void> *slot) {
    return scriptable_->ConnectToOnDeleteSignal(slot);
  }
  virtual bool GetPropertyInfoByName(const char *name,
                                     int *id, Variant *prototype,
                                     bool *is_method) {
    return scriptable_->GetPropertyInfoByName(name, id, prototype, is_method);
  }
  virtual bool GetPropertyInfoById(int id, Variant *prototype,
                                   bool *is_method, const char **name) {
    return scriptable_->GetPropertyInfoById(id, prototype, is_method, name);
  }
  virtual Variant GetProperty(int id) {
    return scriptable_->GetProperty(id);
  }
  virtual bool SetProperty(int id, Variant value) {
    return scriptable_->SetProperty(id, value);
  }
  virtual ScriptableInterface *GetPendingException(bool clear) {
    return scriptable_->GetPendingException(clear);
  }

 private:
  ScriptableInterface *scriptable_;
  bool strict_;
};

} // namespace ggadget

#endif // GGADGET_SCRIPTABLE_DELEGATOR_H__
