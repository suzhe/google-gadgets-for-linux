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

#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <vector>
#include <algorithm>
#include <gtk/gtk.h>

#define MOZILLA_CLIENT
#include <mozilla-config.h>

#ifdef XPCOM_GLUE
#include <nsXPCOMGlue.h>
#include <gtkmozembed_glue.cpp>
#include "../smjs_script_runtime/libmozjs_glue.h"
#endif

#include <gtkmozembed.h>
#include <gtkmozembed_internal.h>
#include <jsapi.h>
#include <jsconfig.h>

#include <nsComponentManagerUtils.h>
#include <nsCOMPtr.h>
#ifdef XPCOM_GLUE
#include <nsCRTGlue.h>
#else
#include <nsCRT.h>
#endif
#include <nsEvent.h>
#include <nsICategoryManager.h>
#include <nsIComponentRegistrar.h>
#include <nsIContentPolicy.h>
#include <nsIDOMAbstractView.h>
#include <nsIDOMDocument.h>
#include <nsIDOMDocumentView.h>
#include <nsIDOMNode.h>
#include <nsIDOMWindow.h>
#include <nsIGenericFactory.h>
#include <nsIInterfaceRequestor.h>
#include <nsIInterfaceRequestorUtils.h>
#include <nsIScriptGlobalObject.h>
#include <nsIScriptNameSpaceManager.h>
#include <nsIURI.h>
#include <nsIWebProgress.h>
#include <nsIXPConnect.h>
#include <nsIXPCScriptable.h>
#include <nsServiceManagerUtils.h>
#include <nsStringAPI.h>
#include <nsXPCOMCID.h>

#include <ggadget/common.h>
#include <ggadget/digest_utils.h>

#include "../smjs_script_runtime/json.h"
#include "browser_child.h"

using ggadget::smjs::JSONEncode;
using ggadget::smjs::JSONDecode;
using ggadget::gtkmoz::kEndOfMessage;
using ggadget::gtkmoz::kEndOfMessageFull;
using ggadget::gtkmoz::kNewBrowserCommand;
using ggadget::gtkmoz::kSetContentCommand;
using ggadget::gtkmoz::kOpenURLCommand;
using ggadget::gtkmoz::kCloseBrowserCommand;
using ggadget::gtkmoz::kQuitCommand;
using ggadget::gtkmoz::kGetPropertyFeedback;
using ggadget::gtkmoz::kSetPropertyFeedback;
using ggadget::gtkmoz::kCallbackFeedback;
using ggadget::gtkmoz::kOpenURLFeedback;
using ggadget::gtkmoz::kPingFeedback;
using ggadget::gtkmoz::kPingAck;
using ggadget::gtkmoz::kPingInterval;

#if JS_VERSION < 180
extern "C" {
  struct JSTracer;
}
#endif

// Default down and ret fds are standard input and up fd is standard output.
// The default values are useful when browser child is tested independently.
static int g_down_fd = 0, g_up_fd = 1, g_ret_fd = 0;
static std::vector<GtkMozEmbed *> g_embeds;

// The singleton GtkMozEmbed instance for temporary use when a new window
// is requested. Though we don't actually allow new window, we still need
// this widget to allow us get the url of the window and open it in the
// browser.
static GtkMozEmbed *g_embed_for_new_window = NULL;
// The parent window of the above widget.
static GtkWidget *g_popup_for_new_window = NULL;
// The GtkMozEmbed instance which just fired the new window request.
static GtkMozEmbed *g_main_embed_for_new_window = NULL;

static const size_t kMaxBrowserId = 256;

#define EXTOBJ_CLASSNAME  "ExternalObject"
#define EXTOBJ_PROPERTY_NAME "external"
#define EXTOBJ_CONTRACTID "@google.com/ggl/extobj;1"
#define EXTOBJ_CID { \
    0x224fb7b5, 0x6db0, 0x48db, \
    { 0xb8, 0x1e, 0x85, 0x15, 0xe7, 0x9f, 0x00, 0x30 } \
}

#define CONTENT_POLICY_CLASSNAME "ContentPolicy"
#define CONTENT_POLICY_CONTRACTID "@google.com/ggl/content-policy;1"
#define CONTENT_POLICY_CID { \
    0x74d0deec, 0xb36b, 0x4b03, \
    { 0xb0, 0x09, 0x36, 0xe3, 0x07, 0x68, 0xc8, 0x2c } \
}

static const char kDataURLPrefix[] = "data:";

static const nsIID kIScriptGlobalObjectIID = NS_ISCRIPTGLOBALOBJECT_IID;

