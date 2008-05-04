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
#include <gdk/gdk.h>
#include <string>
#include <ggadget/gadget_consts.h>
#include <ggadget/logger.h>
#include <ggadget/options_interface.h>
#include <ggadget/gadget.h>
#include "single_view_host.h"
#include "view_widget_binder.h"
#include "cairo_graphics.h"
#include "menu_builder.h"
#include "tooltip.h"
#include "utilities.h"

namespace ggadget {
namespace gtk {

class SingleViewHost::Impl {
 public:
  Impl(ViewHostInterface::Type type,
       SingleViewHost *owner,
       double zoom,
       bool decorated,
       bool remove_on_close,
       bool native_drag_mode,
       int debug_mode)
    : type_(type),
      owner_(owner),
      view_(NULL),
      window_(NULL),
      widget_(NULL),
      fixed_(NULL),
      context_menu_(NULL),
      ok_button_(NULL),
      cancel_button_(NULL),
      tooltip_(new Tooltip(kShowTooltipDelay, kHideTooltipDelay)),
      binder_(NULL),
      debug_mode_(debug_mode),
      feedback_handler_(NULL),
      adjust_window_size_source_(0),
      decorated_(decorated),
      remove_on_close_(remove_on_close),
      native_drag_mode_(native_drag_mode),
      zoom_(zoom),
      win_x_(0),
      win_y_(0),
      cursor_offset_x_(-1),
      cursor_offset_y_(-1) {
    ASSERT(owner);
  }

  ~Impl() {
    Detach();

    delete tooltip_;
    tooltip_ = NULL;
  }

  void Detach() {
    // To make sure that it won't be accessed anymore.
    view_ = NULL;

    if (window_)
      CloseView();

    if (adjust_window_size_source_)
      g_source_remove(adjust_window_size_source_);
    adjust_window_size_source_ = 0;

    delete feedback_handler_;
    feedback_handler_ = NULL;
    delete binder_;
    binder_ = NULL;
    if (window_) {
      gtk_widget_destroy(window_);
      window_ = NULL;
    }
    if (context_menu_) {
      gtk_widget_destroy(context_menu_);
      context_menu_ = NULL;
    }
    widget_ = NULL;
    fixed_ = NULL;
    ok_button_ = NULL;
    cancel_button_ = NULL;
  }

  void SetView(ViewInterface *view) {
    Detach();
    if (view == NULL) return;

    view_ = view;
    bool no_background = false;
    // Initialize window and widget.
    // All views must be held inside GTKFixed widgets in order to support the
    // browser element.
    fixed_ = gtk_fixed_new();
    gtk_widget_show(fixed_);
    if (type_ == ViewHostInterface::VIEW_HOST_OPTIONS) {
      // Options view needs run in a dialog with ok and cancel buttion.
      window_ = gtk_dialog_new();
      gtk_container_add(GTK_CONTAINER(GTK_DIALOG(window_)->vbox), fixed_);
      cancel_button_ = gtk_dialog_add_button(GTK_DIALOG(window_),
                                             GTK_STOCK_CANCEL,
                                             GTK_RESPONSE_CANCEL);
      ok_button_ = gtk_dialog_add_button(GTK_DIALOG(window_),
                                         GTK_STOCK_OK,
                                         GTK_RESPONSE_OK);
      gtk_dialog_set_default_response(GTK_DIALOG(window_), GTK_RESPONSE_OK);
      g_signal_connect(G_OBJECT(window_), "response",
                       G_CALLBACK(DialogResponseHandler), this);
      gtk_fixed_set_has_window(GTK_FIXED(fixed_), TRUE);
      widget_ = fixed_;
    } else {
      // details and main view only need a toplevel window.
      // TODO: buttons of details view shall be provided by view decorator.
      window_ = gtk_window_new(GTK_WINDOW_TOPLEVEL);
      gtk_container_add(GTK_CONTAINER(window_), fixed_);
      if (type_ == ViewHostInterface::VIEW_HOST_MAIN) {
        // Only main view may have transparent background.
        no_background = true;
        DisableWidgetBackground(window_);
        if (!decorated_)
          gtk_window_set_skip_taskbar_hint(GTK_WINDOW(window_), TRUE);
      }
      widget_ = window_;
    }

    gtk_widget_realize(GTK_WIDGET(window_));

    gtk_window_set_decorated(GTK_WINDOW(window_), decorated_);

    g_signal_connect(G_OBJECT(window_), "delete-event",
                     G_CALLBACK(gtk_widget_hide_on_delete), NULL);
    g_signal_connect_after(G_OBJECT(window_), "show",
                           G_CALLBACK(WindowShowHandler), this);
    g_signal_connect(G_OBJECT(window_), "hide",
                     G_CALLBACK(WindowHideHandler), this);
    g_signal_connect(G_OBJECT(window_), "configure-event",
                     G_CALLBACK(ConfigureHandler), this);
    g_signal_connect(G_OBJECT(fixed_), "size-request",
                     G_CALLBACK(FixedSizeRequestHandler), this);
    if (!native_drag_mode_) {
      g_signal_connect(G_OBJECT(window_), "motion-notify-event",
          G_CALLBACK(MotionHandler), this);
      g_signal_connect(G_OBJECT(window_), "button-release-event",
          G_CALLBACK(ButtonHandler), this);
    }

    // For details and main view, the view is bound to the toplevel window
    // instead of the GtkFixed widget, to get better performance and make the
    // input event mask effective.
    binder_ = new ViewWidgetBinder(view_, owner_, widget_, no_background);
  }

