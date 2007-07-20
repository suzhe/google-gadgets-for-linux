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

#ifndef GGADGET_SIGNAL_H__
#define GGADGET_SIGNAL_H__

#include <list>
#include "slot.h"

namespace ggadget {

/**
 * The connection object between a @c Singal and a @c Slot.
 * The caller can use the connection to temporarily block the slot.
 */
class Connection {
 public:

  /**
   * Block the connection.  No more signals will be emitted to the slot.
   * It's useful when the caller know that a <code>MethodSlot</code>'s
   * underlying object has been deleted.
   */
  void Block() { blocked_ = true; }

  /**
   * Disconnect the connection.
   * After disconnection, the connection can't be unblocked any more.
   */
  void Disconnect() { blocked_ = true; slot_ = NULL; }

  void Unblock() { if (slot_) blocked_ = false; }
  bool blocked() { return blocked_; }

 private:
  friend class Signal;

  /**
   * Connection can be only constructed and destructed by @c Signal.
   * The connection owns the slot.
   */
  Connection(Slot *slot) : blocked_(false), slot_(slot) { }
  ~Connection() { if (slot_) delete slot_; }

  bool blocked_;
  Slot *slot_;
};

/**
 * Signal caller that can connect and emit to 0 to many <code>Slot</code>s.
 */
class Signal {
 public:
  virtual ~Signal();

  /**
   * Connect a general @c Slot (compile-time type not specialized from
   * templates).  It's useful to connect to <code>ScriptSlot</code>s
   * and <code>SignalSlot</code>s. Compatability is checked inside of
   * @c ConnectGeneral() at runtime.
   * @param slot the slot to connect. After conntected, this signal takes
   *     the ownership of the slot pointer, so don't share slots with other
   *     owners.
   * @return the connected @c Connection.  The pointer is owned by this signal.
   *     Return @c NULL on any error, such as argument incompatibility.
   */
  Connection *ConnectGeneral(Slot *slot);

  /**
   * Emit the signal in general format.
   * Normally C++ code should use @c operator() in the templated subclasses.
   */
  Variant Emit(int argc, Variant argv[]);

  /**
   * Get metadata of the @c Signal.
   */
  virtual Variant::Type GetReturnType() const { return Variant::TYPE_VOID; }
  /**
   * Get metadata of the @c Signal.
   */
  virtual int GetArgCount() const { return 0; }
  /**
   * Get metadata of the @c Signal.
   */
  virtual const Variant::Type *GetArgTypes() const { return NULL; }

 protected:
  Signal();

  /**
   * Connect to a slot without runtime compatability checking, for use by
   * templated subclasses because their compatability are checked at compile
   * time.
   * @param slot same as @c ConnectGeneral().
   * @return same as @c ConnectGeneral().
   */
  Connection *Connect(Slot *slot);

 private:
  typedef std::list<Connection *> ConnectionList;
  ConnectionList connections_;
};

/**
 * Wrap a @c Signal as a @c Slot to enable complex signal emitting paths.
 */
class SignalSlot : public Slot {
 public:
  /**
   * The @c SignalSlot doesn't has ownership of the signal.
   * @param signal the @c Signal to wrap.
   */
  SignalSlot(Signal *signal) : signal_(signal) { }

  virtual Variant Call(int argc, Variant argv[]) const {
    return signal_->Emit(argc, argv);
  }
  virtual Variant::Type GetReturnType() const {
    return signal_->GetReturnType();
  }
  virtual int GetArgCount() const {
    return signal_->GetArgCount();
  }
  virtual const Variant::Type *GetArgTypes() const {
    return signal_->GetArgTypes();
  }
  virtual bool operator==(const Slot &another) const {
    // Don't support this.
    ASSERT(false);
    return false;
  }
 private:
  Signal *signal_;
};

/**
 * A @c Signal without parameter.
 */
template <typename R>
class Signal0 : public Signal {
 public:
  Signal0() { }
  Connection *Connect(Slot0<R> *slot) { return Signal::Connect(slot); }
  R operator()() { return VariantValue<R>()(Emit(0, NULL)); }
  virtual Variant::Type GetReturnType() const { return VariantType<R>::type; }
};

/**
 * Partial specialized @c Signal0 that returns @c void.
 */
template <>
class Signal0<void> : public Signal {
 public:
  Connection *Connect(Slot0<void> *slot) { return Signal::Connect(slot); }
  void operator()() { Emit(0, NULL); }
};

/**
 * Helper function to create a new @c Signal from a function prototype.
 * This is useful only when <code>Signal *</code> is used polymorphically.
 * @param function the function used as a prototype.
 * @return the created @c Signal.
 */
template <typename R>
inline Signal0<R> *NewSignal(R (*function)()) {
  return new Signal0<R>();
}

/**
 * Helper function to create a new @c Signal from a method prototype.
 * This is useful only when <code>Signal *</code> is used polymorphically.    
 * @param method the method used as a prototype.
 * @return the created @c Signal.
 */
template <typename T, typename R>
inline Signal0<R> *NewSignal(R (T::*method)()) {
  return new Signal0<R>();
}

/**
 * Other <code>Signal</code>s are defined by this macro.
 */
#define DEFINE_SIGNAL(n, _arg_types, _arg_type_names, _init_arg_types,        \
                      _args, _init_args)                                      \
