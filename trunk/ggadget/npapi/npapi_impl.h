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

#ifndef GGADGET_NPAPI_NPAPI_IMPL_H__
#define GGADGET_NPAPI_NPAPI_IMPL_H__

#include <string>

#include <third_party/npapi/npupp.h>
#include <third_party/npapi/npapi.h>
#include <third_party/npapi/npruntime.h>

namespace ggadget {
namespace npapi {

struct _NPIdentifier {
  enum IdType{
    TYPE_INT,
    TYPE_STRING,
  };

  _NPIdentifier(IdType type, int32_t intid)
      : type_(type), intid_(intid) { }
  _NPIdentifier(IdType type, const NPUTF8 *name)
      : type_(type), name_(name) { }

  IdType type_;
  int32_t intid_;
  std::string name_;
};

typedef _NPIdentifier *NPIdentifier;

/**
 * This class defines the browser-side NPAPIs and npruntime extension APIs.
 * The implementation is in npapi_plugin.cc.
 */
class NPAPIImpl {
 public:
  static void InitContainerFuncs(NPNetscapeFuncs *container_funcs);

  /*==============================================================
   *               Native Browser-side NPAPIs
   *============================================================*/

  static NPError NPN_GetURLNotify(NPP instance, const char* url,
                                  const char* target, void* notifyData);

  static NPError NPN_GetURL(NPP instance, const char* url, const char* target);

  static NPError NPN_PostURL(NPP instance, const char* url, const char* target,
                             uint32 len, const char* buf, NPBool file);

  static NPError NPN_PostURLNotify(NPP instance, const char* url,
                                   const char* target, uint32 len,
                                   const char* buf, NPBool file,
                                   void* notifyData);

  static NPError NPN_RequestRead(NPStream* stream, NPByteRange* rangeList);
  static NPError NPN_NewStream(NPP instance, NPMIMEType type,
                               const char* target, NPStream** stream);
  static int32   NPN_Write(NPP instance, NPStream* stream,
                           int32 len, void* buffer);
  static NPError NPN_DestroyStream(NPP instance,
                                   NPStream* stream, NPReason reason);
  static void    NPN_Status(NPP instance, const char* message);
  static const char* NPN_UserAgent(NPP instance);
  static void* NPN_MemAlloc(uint32 size);
  static void NPN_MemFree(void* ptr);
  static uint32 NPN_MemFlush(uint32 size);
  static void    NPN_ReloadPlugins(NPBool reloadPages);
  static JRIEnv* NPN_GetJavaEnv(void);
  static jref NPN_GetJavaPeer(NPP instance);
  static NPError NPN_GetValue(NPP instance, NPNVariable variable, void *value);
  static NPError NPN_SetValue(NPP instance, NPPVariable variable, void *value);
  static void NPN_InvalidateRect(NPP instance, NPRect *invalidRect);
  static void NPN_InvalidateRegion(NPP instance, NPRegion invalidRegion);
  static void NPN_ForceRedraw(NPP instance);
  static void NPN_PushPopupsEnabledState(NPP instance, NPBool enabled);
  static void NPN_PopPopupsEnabledState(NPP instance);
  static void NPN_PluginThreadAsyncCall(NPP instance,
                                        void (*func) (void *), void *userData);

  /*===============================================================
   *                      npruntime APIs.
   *=============================================================*/

  static void NPN_ReleaseVariantValue(NPVariant *variant);
  static NPIdentifier NPN_GetStringIdentifier(const NPUTF8 *name);
  static void NPN_GetStringIdentifiers(const NPUTF8 **names, int32_t nameCount,
                                       NPIdentifier *identifiers);
  static NPIdentifier NPN_GetIntIdentifier(int32_t intid);
  static bool NPN_IdentifierIsString(NPIdentifier identifier);
  static NPUTF8 *NPN_UTF8FromIdentifier(NPIdentifier identifier);
  static int32_t NPN_IntFromIdentifier(NPIdentifier identifier);

  static NPObject *NPN_CreateObject(NPP npp, NPClass *aClass);
  static NPObject *NPN_RetainObject(NPObject *npobj);
  static void NPN_ReleaseObject(NPObject *npobj);
  static bool NPN_Invoke(NPP npp, NPObject *npobj, NPIdentifier methodName,
                         const NPVariant *args, uint32_t argCount,
                         NPVariant *result);
  static bool NPN_InvokeDefault(NPP npp, NPObject *npobj, const NPVariant *args,
                                uint32_t argCount, NPVariant *result);
  static bool NPN_Evaluate(NPP npp, NPObject *npobj, NPString *script,
                           NPVariant *result);
  static bool NPN_GetProperty(NPP npp, NPObject *npobj,
                              NPIdentifier propertyName, NPVariant *result);
  static bool NPN_SetProperty(NPP npp, NPObject *npobj,
                              NPIdentifier propertyName,
                              const NPVariant *value);
  static bool NPN_RemoveProperty(NPP npp, NPObject *npobj,
                                 NPIdentifier propertyName);
  static bool NPN_HasProperty(NPP npp, NPObject *npobj,
                              NPIdentifier propertyName);
  static bool NPN_HasMethod(NPP npp, NPObject *npobj, NPIdentifier methodName);

  static void NPN_SetException(NPObject *npobj, const NPUTF8 *message);
  static bool NPN_Enumerate(NPP npp, NPObject *npobj, NPIdentifier **identifier,
                            uint32_t *count);
  static bool NPN_Construct(NPP npp, NPObject *npobj, const NPVariant *args,
                            uint32_t argCount, NPVariant *result);
};

} // namespace ggadget
} // namespace npapi

#endif // GGADGET_NPAPI_NPAPI_IMPL_H__
