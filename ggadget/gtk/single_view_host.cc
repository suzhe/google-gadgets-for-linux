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
#include <ggadget/file_manager_interface.h>
#include <ggadget/file_manager_factory.h>
#include <ggadget/logger.h>
#include <ggadget/options_interface.h>
#include <ggadget/gadget.h>
#include <ggadget/messages.h>
#include <ggadget/math_utils.h>
#include "single_view_host.h"
#include "view_widget_binder.h"
#include "cairo_graphics.h"
#include "menu_builder.h"
#include "tooltip.h"
#include "utilities.h"
#include "key_convert.h"

// It might not be necessary, because X server will grab the pointer
// implicitly when button is pressed.
// But using explicit mouse grabbing may avoid some issues by preventing some
// events from sending to client window when mouse is grabbed.
#define GRAB_POINTER_EXPLICITLY
namespace ggadget {
namespace gtk {

static const double kMinimumZoom = 0.5;
static const double kMaximumZoom = 2.0;
static const int kStopMoveDragTimeout = 200;
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
      initial_zoom_(zoom),
      decorated_(decorated),
      remove_on_close_(remove_on_close),
      record_states_(record_states),
      debug_mode_(debug_mode),
      stop_move_drag_source_(0),
      win_x_(0),
      win_y_(0),
      win_width_(0),
      win_height_(0),
      resize_view_zoom_(0),
      resize_view_width_(0),
      resize_view_height_(0),
      resize_win_x_(0),
      resize_win_y_(0),
      resize_win_width_(0),
      resize_win_height_(0),
      resize_button_(0),
      resize_mouse_x_(0),
      resize_mouse_y_(0),
      resize_width_mode_(0),
      resize_height_mode_(0),
      is_keep_above_(false),
      move_dragging_(false),
      enable_signals_(true),
      feedback_handler_(NULL),
      can_close_dialog_(false) {
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

    if (stop_move_drag_source_)
      g_source_remove(stop_move_drag_source_);
    stop_move_drag_source_ = 0;

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
      if (!decorated_) {
        gtk_window_set_skip_taskbar_hint(GTK_WINDOW(window_), TRUE);
        gtk_window_set_skip_pager_hint(GTK_WINDOW(window_), TRUE);
        gtk_window_set_role(GTK_WINDOW(window_), kMainViewWindowRole);
      }
      widget_ = window_;
    }

    gtk_window_set_decorated(GTK_WINDOW(window_), decorated_);
    gtk_window_set_gravity(GTK_WINDOW(window_), GDK_GRAVITY_STATIC);
    gtk_window_set_resizable(GTK_WINDOW(window_), TRUE);

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
    g_signal_connect(G_OBJECT(window_), "show",
                     G_CALLBACK(WindowShowHandler), this);
    g_signal_connect_after(G_OBJECT(window_), "hide",
                     G_CALLBACK(WindowHideHandler), this);
    g_signal_connect(G_OBJECT(window_), "configure-event",
                     G_CALLBACK(ConfigureHandler), this);

    // For resize drag.
    g_signal_connect(G_OBJECT(window_), "motion-notify-event",
                     G_CALLBACK(MotionNotifyHandler), this);
    g_signal_connect(G_OBJECT(window_), "button-release-event",
                     G_CALLBACK(ButtonReleaseHandler), this);

    g_signal_connect(G_OBJECT(fixed_), "size-request",
                     G_CALLBACK(FixedSizeRequestHandler), this);

    g_signal_connect(G_OBJECT(fixed_), "size-allocate",
                     G_CALLBACK(FixedSizeAllocateHandler), this);

    // For details and main view, the view is bound to the toplevel window
    // instead of the GtkFixed widget, to get better performance and make the
    // input event mask effective.
    binder_ = new ViewWidgetBinder(view_, owner_, widget_, no_background);

    gtk_widget_realize(window_);
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