template <typename R, _arg_types>                                             \
class Signal##n : public Signal {                                             \
 public:                                                                      \
  Signal##n() { _init_arg_types; }                                            \
  Connection *Connect(Slot##n<R, _arg_type_names> *slot) {                    \
    return Signal::Connect(slot);                                             \
  }                                                                           \
  R operator()(_args) {                                                       \
    Variant vargs[n];                                                         \
    _init_args;                                                               \
    return VariantValue<R>()(Emit(n, vargs));                                 \
  }                                                                           \
  virtual Variant::Type GetReturnType() const { return VariantType<R>::type; }\
  virtual int GetArgCount() const { return n; }                               \
  virtual const Variant::Type *GetArgTypes() const {                          \
    return ArgTypesHelper<_arg_type_names>();                                 \
  }                                                                           \
};                                                                            \
                                                                              \
template <_arg_types>                                                         \
class Signal##n<void, _arg_type_names> : public Signal {                      \
 public:                                                                      \
  Signal##n() { _init_arg_types; }                                            \
  Connection *Connect(Slot##n<void, _arg_type_names> *slot) {                 \
    return Signal::Connect(slot);                                             \
  }                                                                           \
  void operator()(_args) {                                                    \
    Variant vargs[n];                                                         \
    _init_args;                                                               \
    Emit(n, vargs);                                                           \
  }                                                                           \
  virtual int GetArgCount() const { return n; }                               \
  virtual const Variant::Type *GetArgTypes() const {                          \
    return ArgTypesHelper<_arg_type_names>();                                 \
  }                                                                           \
};                                                                            \
                                                                              \
template <typename R, _arg_types>                                             \
inline Signal##n<R, _arg_type_names> *                                        \
NewSignal(R (*function)(_arg_type_names)) {                                   \
  return new Signal##n<R, _arg_type_names>();                                 \
}                                                                             \
template <typename T, typename R, _arg_types>                                 \
inline Signal##n<R, _arg_type_names> *                                        \
NewSignal(R (T::*method)(_arg_type_names)) {                                  \
  return new Signal##n<R, _arg_type_names>();                                 \
}                                                                             \

#define INIT_ARG_TYPE(n) VariantType<P##n>::type
#define INIT_ARG(n)      vargs[n-1] = Variant(p##n)

#define ARG_TYPES1      typename P1
#define ARG_TYPE_NAMES1 P1
#define INIT_ARG_TYPES1 INIT_ARG_TYPE(1)
#define ARGS1           P1 p1
#define INIT_ARGS1      INIT_ARG(1)
DEFINE_SIGNAL(1, ARG_TYPES1, ARG_TYPE_NAMES1,
              INIT_ARG_TYPES1, ARGS1, INIT_ARGS1)

#define ARG_TYPES2      ARG_TYPES1, typename P2
#define ARG_TYPE_NAMES2 ARG_TYPE_NAMES1, P2
#define INIT_ARG_TYPES2 INIT_ARG_TYPES1, INIT_ARG_TYPE(2)
#define ARGS2           ARGS1, P2 p2
#define INIT_ARGS2      INIT_ARGS1; INIT_ARG(2)
DEFINE_SIGNAL(2, ARG_TYPES2, ARG_TYPE_NAMES2,
              INIT_ARG_TYPES2, ARGS2, INIT_ARGS2)

#define ARG_TYPES3      ARG_TYPES2, typename P3
#define ARG_TYPE_NAMES3 ARG_TYPE_NAMES2, P3
#define INIT_ARG_TYPES3 INIT_ARG_TYPES2, INIT_ARG_TYPE(3)
#define ARGS3           ARGS2, P3 p3
#define INIT_ARGS3      INIT_ARGS2; INIT_ARG(3)
DEFINE_SIGNAL(3, ARG_TYPES3, ARG_TYPE_NAMES3,
              INIT_ARG_TYPES3, ARGS3, INIT_ARGS3)

#define ARG_TYPES4      ARG_TYPES3, typename P4
#define ARG_TYPE_NAMES4 ARG_TYPE_NAMES3, P4
#define INIT_ARG_TYPES4 INIT_ARG_TYPES3, INIT_ARG_TYPE(4)
#define ARGS4           ARGS3, P4 p4
#define INIT_ARGS4      INIT_ARGS3; INIT_ARG(4)
DEFINE_SIGNAL(4, ARG_TYPES4, ARG_TYPE_NAMES4,
              INIT_ARG_TYPES4, ARGS4, INIT_ARGS4)

