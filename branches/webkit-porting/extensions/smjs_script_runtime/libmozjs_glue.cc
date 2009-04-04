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

// This file is generated by gen_libmozjs_glue.sh automatically.
// Don't edit it manually.
// Don't forget to regenerate this file if more/less libmozjs functions are
// used.
#ifdef XPCOM_GLUE
#include "libmozjs_glue.h"

#include <dlfcn.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <ggadget/logger.h>
#include <ggadget/system_utils.h>
#include <ggadget/string_utils.h>

namespace ggadget {
namespace libmozjs {

// Undefine api macros so that we can declare the real glue apis."
#undef JS_AddNamedRootRT
#undef JS_AddRoot
#undef JS_BufferIsCompilableUnit
#undef JS_CallFunctionName
#undef JS_CallFunctionValue
#undef JS_ClearPendingException
#undef JS_CompileFunction
#undef JS_CompileUCFunction
#undef JS_CompileUCScript
#undef JS_ConvertStub
#undef JS_DHashTableEnumerate
#undef JS_DefineFunction
#undef JS_DefineFunctions
#undef JS_DefineProperty
#undef JS_DefineUCFunction
#undef JS_DefineUCProperty
#undef JS_DeleteProperty
#undef JS_DeleteUCProperty2
#undef JS_DestroyContext
#undef JS_DestroyIdArray
#undef JS_Finish
#undef JS_DestroyScript
#undef JS_EnterLocalRootScope
#undef JS_Enumerate
#undef JS_EnumerateStub
#undef JS_EvaluateScript
#undef JS_EvaluateUCScript
#undef JS_ExecuteScript
#undef JS_GC
#undef JS_GetArrayLength
#undef JS_GetContextPrivate
#undef JS_GetElement
#undef JS_GetFunctionName
#undef JS_GetFunctionObject
#undef JS_GetGlobalObject
#undef JS_GetOptions
#undef JS_GetPendingException
#undef JS_GetPrivate
#undef JS_GetProperty
#undef JS_GetReservedSlot
#undef JS_GetRuntime
#undef JS_GetStringBytes
#undef JS_GetStringChars
#undef JS_GetStringLength
#undef JS_GetUCProperty
#undef JS_IdToValue
#undef JS_InitClass
#undef JS_InitStandardClasses
#undef JS_IsArrayObject
#undef JS_IsExceptionPending
#undef JS_LeaveLocalRootScope
#undef JS_MarkGCThing
#undef JS_NewArrayObject
#undef JS_NewContext
#undef JS_NewDouble
#undef JS_NewObject
#undef JS_Init
#undef JS_NewString
#undef JS_NewStringCopyN
#undef JS_NewStringCopyZ
#undef JS_NewUCString
#undef JS_NewUCStringCopyN
#undef JS_NewUCStringCopyZ
#undef JS_PropertyStub
#undef JS_RemoveRoot
#undef JS_RemoveRootRT
#undef JS_ReportError
#undef JS_ReportErrorNumber
#undef JS_ReportPendingException
#undef JS_ReportWarning
#undef JS_ResolveStub
#undef JS_SetContextPrivate
#undef JS_SetElement
#undef JS_SetErrorReporter
#undef JS_SetGCParameter
#undef JS_SetGlobalObject
#undef JS_SetLocaleCallbacks
#undef JS_SetOperationCallback
#undef JS_SetOptions
#undef JS_SetPendingException
#undef JS_SetPrivate
#undef JS_SetProperty
#undef JS_SetReservedSlot
#undef JS_SetRuntimePrivate
#undef JS_SetUCProperty
#undef JS_TypeOfValue
#undef JS_ValueToBoolean
#undef JS_ValueToECMAInt32
#undef JS_ValueToFunction
#undef JS_ValueToId
#undef JS_ValueToInt32
#undef JS_ValueToNumber
#undef JS_ValueToString
#undef JS_malloc
#undef JS_realloc
#undef JS_GetClass

// Define real function pointers.
#define MOZJS_FUNC(fname) \
  static void fname##NotFoundHandler() { \
    LOGE("libmozjs symbol %s is missing.", #fname); \
    abort(); \
  } \
  fname##Type fname = { (fname##FuncType)fname##NotFoundHandler };

MOZJS_FUNCTIONS

#undef MOZJS_FUNC

#define MOZJS_FUNC(fname) { #fname, &fname.ptr },

static const nsDynamicFunctionLoad kMOZJSSymbols[] = {
  MOZJS_FUNCTIONS
  { nsnull, nsnull }
};

#undef MOZJS_FUNC

JSBool JS_ConvertStubProxy(JSContext *cx, JSObject *obj, JSType type, jsval *vp) {
  return JS_ConvertStub.func(cx,obj,type,vp);
}

JSBool JS_EnumerateStubProxy(JSContext *cx, JSObject *obj) {
  return JS_EnumerateStub.func(cx,obj);
}

JSBool JS_PropertyStubProxy(JSContext *cx, JSObject *obj, jsval id, jsval *vp) {
  return JS_PropertyStub.func(cx,obj,id,vp);
}

JSBool JS_ResolveStubProxy(JSContext *cx, JSObject *obj, jsval id) {
  return JS_ResolveStub.func(cx,obj,id);
}

#if defined(SUNOS4) || defined(NEXTSTEP) || \
    (defined(OPENBSD) || defined(NETBSD)) && !defined(__ELF__)
#define LEADING_UNDERSCORE "_"
#else
#define LEADING_UNDERSCORE
#endif

#ifndef GGL_MOZJS_LIBNAME
#define GGL_MOZJS_LIBNAME "libmozjs.so"
#endif

static void *g_libmozjs_handler = NULL;

static bool LoadLibmozjs(const char *xpcom_file) {
  std::string dir, filename;
  if (IsAbsolutePath(xpcom_file) &&
      SplitFilePath(xpcom_file, &dir, &filename)) {
    filename = BuildFilePath(dir.c_str(), GGL_MOZJS_LIBNAME, NULL);
    g_libmozjs_handler = dlopen(filename.c_str(), RTLD_GLOBAL | RTLD_LAZY);
  }
  return g_libmozjs_handler != NULL;
}

bool LibmozjsGlueStartup() {
  nsresult rv = NS_OK;
  char xpcom_file[4096];

  static const GREVersionRange kGREVersion = {
    "1.9a", PR_TRUE,
    "1.9.1", PR_TRUE
  };

  rv = GRE_GetGREPathWithProperties(&kGREVersion, 1, nsnull, 0,
                                    xpcom_file, 4096);
  if (NS_FAILED(rv)) {
    LOGE("Failed to find proper Gecko Runtime Environment!");
    return false;
  }

  DLOG("XPCOM Location: %s", xpcom_file);

  if (!LoadLibmozjs(xpcom_file)) {
    LOGE("Failed to load " GGL_MOZJS_LIBNAME "!");
    return false;
  }

  // Load symbols.
  const nsDynamicFunctionLoad *syms = kMOZJSSymbols;
  while(syms->functionName) {
    std::string name =
      StringPrintf(LEADING_UNDERSCORE "%s", syms->functionName);
    NSFuncPtr old = *syms->function;
    *syms->function = (NSFuncPtr) dlsym(g_libmozjs_handler, name.c_str());
    if (*syms->function == old || *syms->function == NULL) {
      LOGE("Missing symbol in " GGL_MOZJS_LIBNAME ": %s", syms->functionName);
      *syms->function = old;
      // Don't fail here, because the missing method might never be called.
    }
    ++syms;
  }

  return rv == NS_OK;
}

void LibmozjsGlueShutdown() {
  if (g_libmozjs_handler) {
    dlclose(g_libmozjs_handler);
    g_libmozjs_handler = NULL;
  }

  const nsDynamicFunctionLoad *syms = kMOZJSSymbols;
  while(syms->functionName) {
    *syms->function = NULL;
    ++syms;
  }
}

nsresult LibmozjsGlueStartupWithXPCOM() {
  return XPCOMGlueLoadXULFunctions(kMOZJSSymbols);
}

} // namespace libmozjs
} // namespace ggadget
#endif // XPCOM_GLUE