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

#ifndef GGADGET_SLOT_H__
#define GGADGET_SLOT_H__

#include <cstring>
#include "common.h"
#include "variant.h"

namespace ggadget {

/**
 * A @c Slot is a calling target.
 * The real targets are implemented in subclasses.
 * The instances are immutable, becuase all methods are @c const.
 */
class Slot {
 public:
  virtual ~Slot() { }

  /**
   * Call the <code>Slot</code>'s target.
   * The type of arguments and the return value must be compatible with the
   * actual calling target.
   * @see Variant
   * @param argc number of arguments.
   * @param argv argument array.  Can be @c NULL if <code>argc==0</code>.
   * @return the return value of the @c Slot target.
   */
  virtual Variant Call(int argc, Variant argv[]) const = 0;

  /**
   * @return @c true if this @c Slot can provide metadata.
   */
  virtual bool HasMetadata() const { return true; }

  /**
   * Get metadata of the @c Slot target.
   */
  virtual Variant::Type GetReturnType() const { return Variant::TYPE_VOID; }
  /**
   * Get metadata of the @c Slot target.
   */
  virtual int GetArgCount() const { return 0; }
  /**
   * Get metadata of the @c Slot target.
   */
  virtual const Variant::Type *GetArgTypes() const { return NULL; }

  /**
   * Equality tester, only for unit testing.
   * The slots to be tested must be of the same type, otherwise the program
   * may crash.
   */
  virtual bool operator==(const Slot &another) const = 0;

 protected:
  Slot() { }
};

/**
 * A @c Slot with no parameter.
 */
template <typename R>
class Slot0 : public Slot {
 public:
  virtual Variant::Type GetReturnType() const {
    CHECK_VARIANT_TYPE(R);
    return VariantType<R>::type;
  }
};

/**
 * Partial specialized @c Slot0 that returns @c void.
 */
template <>
class Slot0<void> : public Slot {
};

/**
 * A @c Slot that is targeted to a functor with no parameter.
 */
template <typename R, typename F>
class FunctorSlot0 : public Slot0<R> {
 public:
  typedef FunctorSlot0<R, F> SelfType;
  FunctorSlot0(F functor) : functor_(functor) { }
  virtual Variant Call(int argc, Variant argv[]) const {
    ASSERT(argc == 0);
    return Variant(functor_());
  }
  virtual bool operator==(const Slot &another) const {
    return functor_ == down_cast<const SelfType *>(&another)->functor_;
  }
 private:
  F functor_;
};

/**
 * Partial specialized @c FunctorSlot0 that returns @c void.
 */
template <typename F>
class FunctorSlot0<void, F> : public Slot0<void> {
 public:
  typedef FunctorSlot0<void, F> SelfType;
  FunctorSlot0(F functor) : functor_(functor) { }
  virtual Variant Call(int argc, Variant argv[]) const {
    ASSERT(argc == 0);
    functor_();
    return Variant();
  }
  virtual bool operator==(const Slot &another) const {
    return functor_ == down_cast<const SelfType *>(&another)->functor_;
  }
 private:
  F functor_;
};

/**
 * A @c Slot that is targeted to a C++ non-static method with no parameter.
 */
template <typename R, typename T, typename M>
class MethodSlot0 : public Slot0<R> {
 public:
  typedef MethodSlot0<R, T, M> SelfType;
  MethodSlot0(T* object, M method) : object_(object), method_(method) { }
  virtual Variant Call(int argc, Variant argv[]) const {
    ASSERT(argc == 0);
    return Variant((object_->*method_)());
  }
  virtual bool operator==(const Slot &another) const {
    return object_ == down_cast<const SelfType *>(&another)->object_ &&
           method_ == down_cast<const SelfType *>(&another)->method_;
  }
 private:
  T *object_;
  M method_;
};

/**
 * Partial specialized @c MethodSlot0 that returns @c void.
 */
template <typename T, typename M>
class MethodSlot0<void, T, M> : public Slot0<void> {
 public:
  typedef MethodSlot0<void, T, M> SelfType;
  MethodSlot0(T* object, M method) : object_(object), method_(method) { }
  virtual Variant Call(int argc, Variant argv[]) const {
    ASSERT(argc == 0);
    (object_->*method_)();
    return Variant();
  }
  virtual bool operator==(const Slot &another) const {
    return object_ == down_cast<const SelfType *>(&another)->object_ &&
           method_ == down_cast<const SelfType *>(&another)->method_;
  }
 private:
  T *object_;
  M method_;
};

/**
 * Helper functor to create a @c FunctorSlot0 instance with a C/C++ function
 * or a static method.
 * The caller should delete the instance after use.
 */
template <typename R>
inline Slot0<R> *NewSlot(R (*functor)()) {
  return new FunctorSlot0<R, R (*)()>(functor);
}

/**
 * Helper functor to create a @c MethodSlot0 instance.
 * The type of method must be of
 * <code>R (T::*)()</code> or <code>R (T::*)() const</code>.
 * The caller should delete the instance after use.
 */
template <typename R, typename T>
inline Slot0<R> *NewSlot(T *object, R (T::*method)()) {
  return new MethodSlot0<R, T, R (T::*)()>(object, method);
}
template <typename R, typename T>
inline Slot0<R> *NewSlot(const T *object, R (T::*method)() const) {
  return new MethodSlot0<R, const T, R (T::*)() const>(object, method);
}

/**
 * Helper functor to create a @c FunctorSlot0 instance with a functor object.
 * The caller should delete the instance after use.
 * Place <code>typename F</code> at the end of the template parameter to
 * let the compiler populate the default type.
 *
 * Usage: <code>NewFunctorSlot<int>(AFunctor());</code>
 */
template <typename R, typename F>
inline Slot0<R> *NewFunctorSlot(F functor) {
  return new FunctorSlot0<R, F>(functor);
}

/**
 * <code>Slot</code>s with 1 or more parameters are defined by this macro.
 */
#define DEFINE_SLOT(n, _arg_types, _arg_type_names,                           \
                    _init_arg_types, _call_args)                              \
