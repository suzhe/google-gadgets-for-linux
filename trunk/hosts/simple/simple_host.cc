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

#include <gtk/gtk.h>

#include "simple_host.h"
#include "gadget_view_widget.h"

#include "ggadget/view_interface.h"
#include "ggadget/graphics/cairo_graphics.h"

using ggadget::CairoGraphics;

SimpleHost::SimpleHost(GadgetViewWidget *gvw) 
  : gvw_(gvw), gfx_(new CairoGraphics(gvw->zoom)) {    
}

SimpleHost::~SimpleHost() {
  delete gfx_;
  gfx_ = NULL;
}

void SimpleHost::QueueDraw() {
  gtk_widget_queue_draw(GTK_WIDGET(gvw_));
}

bool SimpleHost::GrabKeyboardFocus() {
  gtk_widget_grab_focus(GTK_WIDGET(gvw_));
  return true;
}

// Note: SimpleHost doesn't actually support detaching itself if its widget
// is still active. Only detach when the host is about to be destroyed. 
// Otherwise, bad things will happen!
bool SimpleHost::DetachFromView() {
  gvw_->view = NULL;
  return true; 
}