static int FindBrowserIdByJSContext(JSContext *cx) {
  JSObject *js_global = JS_GetGlobalObject(cx);
  if (!js_global) {
    fprintf(stderr, "browser_child: No global object\n");
    return -1;
  }

  JSClass *cls = JS_GET_CLASS(cx, js_global);
  if (!cls || ((~cls->flags) & (JSCLASS_HAS_PRIVATE |
                                JSCLASS_PRIVATE_IS_NSISUPPORTS))) {
    fprintf(stderr, "browser_child: Global object is not a nsISupports\n");
    return -1;
  }
  nsIXPConnectWrappedNative *global_wrapper =
    reinterpret_cast<nsIXPConnectWrappedNative *>(JS_GetPrivate(cx, js_global));
  nsISupports *global = global_wrapper->Native();

  nsresult rv;
  for (std::vector<GtkMozEmbed *>::const_iterator it = g_embeds.begin();
       it != g_embeds.end(); ++it) {
    if (*it) {
      nsCOMPtr<nsIWebBrowser> browser;
      gtk_moz_embed_get_nsIWebBrowser(*it, getter_AddRefs(browser));
      nsCOMPtr<nsIInterfaceRequestor> req(do_QueryInterface(browser, &rv));
      NS_ENSURE_SUCCESS(rv, -1);
      nsISupports *global1 = NULL;
      void *temp = global1;
      rv = req->GetInterface(kIScriptGlobalObjectIID, &temp);
      global1 = static_cast<nsISupports *>(temp);
      NS_ENSURE_SUCCESS(rv, -1);
      global1->Release();
      if (global1 == global)
        return static_cast<int>(it - g_embeds.begin());
    }
  }
  fprintf(stderr, "browser_child: Can't find GtkMozEmbed from JS context\n");
  return -1;
}

static std::string SendFeedbackBuffer(const std::string &buffer) {
  write(g_up_fd, buffer.c_str(), buffer.size());

  std::string reply;
  char ch;
  while (read(g_ret_fd, &ch, 1) == 1 && ch != '\n')
    reply += ch;
  return reply;
}

static std::string SendFeedbackV(const char *type, int browser_id, va_list ap) {
  std::string buffer(type);
  char browser_id_buf[20];
  snprintf(browser_id_buf, sizeof(browser_id_buf), "\n%d", browser_id);
  buffer += browser_id_buf;

  const char *param;
  while ((param = va_arg(ap, const char *)) != NULL) {
    buffer += '\n';
    buffer += param;
  }
  buffer += kEndOfMessageFull;
  return SendFeedbackBuffer(buffer);
}

static std::string SendFeedbackWithBrowserId(const char *type, int browser_id,
                                             ...) {
  va_list ap;
  va_start(ap, browser_id);
  std::string result = SendFeedbackV(type, browser_id, ap);
  va_end(ap);
  return result;
}

// Send a feedback with parameters to the controller through the up channel,
// and return the reply (got in the return value channel).
static std::string SendFeedback(const char *type, JSContext *cx, ...) {
  int browser_id = FindBrowserIdByJSContext(cx);
  if (browser_id == -1)
    return "";

  va_list ap;
  va_start(ap, cx);
  std::string result = SendFeedbackV(type, browser_id, ap);
  va_end(ap);
  return result;
}

static JSBool InvokeFunction(JSContext *cx, JSObject *obj, uintN argc,
                             jsval *argv, jsval *rval) {
  int browser_id = FindBrowserIdByJSContext(cx);
  if (browser_id == -1)
    return JS_FALSE;

  std::string buffer(kCallbackFeedback);
  char browser_id_buf[20];
  snprintf(browser_id_buf, sizeof(browser_id_buf), "\n%d\n", browser_id);
  buffer += browser_id_buf;

  // According to JS stack structure, argv[-2] is the current function object.
  jsval func_val = argv[-2];
  buffer += JS_GetFunctionName(JS_ValueToFunction(cx, func_val));
  for (uintN i = 0; i < argc; i++) {
    buffer += '\n';
    std::string param;
    NS_ENSURE_TRUE(JSONEncode(cx, argv[i], &param), JS_FALSE);
    buffer += param;
  }
  buffer += kEndOfMessageFull;
  std::string result = SendFeedbackBuffer(buffer);
  NS_ENSURE_TRUE(JSONDecode(cx, result.c_str(), rval), JS_FALSE);
  return JS_TRUE;
}

class ExternalObject : public nsIXPCScriptable {
 public:
  NS_DECL_ISUPPORTS

  NS_IMETHOD GetClassName(char **class_name) {
    NS_ENSURE_ARG_POINTER(class_name);
#ifdef XPCOM_GLUE
    *class_name = NS_strdup(EXTOBJ_CLASSNAME);
#else
    *class_name = nsCRT::strdup(EXTOBJ_CLASSNAME);
#endif
    return NS_OK;
  }

