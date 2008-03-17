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
  Impl(SingleViewHost *owner, double zoom, bool decorated,
       ViewInterface::DebugMode debug_mode)
    : owner_(owner),
      view_(NULL),
      gfx_(new CairoGraphics(zoom)),
      window_(NULL),
      widget_(NULL),
      context_menu_(NULL),
      ok_button_(NULL),
      cancel_button_(NULL),
      tooltip_(new Tooltip(kShowTooltipDelay, kHideTooltipDelay)),
      binder_(NULL),
      debug_mode_(debug_mode),
      feedback_handler_(NULL),
      current_view_width_(0),
      current_view_height_(0),
      current_zoom_(0),
      adjust_window_size_source_(0),
      decorated_(decorated) {
    ASSERT(owner);
    ASSERT(gfx_);
  }

  ~Impl() {
    Detach();

    delete tooltip_;
    tooltip_ = NULL;
    delete gfx_;
    gfx_ = NULL;
  }

  void Detach() {
    // To make sure that it won't be accessed anymore.
    view_ = NULL;

    if (adjust_window_size_source_)
      g_source_remove(adjust_window_size_source_);
    adjust_window_size_source_ = 0;

    delete feedback_handler_;
    feedback_handler_ = NULL;
    delete binder_;
    binder_ = NULL;
    if (window_)
      gtk_widget_destroy(window_);
    if (context_menu_)
      gtk_widget_destroy(context_menu_);
    window_ = NULL;
    widget_ = NULL;
    context_menu_ = NULL;
    ok_button_ = NULL;
    cancel_button_ = NULL;

    current_view_width_ = 0;
    current_view_height_ = 0;
    current_zoom_ = 1.0;
  }

  void SetView(ViewInterface *view) {
    Detach();
    if (view == NULL) return;

    view_ = view;
    ViewInterface::ViewType type = view_->GetType();

    bool no_background = false;
    // Initialize window and widget.
    if (type == ViewInterface::VIEW_OPTIONS ||
        type == ViewInterface::VIEW_OLD_OPTIONS) {
      window_ = gtk_dialog_new();
      widget_ = gtk_fixed_new();
      gtk_fixed_set_has_window(GTK_FIXED(widget_), TRUE);
      gtk_container_add(GTK_CONTAINER(GTK_DIALOG(window_)->vbox), widget_);
      cancel_button_ = gtk_dialog_add_button(GTK_DIALOG(window_),
                                             GTK_STOCK_CANCEL,
                                             GTK_RESPONSE_CANCEL);
      ok_button_ = gtk_dialog_add_button(GTK_DIALOG(window_),
                                         GTK_STOCK_OK,
                                         GTK_RESPONSE_OK);
      gtk_dialog_set_default_response(GTK_DIALOG(window_), GTK_RESPONSE_OK);
      g_signal_connect(G_OBJECT(window_), "response",
                       G_CALLBACK(DialogResponseHandler), this);
      gtk_container_set_border_width(GTK_CONTAINER(window_), 0);
      gtk_widget_show(widget_);
    } else if (type == ViewInterface::VIEW_DETAILS) {
      // For details view, we need a GtkFixed widget because Gecko browse
      // element requires it.
      // TODO: buttons of details view shall be provided by view decorator.
      window_ = gtk_window_new(GTK_WINDOW_TOPLEVEL);
      widget_ = gtk_fixed_new();
      gtk_fixed_set_has_window(GTK_FIXED(widget_), TRUE);
      gtk_container_add(GTK_CONTAINER(window_), widget_);
      gtk_container_set_border_width(GTK_CONTAINER(window_), 0);
      gtk_widget_show(widget_);
    } else {
      // For main view, we use a GtkWindow directly, without any child widget.
      window_ = gtk_window_new(GTK_WINDOW_TOPLEVEL);
      widget_ = window_;
      // Only main view may have transparent background.
      no_background = true;
    }

    gtk_window_set_decorated(GTK_WINDOW(window_), decorated_);

    g_signal_connect(G_OBJECT(window_), "delete-event",
                     G_CALLBACK(gtk_widget_hide_on_delete), NULL);
    g_signal_connect_after(G_OBJECT(window_), "show",
                           G_CALLBACK(WindowShowHandler), this);
    g_signal_connect(G_OBJECT(window_), "hide",
                     G_CALLBACK(WindowHideHandler), this);

    binder_ = new ViewWidgetBinder(gfx_, view_, owner_, widget_, no_background);
  }

  void ViewCoordToNativeWidgetCoord(
      double x, double y, double *widget_x, double *widget_y) {
    double zoom = gfx_->GetZoom();
    if (widget_x)
      *widget_x = x * zoom;
    if (widget_y)
      *widget_y = y * zoom;
  }

  void AdjustWindowSize() {
    ASSERT(view_);

    DLOG("Start adjusting window size.");
    int width = view_->GetWidth();
    int height = view_->GetHeight();
    double zoom = gfx_->GetZoom();

    width = static_cast<int>(ceil(width * zoom));
    height = static_cast<int>(ceil(height * zoom));

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

    DLOG("End adjusting window size.");
  }

  static gboolean AdjustWindowSizeHandler(gpointer user_data) {
    Impl *impl = reinterpret_cast<Impl *>(user_data);
    impl->AdjustWindowSize();
    impl->adjust_window_size_source_ = 0;
    gtk_widget_queue_draw(impl->widget_);
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
    GdkScreen *screen;
    gint x, y;
    gdk_display_get_pointer(gdk_display_get_default(), &screen, &x, &y, NULL);
    tooltip_->Show(tooltip, screen, x, y + 20);
  }

  bool ShowView(bool modal, int flags, Slot1<void, int> *feedback_handler) {
    ASSERT(view_);
    if (feedback_handler_)
      delete feedback_handler_;
    feedback_handler_ = feedback_handler;

    ViewInterface::ViewType type = view_->GetType();

    // Adjust the window size just before showing the view, to make sure that
    // the window size has correct default size when showing.
    AdjustWindowSize();
    // FIXME: Needs find a way to get the best screen position for showing the
    // view, especially for options and details view.
    //gtk_window_set_position(GTK_WINDOW(window_), GTK_WIN_POS_MOUSE);
    LoadViewPosition();
    gtk_widget_show(window_);

    // Main view and details view doesn't support modal.
    if (type == ViewInterface::VIEW_OPTIONS ||
        type == ViewInterface::VIEW_OLD_OPTIONS) {
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
    if (GTK_WIDGET_MAPPED(window_))
      SaveViewPosition();

    gtk_widget_hide(window_);
  }

  std::string GetViewPositionOptionPrefix() {
    switch (view_->GetType()) {
      case ViewInterface::VIEW_MAIN:
        return "main_view";
      case ViewInterface::VIEW_OPTIONS:
      case ViewInterface::VIEW_OLD_OPTIONS:
        return "options_view";
      case ViewInterface::VIEW_DETAILS:
        return "details_view";
      default:
        return "view";
    }
    return "";
  }

  void SaveViewPosition() {
    gint x, y;
    gtk_window_get_position(GTK_WINDOW(window_), &x, &y);
    OptionsInterface *options = view_->GetGadget()->GetOptions();
    std::string option_prefix = GetViewPositionOptionPrefix();
    options->PutInternalValue((option_prefix + "_pos_x").c_str(), Variant(x));
    options->PutInternalValue((option_prefix + "_pos_y").c_str(), Variant(y));
    DLOG("Save %s's position: %d %d", option_prefix.c_str(), x, y);
  }

  void LoadViewPosition() {
    Variant vx, vy;
    int x = 0, y = 0;
    OptionsInterface *options = view_->GetGadget()->GetOptions();
    std::string option_prefix = GetViewPositionOptionPrefix();
    vx = options->GetInternalValue((option_prefix + "_pos_x").c_str());
    vy = options->GetInternalValue((option_prefix + "_pos_y").c_str());

    if (vx.ConvertToInt(&x) && vy.ConvertToInt(&y)) {
      DLOG("Load %s's position: %d %d", option_prefix.c_str(), x, y);
      gtk_window_move(GTK_WINDOW(window_), x, y);
    }
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
        default: gtk_button = 3;
      }

      // FIXME: Don't know why using the real button number doesn't work for
      // submenu items.
      gtk_menu_popup(GTK_MENU(context_menu_),
                     NULL, NULL, NULL, NULL,
                     /*gtk_button*/ 0, gtk_get_current_event_time());
      return true;
    }
    return false;
  }

  static void WindowShowHandler(GtkWidget *widget, gpointer user_data) {
    DLOG("View window is shown.");
    Impl *impl = reinterpret_cast<Impl *>(user_data);
    if (impl->view_) {
      SimpleEvent e(Event::EVENT_OPEN);
      impl->view_->OnOtherEvent(e);
    }
  }

  static void WindowHideHandler(GtkWidget *widget, gpointer user_data) {
    DLOG("View window is going to be hidden.");
    Impl *impl = reinterpret_cast<Impl *>(user_data);

    if (impl->view_) {
      if (impl->feedback_handler_ &&
          impl->view_->GetType() == ViewInterface::VIEW_DETAILS) {
        (*impl->feedback_handler_)(ViewInterface::DETAILS_VIEW_FLAG_NONE);
        delete impl->feedback_handler_;
        impl->feedback_handler_ = NULL;
      }
      SimpleEvent e(Event::EVENT_CLOSE);
      impl->view_->OnOtherEvent(e);
    }
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

  SingleViewHost *owner_;
  ViewInterface *view_;
  CairoGraphics *gfx_;

  GtkWidget *window_;
  GtkWidget *widget_;
  GtkWidget *context_menu_;

  // For options view.
  GtkWidget *ok_button_;
  GtkWidget *cancel_button_;

  Tooltip *tooltip_;
  ViewWidgetBinder *binder_;

  ViewInterface::DebugMode debug_mode_;
  Slot1<void, int> *feedback_handler_;

  int current_view_width_;
  int current_view_height_;
  double current_zoom_;
  int adjust_window_size_source_;
  bool decorated_;

  static const unsigned int kShowTooltipDelay = 500;
  static const unsigned int kHideTooltipDelay = 4000;
};

SingleViewHost::SingleViewHost(double zoom, bool decorated,
                               ViewInterface::DebugMode debug_mode)
  : impl_(new Impl(this, zoom, decorated, debug_mode)) {
}

SingleViewHost::~SingleViewHost() {
  delete impl_;
  impl_ = NULL;
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

const GraphicsInterface *SingleViewHost::GetGraphics() const {
  return impl_->gfx_;
}

void *SingleViewHost::GetNativeWidget() const {
  return impl_->widget_;
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

ViewInterface::DebugMode SingleViewHost::GetDebugMode() const {
  return impl_->debug_mode_;
}

GtkWidget *SingleViewHost::GetToplevelWindow() const {
  return impl_->window_;
}

} // namespace gtk
} // namespace ggadget
