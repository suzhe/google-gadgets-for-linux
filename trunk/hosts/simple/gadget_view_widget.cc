// Copyright 2007 Google Inc. All Rights Reserved.
// Author: jimz@google.com (Jim Zhuang)

#include <cairo.h>

#include "ggadget/common.h"
#include "ggadget/event.h"
#include "ggadget/graphics/cairo_canvas.h"
#include "ggadget/graphics/cairo_graphics.h"

#include "gtk_cairo_host.h"
#include "gadget_view_widget.h"

using ggadget::CairoCanvas;
using ggadget::CairoGraphics;
using ggadget::Event;
using ggadget::MouseEvent;
using ggadget::KeyboardEvent;
using ggadget::Color;

static GtkWidgetClass *parent_class = NULL;

static void GadgetViewWidget_destroy(GtkObject* object) {
  GadgetViewWidget *gvw = GADGETVIEWWIDGET(object);
  
  // Can only free host after View detaches it.
  delete gvw->host;
  gvw->host = NULL;
  
  if (GTK_OBJECT_CLASS(parent_class)->destroy) {
    (*GTK_OBJECT_CLASS(parent_class)->destroy)(object);
  }
}

static void GadgetViewWidget_realize(GtkWidget *widget) {
  if (GTK_WIDGET_CLASS(parent_class)->realize) {
    (*GTK_WIDGET_CLASS(parent_class)->realize)(widget);
  }
  GadgetViewWidget *gvw = GADGETVIEWWIDGET(widget);  
  gvw->view->AttachHost(gvw->host);
  
  gvw->widget_width = (gint)(gvw->view->GetWidth() * gvw->zoom);
  gvw->widget_height = (gint)(gvw->view->GetHeight() * gvw->zoom);
}

static void GadgetViewWidget_unrealize(GtkWidget *widget) { 
  GadgetViewWidget *gvw = GADGETVIEWWIDGET(widget);  
  gvw->view->AttachHost(NULL);
  
  if (GTK_WIDGET_CLASS(parent_class)->unrealize) {
    (*GTK_WIDGET_CLASS(parent_class)->unrealize)(widget);
  }
}

static void GadgetViewWidget_size_request(GtkWidget *widget,
                                      GtkRequisition *requisition) {
  GadgetViewWidget *gvw = GADGETVIEWWIDGET(widget);
  requisition->width = (gint)(gvw->view->GetWidth() * gvw->zoom);
  requisition->height = (gint)(gvw->view->GetHeight() * gvw->zoom);
}

static gboolean GadgetViewWidget_configure(GtkWidget           *widget,
                                           GdkEventConfigure   *event) {
  GadgetViewWidget *gvw = GADGETVIEWWIDGET(widget);
  // Capture only changes to width, height, and not x, y.
  if (event->width != gvw->widget_width || 
      event->height != gvw->widget_height) {
    gvw->widget_width = event->width;
    gvw->widget_height = event->height;
    DLOG("configure %d %d", event->width, event->height);
    bool success = gvw->view->SetSize(int(event->width / gvw->zoom), 
                                      int(event->height / gvw->zoom));
    if (!success) {
      // Gdk may not obey this size request, but there's nothing we can do.
      // In that case, the view will still draw itself at the correct size,
      // but the widget display may crop it or show empty spacing around it. 
      gtk_widget_queue_resize(widget);
    }
  }
  
  return FALSE;
}

static gboolean GadgetViewWidget_expose(GtkWidget *widget, GdkEventExpose *event) {
  GadgetViewWidget *gvw = GADGETVIEWWIDGET(widget);
  gint width, height;
  gdk_drawable_get_size(widget->window, &width, &height);  
  cairo_t *cr = gdk_cairo_create(widget->window);
    
  cairo_rectangle(cr, event->area.x, event->area.y, 
                  event->area.width, event->area.height);
  cairo_clip(cr);
  
  bool changed;
  // OK to downcast here since the canvas is created using GraphicsInterface
  // passed from SimpleHost.
  const CairoCanvas *canvas = 
    ggadget::down_cast<const CairoCanvas *>(gvw->view->Draw(&changed));
      
  cairo_set_source_surface(cr, canvas->GetSurface(), 0., 0.);
  cairo_paint(cr);
  
  cairo_destroy(cr);
  cr = NULL;
  
  return FALSE;
}

static gboolean GadgetViewWidget_button_press(GtkWidget *widget, 
                                              GdkEventButton *event) {
  GadgetViewWidget *gvw = GADGETVIEWWIDGET(widget);  
  if (event->type == GDK_BUTTON_PRESS) {
    MouseEvent e(Event::EVENT_MOUSE_DOWN, 
                 event->x / gvw->zoom, event->y / gvw->zoom);
    gvw->view->OnMouseEvent(&e);
  }
  else if (event->type == GDK_2BUTTON_PRESS) {    
    gvw->dbl_click = true;
  }
    
  return FALSE;
}