  void ViewCoordToNativeWidgetCoord(
      double x, double y, double *widget_x, double *widget_y) {
    double zoom = view_->GetGraphics()->GetZoom();
    if (widget_x)
      *widget_x = x * zoom;
    if (widget_y)
      *widget_y = y * zoom;
  }

  void AdjustWindowSize() {
    ASSERT(view_);

    double zoom = view_->GetGraphics()->GetZoom();
    int width = static_cast<int>(ceil(view_->GetWidth() * zoom));
    int height = static_cast<int>(ceil(view_->GetHeight() * zoom));

    // DLOG("New view size: %d %d", width, height);

    GtkRequisition req;
    gtk_widget_set_size_request(widget_, width, height);
    gtk_widget_size_request(window_, &req);

    // DLOG("Required window size: %d %d", req.width, req.height);

    if (gtk_window_get_resizable(GTK_WINDOW(window_))) {
      gtk_widget_set_size_request(widget_, -1, -1);
      gtk_window_resize(GTK_WINDOW(window_), req.width, req.height);
    } else {
      // The window is not resizable, set the size request instead.
      gtk_widget_set_size_request(window_, req.width, req.height);
    }

    // DLOG("End adjusting window size.");
  }

  static gboolean AdjustWindowSizeHandler(gpointer user_data) {
    Impl *impl = reinterpret_cast<Impl *>(user_data);
    impl->AdjustWindowSize();
    impl->adjust_window_size_source_ = 0;
    return FALSE;
  }

  void QueueResize() {
    if (adjust_window_size_source_)
      g_source_remove(adjust_window_size_source_);

    // Use G_PRIORITY_HIGH_IDLE + 15 to make sure that it'll be executed after
    // resize event and before redraw event.
    adjust_window_size_source_ =
        g_idle_add_full(G_PRIORITY_HIGH_IDLE + 15,
                        AdjustWindowSizeHandler, this, NULL);
  }

  void EnableInputShapeMask(bool enable) {
    if (binder_) {
      DLOG("SingleViewHost::EnableInputShapeMask(%s)",
           enable ? "true" : "false");
      binder_->EnableInputShapeMask(enable);
      QueueDraw();
    }
  }

  void QueueDraw() {
    ASSERT(GTK_IS_WIDGET(widget_));
    gtk_widget_queue_draw(widget_);
  }

  void SetResizable(ViewInterface::ResizableMode mode) {
    ASSERT(GTK_IS_WINDOW(window_));
    gtk_window_set_resizable(GTK_WINDOW(window_),
                             mode != ViewInterface::RESIZABLE_FALSE);
  }

  void SetCaption(const char *caption) {
    ASSERT(GTK_IS_WINDOW(window_));
    gtk_window_set_title(GTK_WINDOW(window_), caption);
  }

  void SetShowCaptionAlways(bool always) {
    // SingleViewHost will always show caption when window decorator is shown.
  }

  void SetCursor(int type) {
    GdkCursor *cursor = CreateCursor(type);
    if (widget_->window)
      gdk_window_set_cursor(widget_->window, cursor);
    if (cursor)
      gdk_cursor_unref(cursor);
  }

  void SetTooltip(const char *tooltip) {
    tooltip_->Show(tooltip);
  }

