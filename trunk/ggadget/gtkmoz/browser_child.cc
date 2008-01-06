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

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <gtk/gtk.h>

#include <gtkmozembed.h>
#include <gtkmozembed_internal.h>
#include <jsapi.h>

#include <nsCOMPtr.h>
#include <dom/nsIScriptNameSpaceManager.h>
#include <nsCRT.h>
#include <nsIComponentRegistrar.h>
#include <nsIGenericFactory.h>
#include <nsICategoryManager.h>
#include <nsServiceManagerUtils.h>
#include <nsStringAPI.h>
#include <nsXPCOMCID.h>
#include <xpconnect/nsIXPCScriptable.h>

#include <ggadget/smjs/json.h>

#include "browser_child.h"

using ggadget::smjs::JSONEncode;
using ggadget::smjs::JSONDecode;
using ggadget::gtkmoz::kEndOfMessage;
using ggadget::gtkmoz::kEndOfMessageFull;
using ggadget::gtkmoz::kSetContentCommand;
using ggadget::gtkmoz::kQuitCommand;
using ggadget::gtkmoz::kGetPropertyFeedback;
using ggadget::gtkmoz::kSetPropertyFeedback;
using ggadget::gtkmoz::kCallbackFeedback;
using ggadget::gtkmoz::kOpenURLFeedback;

// Default down and ret fds are standard input and up fd is standard output.
// The default values are useful when browser child is tested independently.
static int g_down_fd = 0, g_up_fd = 1, g_ret_fd = 0;
static GtkMozEmbed *g_embed = NULL;

#define EXTOBJ_CLASSNAME  "ExternalObject"
#define EXTOBJ_PROPERTY_NAME "external" 
#define EXTOBJ_CONTRACTID "@google.com/ggl/extobj;1"
#define EXTOBJ_CID { \
    0x224fb7b5, 0x6db0, 0x48db, \
    { 0xb8, 0x1e, 0x85, 0x15, 0xe7, 0x9f, 0x00, 0x30 } \
}

static std::string SendFeedbackBuffer(const std::string &buffer) {
  write(g_up_fd, buffer.c_str(), buffer.size());

  std::string reply;
  char ch;
  while (read(g_ret_fd, &ch, 1) == 1 && ch != '\n')
    reply += ch;
  return reply;
}

// Send a feedback with parameters to the controller through the up channel,
// and return the reply (got in the return value channel).
static std::string SendFeedback(const char *type, ...) {
  std::string buffer(type);
  va_list ap;
  va_start(ap, type);
  const char *param;
  while ((param = va_arg(ap, const char *)) != NULL) {
    buffer += '\n';
    buffer += param;
  }
  buffer += kEndOfMessageFull;
  return SendFeedbackBuffer(buffer);
}