  NS_IMETHOD GetScriptableFlags(PRUint32 *scriptable_flags) {
    NS_ENSURE_ARG_POINTER(scriptable_flags);
    *scriptable_flags = WANT_GETPROPERTY | WANT_SETPROPERTY;
    return NS_OK;
  }

  NS_IMETHOD GetProperty(nsIXPConnectWrappedNative *wrapper,
                         JSContext *cx, JSObject *obj, jsval id,
                         jsval *vp, PRBool *ret_val) {
    std::string json;
    NS_ENSURE_TRUE(JSONEncode(cx, id, &json), NS_ERROR_FAILURE);
    std::string result = SendFeedback(kGetPropertyFeedback, cx,
                                      json.c_str(), NULL);
    if (result == "\"\\\"function\\\"\"") {
      JSFunction *function = JS_NewFunction(cx, InvokeFunction, 0, 0,
                                            obj, json.c_str());
      NS_ENSURE_TRUE(function, NS_ERROR_FAILURE);
      JSObject *func_obj = JS_GetFunctionObject(function);
      NS_ENSURE_TRUE(func_obj, NS_ERROR_FAILURE);
      *vp = OBJECT_TO_JSVAL(func_obj);
    } else if (result == "\"\\\"undefined\\\"\"") {
      *vp = JSVAL_VOID;
    } else {
      NS_ENSURE_TRUE(JSONDecode(cx, result.c_str(), vp), NS_ERROR_FAILURE);
    }
    *ret_val = PR_TRUE;
    return NS_OK;
  }

  NS_IMETHOD SetProperty(nsIXPConnectWrappedNative *wrapper,
                         JSContext *cx, JSObject *obj, jsval id,
                         jsval *vp, PRBool *ret_val) {
    std::string name_json, value_json;
    NS_ENSURE_TRUE(JSONEncode(cx, id, &name_json), NS_ERROR_FAILURE);
    NS_ENSURE_TRUE(JSONEncode(cx, *vp, &value_json), NS_ERROR_FAILURE);
    SendFeedback(kSetPropertyFeedback, cx,
                 name_json.c_str(), value_json.c_str(), NULL);
    *ret_val = PR_TRUE;
    return NS_OK;
  }

  NS_IMETHOD PreCreate(nsISupports *, JSContext *, JSObject *, JSObject **) {
    return NS_ERROR_NOT_IMPLEMENTED;
  }
  NS_IMETHOD Create(nsIXPConnectWrappedNative *, JSContext *, JSObject *) {
    return NS_ERROR_NOT_IMPLEMENTED;
  }
  NS_IMETHOD PostCreate(nsIXPConnectWrappedNative *, JSContext *, JSObject *) {
    return NS_ERROR_NOT_IMPLEMENTED;
  }
  NS_IMETHOD AddProperty(nsIXPConnectWrappedNative *, JSContext *, JSObject *,
                         jsval, jsval *, PRBool *) {
    return NS_ERROR_NOT_IMPLEMENTED;
  }
  NS_IMETHOD DelProperty(nsIXPConnectWrappedNative *, JSContext *, JSObject *,
                         jsval, jsval *, PRBool *) {
    return NS_ERROR_NOT_IMPLEMENTED;
  }
  NS_IMETHOD NewResolve(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                        JSObject *obj, jsval id, PRUint32 flags,
                        JSObject **objp, PRBool *ret_val) {
    return NS_ERROR_NOT_IMPLEMENTED;
  }
  NS_IMETHOD Enumerate(nsIXPConnectWrappedNative *, JSContext *, JSObject *,
                       PRBool *) {
    return NS_ERROR_NOT_IMPLEMENTED;
  }
  NS_IMETHOD NewEnumerate(nsIXPConnectWrappedNative *, JSContext *, JSObject *,
                          PRUint32, jsval *, jsid *, PRBool *) {
    return NS_ERROR_NOT_IMPLEMENTED;
  }
  NS_IMETHOD Convert(nsIXPConnectWrappedNative *, JSContext *, JSObject *,
                     PRUint32 type, jsval *, PRBool *) {
    return NS_ERROR_NOT_IMPLEMENTED;
  }
  NS_IMETHOD Finalize(nsIXPConnectWrappedNative *, JSContext *, JSObject *) {
    return NS_ERROR_NOT_IMPLEMENTED;
  }
  NS_IMETHOD CheckAccess(nsIXPConnectWrappedNative *, JSContext *, JSObject *,
                         jsval, PRUint32, jsval *, PRBool *) {
    return NS_ERROR_NOT_IMPLEMENTED;
  }
  NS_IMETHOD Call(nsIXPConnectWrappedNative *, JSContext *, JSObject *,
                  PRUint32, jsval *, jsval *, PRBool *) {
    return NS_ERROR_NOT_IMPLEMENTED;
  }
  NS_IMETHOD Construct(nsIXPConnectWrappedNative *, JSContext *, JSObject *,
                       PRUint32, jsval *, jsval *, PRBool *) {
    return NS_ERROR_NOT_IMPLEMENTED;
  }
  NS_IMETHOD HasInstance(nsIXPConnectWrappedNative *, JSContext *, JSObject *,
                         jsval, PRBool *, PRBool *) {
    return NS_ERROR_NOT_IMPLEMENTED;
  }
  NS_IMETHOD Mark(nsIXPConnectWrappedNative *, JSContext *, JSObject *,
                  void *, PRUint32 *) {
    return NS_ERROR_NOT_IMPLEMENTED;
  }
  NS_IMETHOD Equality(nsIXPConnectWrappedNative *, JSContext *, JSObject *,
                      jsval, PRBool *) {
    return NS_ERROR_NOT_IMPLEMENTED;
  }
  NS_IMETHOD OuterObject(nsIXPConnectWrappedNative *, JSContext *, JSObject *,
                         JSObject **) {
    return NS_ERROR_NOT_IMPLEMENTED;
  }
  NS_IMETHOD InnerObject(nsIXPConnectWrappedNative *, JSContext *, JSObject *,
                         JSObject **) {
    return NS_ERROR_NOT_IMPLEMENTED;
  }
  // For Gecko 1.9.
  NS_IMETHOD Trace(nsIXPConnectWrappedNative*, JSTracer *, JSObject *) {
    return NS_ERROR_NOT_IMPLEMENTED;
  }
};

