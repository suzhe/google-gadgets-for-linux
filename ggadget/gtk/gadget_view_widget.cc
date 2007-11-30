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

#include <algorithm>
#include <cairo.h>

#include <ggadget/gtk/cairo_canvas.h>
#include "gadget_view_widget.h"

#include "gtk_view_host.h"
#include "gtk_key_convert.h"

using ggadget::CairoCanvas;
using ggadget::Event;
using ggadget::EventResult;
using ggadget::MouseEvent;
using ggadget::KeyboardEvent;
using ggadget::DragEvent;
using ggadget::Color;
using ggadget::ViewInterface;

static GtkWidgetClass *parent_class = NULL;

static void GadgetViewWidget_destroy(GtkObject* object) {
  if (GTK_OBJECT_CLASS(parent_class)->destroy) {
    (*GTK_OBJECT_CLASS(parent_class)->destroy)(object);
  }
  GadgetViewWidget *gvw = GADGETVIEWWIDGET(object);

  if (gvw->current_drag_event) {
    delete gvw->current_drag_event;
    gvw->current_drag_event = NULL;
  }
}

static void GadgetViewWidget_realize(GtkWidget *widget) {
  if (GTK_WIDGET_CLASS(parent_class)->realize) {
    (*GTK_WIDGET_CLASS(parent_class)->realize)(widget);
  }
  GadgetViewWidget *gvw = GADGETVIEWWIDGET(widget);

  gvw->widget_width = (gint)(gvw->view->GetWidth() * gvw->zoom);
  gvw->widget_height = (gint)(gvw->view->GetHeight() * gvw->zoom);
}

static void GadgetViewWidget_unrealize(GtkWidget *widget) {
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

static gboolean GadgetViewWidget_configure(GtkWidget *widget,
                                           GdkEventConfigure *event) {
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
    // TODO: send onsize event.
  }

  return FALSE;
}

static gboolean GadgetViewWidget_expose(GtkWidget *widget,
                                        GdkEventExpose *event) {
  GadgetViewWidget *gvw = GADGETVIEWWIDGET(widget);
  gint width, height;
  gdk_drawable_get_size(widget->window, &width, &height);
  cairo_t *cr = gdk_cairo_create(widget->window);

  gdk_cairo_region(cr, event->region);
  cairo_clip(cr);

  if (gvw->composited) {
    cairo_operator_t op = cairo_get_operator(cr);
    cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
    cairo_paint(cr);
    cairo_set_operator(cr, op);
  }

  bool changed;
  // OK to downcast here since the canvas is created using GraphicsInterface
  // passed from SimpleHost.
  const CairoCanvas *canvas =
    ggadget::down_cast<const CairoCanvas *>(gvw->view->Draw(&changed));

  // view->Draw may return NULL if it's width or height is 0.
  if (canvas) {
    cairo_surface_t *surface = canvas->GetSurface();

    cairo_set_source_surface(cr, surface, 0., 0.);
    cairo_paint(cr);

    if (changed && gvw->useshapemask) {
      // Set the window shape to be irregular.
      int canvasw = cairo_image_surface_get_width(surface);
      int canvash = cairo_image_surface_get_height(surface);

      // create an identical bitmap to use as shape mask
      GdkBitmap *bitmap = 
        static_cast<GdkBitmap *>(gdk_pixmap_new(NULL, std::min(canvasw, width), 
                                                std::min(canvash, height), 1)); 
      cairo_t *mask = gdk_cairo_create(bitmap);
      // Note: Don't set clipping here, since we're resetting shape mask
      // for the entire widget, including areas outside the exposed region.
      cairo_set_operator(mask, CAIRO_OPERATOR_CLEAR);
      cairo_paint(mask);
      cairo_set_operator(mask, CAIRO_OPERATOR_OVER);
      cairo_set_source_surface(mask, surface, 0., 0.);
      cairo_paint(mask);
      cairo_destroy(mask);
      mask = NULL;

      gtk_widget_shape_combine_mask(widget, bitmap, 0, 0);
      gdk_bitmap_unref(bitmap);
      bitmap = NULL;

      GtkWidget *window = gtk_widget_get_toplevel(widget);
      if (GTK_IS_WINDOW(window)) { 
        // This step is necessary since the widget is probably not toplevel.
        gdk_window_merge_child_shapes(window->window); 
      } else {
        DLOG("Gadget is not inside toplevel window.");
      }
    }    
  }

  cairo_destroy(cr);
  cr = NULL;
  return FALSE;
}