template <_arg_types>                                                         \
inline const Variant::Type *ArgTypesHelper() {                                \
  static Variant::Type arg_types[] = { _init_arg_types };                     \
  return arg_types;                                                           \
}                                                                             \
                                                                              \
template <typename R, _arg_types>                                             \
class Slot##n : public Slot {                                                 \
 public:                                                                      \
  virtual Variant::Type GetReturnType() const {                               \
    CHECK_VARIANT_TYPE(R);                                                    \
    return VariantType<R>::type;                                              \
  }                                                                           \
  virtual int GetArgCount() const { return n; }                               \
  virtual const Variant::Type *GetArgTypes() const {                          \
    return ArgTypesHelper<_arg_type_names>();                                 \
  }                                                                           \
};                                                                            \
                                                                              \
template <_arg_types>                                                         \
class Slot##n<void, _arg_type_names> : public Slot {                          \
 public:                                                                      \
  virtual int GetArgCount() const { return n; }                               \
  virtual const Variant::Type *GetArgTypes() const {                          \
    return ArgTypesHelper<_arg_type_names>();                                 \
  }                                                                           \
};                                                                            \
                                                                              \
template <typename R, _arg_types, typename F>                                 \
class FunctorSlot##n : public Slot##n<R, _arg_type_names> {                   \
 public:                                                                      \
  typedef FunctorSlot##n<R, _arg_type_names, F> SelfType;                     \
  FunctorSlot##n(F functor) : functor_(functor) { }                           \
  virtual Variant Call(int argc, Variant argv[]) const {                      \
    ASSERT(argc == n);                                                        \
    return Variant(functor_(_call_args));                                     \
  }                                                                           \
  virtual bool operator==(const Slot &another) const {                        \
    return functor_ == down_cast<const SelfType *>(&another)->functor_;       \
  }                                                                           \
 private:                                                                     \
  F functor_;                                                                 \
};                                                                            \
                                                                              \