NS_IMPL_ISUPPORTS1(ExternalObject, nsIXPCScriptable)

static ExternalObject g_external_object;
static ExternalObject *GetExternalObject() {
  g_external_object.AddRef();
  return &g_external_object;
}
NS_GENERIC_FACTORY_SINGLETON_CONSTRUCTOR(ExternalObject, GetExternalObject)

static const nsModuleComponentInfo kExternalObjectComponentInfo = {
  EXTOBJ_CLASSNAME, EXTOBJ_CID, EXTOBJ_CONTRACTID, ExternalObjectConstructor
};

static std::string g_down_buffer;

// This function is only used to decode the HTML/Text content sent in CONTENT.
// We can't use JSONDecode because the script context is not available now.
static bool DecodeJSONString(const char *json_string, nsString *result) {
  if (!json_string || *json_string != '"')
    return false;
  while (*++json_string != '"') {
    if (*json_string == '\0') {
      // Unterminated JSON string.
      return false;
    }
    if (*json_string == '\\') {
      switch (*++json_string) {
        case 'b': result->Append('\b'); break;
        case 'f': result->Append('\f'); break;
        case 'n': result->Append('\n'); break;
        case 'r': result->Append('\r'); break;
        case 't': result->Append('\t'); break;
        case 'u': {
          PRUnichar unichar = 0;
          for (int i = 1; i <= 4; i++) {
            int c = json_string[i];
            if (c >= '0' && c <= '9') c -= '0';
            else if (c >= 'A' && c <= 'F') c = c - 'A' + 10;
            else if (c >= 'a' && c <= 'f') c = c - 'a' + 10;
            else return false;
            unichar = static_cast<PRUnichar>((unichar << 4) + c);
          }
          result->Append(unichar);
          json_string += 4;
          break;
        }
        case '\0': return false;
        default: result->Append(*json_string); break;
      }
    } else {
      result->Append(*json_string);
    }
  }
  return true;
}

