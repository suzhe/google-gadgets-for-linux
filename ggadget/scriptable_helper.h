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

#ifndef GGADGET_SCRIPTABLE_HELPER_H__
#define GGADGET_SCRIPTABLE_HELPER_H__

#include <ggadget/common.h>
#include <ggadget/registerable_interface.h>
#include <ggadget/scriptable_interface.h>
#include <ggadget/signals.h>
#include <ggadget/slot.h>
#include <ggadget/variant.h>

namespace ggadget {

namespace internal {

class ScriptableHelperImplInterface : public ScriptableInterface,
                                      public RegisterableInterface {
 public:
  virtual ~ScriptableHelperImplInterface() { }
  virtual void SetInheritsFrom(ScriptableInterface *inherits_from) = 0;
  virtual void SetArrayHandler(Slot *getter, Slot *setter) = 0;
  virtual void SetDynamicPropertyHandler(Slot *getter, Slot *setter) = 0;
  virtual void SetPendingException(ScriptableInterface *exception) = 0;
};

ScriptableHelperImplInterface *NewScriptableHelperImpl(
    Slot0<void> *do_register);

} // namespace internal

/**
 * A @c ScriptableInterface implementation helper.
 */
template <typename I>
class ScriptableHelper : public I, public RegisterableInterface {
 private:
  // Checks at compile time if the argument I is ScriptableInterface or
  // derived from it.
  COMPILE_ASSERT((IsDerived<ScriptableInterface, I>::value),
                 I_must_be_ScriptableInterface_or_derived_from_it);

 public:
  ScriptableHelper()
      : impl_(internal::NewScriptableHelperImpl(
            NewSlot(this, &ScriptableHelper::DoRegister))) {
  }

  virtual ~ScriptableHelper() {
    delete impl_;
  }

  /** @see RegisterableInterface::RegisterProperty() */
  virtual void RegisterProperty(const char *name, Slot *getter, Slot *setter) {
    impl_->RegisterProperty(name, getter, setter);
  }

  /**
   * Register a simple scriptable property that maps to a variable.
   * @param name property name.  It must point to static allocated memory.
   * @param valuep point to a value.
   */
  template <typename T>
  void RegisterSimpleProperty(const char *name, T *valuep) {
    impl_->RegisterProperty(name,
                            NewSimpleGetterSlot<T>(valuep),
                            NewSimpleSetterSlot<T>(valuep));
  }

  /**
   * Register a simple readonly scriptable property that maps to a variable.
   * @param name property name.  It must point to static allocated memory.
   * @param valuep point to a value.
   */
  template <typename T>
  void RegisterReadonlySimpleProperty(const char *name, const T *valuep) {
    impl_->RegisterProperty(name, NewSimpleGetterSlot<T>(valuep), NULL);
  }

  /** @see RegisterableInterface::RegisterStringEnumProperty() */
  virtual void RegisterStringEnumProperty(const char *name,
                                          Slot *getter, Slot *setter,
                                          const char **names, int count) {
    impl_->RegisterStringEnumProperty(name, getter, setter, names, count);
  }

  /** @see RegisterableInterface::RegisterMethod() */
  virtual void RegisterMethod(const char *name, Slot *slot) {
    impl_->RegisterMethod(name, slot);
  }

  /** @see RegisterableInterface::RegisterSignal() */
  virtual void RegisterSignal(const char *name, Signal *signal) {
    impl_->RegisterSignal(name, signal);
  }

  /** @see RegisterableInterface::RegisterVariantConstant() */
  virtual void RegisterVariantConstant(const char *name,
                                       const Variant &value) {
    impl_->RegisterVariantConstant(name, value);
  }

  /**
   * Register a set of constants.
   * @param count number of constants to register.
   * @param names array of names of the constants.  The pointers must point to
   *     static allocated memory.
   * @param values array of constant values.  If it is @c NULL, the values
   *     will be automatically assigned from @c 0 to count-1, which is useful
   *     to define enum values.
   */
  void RegisterConstants(int count,
                         const char *const names[],
                         const Variant values[]) {
    ASSERT(names);
    for (int i = 0; i < count; i++)
      impl_->RegisterVariantConstant(names[i], values ? values[i] : Variant(i));
  }

  /**
   * Register a constant.
   * @param name the constant name.
   * @param value the constant value.
   */
  template <typename T>
  void RegisterConstant(const char *name, T value) {
    impl_->RegisterVariantConstant(name, Variant(value));
  }

  /**
   * Set a object from which this object inherits common properties (including
   * methods and signals).
   * Any operations to properties not registered in current
   * @c ScriptableHelper object are delegated to @a inherits_from.
   * One @a inherits_from object can be shared among multiple
   * <code>ScriptableHelper</code>s.
   */
  void SetInheritsFrom(ScriptableInterface *inherits_from) {
    impl_->SetInheritsFrom(inherits_from);
  }

  /**
   * Set the array handler which will handle array accesses.
   * @param getter handles 'get' accesses.  It accepts an int parameter as the
   *     array index and return the result of any type that can be contained
   *     in a @c Variant.  It should return a @c Variant of type
   *     @c Variant::TYPE_VOID if it doesn't support the property.
   * @param setter handles 'set' accesses.  It accepts an int parameter as the
   *     array index and a value.  If it returns a @c bool value, @c true on
   *     success.
   */
  void SetArrayHandler(Slot *getter, Slot *setter) {
    impl_->SetArrayHandler(getter, setter);
  }