template <_arg_types, typename F>                                             \
class FunctorSlot##n<void, _arg_type_names, F> :                              \
    public Slot##n<void, _arg_type_names> {                                   \
 public:                                                                      \
  typedef FunctorSlot##n<void, _arg_type_names, F> SelfType;                  \
  FunctorSlot##n(F functor) : functor_(functor) { }                           \
  virtual Variant Call(int argc, Variant argv[]) const {                      \
    ASSERT(argc == n);                                                        \
    functor_(_call_args);                                                     \
    return Variant();                                                         \
  }                                                                           \
  virtual bool operator==(const Slot &another) const {                        \
    return functor_ == down_cast<const SelfType *>(&another)->functor_;       \
  }                                                                           \
 private:                                                                     \
  F functor_;                                                                 \
};                                                                            \
                                                                              \
template <typename R, _arg_types, typename T, typename M>                     \
class MethodSlot##n : public Slot##n<R, _arg_type_names> {                    \
 public:                                                                      \
  typedef MethodSlot##n<R, _arg_type_names, T, M> SelfType;                   \
  MethodSlot##n(T *obj, M method) : obj_(obj), method_(method) { }            \
  virtual Variant Call(int argc, Variant argv[]) const {                      \
    ASSERT(argc == n);                                                        \
    return Variant((obj_->*method_)(_call_args));                             \
  }                                                                           \
  virtual bool operator==(const Slot &another) const {                        \
    return obj_ == down_cast<const SelfType *>(&another)->obj_ &&             \
           method_ == down_cast<const SelfType *>(&another)->method_;         \
  }                                                                           \
 private:                                                                     \
  T *obj_;                                                                    \
  M method_;                                                                  \
};                                                                            \
                                                                              \
template <_arg_types, typename T, typename M>                                 \
class MethodSlot##n<void, _arg_type_names, T, M> :                            \
    public Slot##n<void, _arg_type_names> {                                   \
 public:                                                                      \
  typedef MethodSlot##n<void, _arg_type_names, T, M> SelfType;                \
  MethodSlot##n(T *obj, M method) : obj_(obj), method_(method) { }            \
  virtual Variant Call(int argc, Variant argv[]) const {                      \
    ASSERT(argc == n);                                                        \
    (obj_->*method_)(_call_args);                                             \
    return Variant();                                                         \
  }                                                                           \
  virtual bool operator==(const Slot &another) const {                        \
    return obj_ == down_cast<const SelfType *>(&another)->obj_ &&             \
           method_ == down_cast<const SelfType *>(&another)->method_;         \
  }                                                                           \
 private:                                                                     \
  T *obj_;                                                                    \
  M method_;                                                                  \
};                                                                            \
                                                                              \
template <typename R, _arg_types>                                             \
inline Slot##n<R, _arg_type_names> *NewSlot(R (*f)(_arg_type_names)) {        \
  return new FunctorSlot##n<R, _arg_type_names, R (*)(_arg_type_names)>(f);   \
}                                                                             \
template <typename R, _arg_types, typename T>                                 \
inline Slot##n<R, _arg_type_names> *                                          \
NewSlot(T *obj, R (T::*method)(_arg_type_names)) {                            \
  return new MethodSlot##n<R, _arg_type_names, T,                             \
                           R (T::*)(_arg_type_names)>(obj, method);           \
}                                                                             \
template <typename R, _arg_types, typename T>                                 \
inline Slot##n<R, _arg_type_names> *                                          \
NewSlot(const T *obj, R (T::*method)(_arg_type_names) const) {                \
  return new MethodSlot##n<R, _arg_type_names, const T,                       \
                           R (T::*)(_arg_type_names) const>(obj, method);     \
}                                                                             \
template <typename R, _arg_types, typename F>                                 \
inline Slot##n<R, _arg_type_names> * NewFunctorSlot(F f) {                    \
  return new FunctorSlot##n<R, _arg_type_names, F>(f);                        \
}                                                                             \

#define INIT_ARG_TYPE(n) VariantType<P##n>::type
#define GET_ARG(n)       VariantValue<P##n>()(argv[n-1])