static int FindBrowserIdByContentPolicyContext(nsISupports *context,
                                               PRBool *is_loading) {
  nsCOMPtr<nsIDOMWindow> window(do_QueryInterface(context));
  nsresult rv = NS_OK;
  if (!window) {
    nsCOMPtr<nsIDOMDocument> document(do_QueryInterface(context));
    if (!document) {
      nsCOMPtr<nsIDOMNode> node(do_QueryInterface(context, &rv));
      NS_ENSURE_SUCCESS(rv, -1);
      node->GetOwnerDocument(getter_AddRefs(document));
    }
    nsCOMPtr<nsIDOMDocumentView> docview(do_QueryInterface(document, &rv));
    NS_ENSURE_SUCCESS(rv, -1);
    nsCOMPtr<nsIDOMAbstractView> view;
    rv = docview->GetDefaultView(getter_AddRefs(view));
    NS_ENSURE_SUCCESS(rv, -1);
    window = do_QueryInterface(view);
  }

  *is_loading = PR_FALSE;
  for (std::vector<GtkMozEmbed *>::const_iterator it = g_embeds.begin();
       it != g_embeds.end(); ++it) {
    if (*it) {
      nsCOMPtr<nsIWebBrowser> browser;
      gtk_moz_embed_get_nsIWebBrowser(*it, getter_AddRefs(browser));
      nsCOMPtr<nsIDOMWindow> window1;
      rv = browser->GetContentDOMWindow(getter_AddRefs(window1));
      NS_ENSURE_SUCCESS(rv, -1);
      if (window == window1) {
        nsCOMPtr<nsIWebProgress> progress(do_GetInterface(browser));
        if (progress)
          progress->GetIsLoadingDocument(is_loading);
        return static_cast<int>(it - g_embeds.begin());
      }
    }
  }
  fprintf(stderr, "browser_child: Can't find GtkMozEmbed from "
          "ContentPolicy context\n");
  return -1;
}

class ContentPolicy : public nsIContentPolicy {
 public:
  NS_DECL_ISUPPORTS

  NS_IMETHOD ShouldLoad(PRUint32 content_type, nsIURI *content_location,
                        nsIURI *request_origin, nsISupports *context,
                        const nsACString &mime_type_guess, nsISupports *extra,
                        PRInt16 *retval) {
    NS_ENSURE_ARG_POINTER(content_location);
    NS_ENSURE_ARG_POINTER(retval);
    nsCString url_spec, origin_spec, url_scheme;
    content_location->GetSpec(url_spec);
    if (content_type == TYPE_DOCUMENT && g_embed_for_new_window) {
      // Handle a new window request.
      gtk_widget_destroy(g_popup_for_new_window);
      g_popup_for_new_window = NULL;
      g_embed_for_new_window = NULL;
      std::vector<GtkMozEmbed *>::const_iterator it = std::find(
          g_embeds.begin(), g_embeds.end(), g_main_embed_for_new_window);
      if (it != g_embeds.end()) {
        SendFeedbackWithBrowserId(kOpenURLFeedback,
                                  static_cast<int>(it - g_embeds.begin()),
                                  url_spec.get(), NULL);
      }
      // Reject this URL no matter if the controler has opened it.
      *retval = REJECT_OTHER;
      return NS_OK;
    }

    *retval = ACCEPT;
    content_location->GetScheme(url_scheme);
    if (content_type == TYPE_DOCUMENT || content_type == TYPE_SUBDOCUMENT) {
      // If the URL is opened the first time in a blank window or frame,
      // request_origin is NULL or "about:blank".
      if (request_origin) {
        request_origin->GetSpec(origin_spec);
        if (!origin_spec.Equals(nsCString("about:blank")) &&
            !origin_spec.Equals(url_spec) &&
            !url_scheme.Equals(nsCString("javascript"))) {
          PRBool is_loading = PR_FALSE;
          int browser_id = FindBrowserIdByContentPolicyContext(context,
                                                               &is_loading);
          // Allow URLs opened during page loading to be opened in place.
          if (browser_id != -1 && !is_loading) {
            std::string r = SendFeedbackWithBrowserId(kOpenURLFeedback,
                                                      browser_id,
                                                      url_spec.get(), NULL);
            // The controller should have opened the URL, so don't let the
            // embedded browser open it.
            if (r[0] != '0')
              *retval = REJECT_OTHER;
          }
        }
      }
    }
    return NS_OK;
  }

  NS_IMETHOD ShouldProcess(PRUint32 content_type, nsIURI *content_location,
                           nsIURI *request_origin, nsISupports *context,
                           const nsACString & mime_type, nsISupports *extra,
                           PRInt16 *retval) {
    return ShouldLoad(content_type, content_location, request_origin,
                      context, mime_type, extra, retval);
  }
};

NS_IMPL_ISUPPORTS1(ContentPolicy, nsIContentPolicy)

static ContentPolicy g_content_policy;
static ContentPolicy *GetContentPolicy() {
  g_content_policy.AddRef();
  return &g_content_policy;
}
NS_GENERIC_FACTORY_SINGLETON_CONSTRUCTOR(ContentPolicy, GetContentPolicy)

static const nsModuleComponentInfo kContentPolicyComponentInfo = {
  CONTENT_POLICY_CLASSNAME, CONTENT_POLICY_CID, CONTENT_POLICY_CONTRACTID,
  ContentPolicyConstructor
};

