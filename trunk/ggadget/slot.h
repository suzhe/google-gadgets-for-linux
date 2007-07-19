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

#ifndef GGADGETS_SLOT_H__
#define GGADGETS_SLOT_H__

#include "variant.h"

namespace ggadget {

/**
 * A @c Slot is a calling target.
 * The real targets are implemented in subclasses.
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

 protected:
  Slot() { }
};

/**
 * A @c Slot with no parameter.
 */
template <typename R>
class Slot0 : public Slot {
 public:
  virtual Variant::Type GetReturnType() const { return VariantType<R>(); }
};

/**
 * Partial specialized @c Slot0 that returns @c void.
 */
template <>
class Slot0<void> : public Slot {
};

/**
 * A @c Slot that is targeted to a C/C++ function or static method
 * with no parameter.
 */
template <typename R>
class FunctionSlot0 : public Slot0<R> {
 public:
  typedef R (*Function)();
  FunctionSlot0(Function function) : function_(function) { }
  virtual Variant Call(int argc, Variant argv[]) const {
    ASSERT(argc == 0);
    return Variant(function_());
  }
 private:
  Function function_;
};

/**
 * Partial specialized @c FunctionSlot0 that returns @c void.
 */
template <>
class FunctionSlot0<void> : public Slot0<void> {
 public:
  typedef void (*Function)();
  FunctionSlot0(Function function) : function_(function) { }
  virtual Variant Call(int argc, Variant argv[]) const {
    ASSERT(argc == 0);
    function_();
    return Variant();
  }
 private:
  Function function_;
};

/**
 * A @c Slot that is targeted to a C++ non-static method with no parameter.
 */
template <typename T, typename R>
class MethodSlot0 : public Slot0<R> {
 public:
  typedef R (T::*Method)();
  MethodSlot0(T* object, Method method) : object_(object), method_(method) { }
  virtual Variant Call(int argc, Variant argv[]) const {
    ASSERT(argc == 0);
    return Variant((object_->*method_)());
  }
 private:
  T* object_;
  Method method_;
};

/**
 * Partial specialized @c MethodSlot0 that returns @c void.
 */
template <typename T>
class MethodSlot0<T, void> : public Slot0<void> {
 public:
  typedef void (T::*Method)();
  MethodSlot0(T* object, Method method) : object_(object), method_(method) { }
  virtual Variant Call(int argc, Variant argv[]) const {
    ASSERT(argc == 0);
    (object_->*method_)();
    return Variant();
  }
 private:
  T* object_;
  Method method_;
};

/**
 * Helper function to create a @c FunctionSlot0 instance.
 * The caller should delete the instance after use.
 */
template <typename R>
inline Slot0<R> *NewSlot(R (*function)()) {
  return new FunctionSlot0<R>(function);
}

/**
 * Helper function to create a @c MethodSlot0 instance.
 * The caller should delete the instance after use.
 */
template <typename T, typename R>
inline Slot0<R> *NewSlot(T *object, R (T::*method)()) {
  return new MethodSlot0<T, R>(object, method);
}

/**
 * <code>Slot</code>s with 1 or more parameters are defined by this macro.
 */
#define DEFINE_SLOT(n, _arg_types, _arg_type_names,                           \
                    _init_arg_types, _call_args)                              \
template <typename R, _arg_types>                                             \
class Slot##n : public Slot {                                                 \
 public:                                                                      \
  virtual Variant::Type GetReturnType() const { return VariantType<R>(); }    \
  virtual int GetArgCount() const { return n; }                               \
  virtual const Variant::Type *GetArgTypes() const { return arg_types_; }     \
 protected:                                                                   \
  Slot##n() { _init_arg_types; }                                              \
 private:                                                                     \
  Variant::Type arg_types_[n];                                                \
};                                                                            \
                                                                              \
