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

#ifndef HOSTS_SIMPLE_GADGET_VIEW_WIDGET_H__
#define HOSTS_SIMPLE_GADGET_VIEW_WIDGET_H__

#include <glib.h>
#include <gtk/gtk.h>
#include <string>

#include "ggadget/view_interface.h"

using ggadget::ViewInterface;

class GtkCairoHost;

G_BEGIN_DECLS

#define GADGETVIEWWIDGET_TYPE        (GadgetViewWidget_get_type())
#define GADGETVIEWWIDGET(obj)        (G_TYPE_CHECK_INSTANCE_CAST((obj), \
                                      GADGETVIEWWIDGET_TYPE, GadgetViewWidget))
#define GADGETVIEWWIDGET_CLASS(c)    (G_TYPE_CHECK_CLASS_CAST((c), \
                                      GADGETVIEWWIDGET_TYPE, \
                                      GadgetViewWidgetClass))
#define IS_GADGETVIEWWIDGET(obj)     (G_TYPE_CHECK_INSTANCE_TYPE((obj), \
                                      GADGETVIEWWIDGET_TYPE))
#define IS_GADGETVIEWWIDGET_CLASS(c) (G_TYPE_CHECK_CLASS_TYPE((c), \
                                      GADGETVIEWWIDGET_TYPE))

struct GadgetViewWidget {
  GtkDrawingArea drawingarea;

  GtkCairoHost *host;
  ViewInterface *view;
  double zoom;
  int widget_width, widget_height; // stores the old height/width before an allocation 
  bool dbl_click;
};

struct GadgetViewWidgetClass {
  GtkDrawingAreaClass parent_class; 

  void (* gadgetviewwidget)    (GadgetViewWidget *gvw);
};

GType          GadgetViewWidget_get_type();
GtkWidget*     GadgetViewWidget_new(ViewInterface *v, double zoom);

G_END_DECLS

#endif // HOSTS_SIMPLE_GADGET_VIEW_WIDGET_H__