static gboolean GadgetViewWidget_button_press(GtkWidget *widget,
                                              GdkEventButton *event) {
  bool handler_result = true;
  GadgetViewWidget *gvw = GADGETVIEWWIDGET(widget);
  gvw->host->HideTooltip(0);

  if (event->type == GDK_BUTTON_PRESS) {
    if (event->button == 1) {
      MouseEvent e(Event::EVENT_MOUSE_DOWN,
                   event->x / gvw->zoom, event->y / gvw->zoom,
                   MouseEvent::BUTTON_LEFT, 0);
      handler_result = gvw->view->OnMouseEvent(e);
    }
  }
  else if (event->type == GDK_2BUTTON_PRESS) {
    gvw->dbl_click = true;
    // The GTK event sequence here is: press 2press release
    // for the second click.
    Event::Type t;
    int button;
    if (event->button == 1) {
      button = MouseEvent::BUTTON_LEFT;
      t = Event::EVENT_MOUSE_DBLCLICK;
    } else if (event->button == 3) {
      button = MouseEvent::BUTTON_RIGHT;
      t = Event::EVENT_MOUSE_RDBLCLICK;
    } else {
      button = MouseEvent::BUTTON_NONE;
    }
    if (button != MouseEvent::BUTTON_NONE) {
      MouseEvent e(t, event->x / gvw->zoom, event->y / gvw->zoom, button, 0);
      handler_result = gvw->view->OnMouseEvent(e);
    }
  }

  if (handler_result == ggadget::EVENT_RESULT_UNHANDLED && !gvw->window_move) {
    gvw->window_move = true;

    GtkWidget *window = gtk_widget_get_toplevel(widget);
    if (GTK_IS_WINDOW(window)) {
      gint x = 0, y = 0;
      gtk_window_get_position(GTK_WINDOW(window), &x, &y);      
      gvw->window_move_x = event->x_root - x;
      gvw->window_move_y = event->y_root - y;     
    } else {
      gvw->window_move_x = event->x;
      gvw->window_move_y = event->y;
      DLOG("Gadget is not inside toplevel window.");
    }

    // Grab the cursor to prevent losing events.
    gdk_pointer_grab(widget->window, FALSE,
                     (GdkEventMask)(GDK_BUTTON_RELEASE_MASK |
                                    GDK_POINTER_MOTION_MASK),
                     NULL, NULL, event->time);
  }

  return handler_result ? FALSE : TRUE;
}

static gboolean GadgetViewWidget_button_release(GtkWidget *widget,
                                                GdkEventButton *event) {
  bool handler_result = true;
  GadgetViewWidget *gvw = GADGETVIEWWIDGET(widget);
  ASSERT(event->type == GDK_BUTTON_RELEASE);
  gvw->host->HideTooltip(0);

  if (gvw->window_move) {
    gdk_pointer_ungrab(event->time);
    gvw->window_move = false;    
  }

  if (event->button == 1) {
    MouseEvent e(Event::EVENT_MOUSE_UP,
                 event->x / gvw->zoom, event->y / gvw->zoom,
                 MouseEvent::BUTTON_LEFT, 0);
    handler_result = gvw->view->OnMouseEvent(e);
  }

  if (!gvw->dbl_click) {
    Event::Type t;
    int button;
    if (event->button == 1) {
      button = MouseEvent::BUTTON_LEFT;
      t = Event::EVENT_MOUSE_CLICK;
    } else if (event->button == 3) {
      button = MouseEvent::BUTTON_RIGHT;
      t = Event::EVENT_MOUSE_RCLICK;
    } else {
      button = MouseEvent::BUTTON_NONE;
    }
    if (button != MouseEvent::BUTTON_NONE) {
      MouseEvent e2(t, event->x / gvw->zoom, event->y / gvw->zoom, button, 0);
      handler_result = gvw->view->OnMouseEvent(e2);
    }
  } else {
    gvw->dbl_click = false;
  }

  return handler_result ? FALSE : TRUE;
}

static gboolean GadgetViewWidget_enter_notify(GtkWidget *widget,
                                              GdkEventCrossing *event) {
  GadgetViewWidget *gvw = GADGETVIEWWIDGET(widget);
  ASSERT(event->type == GDK_ENTER_NOTIFY);
  MouseEvent e(Event::EVENT_MOUSE_OVER,
               event->x / gvw->zoom, event->y / gvw->zoom,
               MouseEvent::BUTTON_NONE, 0);
  return gvw->view->OnMouseEvent(e) ? FALSE : TRUE;
}