static gboolean GadgetViewWidget_button_release(GtkWidget *widget, 
                                                GdkEventButton *event) {
  GadgetViewWidget *gvw = GADGETVIEWWIDGET(widget); 
  ASSERT(event->type == GDK_BUTTON_RELEASE);
  MouseEvent e(Event::EVENT_MOUSE_UP, 
               event->x / gvw->zoom, event->y / gvw->zoom);
  gvw->view->OnMouseEvent(&e);

  if (gvw->dbl_click) {
    // The GTK event sequence here is: press 2press release
    // for the second click.
    MouseEvent e2(Event::EVENT_MOUSE_DBLCLICK, 
                  event->x / gvw->zoom, event->y / gvw->zoom);
    gvw->view->OnMouseEvent(&e2);
  }
  else {
    MouseEvent e2(Event::EVENT_MOUSE_CLICK, 
                  event->x / gvw->zoom, event->y / gvw->zoom);
    gvw->view->OnMouseEvent(&e2);
  }
  
  gvw->dbl_click = false;
  
  return FALSE;
}

static gboolean GadgetViewWidget_enter_notify(GtkWidget *widget, 
                                              GdkEventCrossing *event) {
  GadgetViewWidget *gvw = GADGETVIEWWIDGET(widget); 
  ASSERT(event->type == GDK_ENTER_NOTIFY);
  MouseEvent e(Event::EVENT_MOUSE_OVER, 
               event->x / gvw->zoom, event->y / gvw->zoom);
  gvw->view->OnMouseEvent(&e);

  return FALSE;
}

static gboolean GadgetViewWidget_leave_notify(GtkWidget *widget, 
                                              GdkEventCrossing *event) {
  GadgetViewWidget *gvw = GADGETVIEWWIDGET(widget); 
  ASSERT(event->type == GDK_LEAVE_NOTIFY);
  MouseEvent e(Event::EVENT_MOUSE_OUT, 
               event->x / gvw->zoom, event->y / gvw->zoom);
  gvw->view->OnMouseEvent(&e);
  
  return FALSE;
}

static gboolean GadgetViewWidget_motion_notify(GtkWidget *widget, 
                                               GdkEventMotion *event) {
  GadgetViewWidget *gvw = GADGETVIEWWIDGET(widget); 
  ASSERT(event->type == GDK_MOTION_NOTIFY);
  MouseEvent e(Event::EVENT_MOUSE_MOVE, 
               event->x / gvw->zoom, event->y / gvw->zoom);
  gvw->view->OnMouseEvent(&e);
  
  // Since motion hint is enabled, we must notify GTK that we're ready to 
  // receive the next motion event.
  // gdk_event_request_motions(event); // requires version 2.12
  int x, y;
  gdk_window_get_pointer(widget->window, &x, &y, NULL);
  
  return FALSE;
}

static gboolean GadgetViewWidget_key_press(GtkWidget *widget, 
                                           GdkEventKey *event) {
  GadgetViewWidget *gvw = GADGETVIEWWIDGET(widget); 
  ASSERT(event->type == GDK_KEY_PRESS);
  
  // TODO handle modifier keys correctly
  if (gvw->last_keyval != event->keyval) {
    KeyboardEvent e(Event::EVENT_KEY_DOWN);
    gvw->view->OnKeyEvent(&e);
    
    gvw->last_keyval = event->keyval;
  }
  
  KeyboardEvent e2(Event::EVENT_KEY_PRESS);
  gvw->view->OnKeyEvent(&e2);
  
  return FALSE;
}

static gboolean GadgetViewWidget_key_release(GtkWidget *widget, 
                                             GdkEventKey *event) {
  GadgetViewWidget *gvw = GADGETVIEWWIDGET(widget); 
  ASSERT(event->type == GDK_KEY_RELEASE);
  KeyboardEvent e(Event::EVENT_KEY_UP);
  gvw->view->OnKeyEvent(&e);
  
  gvw->last_keyval = 0; // reset last_keyval
  
  return FALSE;
}

static gboolean GadgetViewWidget_focus_in(GtkWidget *widget, 
                                          GdkEventFocus *event) {
  GadgetViewWidget *gvw = GADGETVIEWWIDGET(widget); 
  ASSERT(event->type == GDK_FOCUS_CHANGE && event->in == TRUE);
  Event e(Event::EVENT_FOCUS_IN);
  gvw->view->OnOtherEvent(&e);
  
  return FALSE;
}