#define ARG_TYPES5      ARG_TYPES4, typename P5
#define ARG_TYPE_NAMES5 ARG_TYPE_NAMES4, P5
#define INIT_ARG_TYPES5 INIT_ARG_TYPES4, INIT_ARG_TYPE(5)
#define ARGS5           ARGS4, P5 p5
#define INIT_ARGS5      INIT_ARGS4; INIT_ARG(5)
DEFINE_SIGNAL(5, ARG_TYPES5, ARG_TYPE_NAMES5,
              INIT_ARG_TYPES5, ARGS5, INIT_ARGS5)

#define ARG_TYPES6      ARG_TYPES5, typename P6
#define ARG_TYPE_NAMES6 ARG_TYPE_NAMES5, P6
#define INIT_ARG_TYPES6 INIT_ARG_TYPES5, INIT_ARG_TYPE(6)
#define ARGS6           ARGS5, P6 p6
#define INIT_ARGS6      INIT_ARGS5; INIT_ARG(6)
DEFINE_SIGNAL(6, ARG_TYPES6, ARG_TYPE_NAMES6,
              INIT_ARG_TYPES6, ARGS6, INIT_ARGS6)

#define ARG_TYPES7      ARG_TYPES6, typename P7
#define ARG_TYPE_NAMES7 ARG_TYPE_NAMES6, P7
#define INIT_ARG_TYPES7 INIT_ARG_TYPES6, INIT_ARG_TYPE(7)
#define ARGS7           ARGS6, P7 p7
#define INIT_ARGS7      INIT_ARGS6; INIT_ARG(7)
DEFINE_SIGNAL(7, ARG_TYPES7, ARG_TYPE_NAMES7,
              INIT_ARG_TYPES7, ARGS7, INIT_ARGS7)

#define ARG_TYPES8      ARG_TYPES7, typename P8
#define ARG_TYPE_NAMES8 ARG_TYPE_NAMES7, P8
#define INIT_ARG_TYPES8 INIT_ARG_TYPES7, INIT_ARG_TYPE(8)
#define ARGS8           ARGS7, P8 p8
#define INIT_ARGS8      INIT_ARGS7; INIT_ARG(8)
DEFINE_SIGNAL(8, ARG_TYPES8, ARG_TYPE_NAMES8,
              INIT_ARG_TYPES8, ARGS8, INIT_ARGS8)

#define ARG_TYPES9      ARG_TYPES8, typename P9
#define ARG_TYPE_NAMES9 ARG_TYPE_NAMES8, P9
#define INIT_ARG_TYPES9 INIT_ARG_TYPES8, INIT_ARG_TYPE(9)
#define ARGS9           ARGS8, P9 p9
#define INIT_ARGS9      INIT_ARGS8; INIT_ARG(9)
DEFINE_SIGNAL(9, ARG_TYPES9, ARG_TYPE_NAMES9,
              INIT_ARG_TYPES9, ARGS9, INIT_ARGS9)

// Undefine macros to avoid name polution.
#undef DEFINE_SIGNAL
#undef INIT_ARG

#undef ARG_TYPES1
#undef ARG_TYPE_NAMES1
#undef INIT_ARG_TYPES1
#undef INIT_ARGS1
#undef ARGS1
#undef ARG_TYPES2
#undef ARG_TYPE_NAMES2
#undef INIT_ARG_TYPES2
#undef INIT_ARGS2
#undef ARGS2
#undef ARG_TYPES3
#undef ARG_TYPE_NAMES3
#undef INIT_ARG_TYPES3
#undef INIT_ARGS3
#undef ARGS3
#undef ARG_TYPES4
#undef ARG_TYPE_NAMES4
#undef INIT_ARG_TYPES4
#undef INIT_ARGS4
#undef ARGS4
#undef ARG_TYPES5
#undef ARG_TYPE_NAMES5
#undef INIT_ARG_TYPES5
#undef INIT_ARGS5
#undef ARGS5
#undef ARG_TYPES6
#undef ARG_TYPE_NAMES6
#undef INIT_ARG_TYPES6
#undef INIT_ARGS6
#undef ARGS6
#undef ARG_TYPES7
#undef ARG_TYPE_NAMES7
#undef INIT_ARG_TYPES7
#undef INIT_ARGS7
#undef ARGS7
#undef ARG_TYPES8
#undef ARG_TYPE_NAMES8
#undef INIT_ARG_TYPES8
#undef INIT_ARGS8
#undef ARGS8
#undef ARG_TYPES9
#undef ARG_TYPE_NAMES9
#undef INIT_ARG_TYPES9
#undef INIT_ARGS9
#undef ARGS9

} // namespace ggadget

#endif // GGADGET_SIGNAL_H__