  bool ShowView(bool modal, int flags, Slot1<void, int> *feedback_handler) {
    ASSERT(view_);
    if (feedback_handler_)
      delete feedback_handler_;
    feedback_handler_ = feedback_handler;

    // Adjust the window size just before showing the view, to make sure that
    // the window size has correct default size when showing.
    LoadViewGeometricInfo();
    AdjustWindowSize();
    gtk_window_present(GTK_WINDOW(window_));

    // Main view and details view doesn't support modal.
    if (type_ == ViewHostInterface::VIEW_HOST_OPTIONS) {
      if (flags & ViewInterface::OPTIONS_VIEW_FLAG_OK)
        gtk_widget_show(ok_button_);
      else
        gtk_widget_hide(ok_button_);

      if (flags & ViewInterface::OPTIONS_VIEW_FLAG_CANCEL)
        gtk_widget_show(cancel_button_);
      else
        gtk_widget_hide(cancel_button_);

      if (modal)
        gtk_dialog_run(GTK_DIALOG(window_));
    }

    return true;
  }

  void CloseView() {
    gtk_widget_hide(window_);
  }

  std::string GetViewPositionOptionPrefix() {
    switch (type_) {
      case ViewHostInterface::VIEW_HOST_MAIN:
        return "main_view";
      case ViewHostInterface::VIEW_HOST_OPTIONS:
        return "options_view";
      case ViewHostInterface::VIEW_HOST_DETAILS:
        return "details_view";
      default:
        return "view";
    }
    return "";
  }

  void SaveViewGeometricInfo() {
    if (view_ && view_->GetGadget()) {
      OptionsInterface *opt = view_->GetGadget()->GetOptions();
      std::string opt_prefix = GetViewPositionOptionPrefix();
      opt->PutInternalValue((opt_prefix + "_x").c_str(),
                            Variant(win_x_));
      opt->PutInternalValue((opt_prefix + "_y").c_str(),
                            Variant(win_y_));

      // Don't save size and zoom information, it's conflict with view
      // decorator.
      /*
      ViewInterface::ResizableMode mode = view_->GetResizable();
      if (mode == ViewInterface::RESIZABLE_TRUE) {
        opt->PutInternalValue((opt_prefix + "_width").c_str(),
                              Variant(view_->GetWidth()));
        opt->PutInternalValue((opt_prefix + "_height").c_str(),
                              Variant(view_->GetHeight()));
      }

      if (mode != ViewInterface::RESIZABLE_FALSE) {
        opt->PutInternalValue((opt_prefix + "_zoom").c_str(),
                              Variant(view_->GetGraphics()->GetZoom()));
      }
      */
    }
  }

  void LoadViewGeometricInfo() {
    OptionsInterface *opt = view_->GetGadget()->GetOptions();
    std::string opt_prefix = GetViewPositionOptionPrefix();
    Variant vx = opt->GetInternalValue((opt_prefix + "_x").c_str());
    Variant vy = opt->GetInternalValue((opt_prefix + "_y").c_str());
    int x, y;
    if (vx.ConvertToInt(&x) && vy.ConvertToInt(&y)) {
      gtk_window_move(GTK_WINDOW(window_), x, y);
    }

    // Don't load size and zoom information, it's conflict with view
    // decorator.
    /*
    ViewInterface::ResizableMode mode = view_->GetResizable();
    if (mode == ViewInterface::RESIZABLE_TRUE) {
      double w, h;
      Variant vw = opt->GetInternalValue((opt_prefix + "_width").c_str());
      Variant vh = opt->GetInternalValue((opt_prefix + "_height").c_str());
      if (vw.ConvertToDouble(&w) && vh.ConvertToDouble(&h) &&
          view_->OnSizing(&w, &h)) {
        view_->SetSize(w, h);
      }
    }

    if (mode != ViewInterface::RESIZABLE_FALSE) {
      double zoom;
      Variant vzoom = opt->GetInternalValue((opt_prefix + "_zoom").c_str());
      if (vzoom.ConvertToDouble(&zoom) &&
          view_->GetGraphics()->GetZoom() != zoom) {
        view_->GetGraphics()->SetZoom(zoom);
      }
    }
    */
  }