#define ARG_TYPES1      typename P1
#define ARG_TYPE_NAMES1 P1
#define INIT_ARG_TYPES1 INIT_ARG_TYPE(1)
#define CALL_ARGS1      GET_ARG(1)
DEFINE_SLOT(1, ARG_TYPES1, ARG_TYPE_NAMES1, INIT_ARG_TYPES1, CALL_ARGS1)

#define ARG_TYPES2      ARG_TYPES1, typename P2
#define ARG_TYPE_NAMES2 ARG_TYPE_NAMES1, P2
#define INIT_ARG_TYPES2 INIT_ARG_TYPES1, INIT_ARG_TYPE(2)
#define CALL_ARGS2      CALL_ARGS1, GET_ARG(2)
DEFINE_SLOT(2, ARG_TYPES2, ARG_TYPE_NAMES2, INIT_ARG_TYPES2, CALL_ARGS2)

#define ARG_TYPES3      ARG_TYPES2, typename P3
#define ARG_TYPE_NAMES3 ARG_TYPE_NAMES2, P3
#define INIT_ARG_TYPES3 INIT_ARG_TYPES2, INIT_ARG_TYPE(3)
#define CALL_ARGS3      CALL_ARGS2, GET_ARG(3)
DEFINE_SLOT(3, ARG_TYPES3, ARG_TYPE_NAMES3, INIT_ARG_TYPES3, CALL_ARGS3)

#define ARG_TYPES4      ARG_TYPES3, typename P4
#define ARG_TYPE_NAMES4 ARG_TYPE_NAMES3, P4
#define INIT_ARG_TYPES4 INIT_ARG_TYPES3, INIT_ARG_TYPE(4)
#define CALL_ARGS4      CALL_ARGS3, GET_ARG(4)
DEFINE_SLOT(4, ARG_TYPES4, ARG_TYPE_NAMES4, INIT_ARG_TYPES4, CALL_ARGS4)

#define ARG_TYPES5      ARG_TYPES4, typename P5
#define ARG_TYPE_NAMES5 ARG_TYPE_NAMES4, P5
#define INIT_ARG_TYPES5 INIT_ARG_TYPES4, INIT_ARG_TYPE(5)
#define CALL_ARGS5      CALL_ARGS4, GET_ARG(5)
DEFINE_SLOT(5, ARG_TYPES5, ARG_TYPE_NAMES5, INIT_ARG_TYPES5, CALL_ARGS5)

#define ARG_TYPES6      ARG_TYPES5, typename P6
#define ARG_TYPE_NAMES6 ARG_TYPE_NAMES5, P6
#define INIT_ARG_TYPES6 INIT_ARG_TYPES5, INIT_ARG_TYPE(6)
#define CALL_ARGS6      CALL_ARGS5, GET_ARG(6)
DEFINE_SLOT(6, ARG_TYPES6, ARG_TYPE_NAMES6, INIT_ARG_TYPES6, CALL_ARGS6)

#define ARG_TYPES7      ARG_TYPES6, typename P7
#define ARG_TYPE_NAMES7 ARG_TYPE_NAMES6, P7
#define INIT_ARG_TYPES7 INIT_ARG_TYPES6, INIT_ARG_TYPE(7)
#define CALL_ARGS7      CALL_ARGS6, GET_ARG(7)
DEFINE_SLOT(7, ARG_TYPES7, ARG_TYPE_NAMES7, INIT_ARG_TYPES7, CALL_ARGS7)

#define ARG_TYPES8      ARG_TYPES7, typename P8
#define ARG_TYPE_NAMES8 ARG_TYPE_NAMES7, P8
#define INIT_ARG_TYPES8 INIT_ARG_TYPES7, INIT_ARG_TYPE(8)
#define CALL_ARGS8      CALL_ARGS7, GET_ARG(8)
DEFINE_SLOT(8, ARG_TYPES8, ARG_TYPE_NAMES8, INIT_ARG_TYPES8, CALL_ARGS8)