    GtkRequisition req;
    gtk_widget_set_size_request(widget_, width, height);
    gtk_widget_size_request(window_, &req);

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

    DLOG("New window size: %d %d", req.width, req.height);
  }

  void QueueResize() {
    // When doing resize drag, MotionNotifyHandler() is in charge of resizing
    // the window, so don't do it here.
    if (resize_width_mode_ == 0 && resize_height_mode_ == 0)
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
    bool resizable = (mode == ViewInterface::RESIZABLE_TRUE ||
                      (mode == ViewInterface::RESIZABLE_ZOOM &&
                       type_ != ViewHostInterface::VIEW_HOST_OPTIONS));
    gtk_window_set_resizable(GTK_WINDOW(window_), resizable);
  }

  void SetCaption(const char *caption) {
    ASSERT(GTK_IS_WINDOW(window_));
    gtk_window_set_title(GTK_WINDOW(window_), caption);
  }

  void SetShowCaptionAlways(bool always) {
    // SingleViewHost will always show caption when window decorator is shown.
  }

  void SetCursor(int type) {
    // Don't change cursor if it's in resize dragging mode.
    if (resize_width_mode_ || resize_height_mode_)
      return;
    GdkCursor *cursor = CreateCursor(type, view_->GetHitTest());
    if (widget_->window)
      gdk_window_set_cursor(widget_->window, cursor);
    if (cursor)
      gdk_cursor_unref(cursor);
  }

  void SetTooltip(const char *tooltip) {
    tooltip_->Show(tooltip);
  }