  bool ShowContextMenu(int button) {
    ASSERT(view_);
    DLOG("Show context menu.");

    if (context_menu_)
      gtk_widget_destroy(context_menu_);

    context_menu_ = gtk_menu_new();
    MenuBuilder menu_builder(GTK_MENU_SHELL(context_menu_));

    // The return value is ignored, because the context menu will be shown
    // if there is any menu item added.
    view_->OnAddContextMenuItems(&menu_builder);

    if (menu_builder.ItemAdded()) {
      int gtk_button;
      switch (button) {
        case MouseEvent::BUTTON_LEFT: gtk_button = 1; break;
        case MouseEvent::BUTTON_MIDDLE: gtk_button = 2; break;
        case MouseEvent::BUTTON_RIGHT: gtk_button = 3; break;
        default: gtk_button = 3; break;
      }

      gtk_menu_popup(GTK_MENU(context_menu_),
                     NULL, NULL, NULL, NULL,
                     gtk_button, gtk_get_current_event_time());
      return true;
    }
    return false;
  }

  void BeginResizeDrag(int button, ViewInterface::HitTest hittest) {
    if (!gtk_window_get_resizable(GTK_WINDOW(window_)) ||
        !GTK_WIDGET_MAPPED(window_))
      return;

    if (on_resize_drag_signal_(button, hittest))
      return;

    GdkWindowEdge edge;
    // Determine the resize drag edge.
    if (hittest == ViewInterface::HT_LEFT) {
      edge = GDK_WINDOW_EDGE_WEST;
    } else if (hittest == ViewInterface::HT_RIGHT) {
      edge = GDK_WINDOW_EDGE_EAST;
    } else if (hittest == ViewInterface::HT_TOP) {
      edge = GDK_WINDOW_EDGE_NORTH;
    } else if (hittest == ViewInterface::HT_BOTTOM) {
      edge = GDK_WINDOW_EDGE_SOUTH;
    } else if (hittest == ViewInterface::HT_TOPLEFT) {
      edge = GDK_WINDOW_EDGE_NORTH_WEST;
    } else if (hittest == ViewInterface::HT_TOPRIGHT) {
      edge = GDK_WINDOW_EDGE_NORTH_EAST;
    } else if (hittest == ViewInterface::HT_BOTTOMLEFT) {
      edge = GDK_WINDOW_EDGE_SOUTH_WEST;
    } else if (hittest == ViewInterface::HT_BOTTOMRIGHT) {
      edge = GDK_WINDOW_EDGE_SOUTH_EAST;
    } else {
      // Unsupported hittest;
      return;
    }

    int gtk_button = (button == MouseEvent::BUTTON_LEFT ? 1 :
                      button == MouseEvent::BUTTON_MIDDLE ? 2 : 3);
    gint x, y;
    gdk_display_get_pointer(gdk_display_get_default(), NULL, &x, &y, NULL);
    gtk_window_begin_resize_drag(GTK_WINDOW(window_), edge, gtk_button,
                                 x, y, gtk_get_current_event_time());
  }

  void BeginMoveDrag(int button) {
    if (!GTK_WIDGET_MAPPED(window_))
      return;

    if (on_begin_move_drag_signal_(button))
      return;

    gint x, y;
    gdk_display_get_pointer(gdk_display_get_default(), NULL, &x, &y, NULL);
    if (native_drag_mode_) {
      int gtk_button = (button == MouseEvent::BUTTON_LEFT ? 1 :
                        button == MouseEvent::BUTTON_MIDDLE ? 2 : 3);
      gtk_window_begin_move_drag(GTK_WINDOW(window_), gtk_button,
                                 x, y, gtk_get_current_event_time());
    } else {
      gtk_window_get_position(GTK_WINDOW(window_), &win_x_, &win_y_);
      cursor_offset_x_ = x - win_x_;
      cursor_offset_y_ = y - win_y_;
      DLOG("handle move by the window(%p), cursor: %dx%d, window org: %dx%d",
          window_, cursor_offset_x_, cursor_offset_y_, win_x_, win_y_);
    }
  }

  void MoveDrag(int button) {
    on_move_drag_signal_(button);
  }

  void EndMoveDrag(int button) {
    if (!GTK_WIDGET_MAPPED(window_))
      return;
    on_end_move_drag_signal_(button);
  }

  static void WindowShowHandler(GtkWidget *widget, gpointer user_data) {
    DLOG("View window is shown.");
  }

