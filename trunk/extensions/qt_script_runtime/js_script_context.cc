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
#include <map>
#include <string>
#include <ggadget/logger.h>
#include <ggadget/slot.h>
#include <ggadget/unicode_utils.h>
#include <ggadget/js/jscript_massager.h>
#include "js_script_context.h"
#include "js_function_slot.h"
#include "js_native_wrapper.h"
#include "js_script_runtime.h"
#include "converter.h"

#if 1
#undef DLOG
#define DLOG  true ? (void) 0 : LOG
#endif

namespace ggadget {
namespace qt {

static std::map<QScriptEngine*, JSScriptContext*> g_data;

JSScriptContext *GetEngineContext(QScriptEngine *engine) {
  return g_data[engine];
}

// String.substr is not ecma standard and qtscript doesn't provide it, so make
// our own
static QScriptValue substr(QScriptContext *context, QScriptEngine *engine) {
  QScriptValue self = context->thisObject();
  if (context->argumentCount() == 0) return self;
  int start = context->argument(0).toUInt32();
  int length = self.toString().length();
  if (context->argumentCount() >= 2)
    length = context->argument(1).toUInt32();
  return QScriptValue(engine, self.toString().mid(start, length));
}

// Check if obj has pending exception, if so, raise exception with ctx and
// return false.
// NOTE: Due to QT4's problem, sometimes calling throwValue/throwError is not
// enough. The exception has to be returned to JS as return value. In this
// case, you should provide ex so the exception will be stored to it.
static bool CheckException(QScriptContext *ctx, ScriptableInterface *object,
                           QScriptValue *ex) {
  if (!object) return true;
  ScriptableInterface *exception = object->GetPendingException(true);
  if (!exception) return true;

  QScriptValue qt_exception;
  qt_exception = QScriptValue(ctx->engine(), "Error...");
  if (!ConvertNativeToJS(ctx->engine(), Variant(exception), &qt_exception)) {
    qt_exception =
        ctx->throwError("Failed to convert native exception to QScriptValue");
  } else {
    qt_exception = ctx->throwValue(qt_exception);
  }
  if (ex) *ex = qt_exception;
  return false;
}

class ResolverScriptClass : public QScriptClass {
 public:
  ResolverScriptClass(QScriptEngine *engine, ScriptableInterface *object,
                      bool global);
  ~ResolverScriptClass();

  virtual QueryFlags queryProperty(const QScriptValue & object,
                                   const QScriptString & name,
                                   QueryFlags flags,
                                   uint * id);
  virtual QScriptValue property(const QScriptValue & object,
                                const QScriptString & name, uint id);
  virtual void setProperty(QScriptValue &object, const QScriptString &name,
                           uint id, const QScriptValue &value);
  virtual bool supportsExtension(Extension extension) const;
  virtual QVariant extension(Extension extension,
                             const QVariant &argument = QVariant());

  ScriptableInterface *object_;
  Slot *call_slot_;
  bool global_;
  Connection *on_reference_change_connection_;
  void OnRefChange(int, int);
};

class JSScriptContext::Impl {
 public:
  Impl(JSScriptContext *parent)
      : parent_(parent), resolver_(NULL), line_number_(0) {}
  ~Impl() {
    std::map<ScriptableInterface*, ResolverScriptClass*>::iterator iter;
    for (iter = script_classes_.begin(); iter != script_classes_.end(); iter++) {
      delete iter->second;
    }
    if (resolver_) delete resolver_;
  }

  bool SetGlobalObject(ScriptableInterface *global_object) {
    resolver_ = new ResolverScriptClass(&engine_, global_object, true);
    engine_.globalObject().setPrototype(engine_.newObject(resolver_));

    // Add non-standard method substr to String
    QScriptValue string_prototype =
        engine_.globalObject().property("String").property("prototype");
    string_prototype.setProperty("substr", engine_.newFunction(substr));
    return true;
  }

  ResolverScriptClass *GetScriptClass(ScriptableInterface *obj) {
    if (script_classes_.find(obj) == script_classes_.end()) {
      script_classes_[obj] = new ResolverScriptClass(&engine_, obj, false);
    }
    return script_classes_[obj];
  }