  /**
   * Set the dynamic property handler which will handle property accesses not
   * registered statically.
   *
   * @param getter handles 'get' accesses.  It accepts a property name
   *     parameter (<code>const char *</code>) and return the result of any
   *     type that can be contained in a @c Variant.
   * @param setter handles 'set' accesses.  It accepts a property name
   *     parameter (<code>const char *</code>) and a value. If it returns a
   *     @c bool value, @c true on success.
   */
  void SetDynamicPropertyHandler(Slot *getter, Slot *setter) {
    impl_->SetDynamicPropertyHandler(getter, setter);
  }

  /**
   * Sets the exception to be raised to the script engine.
   * There must be no pending exception when this method is called to prevent
   * memory leaks.
   */
  void SetPendingException(ScriptableInterface *exception) {
    impl_->SetPendingException(exception);
  }

  /**
   * Used to register a setter that does nothing. There is no DummyGetter()
   * because a NULL getter acts like it.
   */
  static void DummySetter(const Variant &) { }

  /**
   * @see ScriptableInterface::Ref()
   * Normally this method is not allowed to be overriden.
   */
  virtual void Ref() { impl_->Ref(); }

  /**
   * @see ScriptableInterface::Unref()
   * Normally this method is not allowed to be overriden.
   */
  virtual void Unref(bool transient = false) {
    impl_->Unref(transient);
    if (!transient && impl_->GetRefCount() == 0) delete this;
  }

  /**
   * @see ScriptableInterface::GetRefCount()
   * Normally this method is not allowed to be overriden.
   */
  virtual int GetRefCount() const { return impl_->GetRefCount(); }

  /**
   * Default strict policy.
   * @see ScriptableInterface::IsStrict()
   */
  virtual bool IsStrict() const { return true; }

  /** @see ScriptableInterface::ConnectionOnReferenceChange() */
  virtual Connection *ConnectOnReferenceChange(Slot2<void, int, int> *slot) {
    return impl_->ConnectOnReferenceChange(slot);
  }

  /** @see ScriptableInterface::GetPropertyInfoByName() */
  virtual bool GetPropertyInfoByName(const char *name,
                                     int *id, Variant *prototype,
                                     bool *is_method) {
    return impl_->GetPropertyInfoByName(name, id, prototype, is_method);
  }

  /** @see ScriptableInterface::GetPropertyInfoById() */
  virtual bool GetPropertyInfoById(int id, Variant *prototype,
                                   bool *is_method, const char **name) {
    return impl_->GetPropertyInfoById(id, prototype, is_method, name);
  }

  /** @see ScriptableInterface::GetProperty() */
  virtual Variant GetProperty(int id) {
    return impl_->GetProperty(id);
  }

  /** @see ScriptableInterface::SetProperty() */
  virtual bool SetProperty(int id, const Variant &value) {
    return impl_->SetProperty(id, value);
  }

  /** @see ScriptableInterface::GetPendingException() */
  virtual ScriptableInterface *GetPendingException(bool clear) {
    return impl_->GetPendingException(clear);
  }

  /** @see ScriptableInterface::EnumerateProperties() */
  virtual bool EnumerateProperties(
      ScriptableInterface::EnumeratePropertiesCallback *callback) {
    return impl_->EnumerateProperties(callback);
  }

  /** @see ScriptableInterface::EnumerateElements() */
  virtual bool EnumerateElements(
      ScriptableInterface::EnumerateElementsCallback *callback) {
    return impl_->EnumerateElements(callback);
  }

  /** @see ScriptableInterface::GetRegisterable() */
  virtual RegisterableInterface *GetRegisterable() { return impl_; }

 protected:
  /**
   * The subclass overrides this method to register its scriptable properties
   * (including methods and signals).
   * A subclass can select from two methods to register scriptable properties:
   *   - Registering properties in its constructor;
   *   - Registering properties in this method.
   * If an instance of a class is not used in script immediately after creation,
   * the class should use the latter method to reduce overhead on creation.
   */
  virtual void DoRegister() { }

 private:
  DISALLOW_EVIL_CONSTRUCTORS(ScriptableHelper);

  internal::ScriptableHelperImplInterface *impl_;
};

typedef ScriptableHelper<ScriptableInterface> ScriptableHelperDefault;

/**
 * For objects that are owned by the native side. The native side can neglect
 * the reference counting things and just use the objects as normal C++
 * objects. For example, define the objects as local variables or data members,
 * as well as pointers.
 */
template <typename I>
class ScriptableHelperNativeOwned : public ScriptableHelper<I> {
 public:
  ScriptableHelperNativeOwned() {
    ScriptableHelper<I>::Ref();
  }
  virtual ~ScriptableHelperNativeOwned() {
    ScriptableHelper<I>::Unref(true);
  }
};

typedef ScriptableHelperNativeOwned<ScriptableInterface>
    ScriptableHelperNativeOwnedDefault;

/**
 * A handy class that can be used as simple native owned scriptable object.
 */
class NativeOwnedScriptable : public ScriptableHelperNativeOwnedDefault {
 public:
  DEFINE_CLASS_ID(0x47d47fe768a8496c, ScriptableInterface);
};

/**
 * A handy class that can be used as simple shared scriptable object.
 */
class SharedScriptable : public ScriptableHelperDefault {
 public:
  DEFINE_CLASS_ID(0x47d47fe768a8496c, ScriptableInterface);
};

/**
 * Utility function to get a named property from a scriptable object.
 * It calls <code>scriptable->GetPropertyInfoByName()</code> and then
 * <code>scriptable->GetProperty()</code>.
 */
Variant GetPropertyByName(ScriptableInterface *scriptable, const char *name);

} // namespace ggadget

#endif // GGADGET_SCRIPTABLE_HELPER_H__