static void OnNewWindow(GtkMozEmbed *embed, GtkMozEmbed **retval,
                        gint chrome_mask, gpointer data) {
  if (!GTK_IS_WIDGET(g_embed_for_new_window)) {
    // Create a hidden GtkMozEmbed widget.
    g_embed_for_new_window = GTK_MOZ_EMBED(gtk_moz_embed_new());
    // The GtkMozEmbed widget needs a parent window.
    g_popup_for_new_window = gtk_window_new(GTK_WINDOW_POPUP);
    gtk_container_add(GTK_CONTAINER(g_popup_for_new_window),
                      GTK_WIDGET(g_embed_for_new_window));
    gtk_window_resize(GTK_WINDOW(g_popup_for_new_window), 1, 1);
    gtk_window_move(GTK_WINDOW(g_popup_for_new_window), -10000, -10000);
    gtk_widget_realize(GTK_WIDGET(g_embed_for_new_window));
  }
  // Use the widget temporarily to let our ContentPolicy handle the request.
  *retval = g_embed_for_new_window;
  g_main_embed_for_new_window = embed;
}

static void OnBrowserDestroy(GtkObject *object, gpointer user_data) {
  size_t id = reinterpret_cast<size_t>(user_data);
  if (id < g_embeds.size())
    g_embeds[id] = NULL;
}

static void RemoveBrowser(size_t id) {
  if (id >= g_embeds.size()) {
    fprintf(stderr, "browser_child: Invalid browser id %zd to remove\n", id);
    return;
  }
  GtkMozEmbed *embed = g_embeds[id];
  if (GTK_IS_WIDGET(embed)) {
    GtkWidget *parent = gtk_widget_get_parent(GTK_WIDGET(embed));
    if (GTK_IS_WIDGET(parent)) {
      // In case of standalone testing.
      gtk_widget_destroy(parent);
    } else {
      gtk_widget_destroy(GTK_WIDGET(embed));
    }
  }
  g_embeds[id] = NULL;
}

static void NewBrowser(int param_count, const char **params, size_t id) {
  if (param_count != 3) {
    fprintf(stderr, "browser_child: Incorrect param count for %s: "
            "3 expected, %d given", kSetContentCommand, param_count);
    return;
  }

  // The new id can be less than or equals to the current size.
  if (id > kMaxBrowserId) {
    fprintf(stderr, "browser_child: New browser id is too big: %zd\n", id);
    return;
  }
  if (id >= g_embeds.size()) {
    g_embeds.resize(id + 1, NULL);
  } else if (g_embeds[id] != NULL) {
    fprintf(stderr, "browser_child: Warning: new browser id slot is "
            "not empty: %zd\n", id);
    RemoveBrowser(id);
  }

  GdkNativeWindow socket_id = static_cast<GdkNativeWindow>(strtol(params[2],
                                                                  NULL, 0));
  GtkWidget *window = socket_id ? gtk_plug_new(socket_id) :
                      gtk_window_new(GTK_WINDOW_TOPLEVEL);
  g_signal_connect(window, "destroy", G_CALLBACK(OnBrowserDestroy),
                   reinterpret_cast<gpointer>(id));
  GtkMozEmbed *embed = GTK_MOZ_EMBED(gtk_moz_embed_new());
  g_embeds[id] = embed;
  gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(embed));
  g_signal_connect(embed, "new_window", G_CALLBACK(OnNewWindow), NULL);
  gtk_widget_show_all(window);
}

static GtkMozEmbed *GetGtkEmbedByBrowserId(size_t id) {
  if (id >= g_embeds.size()) {
    fprintf(stderr, "browser_child: Invalid browser id %zd\n", id);
    return NULL;
  }

  GtkMozEmbed *embed = g_embeds[id];
  if (!GTK_IS_WIDGET(embed)) {
    fprintf(stderr, "browser_child: Invalid browser by id %zd\n", id);
    return NULL;
  }
  return embed;
}

static void SetContent(int param_count, const char **params, size_t id) {
  if (param_count != 4) {
    fprintf(stderr, "browser_child: Incorrect param count for %s: "
            "4 expected, %d given\n", kSetContentCommand, param_count);
    return;
  }

  GtkMozEmbed *embed = GetGtkEmbedByBrowserId(id);
  if (!embed)
    return;

  // params[2]: mime type; params[3]: JSON encoded content string.
  nsString content;
  if (!DecodeJSONString(params[3], &content)) {
    fprintf(stderr, "browser_child: Invalid JSON string: %s\n", params[3]);
    return;
  }

  NS_ConvertUTF16toUTF8 utf8(content);
  std::string url(utf8.get(), utf8.Length());
  std::string data;
  if (!ggadget::EncodeBase64(url, false, &data)) {
    fprintf(stderr, "browser_child: Unable to convert to base64: %s\n",
            url.c_str());
    return;
  }

  url = (std::string(kDataURLPrefix) + params[2]) + ";base64," + data;
  //fprintf(stderr, "browser_child: URL: (%d) %s\n", url.size(), url.c_str());
  gtk_moz_embed_load_url(embed, url.c_str());
}