  // When native object is being destroyed, corresponding ResolverClass instance
  // will notice that through OnRefChange. It should remove corresponding
  // ResolverScriptClass and QScriptValue from JSContext by calling this method.
  void RemoveNativeObjectFromJSContext(ScriptableInterface *obj) {
    DLOG("RemoveNativeObjectFromJSContext: %p", obj);
    ASSERT(script_classes_.find(obj) != script_classes_.end());
    script_classes_.erase(obj);
    ASSERT(script_values_.find(obj) != script_values_.end());
    script_values_.erase(obj);
  }

  ScriptableInterface *WrapJSObject(const QScriptValue& qval) {
    ScriptableInterface *wrapper = JSNativeWrapper::UnwrapJSObject(qval);
    if (!wrapper)  wrapper = new JSNativeWrapper(parent_, qval);
    return wrapper;
  }

  // 3 kinds of native objects
  //  - real native objects
  //  - wrapper of js object from this js runtime
  //  - wrapper of js object from another js runtime
  QScriptValue GetScriptValueOfNativeObject(ScriptableInterface *obj) {
    if (obj->IsInstanceOf(JSNativeWrapper::CLASS_ID)) {
      JSNativeWrapper *wrapper = static_cast<JSNativeWrapper*>(obj);
      if (wrapper->context() == parent_)
        return wrapper->js_object();
    }

    if (script_values_.find(obj) == script_values_.end()) {
      ResolverScriptClass *resolver = GetScriptClass(obj);
      script_values_[obj] = engine_.newObject(resolver);
    }
    return script_values_[obj];
  }

  void SetScriptValueOfNativeObject(ScriptableInterface *obj,
                                    const QScriptValue& qval) {
    ASSERT(script_values_.find(obj) == script_values_.end());
    script_values_[obj] = qval;
  }

  QScriptEngine engine_;
  JSScriptContext *parent_;
  std::map<std::string, Slot*> class_constructors_;
  std::map<ScriptableInterface*, ResolverScriptClass*> script_classes_;
  std::map<ScriptableInterface*, QScriptValue> script_values_;
  Signal1<void, const char *> error_reporter_signal_;
  Signal2<bool, const char *, int> script_blocked_signal_;
  ResolverScriptClass *resolver_;
  QString file_name_;
  int line_number_;
};

static int count = 0;
class SlotCallerWrapper : public QObject {
 public:
  SlotCallerWrapper(ScriptableInterface *object, Slot *slot)
    : object_(object), slot_(slot) {
    count++;
    LOG("SlotCallerWrapper:%d", count);
  }
  ~SlotCallerWrapper() {
    count--;
    LOG("delete SlotCallerWrapper:%d", count);
  }

