/*
  Copyright 2008 Google Inc.

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
#include <ggadget/messages.h>
#include "single_view_host.h"
#include "view_widget_binder.h"
#include "cairo_graphics.h"
#include "menu_builder.h"
#include "tooltip.h"
#include "utilities.h"

namespace ggadget {
namespace gtk {

static const int kStopDraggingTimeout = 200;
static const char kMainViewWindowRole[] = "Google-Gadgets";

class SingleViewHost::Impl {
 public:
  Impl(ViewHostInterface::Type type,
       SingleViewHost *owner,
       double zoom,
       bool decorated,
       bool remove_on_close,
       bool record_states,
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
      zoom_(zoom),
      decorated_(decorated),
      remove_on_close_(remove_on_close),
      record_states_(record_states),
      debug_mode_(debug_mode),
      stop_dragging_source_(0),
      win_x_(0),
      win_y_(0),
      win_width_(0),
      win_height_(0),
      is_keep_above_(false),
      move_dragging_(false),
      resize_dragging_(false),
      enable_signals_(true),
      feedback_handler_(NULL) {
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

    if (stop_dragging_source_)
      g_source_remove(stop_dragging_source_);
    stop_dragging_source_ = 0;

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
    if (view_ == view)
      return;

    Detach();

    if (view == NULL) {
      on_view_changed_signal_();
      return;
    }

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
      // buttons of details view shall be provided by view decorator.
      window_ = gtk_window_new(GTK_WINDOW_TOPLEVEL);
      gtk_container_add(GTK_CONTAINER(window_), fixed_);
      no_background = true;
      DisableWidgetBackground(window_);
      if (!decorated_)
        gtk_window_set_skip_taskbar_hint(GTK_WINDOW(window_), TRUE);
      widget_ = window_;
      if (type_ == ViewHostInterface::VIEW_HOST_MAIN)
        gtk_window_set_role(GTK_WINDOW(window_), kMainViewWindowRole);
    }

    gtk_window_set_decorated(GTK_WINDOW(window_), decorated_);
    gtk_window_set_gravity(GTK_WINDOW(window_), GDK_GRAVITY_STATIC);

    g_signal_connect(G_OBJECT(window_), "delete-event",
                     G_CALLBACK(gtk_widget_hide_on_delete), NULL);
    g_signal_connect(G_OBJECT(window_), "focus-in-event",
                     G_CALLBACK(FocusInHandler), this);
#ifdef _DEBUG
    g_signal_connect(G_OBJECT(window_), "focus-out-event",
                     G_CALLBACK(FocusOutHandler), this);
#endif
    g_signal_connect(G_OBJECT(window_), "enter-notify-event",
                     G_CALLBACK(EnterNotifyHandler), this);
    g_signal_connect(G_OBJECT(window_), "window-state-event",
                     G_CALLBACK(WindowStateHandler), this);
    g_signal_connect(G_OBJECT(window_), "show",
                     G_CALLBACK(WindowShowHandler), this);
    g_signal_connect_after(G_OBJECT(window_), "hide",
                     G_CALLBACK(WindowHideHandler), this);
    g_signal_connect(G_OBJECT(window_), "configure-event",
                     G_CALLBACK(ConfigureHandler), this);

    g_signal_connect(G_OBJECT(fixed_), "size-request",
                     G_CALLBACK(FixedSizeRequestHandler), this);

    // For details and main view, the view is bound to the toplevel window
    // instead of the GtkFixed widget, to get better performance and make the
    // input event mask effective.
    binder_ = new ViewWidgetBinder(view_, owner_, widget_, no_background);

    on_view_changed_signal_();
  }

  void ViewCoordToNativeWidgetCoord(
      double x, double y, double *widget_x, double *widget_y) const {
    double zoom = view_->GetGraphics()->GetZoom();
    if (widget_x)
      *widget_x = x * zoom;
    if (widget_y)
      *widget_y = y * zoom;
  }

  void NativeWidgetCoordToViewCoord(double x, double y,
                                    double *view_x, double *view_y) const {
    double zoom = view_->GetGraphics()->GetZoom();
    if (zoom == 0) return;
    if (view_x) *view_x = x / zoom;
    if (view_y) *view_y = y / zoom;
  }

  void AdjustWindowSize() {
    ASSERT(view_);

    double zoom = view_->GetGraphics()->GetZoom();
    int width = static_cast<int>(ceil(view_->GetWidth() * zoom));
    int height = static_cast<int>(ceil(view_->GetHeight() * zoom));

    DLOG("New view size: %d %d", width, height);

    GtkRequisition req;
    gtk_widget_set_size_request(widget_, width, height);
    gtk_widget_size_request(window_, &req);

    DLOG("Required window size: %d %d", req.width, req.height);

    if (gtk_window_get_resizable(GTK_WINDOW(window_))) {
      gtk_widget_set_size_request(widget_, -1, -1);
      gtk_window_resize(GTK_WINDOW(window_), req.width, req.height);
    } else {
      // The window is not resizable, set the size request instead.
      gtk_widget_set_size_request(window_, req.width, req.height);
    }

    // If the window is not mapped yet, then save the window size as initial
    // size.
    if (!GTK_WIDGET_MAPPED(window_)) {
      win_width_ = req.width;
      win_height_ = req.height;
    }

    DLOG("End adjusting window size.");
  }

  void QueueResize() {
    if (!resize_dragging_)
      AdjustWindowSize();
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
    GdkCursor *cursor = CreateCursor(type, view_->GetHitTest());
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
    ASSERT(window_);

    delete feedback_handler_;
    feedback_handler_ = feedback_handler;

    if (type_ == ViewHostInterface::VIEW_HOST_OPTIONS) {
      if (flags & ViewInterface::OPTIONS_VIEW_FLAG_OK)
        gtk_widget_show(ok_button_);
      else
        gtk_widget_hide(ok_button_);

      if (flags & ViewInterface::OPTIONS_VIEW_FLAG_CANCEL)
        gtk_widget_show(cancel_button_);
      else
        gtk_widget_hide(cancel_button_);
    }

    // Adjust the window size just before showing the view, to make sure that
    // the window size has correct default size when showing.
    AdjustWindowSize();

    if (record_states_)
      LoadWindowStates();
    else
      gtk_window_set_position(GTK_WINDOW(window_), GTK_WIN_POS_CENTER);

    gtk_window_present(GTK_WINDOW(window_));
    // Main view and details view doesn't support modal.
    if (type_ == ViewHostInterface::VIEW_HOST_OPTIONS && modal) {
      gtk_dialog_run(GTK_DIALOG(window_));
    }
    return true;
  }

  void CloseView() {
    ASSERT(window_);
    if (window_)
      gtk_widget_hide(window_);
  }

  void SetWindowPosition(int x, int y) {
    ASSERT(window_);
    if (window_)
      gtk_window_move(GTK_WINDOW(window_), x, y);
  }

  void SetKeepAbove(bool keep_above) {
    ASSERT(window_);
    if (window_) {
      bool shown = GTK_WIDGET_MAPPED(window_);
      enable_signals_ = false;
      if (shown) gtk_widget_hide(window_);
      gtk_window_set_keep_above(GTK_WINDOW(window_), keep_above);
      if (shown) gtk_widget_show(window_);
      enable_signals_ = true;
    }
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

  void SaveWindowStates() {
    if (view_ && view_->GetGadget()) {
      OptionsInterface *opt = view_->GetGadget()->GetOptions();
      std::string opt_prefix = GetViewPositionOptionPrefix();
      opt->PutInternalValue((opt_prefix + "_x").c_str(),
                            Variant(win_x_));
      opt->PutInternalValue((opt_prefix + "_y").c_str(),
                            Variant(win_y_));
      opt->PutInternalValue((opt_prefix + "_keep_above").c_str(),
                            Variant(is_keep_above_));
    }
    // Don't save size and zoom information, it's conflict with view
    // decorator.
  }

  void LoadWindowStates() {
    if (view_ && view_->GetGadget()) {
      OptionsInterface *opt = view_->GetGadget()->GetOptions();
      std::string opt_prefix = GetViewPositionOptionPrefix();

      // Restore keep above state first, otherwise it might be affect the
      // restoring of window position.
      Variant keep_above =
          opt->GetInternalValue((opt_prefix + "_keep_above").c_str());
      if (keep_above.type() == Variant::TYPE_BOOL &&
          VariantValue<bool>()(keep_above)) {
        SetKeepAbove(true);
      } else {
        SetKeepAbove(false);
      }

      // Restore window position.
      Variant vx = opt->GetInternalValue((opt_prefix + "_x").c_str());
      Variant vy = opt->GetInternalValue((opt_prefix + "_y").c_str());
      int x, y;
      if (vx.ConvertToInt(&x) && vy.ConvertToInt(&y)) {
        gtk_window_move(GTK_WINDOW(window_), x, y);
      } else {
        // Always place the window to the center of the screen if the window
        // position was not saved before.
        gtk_window_set_position(GTK_WINDOW(window_), GTK_WIN_POS_CENTER);
      }
    }
    // Don't load size and zoom information, it's conflict with view
    // decorator.
  }

  void KeepAboveMenuCallback(const char *, bool keep_above) {
    SetKeepAbove(keep_above);
  }

  bool ShowContextMenu(int button) {
    ASSERT(view_);
    DLOG("Show context menu.");

    if (context_menu_)
      gtk_widget_destroy(context_menu_);

    context_menu_ = gtk_menu_new();
    MenuBuilder menu_builder(GTK_MENU_SHELL(context_menu_));

    // If it returns true, then means that it's allowed to add additional menu
    // items.
    if (view_->OnAddContextMenuItems(&menu_builder) &&
        type_ == ViewHostInterface::VIEW_HOST_MAIN) {
      menu_builder.AddItem(
          GM_("MENU_ITEM_ALWAYS_ON_TOP"),
          is_keep_above_ ? MenuInterface::MENU_ITEM_FLAG_CHECKED : 0,
          NewSlot(this, &Impl::KeepAboveMenuCallback, !is_keep_above_),
          MenuInterface::MENU_ITEM_PRI_HOST);
    }

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
    ASSERT(window_);
    if (!gtk_window_get_resizable(GTK_WINDOW(window_)) ||
        !GTK_WIDGET_MAPPED(window_))
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

    if (on_begin_resize_drag_signal_(button, hittest))
      return;

    resize_dragging_ = true;
    binder_->BeginResizeDrag();

    if (stop_dragging_source_)
      g_source_remove(stop_dragging_source_);

    stop_dragging_source_ =
        g_timeout_add(kStopDraggingTimeout, StopDraggingTimeoutHandler, this);

    int gtk_button = (button == MouseEvent::BUTTON_LEFT ? 1 :
                      button == MouseEvent::BUTTON_MIDDLE ? 2 : 3);
    gint x, y;
    gdk_display_get_pointer(gdk_display_get_default(), NULL, &x, &y, NULL);
    gtk_window_begin_resize_drag(GTK_WINDOW(window_), edge, gtk_button,
                                 x, y, gtk_get_current_event_time());
  }

  void BeginMoveDrag(int button) {
    ASSERT(window_);
    if (!GTK_WIDGET_MAPPED(window_))
      return;

    if (on_begin_move_drag_signal_(button))
      return;

    move_dragging_ = true;

    if (stop_dragging_source_)
      g_source_remove(stop_dragging_source_);

    stop_dragging_source_ =
        g_timeout_add(kStopDraggingTimeout, StopDraggingTimeoutHandler, this);

    gint x, y;
    gdk_display_get_pointer(gdk_display_get_default(), NULL, &x, &y, NULL);
    int gtk_button = (button == MouseEvent::BUTTON_LEFT ? 1 :
                      button == MouseEvent::BUTTON_MIDDLE ? 2 : 3);
    gtk_window_begin_move_drag(GTK_WINDOW(window_), gtk_button,
                               x, y, gtk_get_current_event_time());
  }

  void StopDragging() {
    if (resize_dragging_) {
      DLOG("Stop resize dragging.");
      resize_dragging_ = false;
      binder_->EndResizeDrag();
      on_end_resize_drag_signal_();
      QueueResize();
    }
    if (move_dragging_) {
      DLOG("Stop move dragging.");
      move_dragging_ = false;
      on_end_move_drag_signal_();
    }
    if (stop_dragging_source_) {
      g_source_remove(stop_dragging_source_);
      stop_dragging_source_ = 0;
    }
  }

  // gtk signal handlers.
  static gboolean FocusInHandler(GtkWidget *widget, GdkEventFocus *event,
                                 gpointer user_data) {
    DLOG("FocusInHandler(%p)", widget);
    Impl *impl = reinterpret_cast<Impl *>(user_data);
    if (impl->enable_signals_ &&
        (impl->resize_dragging_ || impl->move_dragging_)) {
      impl->StopDragging();
    }
    return FALSE;
  }

#ifdef _DEBUG
  static gboolean FocusOutHandler(GtkWidget *widget, GdkEventFocus *event,
                                  gpointer user_data) {
    DLOG("FocusOutHandler(%p)", widget);
    return FALSE;
  }
#endif

  static gboolean EnterNotifyHandler(GtkWidget *widget, GdkEventCrossing *event,
                                     gpointer user_data) {
    DLOG("EnterNotifyHandler(%p): %d, %d", widget, event->mode, event->detail);
    Impl *impl = reinterpret_cast<Impl *>(user_data);
    if (impl->enable_signals_ &&
        (impl->resize_dragging_ || impl->move_dragging_)) {
      impl->StopDragging();
    }
    return FALSE;
  }

  static gboolean WindowStateHandler(GtkWidget *widget,
                                     GdkEventWindowState *event,
                                     gpointer user_data) {
    DLOG("WindowStateHandler(%p): 0x%x, 0x%x", widget,
         event->changed_mask, event->new_window_state);
    Impl *impl = reinterpret_cast<Impl *>(user_data);
    bool old_value = impl->is_keep_above_;
    impl->is_keep_above_ = (event->new_window_state & GDK_WINDOW_STATE_ABOVE);
    if (impl->record_states_ && old_value != impl->is_keep_above_)
      impl->SaveWindowStates();
    return FALSE;
  }

  static void WindowShowHandler(GtkWidget *widget, gpointer user_data) {
    DLOG("View window is going to be shown.");
    Impl *impl = reinterpret_cast<Impl *>(user_data);
    if (impl->view_ && impl->enable_signals_)
      impl->on_show_hide_signal_(true);
  }

  static void WindowHideHandler(GtkWidget *widget, gpointer user_data) {
    DLOG("View window is going to be hidden.");
    Impl *impl = reinterpret_cast<Impl *>(user_data);
    if (impl->view_ && impl->enable_signals_) {
      impl->on_show_hide_signal_(false);

      if (impl->feedback_handler_ &&
          impl->type_ == ViewHostInterface::VIEW_HOST_DETAILS) {
        (*impl->feedback_handler_)(ViewInterface::DETAILS_VIEW_FLAG_NONE);
        delete impl->feedback_handler_;
        impl->feedback_handler_ = NULL;
      } else if (impl->type_ == ViewHostInterface::VIEW_HOST_MAIN &&
                 impl->remove_on_close_ && impl->view_->GetGadget()) {
        impl->view_->GetGadget()->RemoveMe(true);
      }
    }
  }

  static gboolean ConfigureHandler(GtkWidget *widget, GdkEventConfigure *event,
                                   gpointer user_data) {
    Impl *impl = reinterpret_cast<Impl *>(user_data);
    if (impl->enable_signals_) {
      bool states_changed = false;
      if (impl->win_x_ != event->x || impl->win_y_ != event->y) {
        impl->win_x_ = event->x;
        impl->win_y_ = event->y;
        impl->on_moved_signal_(event->x, event->y);
        states_changed = true;
      }
      if (impl->win_width_ != event->width ||
          impl->win_height_ != event->height) {
        impl->win_width_ = event->width;
        impl->win_height_ = event->height;
        impl->on_resized_signal_(event->width, event->height);
        states_changed = true;
      }

      if (states_changed && impl->record_states_)
        impl->SaveWindowStates();
    }
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

  static gboolean StopDraggingTimeoutHandler(gpointer data) {
    Impl *impl = reinterpret_cast<Impl *>(data);
    if (impl->move_dragging_ || impl->resize_dragging_) {
      GdkDisplay *display = gtk_widget_get_display(impl->window_);
      GdkModifierType mod;
      gdk_display_get_pointer(display, NULL, NULL, NULL, &mod);
      int btn_mods = GDK_BUTTON1_MASK | GDK_BUTTON2_MASK | GDK_BUTTON3_MASK;
      if ((mod & btn_mods) == 0) {
        impl->stop_dragging_source_ = 0;
        impl->StopDragging();
        return FALSE;
      }
      return TRUE;
    }
    impl->stop_dragging_source_ = 0;
    return FALSE;
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

  double zoom_;
  bool decorated_;
  bool remove_on_close_;
  bool record_states_;

  int debug_mode_;
  int stop_dragging_source_;

  int win_x_;
  int win_y_;
  int win_width_;
  int win_height_;

  bool is_keep_above_;
  bool move_dragging_;
  bool resize_dragging_;
  bool enable_signals_;

  Slot1<void, int> *feedback_handler_;

  Signal0<void> on_view_changed_signal_;
  Signal1<void, bool> on_show_hide_signal_;

  Signal2<bool, int, int> on_begin_resize_drag_signal_;
  Signal2<void, int, int> on_resized_signal_;
  Signal0<void> on_end_resize_drag_signal_;

  Signal1<bool, int> on_begin_move_drag_signal_;
  Signal2<void, int, int> on_moved_signal_;
  Signal0<void> on_end_move_drag_signal_;

  static const unsigned int kShowTooltipDelay = 500;
  static const unsigned int kHideTooltipDelay = 4000;
};

SingleViewHost::SingleViewHost(ViewHostInterface::Type type,
                               double zoom,
                               bool decorated,
                               bool remove_on_close,
                               bool record_states,
                               int debug_mode)
    : impl_(new Impl(type, this, zoom, decorated, remove_on_close,
                     record_states, debug_mode)) {
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

void SingleViewHost::NativeWidgetCoordToViewCoord(
    double x, double y, double *view_x, double *view_y) const {
  impl_->NativeWidgetCoordToViewCoord(x, y, view_x, view_y);
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

GtkWidget *SingleViewHost::GetWindow() const {
  return impl_->window_;
}

void SingleViewHost::GetWindowPosition(int *x, int *y) const {
  if (x) *x = impl_->win_x_;
  if (y) *y = impl_->win_y_;
}

void SingleViewHost::SetWindowPosition(int x, int y) {
  impl_->SetWindowPosition(x, y);
}

void SingleViewHost::GetWindowSize(int *width, int *height) const {
  if (width) *width = impl_->win_width_;
  if (height) *height = impl_->win_height_;
}

bool SingleViewHost::IsKeepAbove() const {
  return impl_->is_keep_above_;
}

void SingleViewHost::SetKeepAbove(bool keep_above) {
  impl_->SetKeepAbove(keep_above);
}

bool SingleViewHost::IsVisible() const {
  return impl_->window_ && GTK_WIDGET_VISIBLE(impl_->window_);
}

Connection *SingleViewHost::ConnectOnViewChanged(Slot0<void> *slot) {
  return impl_->on_view_changed_signal_.Connect(slot);
}

Connection *SingleViewHost::ConnectOnShowHide(Slot1<void, bool> *slot) {
  return impl_->on_show_hide_signal_.Connect(slot);
}

Connection *SingleViewHost::ConnectOnBeginResizeDrag(
    Slot2<bool, int, int> *slot) {
  return impl_->on_begin_resize_drag_signal_.Connect(slot);
}

Connection *SingleViewHost::ConnectOnResized(Slot2<void, int, int> *slot) {
  return impl_->on_resized_signal_.Connect(slot);
}

Connection *SingleViewHost::ConnectOnEndResizeDrag(Slot0<void> *slot) {
  return impl_->on_end_resize_drag_signal_.Connect(slot);
}

Connection *SingleViewHost::ConnectOnBeginMoveDrag(Slot1<bool, int> *slot) {
  return impl_->on_begin_move_drag_signal_.Connect(slot);
}

Connection *SingleViewHost::ConnectOnMoved(Slot2<void, int, int> *slot) {
  return impl_->on_moved_signal_.Connect(slot);
}

Connection *SingleViewHost::ConnectOnEndMoveDrag(Slot0<void> *slot) {
  return impl_->on_end_move_drag_signal_.Connect(slot);
}

} // namespace gtk
} // namespace ggadget
