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

using ggadget::GraphicsInterface;
using ggadget::TimerEvent;

GtkCairoHost::GtkCairoHost(GadgetViewWidget *gvw, int debug_mode) 
  : gvw_(gvw),
    gfx_(NULL),
    debug_mode_(debug_mode) {
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
  // Stop all timers.
  std::set<TimerData *>::iterator i = timers_.begin();
  for (; i != timers_.end(); ++i) {
    // Cannot simply delete the TimerData objects here since GDK might
    // still call the DispatchTimer callback with a stale TimerData object.
    // Therefore only DispatchTimer can free those objects.
    // The only thing we can do here is mark the object to be deleted
    (*i)->host = NULL;
  }
  timers_.clear();
  
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
  TimerData *tmdata = static_cast<TimerData *>(data);

  // Only signal view if timer is not already marked as to be deleted.
  if (!tmdata->host) {
    // tmdata should already have been removed from timers_ here.
    delete tmdata;  
    tmdata = NULL;
    return FALSE;
  }

  TimerEvent tmevent(tmdata->target, tmdata->data, GetCurrentTimeInternal());
  tmdata->host->gvw_->view->OnTimerEvent(&tmevent);
  
  if (!tmevent.GetReceiveMore()) {
    // Event receiver has indicated that this timer should be removed.
    // RemoveTimer may have been called during view->OnTimerEvent, so double
    // check tmdata.
    if (tmdata->host)
      tmdata->host->timers_.erase(tmdata);
    delete tmdata;  
    tmdata = NULL;
    return FALSE;
  }
  return TRUE;
}

void *GtkCairoHost::RegisterTimer(unsigned ms, 
                                  ElementInterface *target, void *data) {
  // We actually don't need to write our own callback to wrap this here, since
  // TimerCallback and GSourceFunc has the same prototype. 
  // But for safety reasons and to ensure clean host detachment, we wrap it
  // anyway.    
  
  TimerData *tmdata = new TimerData(target, data, this);
  timers_.insert(tmdata);
   
  g_timeout_add(ms, DispatchTimer, static_cast<gpointer>(tmdata));
  
  return static_cast<void *>(tmdata);
}

bool GtkCairoHost::RemoveTimer(void *token) {
  TimerData *tmdata = static_cast<TimerData *>(token);
  
  ASSERT(tmdata);
  // Important: can only mark timers for removal here, due to the stale
  // TimerData object problem.
  std::set<TimerData *>::iterator i = timers_.find(tmdata);
  if (i == timers_.end()) {
    return false;
  }  
  timers_.erase(i);
  tmdata->host = NULL;
  return true;
}

uint64_t GtkCairoHost::GetCurrentTime() const {
  return GetCurrentTimeInternal();
}