  ScriptableInterface *object_;
  Slot *slot_;
};

static QScriptValue SlotCaller(QScriptContext *context, QScriptEngine *engine) {
  QScriptValue callee = context->callee();
  SlotCallerWrapper *wrapper =
      static_cast<SlotCallerWrapper*>(callee.data().toQObject());
  ASSERT(wrapper);

  Variant *argv = NULL;
  bool ret = ConvertJSArgsToNative(context, wrapper->slot_, &argv);
  if (!ret) return engine->undefinedValue();

  ResultVariant res = wrapper->slot_->Call(wrapper->object_,
                                           wrapper->slot_->GetArgCount(), argv);
  if (argv) {
    // for (int i = 0; i < context->argumentCount(); i++)
    //   FreeNativeValue(argv[i]);
    delete [] argv;
  }
  QScriptValue exception;
  if (!CheckException(context, wrapper->object_, &exception))
    return exception;

  if (context->isCalledAsConstructor()) {
    JSScriptContext::Impl *impl = GetEngineContext(engine)->impl_;
    ScriptableInterface *scriptable =
        VariantValue<ScriptableInterface *>()(res.v());
    if (scriptable) {
      ResolverScriptClass *resolver = impl->GetScriptClass(scriptable);
      context->thisObject().setScriptClass(resolver);
      impl->SetScriptValueOfNativeObject(scriptable, context->thisObject());
    }
    return engine->undefinedValue();
  } else {
    // Update filename and line number
    JSScriptContext::Impl *impl = g_data[engine]->impl_;
    QScriptContextInfo info(context);
    impl->file_name_ = info.fileName();
    impl->line_number_ = info.lineNumber();

    QScriptValue val;
    ret = ConvertNativeToJS(engine, res.v(), &val);
    ASSERT(ret);
    return val;
  }
}

ResolverScriptClass::ResolverScriptClass(QScriptEngine *engine,
                                         ScriptableInterface *object,
                                         bool global)
    : QScriptClass(engine), object_(object), call_slot_(NULL), global_(global),
      on_reference_change_connection_(NULL) {
  ASSERT(object);
  object->Ref();
  on_reference_change_connection_ = object->ConnectOnReferenceChange(
      NewSlot(this, &ResolverScriptClass::OnRefChange));
  if (object->GetPropertyInfo("", NULL) ==
      ScriptableInterface::PROPERTY_METHOD) {
    ResultVariant p = object->GetProperty("");
    call_slot_ = VariantValue<Slot*>()(p.v());
  }
}

ResolverScriptClass::~ResolverScriptClass() {
  if (object_) {
    on_reference_change_connection_->Disconnect();
    object_->Unref();
  }
}

void ResolverScriptClass::OnRefChange(int ref_count, int change) {
  if (change == 0) {
    LOG("OnRefChange:%p, %p,%d", this, object_, object_->GetRefCount());
    on_reference_change_connection_->Disconnect();
    object_->Unref(true);
    JSScriptContext::Impl *impl = GetEngineContext(engine())->impl_;
    if (!global_) {
      impl->RemoveNativeObjectFromJSContext(object_);
    }
    object_ = NULL;
    // global object will be deleted in JSScriptContext destructor
    if (!global_) delete this;
  }
}

enum {
  PT_NAME,
  PT_INDEX,
  PT_GLOBAL
};

QScriptClass::QueryFlags ResolverScriptClass::queryProperty(
    const QScriptValue &object,
    const QScriptString &property_name,
    QueryFlags flags,
    uint *id) {
  if (!object_) return 0;

  QString name = property_name.toString();

  // Remove me when code is stable
  if (name.compare("trap") == 0)
    return HandlesReadAccess|HandlesWriteAccess;

  // if property_name is an index
  bool ok;
  name.toLong(&ok, 0);
  if (ok) {
    *id = PT_INDEX; // access by index
    return HandlesReadAccess|HandlesWriteAccess; // Accessed as array
  }

  std::string sname = name.toStdString();
  if (global_) {
    JSScriptContext::Impl *impl = g_data[engine()]->impl_;
    if (impl->class_constructors_.find(sname) !=
        impl->class_constructors_.end()) {
      *id = PT_GLOBAL; // access class constructors
      return HandlesReadAccess;
    }
  }

  *id = PT_NAME; // access by name
  ScriptableInterface::PropertyType pt =
      object_->GetPropertyInfo(sname.c_str(), NULL);
  if (!CheckException(engine()->currentContext(), object_, NULL))
    return 0;

  if (pt == ScriptableInterface::PROPERTY_NOT_EXIST)
    return 0;  // This property is not maintained by resolver
  if (pt == ScriptableInterface::PROPERTY_CONSTANT ||
      pt == ScriptableInterface::PROPERTY_METHOD)
    return HandlesReadAccess;
  return HandlesReadAccess|HandlesWriteAccess;
}

QScriptValue ResolverScriptClass::property(const QScriptValue & object,
                                           const QScriptString & name,
                                           uint id) {
  std::string sname = name.toString().toStdString();

  JSScriptContext::Impl *impl = g_data[engine()]->impl_;

  if (id == PT_GLOBAL) {
    Slot *slot = impl->class_constructors_[sname];
    QScriptValue value = engine()->newFunction(SlotCaller);
    QScriptValue data = engine()->newQObject(new SlotCallerWrapper(NULL, slot),
                                             QScriptEngine::ScriptOwnership);
    value.setData(data);
    return value;
  }

  ResultVariant res;
  if (id == PT_INDEX) {
    bool ok;
    long i = name.toString().toLong(&ok, 0);
    ASSERT(ok);
    res = object_->GetPropertyByIndex(i);
  } else {
    ASSERT(id == PT_NAME);
    res = object_->GetProperty(sname.c_str());
  }
  QScriptValue exception;
  if (!CheckException(engine()->currentContext(), object_, &exception))
    return exception;

  if (res.v().type() == Variant::TYPE_SLOT) {
    QScriptValue value = engine()->newFunction(SlotCaller);
    Slot *slot = VariantValue<Slot *>()(res.v());
    LOG("\tfun::%p", slot);
    QScriptValue data = engine()->newQObject(new SlotCallerWrapper(object_, slot),
                                             QScriptEngine::ScriptOwnership);
    value.setData(data);
    return value;
  } else {
    DLOG("\tothers:%s", res.v().Print().c_str());
    QScriptValue qval;
    if (!ConvertNativeToJS(engine(), res.v(), &qval))
      return engine()->currentContext()->throwError(
          "Failed to convert property to QScriptValue");
    return qval;
  }
}

void ResolverScriptClass::setProperty(QScriptValue &object,
                                      const QScriptString &name,
                                      uint id,
                                      const QScriptValue &value) {
  std::string sname = name.toString().toStdString();
  // Remove me when code is stable
  if (sname.compare("trap") == 0)
    return;

  DLOG("setProperty:%s", sname.c_str());
  Variant val;
  Variant proto;
  bool ok;
  long i = name.toString().toLong(&ok, 0);
  if (ok) {
    proto = object_->GetPropertyByIndex(i).v();
    ConvertJSToNative(engine(), proto, value, &val);
    object_->SetPropertyByIndex(i, val);
    DLOG("setPropertyByIndex:%s=%s", sname.c_str(), val.Print().c_str());
  } else {
    Variant proto;
    if (object_->GetPropertyInfo(sname.c_str(), &proto)
        == ScriptableInterface::PROPERTY_DYNAMIC) {
      ConvertJSToNativeVariant(engine(), value, &val);
    } else {
      ConvertJSToNative(engine(), proto, value, &val);
    }
    DLOG("setProperty:proto:%s", proto.Print().c_str());
    object_->SetProperty(sname.c_str(), val);
    DLOG("setProperty:%s=%s", sname.c_str(), val.Print().c_str());
  }
  CheckException(engine()->currentContext(), object_, NULL);
}

bool ResolverScriptClass::supportsExtension(Extension extension) const {
  return call_slot_ && extension == Callable;
}

QVariant ResolverScriptClass::extension(Extension extension,
                                        const QVariant &argument) {
  ASSERT(call_slot_ && extension == Callable);
  LOG("Object called as function");
  QScriptContext *context = qvariant_cast<QScriptContext*>(argument);

  Variant *argv = NULL;
  if (!ConvertJSArgsToNative(context, call_slot_, &argv))
    return QVariant();

  ResultVariant res = call_slot_->Call(object_, call_slot_->GetArgCount(), argv);
  if (!CheckException(context, object_, NULL))
    return QVariant();

  QScriptValue val;
  bool ret = ConvertNativeToJS(engine(), res.v(), &val);
  ASSERT(ret);
  return qVariantFromValue(val);
}

JSScriptContext::JSScriptContext() : impl_(new Impl(this)){
  g_data[&impl_->engine_] = this;
}

JSScriptContext::~JSScriptContext() {
  delete impl_;
}

void JSScriptContext::Destroy() {
  delete this;
}

QScriptEngine *JSScriptContext::engine() const {
  return &impl_->engine_;
}

void JSScriptContext::Execute(const char *script,
                              const char *filename,
                              int lineno) {
  ScopedLogContext log_context(this);
  DLOG("Execute: (%s, %d)", filename, lineno);

  std::string massaged_script = ggadget::js::MassageJScript(script, false,
                                                            filename, lineno);
  QScriptValue val = impl_->engine_.evaluate(
      QString::fromUtf8(massaged_script.c_str()), filename, lineno);
  if (impl_->engine_.hasUncaughtException()) {
    QStringList bt = impl_->engine_.uncaughtExceptionBacktrace();
    LOGE("Backtrace:");
    for (int i = 0; i < bt.size(); i++) {
      LOGE("\t%s", bt[i].toStdString().c_str());
    }
  }
}

Slot *JSScriptContext::Compile(const char *script,
                               const char *filename,
                               int lineno) {
  ScopedLogContext log_context(this);
  DLOG("Compile: (%s, %d)", filename, lineno);
  DLOG("\t%s", script);

  std::string massaged_script = ggadget::js::MassageJScript(script, false,
                                                            filename, lineno);
  return new JSFunctionSlot(NULL, &impl_->engine_, massaged_script.c_str(),
                            filename, lineno);
}

bool JSScriptContext::SetGlobalObject(ScriptableInterface *global_object) {
  return impl_->SetGlobalObject(global_object);
}

bool JSScriptContext::RegisterClass(const char *name, Slot *constructor) {
  ASSERT(constructor);
  ASSERT(constructor->GetReturnType() == Variant::TYPE_SCRIPTABLE);
  DLOG("RegisterClass: %s", name);
  impl_->class_constructors_[name] = constructor;
  return true;
}

bool JSScriptContext::AssignFromContext(ScriptableInterface *dest_object,
                                        const char *dest_object_expr,
                                        const char *dest_property,
                                        ScriptContextInterface *src_context,
                                        ScriptableInterface *src_object,
                                        const char *src_expr) {
  ASSERT(0);
  return false;
}

bool JSScriptContext::AssignFromNative(ScriptableInterface *object,
                                       const char *object_expr,
                                       const char *property,
                                       const Variant &value) {
  ScopedLogContext log_context(this);

  LOG("AssignFromNative: o:%s,p:%s,v:%s", object_expr, property,
      value.Print().c_str());
  QScriptValue obj;
  if (!object_expr || strcmp("", object_expr) == 0) {
    obj = impl_->engine_.globalObject();
  } else {
    obj = impl_->engine_.globalObject().property(object_expr);
    if (!obj.isValid()) return false;
  }
  QScriptValue qval;
  if (!ConvertNativeToJS(&impl_->engine_, value, &qval)) return false;
  obj.setProperty(property, qval);
  return true;
}

Variant JSScriptContext::Evaluate(ScriptableInterface *object,
                                  const char *expr) {
  DLOG("Evaluate: %s", expr);
  ASSERT(0);
  Variant result;
  return result;
}

Connection *JSScriptContext::ConnectScriptBlockedFeedback(
    ScriptBlockedFeedback *feedback) {
  return impl_->script_blocked_signal_.Connect(feedback);
}

void JSScriptContext::CollectGarbage() {
  impl_->engine_.collectGarbage();
}

void JSScriptContext::GetCurrentFileAndLine(std::string *fname, int *lineno) {
  *fname = impl_->file_name_.toUtf8().data();
  *lineno = impl_->line_number_;
}

QScriptValue JSScriptContext::GetScriptValueOfNativeObject(
    ScriptableInterface *obj) {
  return impl_->GetScriptValueOfNativeObject(obj);
}

ScriptableInterface *JSScriptContext::WrapJSObject(const QScriptValue &qval) {
  return impl_->WrapJSObject(qval);
}

ScriptableInterface *GetNativeObject(const QScriptValue &qval) {
  QScriptClass *c = qval.scriptClass();
  ScriptableInterface *obj = NULL;
  if (c != NULL) {
    ResolverScriptClass *mc = static_cast<ResolverScriptClass*>(c);
    obj = mc->object_;
  }
  return obj;
}

} // namespace qt
} // namespace ggadget
