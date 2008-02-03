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

#include <sys/time.h>

#include <ggadget/file_manager_interface.h>
#include <ggadget/gadget_consts.h>
#include <ggadget/logger.h>
#include <ggadget/options_interface.h>
#include <ggadget/script_context_interface.h>
#include <ggadget/script_runtime_interface.h>
#include <ggadget/xml_http_request.h>
#include "gtk_view_host.h"
#include "cairo_graphics.h"
#include "gadget_view_widget.h"

namespace ggadget {
namespace gtk {

/*struct {
  ElementInterface::CursorType type,
  GdkCursorType gdk_type
} CursorTypeMapping;

// Ordering in this array must match the declaration in ElementInterface.
static const CursorTypeMapping[] = {
  { ElementInterface::CURSOR_ARROW, GDK_ARROW }, // need special handling
  { ElementInterface::CURSOR_IBEAM, GDK_XTERM},
  { ElementInterface::CURSOR_WAIT, GDK_WATCH},
  { ElementInterface::CURSOR_CROSS, GDK_CROSS},
  { ElementInterface::CURSOR_UPARROW, GDK_SB_UP_ARROW},
  { ElementInterface::CURSOR_SIZE, GDK_SIZING},
  { ElementInterface::CURSOR_SIZENWSE, GDK_ARROW}, // need special handling
  { ElementInterface::CURSOR_SIZENESW, GDK_ARROW}, // need special handling
  { ElementInterface::CURSOR_SIZEWE, GDK_ARROW}, // need special handling
  { ElementInterface::CURSOR_SIZENS, GDK_ARROW}, // need special handling
  { ElementInterface::CURSOR_SIZEALL, },
  { ElementInterface::CURSOR_NO, GDK_X_CURSOR},
  { ElementInterface::CURSOR_HAND, GDK_HAND1},
  { ElementInterface::CURSOR_BUSY, },
  { ElementInterface::CURSOR_HELP, }
};
*/

GtkViewHost::GtkViewHost(GtkGadgetHost *gadget_host,
                         GadgetHostInterface::ViewType type,
                         ViewInterface *view,
                         bool composited, bool useshapemask,
                         double zoom)
    : gadget_host_(gadget_host),
      view_(view),
      script_context_(NULL),
      gvw_(NULL),
      gfx_(NULL),
      onoptionchanged_connection_(NULL),
      tooltip_timer_(0),
      tooltip_window_(NULL),
      tooltip_label_(NULL),
      details_window_(NULL),
      details_feedback_handler_(NULL) {
  if (type != GadgetHostInterface::VIEW_OLD_OPTIONS) {
    // Only xml based views have standalone script context.
    ScriptRuntimeInterface *script_runtime =
        gadget_host->GetScriptRuntime(GadgetHostInterface::JAVASCRIPT);
    script_context_ = script_runtime->CreateContext();
  }

  view_->AttachHost(this);

  if (type != GadgetHostInterface::VIEW_OLD_OPTIONS) {
    OptionsInterface *options = gadget_host->GetOptions();
    // Continue to initialize the script context.
    onoptionchanged_connection_ = options->ConnectOnOptionChanged(
        NewSlot(view_, &ViewInterface::OnOptionChanged));
  }

  gvw_ = GADGETVIEWWIDGET(GadgetViewWidget_new(this, zoom,
                                               composited, useshapemask));
  gfx_ = new CairoGraphics(zoom);
}

GtkViewHost::~GtkViewHost() {
  if (onoptionchanged_connection_)
    onoptionchanged_connection_->Disconnect();

  CloseDetailsView();
  HideTooltip(0);
  gadget_host_->DestroyContextMenu();

  delete view_;
  view_ = NULL;

  if (script_context_) {
    script_context_->Destroy();
    script_context_ = NULL;
  }

  delete gfx_;
  gfx_ = NULL;
}

XMLHttpRequestInterface *GtkViewHost::NewXMLHttpRequest() {
  return CreateXMLHttpRequest(gadget_host_->GetXMLParser());
}

void *GtkViewHost::GetNativeWidget() {
  return gvw_;
}

void GtkViewHost::ViewCoordToNativeWidgetCoord(
    double x, double y, double *widget_x, double *widget_y) {
  double zoom = gfx_->GetZoom();
  if (widget_x)
    *widget_x = x * zoom;
  if (widget_y)
    *widget_y = y * zoom;
}

void GtkViewHost::QueueDraw() {
  // Use GTK_IS_WIDGET instead of checking for pointer since the widget
  // might be destroyed on shutdown but the pointer is non-NULL.
  if (GTK_IS_WIDGET(gvw_)) {
    gtk_widget_queue_draw(GTK_WIDGET(gvw_));
  }
}

bool GtkViewHost::GrabKeyboardFocus() {
  if (GTK_IS_WIDGET(gvw_)) {
    gtk_widget_grab_focus(GTK_WIDGET(gvw_));
    return true;
  }
  return false;
}

#if 0
// TODO: This host should encapsulate all widget creating/switching jobs
// inside of it, so the interface of this method should be revised.
void GtkViewHost::SwitchWidget(GadgetViewWidget *gvw) {
  if (GTK_IS_WIDGET(gvw_)) {
    gvw_->host = NULL;
  }
  gvw_ = new_gvw;
}
#endif

void GtkViewHost::SetResizable(ViewInterface::ResizableMode mode) {
  // TODO:
}

void GtkViewHost::SetCaption(const char *caption) {
  // TODO:
}

void GtkViewHost::SetShowCaptionAlways(bool always) {
  // TODO:
}

void GtkViewHost::SetCursor(CursorType type) {
  if (GTK_IS_WIDGET(gvw_)) {
    if (type == CURSOR_ARROW) {
      // Use parent cursor in this case.
      gdk_window_set_cursor(GTK_WIDGET(gvw_)->window, NULL);
      return;
    }

    /*
    GdkCursorType gdk_type;
    switch (type) {
     case CURSOR_ARROW:
       gdk_type =
     case CURSOR_IBEAM:
     case CURSOR_WAIT:
     case CURSOR_CROSS:
     case CURSOR_UPARROW:
     case CURSOR_SIZE:
     case CURSOR_SIZENWSE:
     case CURSOR_SIZENESW:
     case CURSOR_SIZEWE:
     case CURSOR_SIZENS:
     case CURSOR_SIZEALL:
     case CURSOR_NO:
     case CURSOR_HAND:
     case CURSOR_BUSY:
     case CURSOR_HELP:
     default:
       return;
    };
    */

  }
}

void GtkViewHost::SetTooltip(const char *tooltip) {
  HideTooltip(0);
  if (tooltip && *tooltip) {
    tooltip_ = tooltip;
    tooltip_timer_ = GetGlobalMainLoop()->AddTimeoutWatch(
        kShowTooltipDelay,
        new WatchCallbackSlot(NewSlot(this, &GtkViewHost::ShowTooltip)));
  }
}

static gboolean PaintTooltipWindow(GtkWidget *widget, GdkEventExpose *event,
                                   gpointer user_data) {
  GtkRequisition req;
  gtk_widget_size_request(widget, &req);
  gtk_paint_flat_box(widget->style, widget->window,
                     GTK_STATE_NORMAL, GTK_SHADOW_OUT,
                     NULL, widget, "tooltip",
                     0, 0, req.width, req.height);
  return FALSE;
}

bool GtkViewHost::ShowTooltip(int timer_id) {
  // This method can only be called by the timer.
  tooltip_window_ = gtk_window_new(GTK_WINDOW_POPUP);
  // gtk_window_set_type_hint(GTK_WINDOW(tooltip_window_),
  //                          GDK_WINDOW_TYPE_HINT_TOOLTIP);
  gtk_widget_set_app_paintable(tooltip_window_, TRUE);
  gtk_window_set_resizable(GTK_WINDOW(tooltip_window_), FALSE);
  gtk_container_set_border_width(GTK_CONTAINER(tooltip_window_), 4);
  // TODO: Is there better way?
  GdkColor color = { 0, 0xffff, 0xffff, 0xb000 };
  gtk_widget_modify_bg(tooltip_window_, GTK_STATE_NORMAL, &color);
  g_signal_connect(tooltip_window_, "expose_event",
                   G_CALLBACK(PaintTooltipWindow), NULL);
  g_signal_connect(tooltip_window_, "destroy",
                   G_CALLBACK(gtk_widget_destroyed), &tooltip_window_);

  tooltip_label_ = gtk_label_new(tooltip_.c_str());
  gtk_label_set_line_wrap(GTK_LABEL(tooltip_label_), TRUE);
  gtk_misc_set_alignment(GTK_MISC(tooltip_label_), 0.5, 0.5);
  gtk_container_add(GTK_CONTAINER(tooltip_window_), tooltip_label_);

  gint x, y;
  gdk_display_get_pointer(gdk_display_get_default(), NULL, &x, &y, NULL);
  gtk_window_move(GTK_WINDOW(tooltip_window_), x, y + 20);
  gtk_widget_show_all(tooltip_window_);

  tooltip_timer_ = GetGlobalMainLoop()->AddTimeoutWatch(
      kHideTooltipDelay,
      new WatchCallbackSlot(NewSlot(this, &GtkViewHost::HideTooltip)));
  return false;
}

bool GtkViewHost::HideTooltip(int timer_id) {
  // This method may be called by the timer, or directly from this class.
  if (tooltip_timer_) {
    GetGlobalMainLoop()->RemoveWatch(tooltip_timer_);
    tooltip_timer_ = 0;
  }
  if (tooltip_window_) {
    gtk_widget_destroy(tooltip_window_);
    tooltip_window_ = NULL;
  }
  return false;
}

struct DialogData {
  GtkDialog *dialog;
  ViewInterface *view;
};

static void OnDialogCancel(GtkButton *button, gpointer user_data) {
  DialogData *dialog_data = static_cast<DialogData *>(user_data);
  SimpleEvent event(Event::EVENT_CANCEL);
  if (dialog_data->view->OnOtherEvent(event, NULL)) {
    gtk_dialog_response(dialog_data->dialog, GTK_RESPONSE_CANCEL);
  }
}

static void OnDialogOK(GtkButton *button, gpointer user_data) {
  DialogData *dialog_data = static_cast<DialogData *>(user_data);
  SimpleEvent event(Event::EVENT_OK);
  if (dialog_data->view->OnOtherEvent(event, NULL)) {
    gtk_dialog_response(dialog_data->dialog, GTK_RESPONSE_OK);
  }
}

void GtkViewHost::RunDialog() {
  GtkDialog *dialog = GTK_DIALOG(gtk_dialog_new_with_buttons("Options", NULL,
                                                             GTK_DIALOG_MODAL,
                                                             NULL));
  DialogData dialog_data = { dialog, view_ };

  GtkWidget *cancel_button = gtk_dialog_add_button(dialog,
                                                   GTK_STOCK_CANCEL,
                                                   GTK_RESPONSE_CANCEL);
  GtkWidget *ok_button = gtk_dialog_add_button(dialog,
                                               GTK_STOCK_OK,
                                               GTK_RESPONSE_OK);
  gtk_widget_grab_default(ok_button);
  g_signal_connect(cancel_button, "clicked",
                   G_CALLBACK(OnDialogCancel), &dialog_data);
  g_signal_connect(ok_button, "clicked",
                   G_CALLBACK(OnDialogOK), &dialog_data);

  gtk_container_add(GTK_CONTAINER(dialog->vbox), GTK_WIDGET(gvw_));
  gtk_widget_show_all(GTK_WIDGET(dialog));
  gtk_dialog_run(dialog);
  gtk_widget_destroy(GTK_WIDGET(dialog));
}

void GtkViewHost::ShowInDetailsView(const char *title, int flags,
                                    Slot1<void, int> *feedback_handler) {
  CloseDetailsView();
  details_feedback_handler_ = feedback_handler;
  details_window_ = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  g_signal_connect(details_window_, "destroy",
                   G_CALLBACK(OnDetailsViewDestroy), this);
  gtk_window_set_title(GTK_WINDOW(details_window_), title);
  // TODO: flags.
  GtkBox *vbox = GTK_BOX(gtk_vbox_new(FALSE, 0));
  gtk_container_add(GTK_CONTAINER(details_window_), GTK_WIDGET(vbox));
  gtk_box_pack_start(vbox, GTK_WIDGET(gvw_), TRUE, TRUE, 0);
  gtk_widget_show_all(details_window_);
}

void GtkViewHost::OnDetailsViewDestroy(GtkObject *object, gpointer user_data) {
  GtkViewHost *this_p = static_cast<GtkViewHost *>(user_data);
  if (this_p->details_window_) {
    if (this_p->details_feedback_handler_) {
      (*this_p->details_feedback_handler_)(DETAILS_VIEW_FLAG_NONE);
      delete this_p->details_feedback_handler_;
    }
    this_p->details_window_ = NULL;
  }
}

void GtkViewHost::CloseDetailsView() {
  if (details_window_) {
    gtk_widget_destroy(details_window_);
    details_window_ = NULL;
  }
}

void GtkViewHost::Alert(const char *message) {
  GtkWidget *dialog = gtk_message_dialog_new(NULL,
                                             GTK_DIALOG_MODAL,
                                             GTK_MESSAGE_INFO,
                                             GTK_BUTTONS_OK,
                                             "%s", message);
  gtk_window_set_title(GTK_WINDOW(dialog), view_->GetCaption().c_str());
  gtk_dialog_run(GTK_DIALOG(dialog));
  gtk_widget_destroy(dialog);
}

bool GtkViewHost::Confirm(const char *message) {
  GtkWidget *dialog = gtk_message_dialog_new(NULL,
                                             GTK_DIALOG_MODAL,
                                             GTK_MESSAGE_QUESTION,
                                             GTK_BUTTONS_YES_NO,
                                             "%s", message);
  gtk_window_set_title(GTK_WINDOW(dialog), view_->GetCaption().c_str());
  gint result = gtk_dialog_run(GTK_DIALOG(dialog));
  gtk_widget_destroy(dialog);
  return result == GTK_RESPONSE_YES;
}

std::string GtkViewHost::Prompt(const char *message,
                                const char *default_value) {
  GtkWidget *dialog = gtk_dialog_new_with_buttons(
      view_->GetCaption().c_str(), NULL,
      static_cast<GtkDialogFlags>(GTK_DIALOG_MODAL | GTK_DIALOG_NO_SEPARATOR),
      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
      GTK_STOCK_OK, GTK_RESPONSE_OK,
      NULL);
  gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
  gtk_window_set_skip_taskbar_hint(GTK_WINDOW(dialog), TRUE);
  gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);