#define ARG_TYPES9      ARG_TYPES8, typename P9
#define ARG_TYPE_NAMES9 ARG_TYPE_NAMES8, P9
#define INIT_ARG_TYPES9 INIT_ARG_TYPES8, INIT_ARG_TYPE(9)
#define CALL_ARGS9      CALL_ARGS8, GET_ARG(9)
DEFINE_SLOT(9, ARG_TYPES9, ARG_TYPE_NAMES9, INIT_ARG_TYPES9, CALL_ARGS9)

// Undefine macros to avoid name polution.
#undef DEFINE_SLOT
#undef INIT_ARG_TYPE
#undef GET_ARG

#undef ARG_TYPES1
#undef ARG_TYPE_NAMES1
#undef INIT_ARG_TYPES1
#undef CALL_ARGS1
#undef ARG_TYPES2
#undef ARG_TYPE_NAMES2
#undef INIT_ARG_TYPES2
#undef CALL_ARGS2
#undef ARG_TYPES3
#undef ARG_TYPE_NAMES3
#undef INIT_ARG_TYPES3
#undef CALL_ARGS3
#undef ARG_TYPES4
#undef ARG_TYPE_NAMES4
#undef INIT_ARG_TYPES4
#undef CALL_ARGS4
#undef ARG_TYPES5
#undef ARG_TYPE_NAMES5
#undef INIT_ARG_TYPES5
#undef CALL_ARGS5
#undef ARG_TYPES6
#undef ARG_TYPE_NAMES6
#undef INIT_ARG_TYPES6
#undef CALL_ARGS6
#undef ARG_TYPES7
#undef ARG_TYPE_NAMES7
#undef INIT_ARG_TYPES7
#undef CALL_ARGS7
#undef ARG_TYPES8
#undef ARG_TYPE_NAMES8
#undef INIT_ARG_TYPES8
#undef CALL_ARGS8
#undef ARG_TYPES9
#undef ARG_TYPE_NAMES9
#undef INIT_ARG_TYPES9
#undef CALL_ARGS9

template <typename T>
class FixedGetter {
 public:
  FixedGetter(T value) : value_(value) { }
  T operator()() const { return value_; }
  bool operator==(FixedGetter<T> another) const {
    return value_ == another.value_;
  }
 private:
  T value_;
};

template <typename T>
class SimpleGetter {
 public:
  SimpleGetter(const T *value_ptr) : value_ptr_(value_ptr) { }
  T operator()() const { return *value_ptr_; }
  bool operator==(SimpleGetter<T> another) const {
    return value_ptr_ == another.value_ptr_;
  }
 private:
  const T *value_ptr_;
};

template <typename T>
class SimpleSetter {
 public:
  SimpleSetter(T *value_ptr) : value_ptr_(value_ptr) { }
  void operator()(T value) const { *value_ptr_ = value; }
  bool operator==(SimpleSetter<T> another) const {
    return value_ptr_ == another.value_ptr_;
  }
 private:
  T *value_ptr_;
};

template <typename T>
class StringEnumGetter {
 private:
  struct SlotWrapper {
    Slot0<T> *slot;
    const char **names;
    int count;
    int ref_count;
  };

 public:
  StringEnumGetter(Slot0<T> *slot, const char **names, int count)
      : wrapper_(new SlotWrapper) {
    wrapper_->ref_count = 1;
    wrapper_->slot = slot;
    wrapper_->names = names;
    wrapper_->count = count;
  }
  StringEnumGetter(const StringEnumGetter &other) : wrapper_(NULL) {
    operator=(other);
  }
  ~StringEnumGetter() {
    Free();
  }
  const char *operator()() const {
    int index = VariantValue<int>()(wrapper_->slot->Call(0, NULL));
    return (index >= 0 &&
        index < wrapper_->count) ? wrapper_->names[index] : NULL;
  }
  bool operator==(StringEnumGetter<T> another) const {
    return wrapper_ == another.wrapper_;
  }
  StringEnumGetter &operator=(const StringEnumGetter &other) {
    if (this != &other) {
      Free();
      ++(other.wrapper_->ref_count);
      wrapper_ = other.wrapper_;
    }
    return *this;
  }
 private:
  void Free() {
    if (wrapper_) {
      if (--(wrapper_->ref_count) == 0) {
        delete wrapper_->slot;
        delete wrapper_;
      }
    }
  }
 private:
  SlotWrapper *wrapper_;
};