  static void WindowHideHandler(GtkWidget *widget, gpointer user_data) {
    DLOG("View window is going to be hidden.");
    Impl *impl = reinterpret_cast<Impl *>(user_data);

    if (impl->view_) {
      impl->SaveViewGeometricInfo();
      if (impl->feedback_handler_ &&
          impl->type_ == ViewHostInterface::VIEW_HOST_DETAILS) {
        (*impl->feedback_handler_)(ViewInterface::DETAILS_VIEW_FLAG_NONE);
        delete impl->feedback_handler_;
        impl->feedback_handler_ = NULL;
      } else if (impl->type_ == ViewHostInterface::VIEW_HOST_MAIN &&
                 impl->remove_on_close_) {
        impl->view_->GetGadget()->RemoveMe(true);
      }
    }
  }

  static gboolean MotionHandler(GtkWidget *widget, GdkEventButton *event,
                                gpointer user_data) {
    Impl *impl = reinterpret_cast<Impl *>(user_data);
    if (impl->cursor_offset_x_ < 0 || impl->cursor_offset_y_ < 0)
      return FALSE;
    gdk_pointer_grab(impl->widget_->window, FALSE,
                     (GdkEventMask)(GDK_BUTTON_RELEASE_MASK |
                                    GDK_POINTER_MOTION_MASK |
                                    GDK_POINTER_MOTION_HINT_MASK),
                     NULL, NULL, event->time);
    int x = static_cast<int>(event->x_root) - impl->cursor_offset_x_;
    int y = static_cast<int>(event->y_root) - impl->cursor_offset_y_;
    gtk_window_move(GTK_WINDOW(widget), x, y);
    impl->MoveDrag(event->button);
    return TRUE;
  }

  static gboolean ButtonHandler(GtkWidget *widget, GdkEventButton *event,
                                gpointer user_data) {
    Impl *impl = reinterpret_cast<Impl *>(user_data);
    if (impl->cursor_offset_x_ < 0 || impl->cursor_offset_y_ < 0)
      return FALSE;
    DLOG("Handle button release event.");
    impl->EndMoveDrag(event->button);
    impl->cursor_offset_x_ = -1;
    impl->cursor_offset_y_ = -1;
    gdk_pointer_ungrab(event->time);
    return TRUE;
  }

  static gboolean ConfigureHandler(GtkWidget *widget, GdkEventConfigure *event,
                                   gpointer user_data) {
    Impl *impl = reinterpret_cast<Impl *>(user_data);
    gtk_window_get_position(GTK_WINDOW(widget), &impl->win_x_, &impl->win_y_);
    return FALSE;
  }

  static void DialogResponseHandler(GtkDialog *dialog, gint response,
                                    gpointer user_data) {
    DLOG("%s button clicked in options dialog.",
         response == GTK_RESPONSE_OK ? "Ok" :
         response == GTK_RESPONSE_CANCEL ? "Cancel" : "No");

    Impl *impl = reinterpret_cast<Impl *>(user_data);
    if (impl->feedback_handler_) {
      (*impl->feedback_handler_)(response == GTK_RESPONSE_OK ?
                                 ViewInterface::OPTIONS_VIEW_FLAG_OK :
                                 ViewInterface::OPTIONS_VIEW_FLAG_CANCEL);
      delete impl->feedback_handler_;
      impl->feedback_handler_ = NULL;
    }
    impl->CloseView();
  }

  static void FixedSizeRequestHandler(GtkWidget *widget,
                                      GtkRequisition *requisition,
                                      gpointer user_data) {
    // To make sure that user can resize the toplevel window freely.
    requisition->width = 1;
    requisition->height = 1;
  }

  ViewHostInterface::Type type_;
  SingleViewHost *owner_;
  ViewInterface *view_;

  GtkWidget *window_;
  GtkWidget *widget_;
  GtkWidget *fixed_;
  GtkWidget *context_menu_;

  // For options view.
  GtkWidget *ok_button_;
  GtkWidget *cancel_button_;

  Tooltip *tooltip_;
  ViewWidgetBinder *binder_;

  int debug_mode_;
  Slot1<void, int> *feedback_handler_;

  int adjust_window_size_source_;
  bool decorated_;
  bool remove_on_close_;
  bool native_drag_mode_;
  double zoom_;
  int win_x_;
  int win_y_;
  int cursor_offset_x_;
  int cursor_offset_y_;

  Signal2<bool, int, int> on_resize_drag_signal_;

  Signal1<bool, int> on_begin_move_drag_signal_;
  Signal1<void, int> on_end_move_drag_signal_;
  Signal1<void, int> on_move_drag_signal_;
  Signal0<void> on_dock_signal_;
  Signal0<void> on_undock_signal_;
  Signal0<void> on_expand_signal_;
  Signal0<void> on_unexpand_signal_;

