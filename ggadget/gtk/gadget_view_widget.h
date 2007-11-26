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

#ifndef GGADGET_GTK_GADGET_VIEW_WIDGET_H__
#define GGADGET_GTK_GADGET_VIEW_WIDGET_H__

#include <glib.h>
#include <gtk/gtk.h>
#include <string>

#include <ggadget/ggadget.h>

G_BEGIN_DECLS

namespace ggadget {
  class GtkViewHost;
}

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

  ggadget::GtkViewHost *host;
  ggadget::ViewInterface *view;
  double zoom;
  bool composited, useshapemask;
  // Stores the old height/width before an allocation.
  int widget_width, widget_height;
  bool dbl_click;
  bool window_move;
  double window_move_x, window_move_y;

  ggadget::DragEvent current_drag_event;
};

struct GadgetViewWidgetClass {
  GtkDrawingAreaClass parent_class;

  void (* gadgetviewwidget)(GadgetViewWidget *gvw);
};

GType GadgetViewWidget_get_type();
GtkWidget* GadgetViewWidget_new(ggadget::GtkViewHost *host, double zoom, 
                                bool composited, bool useshapemask);

G_END_DECLS

#endif // GGADGET_GTK_GADGET_VIEW_WIDGET_H__