template <_arg_types>                                                         \
class Slot##n<void, _arg_type_names> : public Slot {                          \
 public:                                                                      \
  virtual int GetArgCount() const { return n; }                               \
  virtual const Variant::Type *GetArgTypes() const { return arg_types_; }     \
 protected:                                                                   \
  Slot##n() { _init_arg_types; }                                              \
 private:                                                                     \
  Variant::Type arg_types_[n];                                                \
};                                                                            \
                                                                              \
template <typename R, _arg_types>                                             \
class FunctionSlot##n : public Slot##n<R, _arg_type_names> {                  \
 public:                                                                      \
  typedef R (*Function)(_arg_type_names);                                     \
  FunctionSlot##n(Function function) : function_(function) { }                \
  virtual Variant Call(int argc, Variant argv[]) const {                      \
    ASSERT(argc == n);                                                        \
    return Variant(function_(_call_args));                                    \
  }                                                                           \
 private:                                                                     \
  Function function_;                                                         \
};                                                                            \
                                                                              \
template <_arg_types>                                                         \
class FunctionSlot##n<void, _arg_type_names> :                                \
    public Slot##n<void, _arg_type_names> {                                   \
 public:                                                                      \
  typedef void (*Function)(_arg_type_names);                                  \
  FunctionSlot##n(Function function) : function_(function) { }                \
  virtual Variant Call(int argc, Variant argv[]) const {                      \
    ASSERT(argc == n);                                                        \
    function_(_call_args);                                                    \
    return Variant();                                                         \
  }                                                                           \
 private:                                                                     \
  Function function_;                                                         \
};                                                                            \
                                                                              \
template <typename T, typename R, _arg_types>                                 \
class MethodSlot##n : public Slot##n<R, _arg_type_names> {                    \
 public:                                                                      \
  typedef R (T::*Method)(_arg_type_names);                                    \
  MethodSlot##n(T *obj, Method method) : obj_(obj), method_(method) { }       \
  virtual Variant Call(int argc, Variant argv[]) const {                      \
    ASSERT(argc == n);                                                        \
    return Variant((obj_->*method_)(_call_args));                             \
  }                                                                           \
 private:                                                                     \
  T* obj_;                                                                    \
  Method method_;                                                             \
};                                                                            \
                                                                              \
template <typename T, _arg_types>                                             \
class MethodSlot##n<T, void, _arg_type_names> :                               \
    public Slot##n<void, _arg_type_names> {                                   \
 public:                                                                      \
  typedef void (T::*Method)(_arg_type_names);                                 \
  MethodSlot##n(T *obj, Method method) : obj_(obj), method_(method) { }       \
  virtual Variant Call(int argc, Variant argv[]) const {                      \
    ASSERT(argc == n);                                                        \
    (obj_->*method_)(_call_args);                                             \
    return Variant();                                                         \
  }                                                                           \
 private:                                                                     \
  T* obj_;                                                                    \
  Method method_;                                                             \
};                                                                            \
                                                                              \
template <typename R, _arg_types>                                             \
inline Slot##n<R, _arg_type_names> *                                          \
NewSlot(R (*function)(_arg_type_names)) {                                     \
  return new FunctionSlot##n<R, _arg_type_names>(function);                   \
}                                                                             \
template <typename T, typename R, _arg_types>                                 \
inline Slot##n<R, _arg_type_names> *                                          \
NewSlot(T *obj, R (T::*method)(_arg_type_names)) {                            \
  return new MethodSlot##n<T, R, _arg_type_names>(obj, method);               \
}                                                                             \

#define INIT_ARG_TYPE(n) arg_types_[n-1] = VariantType<P##n>()
#define GET_ARG(n)       VariantValue<P##n>()(argv[n-1])

#define ARG_TYPES1      typename P1
#define ARG_TYPE_NAMES1 P1
#define INIT_ARG_TYPES1      INIT_ARG_TYPE(1)
#define CALL_ARGS1      GET_ARG(1)
DEFINE_SLOT(1, ARG_TYPES1, ARG_TYPE_NAMES1, INIT_ARG_TYPES1, CALL_ARGS1)