JSBool InvokeFunction(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                      jsval *rval) {
  std::string buffer(kCallbackFeedback);
  buffer += '\n';
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
  ExternalObject() {
  }

  NS_DECL_ISUPPORTS

  NS_IMETHOD GetClassName(char **class_name) {
    NS_ENSURE_ARG_POINTER(class_name);
    *class_name = nsCRT::strdup(EXTOBJ_CLASSNAME);
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
    std::string result = SendFeedback(kGetPropertyFeedback, json.c_str(), NULL);
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
    SendFeedback(kSetPropertyFeedback, name_json.c_str(), value_json.c_str(),
                 NULL);
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
};

NS_IMPL_ISUPPORTS1(ExternalObject, nsIXPCScriptable)

ExternalObject g_external_object;
static ExternalObject *GetExternalObject() {
  g_external_object.AddRef();
  return &g_external_object;
}
NS_GENERIC_FACTORY_SINGLETON_CONSTRUCTOR(ExternalObject, GetExternalObject)

static const nsModuleComponentInfo kComponentInfo = {
  EXTOBJ_CLASSNAME, EXTOBJ_CID, EXTOBJ_CONTRACTID, ExternalObjectConstructor
};

nsresult InitExternalObject() {
  nsCOMPtr<nsIComponentRegistrar> registrar;
  nsresult rv = NS_GetComponentRegistrar(getter_AddRefs(registrar));
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIGenericFactory> factory;
  rv = NS_NewGenericFactory(getter_AddRefs(factory), &kComponentInfo);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = registrar->RegisterFactory(kComponentInfo.mCID,
                                  EXTOBJ_CLASSNAME, EXTOBJ_CONTRACTID,
                                  factory);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsICategoryManager> categoryManager =
        do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  return categoryManager->AddCategoryEntry(JAVASCRIPT_GLOBAL_PROPERTY_CATEGORY,
                                           EXTOBJ_PROPERTY_NAME,
                                           EXTOBJ_CONTRACTID,
                                           PR_FALSE, PR_TRUE, nsnull);
}

std::string g_down_buffer;

// This function is only used to decode the HTML/Text content sent in CONTENT.
// We can't use JSONDecode because the script context is not available now.
bool DecodeJSONString(const char *json_string, nsString *result) {
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
          ++json_string;
          PRUnichar unichar = 0;
          for (int i = 0; i < 4; i++) {
            char c = json_string[i];
            if (c >= '0' && c <= '9') c -= '0';
            else if (c >= 'A' && c <= 'F') c = c - 'A' + 10;
            else if (c >= 'a' && c <= 'f') c = c - 'a' + 10;
            else return false;
            unichar = unichar << 4 + c; 
          }
          result->Append(unichar);
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

static void ProcessDownMessage(int param_count, const char **params) {
  NS_ASSERTION(param_count > 0, "");
  if (strcmp(params[0], kSetContentCommand) == 0) {
    if (param_count == 3) {
      // params[1]: mime type; params[2]: JSON encoded content string.
      nsString content;
      if (!DecodeJSONString(params[2], &content)) {
        fprintf(stderr, "browser_child: Invalid JSON string: %s\n", params[2]);
      } else {
        NS_ConvertUTF16toUTF8 utf8(content);
        gtk_moz_embed_render_data(g_embed, utf8.get(), utf8.Length(),
                                  // Base URI and MIME type.
                                  "file:///dev/null", params[1]);
      }
    } else {
      fprintf(stderr, "browser_child: Incorrect param count for %s: "
              "3 expected, %d given", kSetContentCommand, param_count);
    }
  } else if (strcmp(params[0], kQuitCommand) == 0) {
    gtk_main_quit();
  } else {
    fprintf(stderr, "browser_child: Invalid command: %s\n", params[0]);
  }
}

static void ProcessDownMessages() {
  const int kMaxParams = 3;
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
    curr_pos += sizeof(kEndOfMessageFull) - 1;
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

static void OnNewWindow(GtkMozEmbed *embed, GtkMozEmbed **retval,
                        gint chrome_mask, gpointer data) {
  // TODO:
  *retval = NULL; 
}

static gint OnOpenURL(GtkMozEmbed *embed, const char *url, gpointer data) {
  SendFeedback(kOpenURLFeedback, url, NULL);
  // The controller should have opened the URL, so don't let the embedded
  // browser open it.
  return FALSE;
}

int main(int argc, char **argv) {
  gtk_init(&argc, &argv);

  GdkNativeWindow socket_id = 0;
  if (argc >= 2)
    socket_id = strtol(argv[1], NULL, 0);
  if (argc >= 3)
    g_down_fd = g_ret_fd = strtol(argv[2], NULL, 0);
  if (argc >= 4)
    g_up_fd = strtol(argv[3], NULL, 0);
  if (argc >= 5)
    g_ret_fd = strtol(argv[4], NULL, 0);

  // Set the down FD to non-blocking mode to make the gtk main loop happy.
  int down_fd_flags = fcntl(g_down_fd, F_GETFL);
  down_fd_flags |= O_NONBLOCK;
  fcntl(g_down_fd, F_SETFL, down_fd_flags);

  GIOChannel *channel = g_io_channel_unix_new(g_down_fd);
  int down_fd_watch = g_io_add_watch(channel, G_IO_IN, OnDownFDReady, NULL);
  g_io_channel_unref(channel);

  GtkWidget *window = socket_id ? gtk_plug_new(socket_id) :
                      gtk_window_new(GTK_WINDOW_TOPLEVEL);
  g_signal_connect(window, "destroy", gtk_main_quit, NULL);

  g_embed = GTK_MOZ_EMBED(gtk_moz_embed_new());
  InitExternalObject();
  gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(g_embed));
  g_signal_connect(g_embed, "new_window", G_CALLBACK(OnNewWindow), NULL);
  g_signal_connect(g_embed, "open_uri", G_CALLBACK(OnOpenURL), NULL);
  // gtk_moz_embed_load_url(g_embed, "http://www.google.com");
  gtk_widget_show_all(window);
  gtk_main();
  g_source_remove(down_fd_watch);
  return 0;
}