  static const unsigned int kShowTooltipDelay = 500;
  static const unsigned int kHideTooltipDelay = 4000;
};

SingleViewHost::SingleViewHost(ViewHostInterface::Type type,
                               double zoom,
                               bool decorated,
                               bool remove_on_close,
                               bool native_drag_mode,
                               int debug_mode)
    : impl_(new Impl(type, this, zoom, decorated, remove_on_close,
                   native_drag_mode, debug_mode)) {
}

SingleViewHost::~SingleViewHost() {
  DLOG("SingleViewHost Dtor: %p", this);
  delete impl_;
  impl_ = NULL;
}

ViewHostInterface::Type SingleViewHost::GetType() const {
  return impl_->type_;
}

void SingleViewHost::Destroy() {
  delete this;
}

void SingleViewHost::SetView(ViewInterface *view) {
  impl_->SetView(view);
}

ViewInterface *SingleViewHost::GetView() const {
  return impl_->view_;
}

void *SingleViewHost::GetNativeWidget() const {
  return impl_->fixed_;
}

void SingleViewHost::ViewCoordToNativeWidgetCoord(
    double x, double y, double *widget_x, double *widget_y) const {
  impl_->ViewCoordToNativeWidgetCoord(x, y, widget_x, widget_y);
}

void SingleViewHost::QueueDraw() {
  impl_->QueueDraw();
}

void SingleViewHost::QueueResize() {
  impl_->QueueResize();
}

void SingleViewHost::EnableInputShapeMask(bool enable) {
  impl_->EnableInputShapeMask(enable);
}

void SingleViewHost::SetResizable(ViewInterface::ResizableMode mode) {
  impl_->SetResizable(mode);
}

void SingleViewHost::SetCaption(const char *caption) {
  impl_->SetCaption(caption);
}

void SingleViewHost::SetShowCaptionAlways(bool always) {
  impl_->SetShowCaptionAlways(always);
}

void SingleViewHost::SetCursor(int type) {
  impl_->SetCursor(type);
}

void SingleViewHost::SetTooltip(const char *tooltip) {
  impl_->SetTooltip(tooltip);
}

bool SingleViewHost::ShowView(bool modal, int flags,
                              Slot1<void, int> *feedback_handler) {
  return impl_->ShowView(modal, flags, feedback_handler);
}

void SingleViewHost::CloseView() {
  impl_->CloseView();
}

bool SingleViewHost::ShowContextMenu(int button) {
  return impl_->ShowContextMenu(button);
}

void SingleViewHost::BeginResizeDrag(int button,
                                     ViewInterface::HitTest hittest) {
  impl_->BeginResizeDrag(button, hittest);
}

void SingleViewHost::BeginMoveDrag(int button) {
  impl_->BeginMoveDrag(button);
}

void SingleViewHost::MoveDrag(int button) {
  impl_->MoveDrag(button);
}

void SingleViewHost::EndMoveDrag(int button) {
  impl_->EndMoveDrag(button);
}

void SingleViewHost::Alert(const char *message) {
  ShowAlertDialog(impl_->view_->GetCaption().c_str(), message);
}

bool SingleViewHost::Confirm(const char *message) {
  return ShowConfirmDialog(impl_->view_->GetCaption().c_str(), message);
}

std::string SingleViewHost::Prompt(const char *message,
                                   const char *default_value) {
  return ShowPromptDialog(impl_->view_->GetCaption().c_str(),
                          message, default_value);
}

int SingleViewHost::GetDebugMode() const {
  return impl_->debug_mode_;
}

GraphicsInterface *SingleViewHost::NewGraphics() const {
  return new CairoGraphics(impl_->zoom_);
}

Connection *SingleViewHost::ConnectOnResizeDrag(Slot2<bool, int, int> *slot) {
  return impl_->on_resize_drag_signal_.Connect(slot);
}

Connection *SingleViewHost::ConnectOnBeginMoveDrag(Slot1<bool, int> *slot) {
  return impl_->on_begin_move_drag_signal_.Connect(slot);
}

Connection *SingleViewHost::ConnectOnEndMoveDrag(Slot1<void, int> *slot) {
  return impl_->on_end_move_drag_signal_.Connect(slot);
}

Connection *SingleViewHost::ConnectOnMoveDrag(Slot1<void, int> *slot) {
  return impl_->on_move_drag_signal_.Connect(slot);
}

} // namespace gtk
} // namespace ggadget
