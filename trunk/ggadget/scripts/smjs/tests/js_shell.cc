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

// This file was written using SpiderMonkey's js.c as a sample.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <jsapi.h>
#ifdef HAS_READLINE
#include <readline/readline.h>
#ifdef HAS_HISTORY
#include <readline/history.h>
#endif
#endif

// The exception value thrown by Assert function.
const int kAssertExceptionMagic = 135792468;

bool g_interactive = false;

enum QuitCode {
  QUIT_OK = 0,
  DONT_QUIT = 1,
  QUIT_ERROR = -1,
  QUIT_JSERROR = -2,
  QUIT_ASSERT = -3,
};
QuitCode g_quit_code = DONT_QUIT;

static JSBool GetLine(FILE *file, char *buffer, int size, const char *prompt) {
#ifdef HAS_READLINE
  if (g_interactive) {
    char *linep = readline(prompt);
    if (!linep)
      return JS_FALSE;
#ifdef HAS_HISTORY
    if (linep[0] != '\0')
      add_history(linep);
#endif
    strncpy(buffer, linep, size - 2);
    free(linep);
    buffer[size - 2] = '\0';
    strcat(buffer, "\n");
  } else {
    if (!fgets(buffer, size, file))
      return JS_FALSE;
  }
  return JS_TRUE;
#else // HAS_READLINE
  if (g_interactive) {
    printf("%s", prompt);
    fflush(stdout);
  }
  if (!fgets(buffer, size, file))
    return JS_FALSE;
  return JS_TRUE;
#endif // else HAS_READLINE
}

static const int kBufferSize = 65536;
static char g_buffer[kBufferSize];
static void Process(JSContext *cx, JSObject *obj, const char *filename) {
  FILE *file;
  if (!filename || strcmp(filename, "-") == 0) {
    g_interactive = true;
    file = stdin;
    filename = "(stdin)";
  } else {
    g_interactive = false;
    file = fopen(filename, "r");
    if (!file) {
      fprintf(stderr, "Can't open file: %s\n", filename);
      g_quit_code = QUIT_ERROR;
      return;
    }
  }

  int lineno = 1;
  bool eof = false;
  do {
    char *bufp = g_buffer;
    int remain_size = kBufferSize;
    *bufp = '\0';
    int startline = lineno;
    do {
      if (!GetLine(file, bufp, remain_size,
                   startline == lineno ? "js> " : "  > ")) {
        eof = true;
        break;
      }
      int line_len = strlen(bufp);
      bufp += line_len;
      remain_size -= line_len;
      lineno++;
    } while (!JS_BufferIsCompilableUnit(cx, obj, g_buffer, strlen(g_buffer)));

    JSScript *script = JS_CompileScript(cx, obj, g_buffer, strlen(g_buffer),
                                        filename, startline);
    if (script) {
      jsval result;
      if (JS_ExecuteScript(cx, obj, script, &result) &&
          result != JSVAL_VOID &&
          g_interactive) {
        JSString *str = JS_ValueToString(cx, result);
        if (str)
          puts(JS_GetStringBytes(str));
      }
      JS_DestroyScript(cx, script);
    }
    // printf("%s:%d: %s\n", filename, startline, g_buffer);
    JS_ClearPendingException(cx);
  } while (!eof && g_quit_code == DONT_QUIT);
}

static JSBool Print(JSContext *cx, JSObject *obj,
                    uintN argc, jsval *argv, jsval *rval) {
  for (uintN i = 0; i < argc; i++) {
    JSString *str = JS_ValueToString(cx, argv[i]);
    if (!str)
      return JS_FALSE;
    printf("%s ", JS_GetStringBytes(str));
  }
  putchar('\n');
  return JS_TRUE;
}

static JSBool Quit(JSContext *cx, JSObject *obj,
                   uintN argc, jsval *argv, jsval *rval) {
  int32 code = QUIT_OK;
  if (argc >= 1)
    JS_ValueToInt32(cx, argv[0], &code);
  g_quit_code = static_cast<QuitCode>(code);
  return JS_FALSE;
}

static JSBool GC(JSContext *cx, JSObject *obj,
                 uintN argc, jsval *argv, jsval *rval) {
  JS_GC(cx);
  return JS_TRUE;
}

const char kAssertFailurePrefix[] = "Failure\n";