  GtkWidget *image = gtk_image_new_from_stock(GTK_STOCK_DIALOG_QUESTION,
                                              GTK_ICON_SIZE_DIALOG);
  GtkWidget *label = gtk_label_new(message);
  gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
  gtk_label_set_selectable(GTK_LABEL(label), TRUE);
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 1.0);
  GtkWidget *entry = gtk_entry_new();
  if (default_value)
    gtk_entry_set_text(GTK_ENTRY(entry), default_value);

  GtkWidget *hbox = gtk_hbox_new(FALSE, 12);
  GtkWidget *vbox = gtk_vbox_new(FALSE, 12);
  gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), entry, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), image, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox, FALSE, FALSE, 0);

  gtk_container_set_border_width(GTK_CONTAINER(hbox), 10);
  gtk_container_set_border_width(
      GTK_CONTAINER(GTK_DIALOG(dialog)->action_area), 10);

  gtk_widget_show_all(dialog);
  gint result = gtk_dialog_run(GTK_DIALOG(dialog));
  std::string text;
  if (result == GTK_RESPONSE_OK)
    text = gtk_entry_get_text(GTK_ENTRY(entry));
  gtk_widget_destroy(dialog);
  return text;
}

void GtkViewHost::ChangeZoom(double zoom) {
  gfx_->SetZoom(zoom);
  view_->MarkRedraw();
}

} // namespace gtk
} // namespace ggadget