static gboolean GadgetViewWidget_leave_notify(GtkWidget *widget,
                                              GdkEventCrossing *event) {
  GadgetViewWidget *gvw = GADGETVIEWWIDGET(widget);
  ASSERT(event->type == GDK_LEAVE_NOTIFY);
  gvw->host->HideTooltip(0);

  MouseEvent e(Event::EVENT_MOUSE_OUT,
               event->x / gvw->zoom, event->y / gvw->zoom,
               MouseEvent::BUTTON_NONE, 0);
  return gvw->view->OnMouseEvent(e) != ggadget::EVENT_RESULT_UNHANDLED;
}

static gboolean GadgetViewWidget_motion_notify(GtkWidget *widget,
                                               GdkEventMotion *event) {
  GadgetViewWidget *gvw = GADGETVIEWWIDGET(widget);
  ASSERT(event->type == GDK_MOTION_NOTIFY);
  int button = MouseEvent::BUTTON_NONE;
  if (event->state & GDK_BUTTON1_MASK) {
    button |= MouseEvent::BUTTON_LEFT;
  }
  if (event->state & GDK_BUTTON2_MASK) {
    button |= MouseEvent::BUTTON_MIDDLE;
  }
  if (event->state & GDK_BUTTON3_MASK) {
    button |= MouseEvent::BUTTON_RIGHT;
  }
  MouseEvent e(Event::EVENT_MOUSE_MOVE,
               event->x / gvw->zoom, event->y / gvw->zoom, button, 0);
  bool handler_result = gvw->view->OnMouseEvent(e);

  if (handler_result == ggadget::EVENT_RESULT_UNHANDLED &&
      gvw->window_move && (event->state & GDK_BUTTON1_MASK)) {
    GtkWidget *window = gtk_widget_get_toplevel(widget);
    if (GTK_IS_WINDOW(window)) {
      gint new_pos_x = static_cast<gint>(event->x_root - gvw->window_move_x);
      gint new_pos_y = static_cast<gint>(event->y_root - gvw->window_move_y);
      gtk_window_move(GTK_WINDOW(window), new_pos_x, new_pos_y);
    } else {
      DLOG("Gadget is not inside toplevel window.");
    }
  }

  // Since motion hint is enabled, we must notify GTK that we're ready to
  // receive the next motion event.
  // gdk_event_request_motions(event); // requires version 2.12
  int x, y;
  gdk_window_get_pointer(widget->window, &x, &y, NULL);

  return handler_result ? FALSE : TRUE;
}

static gboolean GadgetViewWidget_key_press(GtkWidget *widget,
                                           GdkEventKey *event) {
  EventResult handler_result = ggadget::EVENT_RESULT_UNHANDLED;
  GadgetViewWidget *gvw = GADGETVIEWWIDGET(widget);
  ASSERT(event->type == GDK_KEY_PRESS);
  gvw->host->HideTooltip(0);

  unsigned int key_code = ggadget::ConvertGdkKeyvalToKeyCode(event->keyval);
  if (key_code) {
    KeyboardEvent e(Event::EVENT_KEY_DOWN, key_code);
    handler_result = gvw->view->OnKeyEvent(e);
  } else {
    LOG("Unknown key: 0x%x", event->keyval);
  }

  if ((event->state & (GDK_CONTROL_MASK | GDK_MOD1_MASK)) == 0) {
    guint32 key_char;
    if (key_code == KeyboardEvent::KEY_ESCAPE ||
        key_code == KeyboardEvent::KEY_RETURN ||
        key_code == KeyboardEvent::KEY_BACK ||
        key_code == KeyboardEvent::KEY_TAB) {
      // gdk_keyval_to_unicode doesn't support the above keys.
      key_char = key_code;
    } else {
      key_char = gdk_keyval_to_unicode(event->keyval);
    }

    if (key_char) {
      // Send the char code in KEY_PRESS event.
      KeyboardEvent e2(Event::EVENT_KEY_PRESS, key_char);
      gvw->view->OnKeyEvent(e2);
    }
  }

  return handler_result != ggadget::EVENT_RESULT_UNHANDLED;
}