static void OpenURL(int param_count, const char **params, size_t id) {
  if (param_count != 3) {
    fprintf(stderr, "browser_child: Incorrect param count for %s: "
            "3 expected, %d given\n", kOpenURLCommand, param_count);
    return;
  }

  GtkMozEmbed *embed = GetGtkEmbedByBrowserId(id);
  if (!embed)
    return;
  // params[2]: URL.
  gtk_moz_embed_load_url(embed, params[2]);
}

static void ProcessDownMessage(int param_count, const char **params) {
  NS_ASSERTION(param_count > 0, "");
  if (strcmp(params[0], kQuitCommand) == 0) {
    gtk_main_quit();
    return;
  }
  if (param_count < 2) {
    fprintf(stderr, "browser_child: No enough command parameter\n");
    return;
  }

  size_t id = static_cast<size_t>(strtol(params[1], NULL, 0));
  if (strcmp(params[0], kNewBrowserCommand) == 0) {
    NewBrowser(param_count, params, id);
    return;
  }
  if (strcmp(params[0], kSetContentCommand) == 0) {
    SetContent(param_count, params, id);
    return;
  }
  if (strcmp(params[0], kOpenURLCommand) == 0) {
    OpenURL(param_count, params, id);
    return;
  }
  if (strcmp(params[0], kCloseBrowserCommand) == 0) {
    RemoveBrowser(id);
    return;
  }
  fprintf(stderr, "browser_child: Invalid command: %s\n", params[0]);
}

static void ProcessDownMessages() {
  const int kMaxParams = 4;
  size_t curr_pos = 0;
  size_t eom_pos;
  while ((eom_pos = g_down_buffer.find(kEndOfMessageFull, curr_pos)) !=
         g_down_buffer.npos) {
    const char *params[kMaxParams];
    int param_count = 0;
    while (curr_pos < eom_pos) {
      size_t end_of_line_pos = g_down_buffer.find('\n', curr_pos);
      NS_ASSERTION(end_of_line_pos != g_down_buffer.npos, "");
      g_down_buffer[end_of_line_pos] = '\0';
      const char *start = g_down_buffer.c_str() + curr_pos;
      if (param_count < kMaxParams) {
        params[param_count] = start;
        param_count++;
      } else {
        fprintf(stderr, "browser_child: Extra parameter: %s\n", start);
        // Don't exit to recover from the error status.
      }
      curr_pos = end_of_line_pos + 1;
    }
    NS_ASSERTION(curr_pos = eom_pos + 1, "");
    curr_pos += arraysize(kEndOfMessageFull) - 2;
    if (param_count > 0)
      ProcessDownMessage(param_count, params);
  }
  g_down_buffer.erase(0, curr_pos);
}

static gboolean OnDownFDReady(GIOChannel *channel, GIOCondition condition,
                              gpointer data) {
  int fd = g_io_channel_unix_get_fd(channel);
  NS_ASSERTION(fd == g_down_fd, "Invalid callback fd");

  char buffer[4096];
  ssize_t read_bytes;
  while ((read_bytes = read(fd, buffer, sizeof(buffer))) > 0) {
    g_down_buffer.append(buffer, read_bytes);
    if (read_bytes < static_cast<ssize_t>(sizeof(buffer)))
      break;
  }
  ProcessDownMessages();
  return TRUE;
}

static void OnSigPipe(int sig) {
  fprintf(stderr, "browser_child: SIGPIPE occured, exiting...\n");
  gtk_main_quit();
}

static gboolean CheckController(gpointer data) {
  std::string buffer(kPingFeedback);
  buffer += kEndOfMessageFull;
  std::string result = SendFeedbackBuffer(buffer);
  if (result != kPingAck) {
    fprintf(stderr, "browser_child: Ping failed, exiting...\n");
    gtk_main_quit();
    return FALSE;
  }
  return TRUE;
}

