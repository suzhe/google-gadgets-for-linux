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

#include <sys/time.h>

#include "gtk_gadget_host.h"
#include "ggadget/file_manager.h"
#include "ggadget/element_factory.h"
#include "ggadget/gadget.h"
#include "ggadget/scripts/smjs/js_script_runtime.h"
#include "ggadget/xml_http_request.h"
#include "gtk_view_host.h"
#include "options.h"
#include "simplehost_file_manager.h"

#include "ggadget/button_element.h"
#include "ggadget/div_element.h"
#include "ggadget/img_element.h"
#include "ggadget/scrollbar_element.h"

class GtkGadgetHost::CallbackData {
 public:
  CallbackData(ggadget::Slot *s, GtkGadgetHost *h)
      : callback(s), host(h) { }
  virtual ~CallbackData() {
    delete callback;
  }

  int id;
  ggadget::Slot *callback;
  GtkGadgetHost *host;
};

GtkGadgetHost::GtkGadgetHost()
    : script_runtime_(new ggadget::JSScriptRuntime()),
      element_factory_(NULL), global_file_manager_(new SimpleHostFileManager()) {
  ggadget::ElementFactory *factory = new ggadget::ElementFactory();
  factory->RegisterElementClass("button",
                                &ggadget::ButtonElement::CreateInstance);
  factory->RegisterElementClass("div",
                                &ggadget::DivElement::CreateInstance);
  factory->RegisterElementClass("img",
                                &ggadget::ImgElement::CreateInstance);
  factory->RegisterElementClass("scrollbar",
                                &ggadget::ScrollBarElement::CreateInstance);
  element_factory_ = factory;

  global_file_manager_->Init(NULL);
  
  script_runtime_->ConnectErrorReporter(
      NewSlot(this, &GtkGadgetHost::ReportScriptError));
}

GtkGadgetHost::~GtkGadgetHost() {
  CallbackMap::iterator i = callbacks_.begin();
  while (i != callbacks_.end()) {
    CallbackMap::iterator next = i;
    ++next;
    RemoveCallback(i->first);
    i = next;
  }
  ASSERT(callbacks_.empty());

  delete element_factory_;
  element_factory_ = NULL;
  delete script_runtime_;
  script_runtime_ = NULL;
  delete global_file_manager_;
  global_file_manager_ = NULL;
}

ggadget::ScriptRuntimeInterface *GtkGadgetHost::GetScriptRuntime(
    ScriptRuntimeType type) {
  return script_runtime_;
}

ggadget::ElementFactoryInterface *GtkGadgetHost::GetElementFactory() {
  return element_factory_;
}

ggadget::FileManagerInterface *GtkGadgetHost::GetGlobalFileManager() {
  return global_file_manager_;  
}

ggadget::XMLHttpRequestInterface *GtkGadgetHost::NewXMLHttpRequest() {
  return new ggadget::XMLHttpRequest(this);
}

ggadget::ViewHostInterface *GtkGadgetHost::NewViewHost(
    ViewType type,
    ggadget::ScriptableInterface *prototype,
    ggadget::OptionsInterface *options) {
  return new GtkViewHost(this, type, options, prototype);
}

void GtkGadgetHost::DebugOutput(DebugLevel level, const char *message) {
  const char *str_level;
  switch (level) {
    case DEBUG_TRACE: str_level = "TRACE: "; break;
    case DEBUG_WARNING: str_level = "WARNING: "; break;
    case DEBUG_ERROR: str_level = "ERROR: "; break;
    default: break;
  }
  // TODO: actual debug console.
  printf("%s%s\n", str_level, message);
}

uint64_t GtkGadgetHost::GetCurrentTime() const {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return static_cast<uint64_t>(tv.tv_sec) * 1000000 + tv.tv_usec;
}