// This function is used in JavaScript unittests.
// It checks the result of a predicate function that returns a blank string
// on success or otherwise a string containing the assertion failure message.
// Usage: ASSERT(EQ(a, b), "Test a and b");
static JSBool Assert(JSContext *cx, JSObject *obj,
                     uintN argc, jsval *argv, jsval *rval) {
  JSString *predicate_result = JS_ValueToString(cx, argv[0]);
  if (!predicate_result)
    return JS_FALSE;

  if (JS_GetStringLength(predicate_result) > 0) {
    jsval message_jsval = argc > 1 ? argv[1] : JS_GetEmptyStringValue(cx);
    JSString *message = JS_ValueToString(cx, message_jsval);
    if (!message)
      return JS_FALSE;

    if (JS_GetStringLength(message) > 0)
      JS_ReportError(cx, "%s%s\n%s", kAssertFailurePrefix,
                     JS_GetStringBytes(predicate_result),
                     JS_GetStringBytes(message));
    else
      JS_ReportError(cx, "%s%s", kAssertFailurePrefix,
                     JS_GetStringBytes(predicate_result));

    // Let the JavaScript test framework know the failure.
    // The exception value is null to tell the catcher not to print it again.
    JS_SetPendingException(cx, INT_TO_JSVAL(kAssertExceptionMagic));
    return JS_FALSE;
  }
  return JS_TRUE;
}

JSBool g_verbose = JS_TRUE;

static void ErrorReporter(JSContext *cx, const char *message,
                          JSErrorReport *report) {
  if (!g_interactive &&
      // If the error is an assertion failure, don't quit now because
      // we have thrown an exception to be handled by the JavaScript code.
      strncmp(message, kAssertFailurePrefix,
              sizeof(kAssertFailurePrefix) - 1) != 0) {
    if (JSREPORT_IS_EXCEPTION(report->flags) ||
        JSREPORT_IS_STRICT(report->flags))
      // Unhandled exception or strict errors, quit.
      g_quit_code = QUIT_JSERROR;
    else
      // Convert this error into an exception, to make the tester be able to
      // catch it.
      JS_SetPendingException(cx,
          STRING_TO_JSVAL(JS_NewString(cx, strdup(message), strlen(message))));
  }

  fflush(stdout);
  if (g_verbose)
    fprintf(stderr, "%s:%d: %s\n", report->filename, report->lineno, message);
  fflush(stderr);
}

static JSBool SetVerbose(JSContext *cx, JSObject *obj,
                         uintN argc, jsval *argv, jsval *rval) {
  return JS_ValueToBoolean(cx, argv[0], &g_verbose);
}

static void TempErrorReporter(JSContext *cx, const char *message,
                              JSErrorReport *report) {
  printf("%s:%d\n", report->filename, report->lineno);
}

static JSBool ShowFileAndLine(JSContext *cx, JSObject *obj,
                              uintN argc, jsval *argv, jsval *rval) {
  JSErrorReporter old_reporter = JS_SetErrorReporter(cx, TempErrorReporter);
  JS_ReportError(cx, "");
  JS_SetErrorReporter(cx, old_reporter);
  return JS_TRUE;
}

static JSFunctionSpec global_functions[] = {
  { "print", Print, 0 },
  { "quit", Quit, 0 },
  { "gc", GC, 0 },
  { "setVerbose", SetVerbose, 1 },
  { "showFileAndLine", ShowFileAndLine, 0 },
  { "ASSERT", Assert, 1 },
  { 0 }
};

int main(int argc, char *argv[]) {
  JSRuntime *rt = JS_NewRuntime(64L * 1024L * 1024L);
  if (!rt)
    return QUIT_ERROR;

  JSContext *cx = JS_NewContext(rt, 8192);
  if (!cx)
    return QUIT_ERROR;

  JS_SetErrorReporter(cx, ErrorReporter);

  JSClass global_class = {
      "global", 0,
      JS_PropertyStub,  JS_PropertyStub, JS_PropertyStub,  JS_PropertyStub,
      JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub,   JS_FinalizeStub
  };
  JSObject *global = JS_NewObject(cx, &global_class, NULL, NULL);
  if (!global)
    return QUIT_ERROR;
  if (!JS_InitStandardClasses(cx, global))
    return QUIT_ERROR;
  if (!JS_DefineFunctions(cx, global, global_functions))
    return QUIT_ERROR;

#ifdef INIT_CUSTOM_OBJECTS
  JSBool INIT_CUSTOM_OBJECTS(JSContext *cx, JSObject *obj);
  if (!INIT_CUSTOM_OBJECTS(cx, global))
    return QUIT_ERROR;
#endif

  if (argc > 1) {
    for (int i = 1; i < argc; i++) {
      Process(cx, global, argv[i]);
      if (g_quit_code != DONT_QUIT)
        break;
    }
  } else {
    Process(cx, global, NULL);
  }

  JS_DestroyContext(cx);
  JS_DestroyRuntime(rt);
  JS_ShutDown();

  if (g_quit_code == DONT_QUIT)
    g_quit_code = QUIT_OK;
  return g_quit_code;
}
