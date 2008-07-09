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

#ifndef GGADGET_QT_JS_SCRIPT_CONTEXT_H__
#define GGADGET_QT_JS_SCRIPT_CONTEXT_H__

#include <QtScript/QScriptEngine>
#include <QtScript/QScriptClass>
#include <ggadget/scriptable_interface.h>
#include <ggadget/script_context_interface.h>
#include <ggadget/signals.h>

Q_DECLARE_METATYPE(QScriptContext*)
Q_DECLARE_METATYPE(QScriptValue)

namespace ggadget {
namespace qt {

class JSScriptRuntime;
class JSFunctionSlot;

class ResolverScriptClass : public QScriptClass {
 public:
  ResolverScriptClass(QScriptEngine *engine, ScriptableInterface *object);
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
  Connection *on_reference_change_connection_;
  void OnRefChange(int, int);
};

/**
 * @c ScriptContext implementation for QtScript engine.
 */
class JSScriptContext : public ScriptContextInterface {
 public:
  JSScriptContext();
  virtual ~JSScriptContext();

  QScriptEngine *engine() const;

  /** @see ScriptContextInterface::Destroy() */
  virtual void Destroy();
  /** @see ScriptContextInterface::Execute() */
  virtual void Execute(const char *script,
                       const char *filename,
                       int lineno);
  /** @see ScriptContextInterface::Compile() */
  virtual Slot *Compile(const char *script,
                        const char *filename,
                        int lineno);
  /** @see ScriptContextInterface::SetGlobalObject() */
  virtual bool SetGlobalObject(ScriptableInterface *global_object);
  /** @see ScriptContextInterface::RegisterClass() */
  virtual bool RegisterClass(const char *name, Slot *constructor);
  /** @see ScriptContextInterface::AssignFromContext() */
  virtual bool AssignFromContext(ScriptableInterface *dest_object,
                                 const char *dest_object_expr,
                                 const char *dest_property,
                                 ScriptContextInterface *src_context,
                                 ScriptableInterface *src_object,
                                 const char *src_expr);
  /** @see ScriptContextInterface::AssignFromContext() */
  virtual bool AssignFromNative(ScriptableInterface *object,
                                const char *object_expr,
                                const char *property,
                                const Variant &value);
  /** @see ScriptContextInterface::Evaluate() */
  virtual Variant Evaluate(ScriptableInterface *object, const char *expr);

  virtual Connection *ConnectScriptBlockedFeedback(
      ScriptBlockedFeedback *feedback);
  virtual void CollectGarbage();
  virtual void GetCurrentFileAndLine(std::string *filename, int *lineno);

  QScriptValue GetScriptValueOfNativeObject(ScriptableInterface *obj);
  class Impl;
  Impl *impl_;
 private:
  DISALLOW_EVIL_CONSTRUCTORS(JSScriptContext);
};

JSScriptContext *GetEngineContext(QScriptEngine *engine);

} // namespace qt
} // namespace ggadget

#endif  // GGADGET_QT_JS_SCRIPT_CONTEXT_H__
