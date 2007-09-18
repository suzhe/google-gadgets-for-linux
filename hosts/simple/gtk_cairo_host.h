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

#ifndef HOSTS_SIMPLE_GTK_CAIRO_HOST_H__
#define HOSTS_SIMPLE_GTK_CAIRO_HOST_H__

#include <set>

#include <gtk/gtk.h>
#include "ggadget/common.h"
#include "ggadget/host_interface.h"

using ggadget::HostInterface;
using ggadget::GraphicsInterface;
using ggadget::ViewInterface;
using ggadget::ElementInterface;

class GadgetViewWidget;

/**
 * An implementation of HostInterface for the simple gadget host. 
 * In this implementation, there is one instance of GtkCairoHost per view, 
 * and one instance of GraphicsInterface per GtkCairoHost.
 */
class GtkCairoHost : public HostInterface {
 public:
  GtkCairoHost(GadgetViewWidget *gvw);
  virtual ~GtkCairoHost();

  virtual const GraphicsInterface *GetGraphics() const { return gfx_; };
 
  virtual void QueueDraw();
   
  virtual bool GrabKeyboardFocus();

  virtual bool DetachFromView();

  virtual void SetResizeable();  
  virtual void SetCaption(const char *caption); 
  virtual void SetShowCaptionAlways(bool always);
  
  virtual void *RegisterTimer(unsigned ms, 
                              ElementInterface *target, void *data);
  virtual bool RemoveTimer(void *token);
  
 private:     
  struct TimerData {
    TimerData(ElementInterface *t, void *d, GtkCairoHost *h) 
        : target(t), data(d), host(h) {}
    ElementInterface *target;
    void *data;
    GtkCairoHost *host; // When this is NULL, the object is marked to be freed.
  };
   
  GadgetViewWidget *gvw_;
  GraphicsInterface *gfx_;
  std::set<TimerData *> timers_;
   
  static gboolean DispatchTimer(gpointer data); 
  
  DISALLOW_EVIL_CONSTRUCTORS(GtkCairoHost);
};

#endif // HOSTS_SIMPLE_GTK_CAIRO_HOST_H__