gboolean GtkGadgetHost::DispatchTimer(gpointer data) {
  CallbackData *tmdata = static_cast<CallbackData *>(data);
  // view->OnTimerEvent may call RemoveTimer, causing tmdata pointer invalid,
  // so save the valid host pointer first.
  GtkGadgetHost *host = tmdata->host;
  int id = tmdata->id;
  // DLOG("DispatchTimer id=%d", id);

  ggadget::Variant param(id);
  ggadget::Variant result = tmdata->callback->Call(1, &param);
  if (!ggadget::VariantValue<bool>()(result)) {
    // Event receiver has indicated that this timer should be removed.
    host->RemoveCallback(id);
    return FALSE;
  }
  return TRUE;
}

int GtkGadgetHost::RegisterTimer(unsigned ms, TimerCallback *callback) {
  ASSERT(callback);

  CallbackData *tmdata = new CallbackData(callback, this);
  tmdata->id = static_cast<int>(g_timeout_add(ms, DispatchTimer, tmdata));
  callbacks_[tmdata->id] = tmdata;
  // DLOG("RegisterTimer id=%d", tmdata->id);
  return tmdata->id;
}

bool GtkGadgetHost::RemoveTimer(int token) {
  // DLOG("RemoveTimer id=%d", token);
  return RemoveCallback(token);
}

gboolean GtkGadgetHost::DispatchIOWatch(GIOChannel *source,
                                        GIOCondition cond,
                                        gpointer data) {
  CallbackData *iodata = static_cast<CallbackData *>(data);
  ggadget::Variant param(g_io_channel_unix_get_fd(source));
  iodata->callback->Call(1, &param);
  return TRUE;
}

int GtkGadgetHost::RegisterIOWatch(bool read_or_write, int fd,
                                  IOWatchCallback *callback) {
  ASSERT(callback);

  GIOCondition cond = read_or_write ? G_IO_IN : G_IO_OUT;
  GIOChannel *channel = g_io_channel_unix_new(fd);
  CallbackData *iodata = new CallbackData(callback, this);
  iodata->id = static_cast<int>(g_io_add_watch(channel, cond,
                                               DispatchIOWatch, iodata));
  callbacks_[iodata->id] = iodata;
  return iodata->id;
}

int GtkGadgetHost::RegisterReadWatch(int fd, IOWatchCallback *callback) {
  return RegisterIOWatch(true, fd, callback);
}

int GtkGadgetHost::RegisterWriteWatch(int fd, IOWatchCallback *callback) {
  return RegisterIOWatch(false, fd, callback);
}

bool GtkGadgetHost::RemoveIOWatch(int token) {
  return RemoveCallback(token);
}

bool GtkGadgetHost::RemoveCallback(int token) {
  ASSERT(token);

  CallbackMap::iterator i = callbacks_.find(token);
  if (i == callbacks_.end())
    // This data may be a stale pointer.
    return false;

  if (!g_source_remove(token))
    return false;

  delete i->second;
  callbacks_.erase(i);
  return true;
}

void GtkGadgetHost::ReportScriptError(const char *message) {
  DebugOutput(DEBUG_ERROR, (std::string("Script error: " ) + message).c_str());
}

ggadget::GadgetInterface *GtkGadgetHost::LoadGadget(const char *base_path,
                                                    double zoom,
                                                    int debug_mode) {
  // TODO: store zoom (and debug_mode?) into options repository.
  ggadget::FileManagerInterface *file_manager = new ggadget::FileManager();
  if (!file_manager->Init(base_path)) {
    delete file_manager;
    return NULL;
  }

  ggadget::OptionsInterface *options = new Options();
  options->PutValue(ggadget::kOptionZoom, ggadget::Variant(zoom));
  options->PutValue(ggadget::kOptionDebugMode, ggadget::Variant(debug_mode));

  ggadget::GadgetInterface *gadget = new ggadget::Gadget(this, options);
  if (!gadget->Init(file_manager)) {
    delete gadget;
    return NULL;
  }
  return gadget;
}
