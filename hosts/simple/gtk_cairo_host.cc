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

#include "gtk_cairo_host.h"
#include "gadget_view_widget.h"
#include "ggadget/event.h"
#include "ggadget/graphics_interface.h"
#include "ggadget/view_interface.h"
#include "xml_http_request.h"

using ggadget::HostInterface;
using ggadget::GraphicsInterface;
using ggadget::ViewInterface;
using ggadget::TimerEvent;
using ggadget::Variant;
using ggadget::XMLHttpRequestInterface;

class GtkCairoHost::CallbackData {
 public:
  CallbackData(void *d, GtkCairoHost *h) 
      : data(d), host(h) { }
  virtual ~CallbackData() { }
  int id;
  void *data;
  GtkCairoHost *host; // When this is NULL, the object is marked to be freed.
};

GtkCairoHost::GtkCairoHost(GadgetViewWidget *gvw, int debug_mode) 
  : gvw_(gvw), gfx_(NULL), debug_mode_(debug_mode) {    
}

GtkCairoHost::~GtkCairoHost() {
  delete gfx_;
  gfx_ = NULL;
}

void GtkCairoHost::SetGraphics(GraphicsInterface *gfx) {
  delete gfx_;
  gfx_ = gfx;
}

void GtkCairoHost::QueueDraw() {
  gtk_widget_queue_draw(GTK_WIDGET(gvw_));
}

bool GtkCairoHost::GrabKeyboardFocus() {
  gtk_widget_grab_focus(GTK_WIDGET(gvw_));
  return true;
}

// Note: GtkCairoHost doesn't actually support detaching itself if its widget
// is still active. Only detach when the host is about to be destroyed. 
// Otherwise, bad things will happen!
bool GtkCairoHost::DetachFromView() {
  // Remove all timer and IO watch callbacks.
  CallbackMap::iterator i = callbacks_.begin();
  while (i != callbacks_.end()) {
    CallbackMap::iterator next = i;
    ++next;
    RemoveCallback(i->first);
    i = next;
  }
  ASSERT(callbacks_.empty());

  gvw_->view = NULL;
  return true;
}

void GtkCairoHost::SwitchWidget(GadgetViewWidget *new_gvw, int debug_mode) {
  if (gvw_) {
    gvw_->host = NULL; 
  }
  gvw_ = new_gvw;
  debug_mode_ = debug_mode;
}

void GtkCairoHost::SetResizeable() {
  // todo
}

void GtkCairoHost::SetCaption(const char *caption) {
  // todo
}

void GtkCairoHost::SetShowCaptionAlways(bool always) {
  // todo
}

static uint64_t GetCurrentTimeInternal() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return static_cast<uint64_t>(tv.tv_sec) * 1000000 + tv.tv_usec;
}

gboolean GtkCairoHost::DispatchTimer(gpointer data) {
  CallbackData *tmdata = static_cast<CallbackData *>(data);
  // view->OnTimerEvent may call RemoveTimer, causing tmdata pointer invalid,
  // so save the valid host pointer first.
  GtkCairoHost *host = tmdata->host;
  int id = tmdata->id;
  // DLOG("DispatchTimer id=%d", id);
  TimerEvent tmevent(tmdata->data, GetCurrentTimeInternal());
  host->gvw_->view->OnTimerEvent(&tmevent);

  if (!tmevent.GetReceiveMore()) {
    // Event receiver has indicated that this timer should be removed.
    host->RemoveCallback(id);
    return FALSE;
  }
  return TRUE;
}

int GtkCairoHost::RegisterTimer(unsigned ms, void *data) {
  CallbackData *tmdata = new CallbackData(data, this);
  tmdata->id = static_cast<int>(g_timeout_add(ms, DispatchTimer, tmdata));
  callbacks_[tmdata->id] = tmdata;
  // DLOG("RegisterTimer id=%d", tmdata->id);
  return tmdata->id;
}

bool GtkCairoHost::RemoveTimer(int token) {
  // DLOG("RemoveTimer id=%d", token);
  return RemoveCallback(token);
}

class GtkCairoHost::IOWatchCallbackData : public GtkCairoHost::CallbackData {
 public:
  IOWatchCallbackData(IOWatchCallback *callback, GtkCairoHost *h) 
      : CallbackData(callback, h) { }
  virtual ~IOWatchCallbackData() {
    delete static_cast<IOWatchCallback *>(data);
  }
};

gboolean GtkCairoHost::DispatchIOWatch(GIOChannel *source,
                                       GIOCondition cond,
                                       gpointer data) {
  CallbackData *iodata = static_cast<CallbackData *>(data);
  IOWatchCallback *slot = static_cast<IOWatchCallback *>(iodata->data);
  if (slot) {
    Variant param(g_io_channel_unix_get_fd(source));
    slot->Call(1, &param);
  }
  return TRUE;
}

int GtkCairoHost::RegisterIOWatch(bool read_or_write, int fd,
                                  IOWatchCallback *callback) {
  GIOCondition cond = read_or_write ? G_IO_IN : G_IO_OUT;
  GIOChannel *channel = g_io_channel_unix_new(fd);
  IOWatchCallbackData *iodata = new IOWatchCallbackData(callback, this);
  iodata->id = static_cast<int>(g_io_add_watch(channel, cond,
                                               DispatchIOWatch, iodata));
  callbacks_[iodata->id] = iodata;
  return iodata->id;
}

int GtkCairoHost::RegisterReadWatch(int fd, IOWatchCallback *callback) {
  return RegisterIOWatch(true, fd, callback);
}

int GtkCairoHost::RegisterWriteWatch(int fd, IOWatchCallback *callback) {
  return RegisterIOWatch(false, fd, callback);
}

bool GtkCairoHost::RemoveIOWatch(int token) {
  return RemoveCallback(token);
}

bool GtkCairoHost::RemoveCallback(int token) {
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

uint64_t GtkCairoHost::GetCurrentTime() const {
  return GetCurrentTimeInternal();
}

XMLHttpRequestInterface *GtkCairoHost::NewXMLHttpRequest() {
  return new XMLHttpRequest(this);
}