template <typename T>
class StringEnumSetter {
 private:
  struct SlotWrapper {
    Slot1<void, T> *slot;
    const char **names;
    int count;
    int ref_count;
  };

 public:
  StringEnumSetter(Slot1<void, T> *slot, const char **names, int count)
      : wrapper_(new SlotWrapper) {
    wrapper_->ref_count = 1;
    wrapper_->slot = slot;
    wrapper_->names = names;
    wrapper_->count = count;
  }
  StringEnumSetter(const StringEnumSetter &other) : wrapper_(NULL) {
    operator=(other);
  }
  ~StringEnumSetter() {
    Free();
  }
  void operator()(const char *name) const {
    for (int i = 0; i < wrapper_->count; i++)
      if (strcmp(name, wrapper_->names[i]) == 0) {
        Variant param(i);
        wrapper_->slot->Call(1, &param);
        return;
      }
    LOG("Invalid enumerated name: %s", name);
  }
  bool operator==(StringEnumSetter<T> another) const {
    return wrapper_ == another.wrapper_;
  }
  StringEnumSetter &operator=(const StringEnumSetter &other) {
    if (this != &other) {
      Free();
      ++(other.wrapper_->ref_count);
      wrapper_ = other.wrapper_;
    }
    return *this;
  }
 private:
  void Free() {
    if (wrapper_) {
      if (--(wrapper_->ref_count) == 0) {
        delete wrapper_->slot;
        delete wrapper_;
      }
    }
  }
 private:
   SlotWrapper *wrapper_;
};

/**
 * Helper function to new a @c Slot that always return a fixed @a value.
 * @param value the fixed value.
 * @return the getter @c Slot.
 */
template <typename T>
inline Slot0<T> *NewFixedGetterSlot(T value) {
  return NewFunctorSlot<T>(FixedGetter<T>(value));
}

/**
 * Helper function to new a @c Slot that can get a value from @a value_ptr.
 * @param value_ptr pointer to a value.
 * @return the getter @c Slot.
 */
template <typename T>
inline Slot0<T> *NewSimpleGetterSlot(const T *value_ptr) {
  return NewFunctorSlot<T>(SimpleGetter<T>(value_ptr));
}

/**
 * Helper function to new a @c Slot that can set a value to @a value_ptr.
 * @param value_ptr pointer to a value.
 * @return the getter @c Slot.
 */
template <typename T>
inline Slot1<void, T> *NewSimpleSetterSlot(T *value_ptr) {
  return NewFunctorSlot<void, T>(SimpleSetter<T>(value_ptr));
}

/**
 * Helper function to decorate another getter slot returning an enum
 * value into a slot returning a <code>const char *</code> value.
 * @param slot a getter slot returning an enum value.
 * @param names a table containing string values of every enum value.
 * @param count number of entries in the table.
 * @return the decorated slot.
 */  
template <typename T>
inline Slot0<const char *> *NewStringEnumGetterSlot(Slot0<T> *slot,
                                                    const char **names,
                                                    int count) {
  return NewFunctorSlot<const char *>(StringEnumGetter<T>(slot, names, count));
}

/**
 * Helper function to decorate another setter slot accepting an enum
 * value into a slot accepting a <code>const char *</code> value.
 * @param slot a setter slot accepting an enum value.
 * @param names a table containing string values of every enum value.
 * @param count number of entries in the table.
 * @return the decorated slot.
 */  
template <typename T>
inline Slot1<void, const char *> *NewStringEnumSetterSlot(Slot1<void, T> *slot,
                                                          const char **names,
                                                          int count) {
  return NewFunctorSlot<void, const char *>(
      StringEnumSetter<T>(slot, names, count));
}

} // namespace ggadget

#endif // GGADGET_SLOT_H__