  bool ShowView(bool modal, int flags, Slot1<bool, int> *feedback_handler) {
    ASSERT(view_);
    ASSERT(window_);

    delete feedback_handler_;
    feedback_handler_ = feedback_handler;

    SetGadgetWindowIcon(GTK_WINDOW(window_), view_->GetGadget());

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

    // Can't use gtk_widget_show_now() here, because in some cases, it'll cause
    // nested main loop and prevent ggl-gtk from being quitted.
    gtk_widget_show(window_);
    gtk_window_present(GTK_WINDOW(window_));
    gdk_window_raise(window_->window);

    // gtk_window_stick() must be called everytime.
    if (!decorated_)
      gtk_window_stick(GTK_WINDOW(window_));

    // Load window states again to make sure it's still correct
    // after the window is shown.
    if (record_states_)
      LoadWindowStates();

    // Main view and details view doesn't support modal.
    if (type_ == ViewHostInterface::VIEW_HOST_OPTIONS && modal) {
      can_close_dialog_ = false;
      while (!can_close_dialog_)
        gtk_dialog_run(GTK_DIALOG(window_));
      CloseView();
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
    if (window_) {
      win_x_ = x;
      win_y_ = y;
      gtk_window_move(GTK_WINDOW(window_), x, y);
      SaveWindowStates(true, false);
    }
  }

  void SetKeepAbove(bool keep_above) {
    ASSERT(window_);
    if (window_ && window_->window) {
      gtk_window_set_keep_above(GTK_WINDOW(window_), keep_above);
      if (is_keep_above_ != keep_above) {
        is_keep_above_ = keep_above;
        SaveWindowStates(false, true);
      }
    }
  }

  void SetWindowType(GdkWindowTypeHint type) {
    ASSERT(window_);
    if (window_ && window_->window) {
      gdk_window_set_type_hint(window_->window, type);
      gtk_window_set_keep_above(GTK_WINDOW(window_), is_keep_above_);
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

  void SaveWindowStates(bool save_position, bool save_keep_above) {
    if (record_states_ && view_ && view_->GetGadget()) {
      OptionsInterface *opt = view_->GetGadget()->GetOptions();
      std::string opt_prefix = GetViewPositionOptionPrefix();
      if (save_position) {
        opt->PutInternalValue((opt_prefix + "_x").c_str(),
                              Variant(win_x_));
        opt->PutInternalValue((opt_prefix + "_y").c_str(),
                              Variant(win_y_));
      }
      if (save_keep_above) {
        opt->PutInternalValue((opt_prefix + "_keep_above").c_str(),
                              Variant(is_keep_above_));
      }
    }
    // Don't save size and zoom information, it's conflict with view
    // decorator.
  }

  void LoadWindowStates() {
    if (record_states_ && view_ && view_->GetGadget()) {
      OptionsInterface *opt = view_->GetGadget()->GetOptions();
      std::string opt_prefix = GetViewPositionOptionPrefix();

      // Restore window position.
      Variant vx = opt->GetInternalValue((opt_prefix + "_x").c_str());
      Variant vy = opt->GetInternalValue((opt_prefix + "_y").c_str());
      int x, y;
      if (vx.ConvertToInt(&x) && vy.ConvertToInt(&y)) {
        win_x_ = x;
        win_y_ = y;
        gtk_window_move(GTK_WINDOW(window_), x, y);
      } else {
        // Always place the window to the center of the screen if the window
        // position was not saved before.
        gtk_window_set_position(GTK_WINDOW(window_), GTK_WIN_POS_CENTER);
      }

      // Restore keep above state.
      Variant keep_above =
          opt->GetInternalValue((opt_prefix + "_keep_above").c_str());
      if (keep_above.ConvertToBool(&is_keep_above_))
        SetKeepAbove(is_keep_above_);
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
          is_keep_above_ ? MenuInterface::MENU_ITEM_FLAG_CHECKED : 0, 0,
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
    if (!GTK_WIDGET_MAPPED(window_))
      return;

    // Determine the resize drag edge.
    resize_width_mode_ = 0;
    resize_height_mode_ = 0;
    if (hittest == ViewInterface::HT_LEFT) {
      resize_width_mode_ = -1;
    } else if (hittest == ViewInterface::HT_RIGHT) {
      resize_width_mode_ = 1;
    } else if (hittest == ViewInterface::HT_TOP) {
      resize_height_mode_ = -1;
    } else if (hittest == ViewInterface::HT_BOTTOM) {
      resize_height_mode_ = 1;
    } else if (hittest == ViewInterface::HT_TOPLEFT) {
      resize_height_mode_ = -1;
      resize_width_mode_ = -1;
    } else if (hittest == ViewInterface::HT_TOPRIGHT) {
      resize_height_mode_ = -1;
      resize_width_mode_ = 1;
    } else if (hittest == ViewInterface::HT_BOTTOMLEFT) {
      resize_height_mode_ = 1;
      resize_width_mode_ = -1;
    } else if (hittest == ViewInterface::HT_BOTTOMRIGHT) {
      resize_height_mode_ = 1;
      resize_width_mode_ = 1;
    } else {
      // Unsupported hittest;
      return;
    }

    if (on_begin_resize_drag_signal_(button, hittest)) {
      resize_width_mode_ = 0;
      resize_height_mode_ = 0;
      return;
    }

    resize_view_zoom_ = view_->GetGraphics()->GetZoom();
    resize_view_width_ = view_->GetWidth();
    resize_view_height_ = view_->GetHeight();
    resize_win_x_ = win_x_;
    resize_win_y_ = win_y_;
    resize_win_width_ = win_width_;
    resize_win_height_ = win_height_;
    resize_button_ = button;

    GdkEvent *event = gtk_get_current_event();
    if (!event || !gdk_event_get_root_coords(event, &resize_mouse_x_,
                                             &resize_mouse_y_)) {
      gint x, y;
      gdk_display_get_pointer(gdk_display_get_default(), NULL, &x, &y, NULL);
      resize_mouse_x_ = x;
      resize_mouse_y_ = y;
    }

    if (event)
      gdk_event_free(event);

#ifdef GRAB_POINTER_EXPLICITLY
    // Grabbing mouse explicitly is not necessary.
#ifdef _DEBUG
    GdkGrabStatus grab_status =
#endif
    gdk_pointer_grab(window_->window, FALSE,
                     (GdkEventMask)(GDK_BUTTON_RELEASE_MASK |
                                    GDK_BUTTON_MOTION_MASK |
                                    GDK_POINTER_MOTION_MASK |
                                    GDK_POINTER_MOTION_HINT_MASK),
                     NULL, NULL, gtk_get_current_event_time());
#ifdef _DEBUG
    DLOG("BeginResizeDrag: grab status: %d", grab_status);
#endif
#endif
  }

  void StopResizeDrag() {
    if (resize_width_mode_ || resize_height_mode_) {
      resize_width_mode_ = 0;
      resize_height_mode_ = 0;
#ifdef GRAB_POINTER_EXPLICITLY
      gdk_pointer_ungrab(gtk_get_current_event_time());
#endif
      QueueResize();
      on_end_resize_drag_signal_();
    }
  }

  void BeginMoveDrag(int button) {
    ASSERT(window_);
    if (!GTK_WIDGET_MAPPED(window_))
      return;

    if (on_begin_move_drag_signal_(button))
      return;

    move_dragging_ = true;

    if (stop_move_drag_source_)
      g_source_remove(stop_move_drag_source_);

    stop_move_drag_source_ =
        g_timeout_add(kStopMoveDragTimeout,
                      StopMoveDragTimeoutHandler, this);

    gint x, y;
    gdk_display_get_pointer(gdk_display_get_default(), NULL, &x, &y, NULL);
    int gtk_button = (button == MouseEvent::BUTTON_LEFT ? 1 :
                      button == MouseEvent::BUTTON_MIDDLE ? 2 : 3);
    gtk_window_begin_move_drag(GTK_WINDOW(window_), gtk_button,
                               x, y, gtk_get_current_event_time());
  }

  void StopMoveDrag() {
    if (move_dragging_) {
      DLOG("Stop move dragging.");
      move_dragging_ = false;
      on_end_move_drag_signal_();
    }
    if (stop_move_drag_source_) {
      g_source_remove(stop_move_drag_source_);
      stop_move_drag_source_ = 0;
    }
  }

  // gtk signal handlers.
  static gboolean FocusInHandler(GtkWidget *widget, GdkEventFocus *event,
                                 gpointer user_data) {
    DLOG("FocusInHandler(%p)", widget);
    Impl *impl = reinterpret_cast<Impl *>(user_data);
    if (impl->move_dragging_)
      impl->StopMoveDrag();
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
    if (impl->move_dragging_)
      impl->StopMoveDrag();
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
      if (impl->win_x_ != event->x || impl->win_y_ != event->y) {
        impl->win_x_ = event->x;
        impl->win_y_ = event->y;
        impl->on_moved_signal_(event->x, event->y);
        // SaveWindowStates() only saves window position.
        impl->SaveWindowStates(true, false);
      }
      if (impl->win_width_ != event->width ||
          impl->win_height_ != event->height) {
        impl->win_width_ = event->width;
        impl->win_height_ = event->height;
        impl->on_resized_signal_(event->width, event->height);
      }
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
      bool result = (*impl->feedback_handler_)(
          response == GTK_RESPONSE_OK ?
              ViewInterface::OPTIONS_VIEW_FLAG_OK :
              ViewInterface::OPTIONS_VIEW_FLAG_CANCEL);
      // 5.8 API allows the onok handler to cancel the default action.
      if (response != GTK_RESPONSE_OK || result) {
        delete impl->feedback_handler_;
        impl->feedback_handler_ = NULL;
        impl->can_close_dialog_ = true;
      }
    } else {
      impl->can_close_dialog_ = true;
    }
  }

  static gboolean MotionNotifyHandler(GtkWidget *widget, GdkEventMotion *event,
                                      gpointer user_data) {
    Impl *impl = reinterpret_cast<Impl *>(user_data);
    if (impl->resize_width_mode_ || impl->resize_height_mode_) {
      int button = ConvertGdkModifierToButton(event->state);
      if (button == impl->resize_button_) {
        double original_width =
            impl->resize_view_width_ * impl->resize_view_zoom_;
        double original_height =
            impl->resize_view_height_ * impl->resize_view_zoom_;
        double delta_x = event->x_root - impl->resize_mouse_x_;
        double delta_y = event->y_root - impl->resize_mouse_y_;
        double width = original_width;
        double height = original_height;
        double new_width = width + impl->resize_width_mode_ * delta_x;
        double new_height = height + impl->resize_height_mode_ * delta_y;

        if (impl->view_->GetResizable() == ViewInterface::RESIZABLE_TRUE) {
          double view_width = new_width / impl->resize_view_zoom_;
          double view_height = new_height / impl->resize_view_zoom_;
          if (impl->view_->OnSizing(&view_width, &view_height)) {
            DLOG("Resize view to: %lf %lf", view_width, view_height);
            impl->view_->SetSize(view_width, view_height);
            width = impl->view_->GetWidth() * impl->resize_view_zoom_;
            height = impl->view_->GetHeight() * impl->resize_view_zoom_;
          }
        } else if (impl->resize_view_width_ && impl->resize_view_height_) {
          double xzoom = new_width / impl->resize_view_width_;
          double yzoom = new_height / impl->resize_view_height_;
          double zoom = std::min(xzoom, yzoom);
          zoom = Clamp(zoom, kMinimumZoom, kMaximumZoom);
          DLOG("Zoom view to: %lf", zoom);
          impl->view_->GetGraphics()->SetZoom(zoom);
          width = impl->resize_view_width_ * zoom;
          height = impl->resize_view_height_ * zoom;
        }

        if (width != original_width || height != original_height) {
          delta_x = width - original_width;
          delta_y = height - original_height;
          int x = impl->resize_win_x_;
          int y = impl->resize_win_y_;
          if (impl->resize_width_mode_ == -1)
            x -= int(delta_x);
          if (impl->resize_height_mode_ == -1)
            y -= int(delta_y);
          int win_width = impl->resize_win_width_ + int(delta_x);
          int win_height = impl->resize_win_height_ + int(delta_y);
          gdk_window_move_resize(widget->window, x, y, win_width, win_height);
          gdk_window_process_updates(widget->window, TRUE);
          DLOG("Move resize window: x:%d, y:%d, w:%d, h:%d", x, y,
               win_width, win_height);
        }
        // Since motion hint is enabled, we must notify GTK that we're ready to
        // receive the next motion event.
#if GTK_CHECK_VERSION(2,12,0)
        gdk_event_request_motions(event); // requires version 2.12
#else
        int x, y;
        gdk_window_get_pointer(widget->window, &x, &y, NULL);
#endif
        return TRUE;
      } else {
        impl->StopResizeDrag();
      }
    }
    return FALSE;
  }

  static gboolean ButtonReleaseHandler(GtkWidget *widget, GdkEventButton *event,
                                       gpointer user_data) {
    Impl *impl = reinterpret_cast<Impl *>(user_data);
    if (impl->resize_width_mode_ || impl->resize_height_mode_) {
      impl->StopResizeDrag();
      return TRUE;
    }
    return FALSE;
  }

  static void FixedSizeRequestHandler(GtkWidget *widget,
                                      GtkRequisition *requisition,
                                      gpointer user_data) {
    Impl *impl = reinterpret_cast<Impl *>(user_data);
    if (impl->type_ == ViewHostInterface::VIEW_HOST_OPTIONS) {
      // Don't allow user to shrink options dialog.
      double zoom = impl->view_->GetGraphics()->GetZoom();
      double default_width, default_height;
      impl->view_->GetDefaultSize(&default_width, &default_height);
      requisition->width = static_cast<int>(ceil(default_width * zoom));
      requisition->height = static_cast<int>(ceil(default_height * zoom));
    } else {
      // To make sure that user can resize the toplevel window freely.
      requisition->width = 1;
      requisition->height = 1;
    }
  }

  // Only for options view.
  static void FixedSizeAllocateHandler(GtkWidget *widget,
                                       GtkAllocation *allocation,
                                       gpointer user_data) {
    Impl *impl = reinterpret_cast<Impl *>(user_data);
    DLOG("Size allocate(%d, %d)", allocation->width, allocation->height);
    if (impl->type_ == ViewHostInterface::VIEW_HOST_OPTIONS &&
        impl->view_->GetResizable() == ViewInterface::RESIZABLE_TRUE &&
        allocation->width > 1 && allocation->height > 1) {
      double zoom = impl->view_->GetGraphics()->GetZoom();
      double new_width = allocation->width / zoom;
      double new_height = allocation->height / zoom;
      if (new_width != impl->view_->GetWidth() ||
          new_height != impl->view_->GetHeight()) {
        if (impl->view_->OnSizing(&new_width, &new_height)) {
          DLOG("Resize options view to: %lf %lf", new_width, new_height);
          impl->view_->SetSize(new_width, new_height);
        }
      }
    }
  }

  static gboolean StopMoveDragTimeoutHandler(gpointer data) {
    Impl *impl = reinterpret_cast<Impl *>(data);
    if (impl->move_dragging_) {
      GdkDisplay *display = gtk_widget_get_display(impl->window_);
      GdkModifierType mod;
      gdk_display_get_pointer(display, NULL, NULL, NULL, &mod);
      int btn_mods = GDK_BUTTON1_MASK | GDK_BUTTON2_MASK | GDK_BUTTON3_MASK;
      if ((mod & btn_mods) == 0) {
        impl->stop_move_drag_source_ = 0;
        impl->StopMoveDrag();
        return FALSE;
      }
      return TRUE;
    }
    impl->stop_move_drag_source_ = 0;
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

  double initial_zoom_;
  bool decorated_;
  bool remove_on_close_;
  bool record_states_;

  int debug_mode_;
  int stop_move_drag_source_;

  int win_x_;
  int win_y_;
  int win_width_;
  int win_height_;

  // For resize drag
  double resize_view_zoom_;
  double resize_view_width_;
  double resize_view_height_;

  int resize_win_x_;
  int resize_win_y_;
  int resize_win_width_;
  int resize_win_height_;

  int resize_button_;
  gdouble resize_mouse_x_;
  gdouble resize_mouse_y_;

  // -1 to resize left, 1 to resize right.
  int resize_width_mode_;
  // -1 to resize top, 1 to resize bottom.
  int resize_height_mode_;
  // End of resize drag variants.

  bool is_keep_above_;
  bool move_dragging_;
  bool enable_signals_;

  Slot1<bool, int> *feedback_handler_;
  bool can_close_dialog_; // Only useful when a model dialog is running.

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
                              Slot1<bool, int> *feedback_handler) {
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

void SingleViewHost::Alert(const ViewInterface *view, const char *message) {
  ShowAlertDialog(view->GetCaption().c_str(), message);
}

bool SingleViewHost::Confirm(const ViewInterface *view, const char *message) {
  return ShowConfirmDialog(view->GetCaption().c_str(), message);
}

std::string SingleViewHost::Prompt(const ViewInterface *view,
                                   const char *message,
                                   const char *default_value) {
  return ShowPromptDialog(view->GetCaption().c_str(), message, default_value);
}

int SingleViewHost::GetDebugMode() const {
  return impl_->debug_mode_;
}

GraphicsInterface *SingleViewHost::NewGraphics() const {
  return new CairoGraphics(impl_->initial_zoom_);
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

void SingleViewHost::SetWindowType(GdkWindowTypeHint type) {
  impl_->SetWindowType(type);
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