static gboolean GadgetViewWidget_key_release(GtkWidget *widget,
                                             GdkEventKey *event) {
  EventResult handler_result = ggadget::EVENT_RESULT_UNHANDLED;
  GadgetViewWidget *gvw = GADGETVIEWWIDGET(widget);
  ASSERT(event->type == GDK_KEY_RELEASE);

  unsigned int key_code = ggadget::ConvertGdkKeyvalToKeyCode(event->keyval);
  if (key_code) {
    KeyboardEvent e(Event::EVENT_KEY_UP, key_code);
    handler_result = gvw->view->OnKeyEvent(e);
  } else {
    LOG("Unknown key: 0x%x", event->keyval);
  }

  return handler_result != ggadget::EVENT_RESULT_UNHANDLED;
}

static gboolean GadgetViewWidget_focus_in(GtkWidget *widget,
                                          GdkEventFocus *event) {
  GadgetViewWidget *gvw = GADGETVIEWWIDGET(widget);
  ASSERT(event->type == GDK_FOCUS_CHANGE && event->in == TRUE);
  Event e(Event::EVENT_FOCUS_IN);
  return gvw->view->OnOtherEvent(e, NULL) != ggadget::EVENT_RESULT_UNHANDLED;
}

static gboolean GadgetViewWidget_focus_out(GtkWidget *widget,
                                           GdkEventFocus *event) {
  GadgetViewWidget *gvw = GADGETVIEWWIDGET(widget);
  ASSERT(event->type == GDK_FOCUS_CHANGE && event->in == FALSE);
  Event e(Event::EVENT_FOCUS_OUT);
  return gvw->view->OnOtherEvent(e, NULL) != ggadget::EVENT_RESULT_UNHANDLED;
}

static gboolean GadgetViewWidget_scroll(GtkWidget *widget,
                                        GdkEventScroll *event) {
  GadgetViewWidget *gvw = GADGETVIEWWIDGET(widget);
  ASSERT(event->type == GDK_SCROLL);
  int delta;
  if (event->direction == GDK_SCROLL_UP) {
    delta = MouseEvent::kWheelDelta;
  } else if (event->direction == GDK_SCROLL_DOWN) {
    delta = -MouseEvent::kWheelDelta;
  } else {
    delta = 0;
  }
  MouseEvent e(Event::EVENT_MOUSE_WHEEL,
               event->x / gvw->zoom, event->y / gvw->zoom,
               // TODO: button
               MouseEvent::BUTTON_NONE, delta);
  return gvw->view->OnMouseEvent(e) != ggadget::EVENT_RESULT_UNHANDLED;
}

static void GadgetViewWidget_init(GadgetViewWidget *gvw) {
  gvw->dbl_click = false;
  gvw->window_move = false;
  gvw->window_move_x = gvw->window_move_y = 0;
  gvw->current_drag_event = NULL;

  gtk_widget_set_events(GTK_WIDGET(gvw),
                        gtk_widget_get_events(GTK_WIDGET(gvw)) |
                        GDK_EXPOSURE_MASK |
                        GDK_FOCUS_CHANGE_MASK |
                        GDK_ENTER_NOTIFY_MASK |
                        GDK_LEAVE_NOTIFY_MASK |
                        GDK_BUTTON_PRESS_MASK |
                        GDK_BUTTON_RELEASE_MASK |
                        GDK_POINTER_MOTION_MASK |
                        GDK_POINTER_MOTION_HINT_MASK);

  GTK_WIDGET_SET_FLAGS(GTK_WIDGET(gvw), GTK_CAN_FOCUS);
}

static const char *kUriListTarget = "text/uri-list";

static void DisableDrag(GtkWidget *widget, GdkDragContext *context,
                        guint time) {
  gdk_drag_status(context, static_cast<GdkDragAction>(0), time);
  gtk_drag_unhighlight(widget);
}

static gboolean OnDragEvent(GtkWidget *widget, GdkDragContext *context,
                            gint x, gint y, guint time, Event::Type event_type) {
  GadgetViewWidget *gvw = GADGETVIEWWIDGET(widget);
  if (gvw->current_drag_event) {
    // There are some cases that multiple drag events are fired in one event
    // loop. For example, drag_leave and drag_drop, we use the latter one.
    delete gvw->current_drag_event;
    gvw->current_drag_event = NULL;
  }

  gvw->current_drag_event = new DragEvent(event_type, x, y, NULL);
  LOG("Drag Event: %d", event_type);

  GdkAtom target = gtk_drag_dest_find_target(
      widget, context, gtk_drag_dest_get_target_list(widget));
  if (target != GDK_NONE) {
    gtk_drag_get_data(widget, context, target, time);
    LOG("TRUE");
    return TRUE;
  } else {
    DLOG("Drag target or action not acceptable");
    DisableDrag(widget, context, time);
    LOG("FALSE");
    return FALSE;
  }
}