static nsresult InitCustomComponents() {
  nsCOMPtr<nsIComponentRegistrar> registrar;
  nsresult rv = NS_GetComponentRegistrar(getter_AddRefs(registrar));
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsICategoryManager> category_manager =
        do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Register external object (Javascript window.external object).
  g_external_object.AddRef();
  nsCOMPtr<nsIGenericFactory> factory;
  factory = do_CreateInstance ("@mozilla.org/generic-factory;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  factory->SetComponentInfo(&kExternalObjectComponentInfo);
  rv = registrar->RegisterFactory(kExternalObjectComponentInfo.mCID,
                                  EXTOBJ_CLASSNAME, EXTOBJ_CONTRACTID,
                                  factory);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = category_manager->AddCategoryEntry(JAVASCRIPT_GLOBAL_PROPERTY_CATEGORY,
                                          EXTOBJ_PROPERTY_NAME,
                                          EXTOBJ_CONTRACTID,
                                          PR_FALSE, PR_TRUE, nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  // Register customized content policy.
  g_content_policy.AddRef();
  factory = do_CreateInstance ("@mozilla.org/generic-factory;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  factory->SetComponentInfo(&kContentPolicyComponentInfo);
  rv = registrar->RegisterFactory(kContentPolicyComponentInfo.mCID,
                                  CONTENT_POLICY_CLASSNAME,
                                  CONTENT_POLICY_CONTRACTID,
                                  factory);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = category_manager->AddCategoryEntry("content-policy",
                                          CONTENT_POLICY_CONTRACTID,
                                          CONTENT_POLICY_CONTRACTID,
                                          PR_FALSE, PR_TRUE, nsnull);
  NS_ENSURE_SUCCESS(rv, rv);
  return rv;
}

static bool InitGecko() {
#ifdef XPCOM_GLUE
  nsresult rv;

  static const GREVersionRange kGREVersion = {
    "1.9a", PR_TRUE,
    "1.9.0.*", PR_TRUE
  };

  char xpcom_location[4096];
  rv = GRE_GetGREPathWithProperties(&kGREVersion, 1, nsnull, 0,
                                    xpcom_location, 4096);
  if (NS_FAILED(rv)) {
    g_warning("Failed to find proper Gecko Runtime Environment!");
    return false;
  }

  printf("XPCOM location: %s\n", xpcom_location);

  // Startup the XPCOM Glue that links us up with XPCOM.
  rv = XPCOMGlueStartup(xpcom_location);
  if (NS_FAILED(rv)) {
    g_warning("Failed to startup XPCOM Glue!");
    return false;
  }

  rv = GTKEmbedGlueStartup();
  if (NS_FAILED(rv)) {
    g_warning("Failed to startup Gtk Embed Glue!");
    return false;
  }

  rv = GTKEmbedGlueStartupInternal();
  if (NS_FAILED(rv)) {
    g_warning("Failed to startup Gtk Embed Glue (internal)!");
    return false;
  }

  rv = ggadget::libmozjs::LibmozjsGlueStartupWithXPCOM();
  if (NS_FAILED(rv)) {
    g_warning("Failed to startup SpiderMonkey Glue!");
    return false;
  }

  char *last_slash = strrchr(xpcom_location, '/');
  if (last_slash)
    *last_slash = '\0';

  gtk_moz_embed_set_path(xpcom_location);
#elif defined(MOZILLA_FIVE_HOME)
  gtk_moz_embed_set_comp_path(MOZILLA_FIVE_HOME);
#endif
  return true;
}

int main(int argc, char **argv) {
  if (!g_thread_supported())
    g_thread_init(NULL);

  gtk_init(&argc, &argv);

  if (!InitGecko()) {
    g_warning("Failed to initialize Gecko.");
    return 1;
  }

  signal(SIGPIPE, OnSigPipe);
  if (argc >= 2)
    g_down_fd = g_ret_fd = static_cast<int>(strtol(argv[1], NULL, 0));
  if (argc >= 3)
    g_up_fd = static_cast<int>(strtol(argv[2], NULL, 0));
  if (argc >= 4)
    g_ret_fd = static_cast<int>(strtol(argv[3], NULL, 0));

  // Set the down FD to non-blocking mode to make the gtk main loop happy.
  int down_fd_flags = fcntl(g_down_fd, F_GETFL);
  down_fd_flags |= O_NONBLOCK;
  fcntl(g_down_fd, F_SETFL, down_fd_flags);

  GIOChannel *channel = g_io_channel_unix_new(g_down_fd);
  int down_fd_watch = g_io_add_watch(channel, G_IO_IN, OnDownFDReady, NULL);
  g_io_channel_unref(channel);

  gtk_moz_embed_push_startup();
  InitCustomComponents();
  if (g_ret_fd != g_down_fd) {
    // Only start ping timer in actual environment to ease testing.
    g_timeout_add(kPingInterval, CheckController, NULL);
  }

  gtk_main();
  g_source_remove(down_fd_watch);
  gtk_moz_embed_pop_startup();
  return 0;
}