static gboolean GadgetViewWidget_focus_out(GtkWidget *widget, 
                                           GdkEventFocus *event) {
  GadgetViewWidget *gvw = GADGETVIEWWIDGET(widget); 
  ASSERT(event->type == GDK_FOCUS_CHANGE && event->in == FALSE);
  Event e(Event::EVENT_FOCUS_OUT);
  gvw->view->OnOtherEvent(&e);
  
  return FALSE;
}

static gboolean GadgetViewWidget_scroll(GtkWidget *widget, 
                                        GdkEventScroll *event) {
  GadgetViewWidget *gvw = GADGETVIEWWIDGET(widget); 
  ASSERT(event->type == GDK_SCROLL);
  MouseEvent e(Event::EVENT_MOUSE_WHEEL, 
               event->x / gvw->zoom, event->y / gvw->zoom);
  gvw->view->OnMouseEvent(&e);
  
  return FALSE;
}

static void GadgetViewWidget_class_init(GadgetViewWidgetClass *c) {
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(c);
  GtkObjectClass *object_class = GTK_OBJECT_CLASS(c);

  parent_class = GTK_WIDGET_CLASS(gtk_type_class(gtk_drawing_area_get_type()));

  object_class->destroy = GadgetViewWidget_destroy;

  widget_class->realize = GadgetViewWidget_realize;
  widget_class->unrealize = GadgetViewWidget_unrealize;
  
  widget_class->configure_event = GadgetViewWidget_configure;
  widget_class->expose_event = GadgetViewWidget_expose;
  widget_class->size_request = GadgetViewWidget_size_request;
  
  widget_class->button_press_event = GadgetViewWidget_button_press;
  widget_class->button_release_event = GadgetViewWidget_button_release;  
  widget_class->motion_notify_event = GadgetViewWidget_motion_notify;
  widget_class->leave_notify_event = GadgetViewWidget_leave_notify;
  widget_class->enter_notify_event = GadgetViewWidget_enter_notify;

  widget_class->key_press_event = GadgetViewWidget_key_press;
  widget_class->key_release_event = GadgetViewWidget_key_release;
  
  widget_class->focus_in_event = GadgetViewWidget_focus_in;
  widget_class->focus_out_event = GadgetViewWidget_focus_out;
  
  widget_class->scroll_event = GadgetViewWidget_scroll;
}

static void GadgetViewWidget_init(GadgetViewWidget *gvw) {
  gvw->dbl_click = false;
  gvw->last_keyval = 0; // 0 isn't used as a key value in GDK
  
  gtk_widget_set_events(GTK_WIDGET(gvw), 
                        gtk_widget_get_events(GTK_WIDGET(gvw))
                        | GDK_EXPOSURE_MASK
                        | GDK_FOCUS_CHANGE_MASK
                        | GDK_ENTER_NOTIFY_MASK
            			| GDK_LEAVE_NOTIFY_MASK
            			| GDK_BUTTON_PRESS_MASK            			
            			| GDK_BUTTON_RELEASE_MASK
            			| GDK_POINTER_MOTION_MASK
            			| GDK_POINTER_MOTION_HINT_MASK);
  
  GTK_WIDGET_SET_FLAGS(GTK_WIDGET(gvw), GTK_CAN_FOCUS);
}

GType GadgetViewWidget_get_type() {
  static GType gw_type = 0;

  if (!gw_type) {
    static const GTypeInfo gw_info = {
      sizeof(GadgetViewWidgetClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)GadgetViewWidget_class_init,
      NULL, // class_finalize
      NULL, // class_data
      sizeof(GadgetViewWidget),
      0, // n_preallocs
      (GInstanceInitFunc)GadgetViewWidget_init,
    };

    gw_type = g_type_register_static(GTK_TYPE_DRAWING_AREA, 
                                     "GadgetViewWidget", 
                                     &gw_info, 
                                     (GTypeFlags)0);
  }

  return gw_type;
}

GtkWidget *GadgetViewWidget_new(ViewInterface *v, double zoom, 
                                GtkCairoHost *host) {  
  GtkWidget *widget = GTK_WIDGET(g_object_new(GadgetViewWidget_get_type(),
                                 NULL));
  GadgetViewWidget *gvw = GADGETVIEWWIDGET(widget);
  ASSERT(v);
  gvw->view = v;
  gvw->zoom = zoom;
  
  CairoGraphics *gfx = new CairoGraphics(zoom);
  if (host) {
    host->SwitchWidget(gvw);
  }
  else {
    host = new GtkCairoHost(gvw);
  }
  host->SetGraphics(gfx);
  gvw->host = host;
   
  return widget;
}