#define ARG_TYPES2      ARG_TYPES1, typename P2
#define ARG_TYPE_NAMES2 ARG_TYPE_NAMES1, P2
#define INIT_ARG_TYPES2      INIT_ARG_TYPES1; INIT_ARG_TYPE(2)
#define CALL_ARGS2      CALL_ARGS1, GET_ARG(2)
DEFINE_SLOT(2, ARG_TYPES2, ARG_TYPE_NAMES2, INIT_ARG_TYPES2, CALL_ARGS2)

#define ARG_TYPES3      ARG_TYPES2, typename P3
#define ARG_TYPE_NAMES3 ARG_TYPE_NAMES2, P3
#define INIT_ARG_TYPES3      INIT_ARG_TYPES2; INIT_ARG_TYPE(3)
#define CALL_ARGS3      CALL_ARGS2, GET_ARG(3)
DEFINE_SLOT(3, ARG_TYPES3, ARG_TYPE_NAMES3, INIT_ARG_TYPES3, CALL_ARGS3)

#define ARG_TYPES4      ARG_TYPES3, typename P4
#define ARG_TYPE_NAMES4 ARG_TYPE_NAMES3, P4
#define INIT_ARG_TYPES4      INIT_ARG_TYPES3; INIT_ARG_TYPE(4)
#define CALL_ARGS4      CALL_ARGS3, GET_ARG(4)
DEFINE_SLOT(4, ARG_TYPES4, ARG_TYPE_NAMES4, INIT_ARG_TYPES4, CALL_ARGS4)

#define ARG_TYPES5      ARG_TYPES4, typename P5
#define ARG_TYPE_NAMES5 ARG_TYPE_NAMES4, P5
#define INIT_ARG_TYPES5      INIT_ARG_TYPES4; INIT_ARG_TYPE(5)
#define CALL_ARGS5      CALL_ARGS4, GET_ARG(5)
DEFINE_SLOT(5, ARG_TYPES5, ARG_TYPE_NAMES5, INIT_ARG_TYPES5, CALL_ARGS5)

#define ARG_TYPES6      ARG_TYPES5, typename P6
#define ARG_TYPE_NAMES6 ARG_TYPE_NAMES5, P6
#define INIT_ARG_TYPES6      INIT_ARG_TYPES5; INIT_ARG_TYPE(6)
#define CALL_ARGS6      CALL_ARGS5, GET_ARG(6)
DEFINE_SLOT(6, ARG_TYPES6, ARG_TYPE_NAMES6, INIT_ARG_TYPES6, CALL_ARGS6)

#define ARG_TYPES7      ARG_TYPES6, typename P7
#define ARG_TYPE_NAMES7 ARG_TYPE_NAMES6, P7
#define INIT_ARG_TYPES7      INIT_ARG_TYPES6; INIT_ARG_TYPE(7)
#define CALL_ARGS7      CALL_ARGS6, GET_ARG(7)
DEFINE_SLOT(7, ARG_TYPES7, ARG_TYPE_NAMES7, INIT_ARG_TYPES7, CALL_ARGS7)

#define ARG_TYPES8      ARG_TYPES7, typename P8
#define ARG_TYPE_NAMES8 ARG_TYPE_NAMES7, P8
#define INIT_ARG_TYPES8      INIT_ARG_TYPES7; INIT_ARG_TYPE(8)
#define CALL_ARGS8      CALL_ARGS7, GET_ARG(8)
DEFINE_SLOT(8, ARG_TYPES8, ARG_TYPE_NAMES8, INIT_ARG_TYPES8, CALL_ARGS8)

#define ARG_TYPES9      ARG_TYPES8, typename P9
#define ARG_TYPE_NAMES9 ARG_TYPE_NAMES8, P9
#define INIT_ARG_TYPES9      INIT_ARG_TYPES8; INIT_ARG_TYPE(9)
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

} // namespace ggadget

#endif // GGADGETS_SLOT_H__
