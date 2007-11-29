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

#include <ggadget/ggadget.h>
#include <ggadget/view.h>
#include <ggadget/xml_dom.h>
#include <ggadget/xml_http_request.h>
#include "gtk_view_host.h"
#include "gadget_view_widget.h"
#include "cairo_graphics.h"

namespace ggadget {

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

GtkViewHost::GtkViewHost(GadgetHostInterface *gadget_host,
                         GadgetHostInterface::ViewType type,
                         ScriptableInterface *prototype,
                         bool composited, bool useshapemask, 
			 double zoom, int debug_mode)
    : gadget_host_(gadget_host),
      view_(NULL),
      script_context_(NULL),
      gvw_(NULL),
      gfx_(NULL),
      onoptionchanged_connection_(NULL),
      details_window_(NULL),
      details_feedback_handler_(NULL) {
  if (type != GadgetHostInterface::VIEW_OLD_OPTIONS) {
    // Only xml based views have standalone script context.
    ScriptRuntimeInterface *script_runtime =
        gadget_host->GetScriptRuntime(GadgetHostInterface::JAVASCRIPT);
    script_context_ = script_runtime->CreateContext();
  }

  view_ = new View(this, prototype, gadget_host->GetElementFactory(),
                   debug_mode);

  OptionsInterface *options = gadget_host->GetOptions();
  if (type != GadgetHostInterface::VIEW_OLD_OPTIONS) {
    // Continue to initialize the script context.
    onoptionchanged_connection_ = options->ConnectOnOptionChanged(
        NewSlot(view_, &ViewInterface::OnOptionChanged));

    // Register global classes into script context.
    script_context_->RegisterClass(
        "DOMDocument", NewSlot(CreateDOMDocument));
    script_context_->RegisterClass(
        "XMLHttpRequest", NewSlot(this, &GtkViewHost::NewXMLHttpRequest));
    script_context_->RegisterClass(
        "DetailsView", NewSlot(DetailsView::CreateInstance));

    // Execute common.js to initialize global constants and compatibility
    // adapters.
    std::string common_js_contents;
    std::string common_js_path;
    FileManagerInterface *global_file_manager =
        gadget_host->GetGlobalFileManager();
    if (global_file_manager->GetFileContents(kCommonJS,
                                             &common_js_contents,
                                             &common_js_path)) {
      script_context_->Execute(common_js_contents.c_str(),
                               common_js_path.c_str(), 1);
    } else {
      LOG("Failed to load %s.", kCommonJS);
    }
  }

  gvw_ = GADGETVIEWWIDGET(GadgetViewWidget_new(this, zoom, 
                                               composited, useshapemask));
  gfx_ = new CairoGraphics(zoom);
}

GtkViewHost::~GtkViewHost() {
  if (onoptionchanged_connection_)
    onoptionchanged_connection_->Disconnect();

  CloseDetailsView();

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
  return CreateXMLHttpRequest(gadget_host_, script_context_);
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

void GtkViewHost::SetCursor(ElementInterface::CursorType type) {
  if (GTK_IS_WIDGET(gvw_)) {
    if (type == ElementInterface::CURSOR_ARROW) {
      // Use parent cursor in this case.
      gdk_window_set_cursor(GTK_WIDGET(gvw_)->window, NULL);
      return;
    }

    /*
    GdkCursorType gdk_type;
    switch (type) {
     case ElementInterface::CURSOR_ARROW:
       gdk_type = 
     case ElementInterface::CURSOR_IBEAM:
     case ElementInterface::CURSOR_WAIT:
     case ElementInterface::CURSOR_CROSS:
     case ElementInterface::CURSOR_UPARROW:
     case ElementInterface::CURSOR_SIZE:
     case ElementInterface::CURSOR_SIZENWSE:
     case ElementInterface::CURSOR_SIZENESW:
     case ElementInterface::CURSOR_SIZEWE:
     case ElementInterface::CURSOR_SIZENS:
     case ElementInterface::CURSOR_SIZEALL:
     case ElementInterface::CURSOR_NO:
     case ElementInterface::CURSOR_HAND:
     case ElementInterface::CURSOR_BUSY:
     case ElementInterface::CURSOR_HELP:
     default:
       return;
    };
    */

  }
}

struct DialogData {
  GtkDialog *dialog;
  ViewInterface *view;
};

static void OnDialogCancel(GtkButton *button, gpointer user_data) {
  DialogData *dialog_data = static_cast<DialogData *>(user_data);
  Event event(Event::EVENT_CANCEL);
  if (dialog_data->view->OnOtherEvent(&event)) {
    gtk_dialog_response(dialog_data->dialog, GTK_RESPONSE_CANCEL);
  }
}

static void OnDialogOK(GtkButton *button, gpointer user_data) {
  DialogData *dialog_data = static_cast<DialogData *>(user_data);
  Event event(Event::EVENT_OK);
  if (dialog_data->view->OnOtherEvent(&event)) {
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
  gtk_window_set_title(GTK_WINDOW(dialog), view_->GetCaption());
  gtk_dialog_run(GTK_DIALOG(dialog));
  gtk_widget_destroy(dialog);
}

bool GtkViewHost::Confirm(const char *message) {
  GtkWidget *dialog = gtk_message_dialog_new(NULL,
                                             GTK_DIALOG_MODAL,
                                             GTK_MESSAGE_QUESTION,
                                             GTK_BUTTONS_YES_NO,
                                             "%s", message);
  gtk_window_set_title(GTK_WINDOW(dialog), view_->GetCaption());
  gint result = gtk_dialog_run(GTK_DIALOG(dialog));
  gtk_widget_destroy(dialog);
  return result == GTK_RESPONSE_YES;
}

std::string GtkViewHost::Prompt(const char *message,
                                const char *default_value) {
  GtkWidget *dialog = gtk_dialog_new_with_buttons(
      view_->GetCaption(), NULL,
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

} // namespace ggadget
