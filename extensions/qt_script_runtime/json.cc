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

#include <vector>
#include <QtScript/QScriptValueIterator>
#include <ggadget/common.h>
#include "json.h"

namespace ggadget {
namespace qt {

// Use Microsoft's method to encode/decode Date object in JSON.
// See http://msdn2.microsoft.com/en-us/library/bb299886.aspx.
static const char kDatePrefix[] = "\"\\/Date(";
static const char kDatePrefixReplace[] = "new Date(";
static const char kDatePostfix[] = ")\\/\"";
static const char kDatePostfixReplace[] = ")";

static void AppendJSON(QScriptEngine *engine, QScriptValue qval,
                       std::string *json, std::vector<QScriptValue> *stack);

static void AppendArrayToJSON(QScriptEngine *engine, QScriptValue qval,
                              std::string *json, std::vector<QScriptValue> *stack) {
  (*json) += '[';
  int length = qval.property("length").toInt32();
  for (int i = 0; i < length; i++) {
    QScriptValue v = qval.property(i);
    AppendJSON(engine, v, json, stack);
    if (i != length - 1)
      (*json) += ',';
  }
  (*json) += ']';
}

static void AppendStringToJSON(QScriptEngine *engine, const QString &str,
                               std::string *json) {
  *json += '"';
  std::string s = str.toStdString().c_str();
  const char *chars = s.c_str();
  if (chars) {
    for (const char *p = chars; *p; p++) {
      switch (*p) {
        // The following special chars are not so complete, but also works.
        case '"': *json += "\\\""; break;
        case '\\': *json += "\\\\"; break;
        case '\n': *json += "\\n"; break;
        case '\r': *json += "\\r"; break;
        default:
          if (*p >= 0x7f || *p < 0x20) {
            char buf[10];
            snprintf(buf, sizeof(buf), "\\u%04X", *p);
            *json += buf;
          } else {
            *json += *p;
          }
          break;
      }
    }
  }
  *json += '"';
}

static void AppendObjectToJSON(QScriptEngine *engine, QScriptValue qval,
                               std::string *json, std::vector<QScriptValue> *stack) {
  (*json) += '{';
  QScriptValueIterator it(qval);

  while (it.hasNext()) {
    it.next();
    // Don't output methods.
    if (!it.value().isFunction()) {
      AppendStringToJSON(engine, it.name(), json);
      (*json) += ':';
      AppendJSON(engine, it.value(), json, stack);
      (*json) += ',';
    }
  }
  if (json->length() > 0 && *(json->end() - 1) == ',')
    json->erase(json->end() - 1);
  (*json) += '}';
}

static void AppendNumberToJSON(QScriptEngine *engine, QScriptValue qval,
                               std::string *json) {
  *json += qval.toString().toStdString();
}

static void AppendDateToJSON(QScriptEngine *engine, QScriptValue qval,
                               std::string *json) {
  *json += kDatePrefix;
  uint64_t v = static_cast<uint64_t>(qval.toNumber());
  char buf[30];
  snprintf(buf, 30, "%llu", v);
  *json += buf;
  *json += kDatePostfix;
}

static void AppendJSON(QScriptEngine *engine, QScriptValue qval,
                       std::string *json, std::vector<QScriptValue> *stack) {
  if (qval.isObject()) {
    for (size_t i = 0; i < stack->size(); i++) {
      if ((*stack)[i].strictlyEquals(qval)) {
        (*json) += "null";
        return;
      }
    }
    stack->push_back(qval);
    AppendObjectToJSON(engine, qval, json, stack);
    stack->pop_back();
  } else if (qval.isDate()) {
    AppendDateToJSON(engine, qval, json);
  } else if (qval.isString()) {
    AppendStringToJSON(engine, qval.toString(), json);
  } else if (qval.isNumber()) {
    AppendNumberToJSON(engine, qval, json);
  } else if (qval.isBoolean()) {
    (*json) += qval.toBoolean() ? "true" : "false";
  } else if (qval.isArray()) {
    AppendArrayToJSON(engine, qval, json, stack);
  } else {
    (*json) += "null";
  }
}

bool JSONEncode(QScriptEngine *engine, const QScriptValue &qval,
                std::string *json) {
  json->clear();
  std::vector<QScriptValue> stack;
  AppendJSON(engine, qval, json, &stack);
  return true;
}

bool JSONDecode(QScriptEngine* engine, const char *json, QScriptValue *qval) {
  *qval = engine->evaluate(json);
  return true;
}
} // namespace qt
} // namespace ggadget