static gboolean GadgetViewWidget_drag_motion(GtkWidget *widget,
                                             GdkDragContext *context,
                                             gint x, gint y, guint time) {
  return OnDragEvent(widget, context, x, y, time, Event::EVENT_DRAG_MOTION);
}

static gboolean GadgetViewWidget_drag_drop(GtkWidget *widget,
                                           GdkDragContext *context,
                                           gint x, gint y, guint time) {
  gboolean result = OnDragEvent(widget, context, x, y, time,
                                Event::EVENT_DRAG_DROP);
  gtk_drag_finish(context, result, FALSE, time);
  return result;
}

static void GadgetViewWidget_drag_leave(GtkWidget *widget,
                                        GdkDragContext *context, guint time) {
  OnDragEvent(widget, context, 0, 0, time, Event::EVENT_DRAG_OUT);
}

static void GadgetViewWidget_drag_data_received(GtkWidget *widget, 
                                                GdkDragContext *context,
                                                gint x, gint y,
                                                GtkSelectionData *data,
                                                guint info, guint time) {
  GadgetViewWidget *gvw = GADGETVIEWWIDGET(widget);
  if (!gvw->current_drag_event) {
    // There are some cases that multiple drag events are fired in one event
    // loop. For example, drag_leave and drag_drop. Although current_drag_event
    // only contains the latter one, there are still multiple
    // drag_data_received events fired.
    return;
  }

  gchar **uris = gtk_selection_data_get_uris(data);
  if (!uris) {
    DLOG("No URI in drag data");
    DisableDrag(widget, context, time);
    return;
  }

  guint count = g_strv_length(uris);
  const char **drag_files = new const char *[count + 1];

  guint accepted_count = 0;
  for (guint i = 0; i < count; i++) {
    gchar *hostname;
    gchar *filename = g_filename_from_uri(uris[i], &hostname, NULL);
    if (filename) {
      if (!hostname)
        drag_files[accepted_count++] = filename;
      else
        g_free(filename);
    }
    g_free(hostname);
  }

  if (accepted_count == 0) {
    DLOG("No acceptable URI in drag data");
    DisableDrag(widget, context, time);
    return;
  }

  drag_files[accepted_count] = NULL;

  gvw->current_drag_event->SetDragFiles(drag_files);
  EventResult result = gvw->view->OnDragEvent(*gvw->current_drag_event);
  if (result == ggadget::EVENT_RESULT_HANDLED) {
    Event::Type type = gvw->current_drag_event->GetType();
    if (type == Event::EVENT_DRAG_DROP || type == Event::EVENT_DRAG_OUT) {
      gtk_drag_unhighlight(widget);
    } else {
      gdk_drag_status(context, GDK_ACTION_COPY, time);
      gtk_drag_highlight(widget);
    }
  } else {
    // Drag event is not accepted by the gadget.
    DisableDrag(widget, context, time);
  }

  delete gvw->current_drag_event;
  gvw->current_drag_event = NULL;
  for (guint i = 0; i < count; i++) {
    g_free(const_cast<gchar *>(drag_files[i]));
  }
  delete [] drag_files;

  g_strfreev(uris);
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

  widget_class->drag_motion = GadgetViewWidget_drag_motion;
  widget_class->drag_leave = GadgetViewWidget_drag_leave;
  widget_class->drag_drop = GadgetViewWidget_drag_drop;
  widget_class->drag_data_received = GadgetViewWidget_drag_data_received;

  widget_class->scroll_event = GadgetViewWidget_scroll;
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

GtkWidget *GadgetViewWidget_new(ggadget::GtkViewHost *host, double zoom,
                                bool composited, bool useshapemask) {
  GtkWidget *widget = GTK_WIDGET(g_object_new(GadgetViewWidget_get_type(),
                                 NULL));
  GadgetViewWidget *gvw = GADGETVIEWWIDGET(widget);
  gvw->host = host;
  gvw->view = host->GetView();
  ASSERT(gvw->view);
  gvw->zoom = zoom;
  gvw->composited = composited;
  gvw->useshapemask = useshapemask;

  static const GtkTargetEntry kDragTargets[] = {
    { const_cast<char *>(kUriListTarget), 0, 0 },
  };

  gtk_drag_dest_set(widget, static_cast<GtkDestDefaults>(0),
                    kDragTargets, arraysize(kDragTargets), GDK_ACTION_COPY);
  return widget;
}
