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

#include <QtScript/QScriptValue>
#include <QtCore/QStringList>
#include <ggadget/scoped_ptr.h>
#include <ggadget/string_utils.h>
#include "js_function_slot.h"
#include "js_script_context.h"
#include "converter.h"

namespace ggadget {
namespace qt {

JSFunctionSlot::JSFunctionSlot(const Slot* prototype,
                               QScriptEngine *engine,
                               const char *script,
                               const char *file_name,
                               int lineno)
    : prototype_(prototype),
      engine_(engine),
      code_(true),
      script_(script),
      file_name_(file_name),
      line_no_(lineno) {
}

JSFunctionSlot::JSFunctionSlot(const Slot* prototype,
                               QScriptEngine *engine, QScriptValue function)
    : prototype_(prototype),
      engine_(engine),
      code_(false),
      function_(function) {
}

JSFunctionSlot::~JSFunctionSlot() {
  DLOG("JSFunctionSlot deleted");
}

ResultVariant JSFunctionSlot::Call(ScriptableInterface *object,
                                   int argc, const Variant argv[]) const {
  QScriptValue qval;
  if (code_) {
    DLOG("JSFunctionSlot::Call: %s", script_.c_str());
    qval = engine_->evaluate(script_.c_str(), file_name_.c_str(), line_no_);
  } else {
    DLOG("JSFunctionSlot::Call function");
    QScriptValue fun(function_);
    QScriptValueList args;
    ConvertNativeArgvToJS(engine_, argc, argv, &args);
    qval = fun.call(QScriptValue(), args);
  }
  if (engine_->hasUncaughtException()) {
    QStringList bt = engine_->uncaughtExceptionBacktrace();
    LOG("Backtrace:");
    for (int i = 0; i < bt.size(); i++) {
      LOG("\t%s", bt[i].toStdString().c_str());
    }
  }

  Variant return_value(GetReturnType());
  ConvertJSToNative(engine_, return_value, qval, &return_value);
  DLOG("JSFunctionSlot:end");
  return ResultVariant(return_value);
}

} // namespace qt
} // namespace ggadget
