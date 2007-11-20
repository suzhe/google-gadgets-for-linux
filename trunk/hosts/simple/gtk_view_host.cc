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

#include <ggadget/gtk/cairo_graphics.h>
#include <ggadget/view.h>
#include <ggadget/xml_dom.h>
#include <ggadget/xml_http_request.h>
#include "gtk_view_host.h"
#include "gadget_view_widget.h"

using ggadget::ElementInterface;

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

GtkViewHost::GtkViewHost(ggadget::GadgetHostInterface *gadget_host,
                         ggadget::GadgetHostInterface::ViewType type,
                         ggadget::ScriptableInterface *prototype,
                         bool composited)
    : gadget_host_(gadget_host),
      view_(NULL),
      script_context_(NULL),
      gvw_(NULL),
      gfx_(NULL),
      onoptionchanged_connection_(NULL) {
  ggadget::ScriptRuntimeInterface *script_runtime =
      gadget_host->GetScriptRuntime(ggadget::GadgetHostInterface::JAVASCRIPT);
  script_context_ = script_runtime->CreateContext();

  ggadget::OptionsInterface *options = gadget_host->GetOptions();
  int debug_mode = ggadget::VariantValue<int>()(options->GetValue(
      ggadget::kOptionDebugMode));
  view_ = new ggadget::View(this, prototype, gadget_host->GetElementFactory(),
                            debug_mode);

  onoptionchanged_connection_ = options->ConnectOnOptionChanged(
      NewSlot(view_, &ggadget::ViewInterface::OnOptionChanged));

  // Register global classes into script context.
  script_context_->RegisterClass("DOMDocument",
                                 NewSlot(ggadget::CreateDOMDocument));
  script_context_->RegisterClass(
      "XMLHttpRequest", NewSlot(this, &GtkViewHost::NewXMLHttpRequest));

  // Execute common.js to initialize global constants and compatibility
  // adapters.
  std::string common_js_contents;
  std::string common_js_path;
  ggadget::FileManagerInterface *global_file_manager =
      gadget_host->GetGlobalFileManager();
  if (global_file_manager->GetFileContents(ggadget::kCommonJS,
                                           &common_js_contents,
                                           &common_js_path)) {
    script_context_->Execute(common_js_contents.c_str(),
                             common_js_path.c_str(), 1);
  } else {
    LOG("Failed to load %s.", ggadget::kCommonJS);
  }

  double zoom = ggadget::VariantValue<double>()(options->GetValue(
      ggadget::kOptionZoom));
  gvw_ = GADGETVIEWWIDGET(GadgetViewWidget_new(this, zoom, composited));
  gfx_ = new ggadget::CairoGraphics(zoom);
}

GtkViewHost::~GtkViewHost() {
  if (onoptionchanged_connection_)
    onoptionchanged_connection_->Disconnect();

  delete view_;
  view_ = NULL;

  if (script_context_) {
    script_context_->Destroy();
    script_context_ = NULL;
  }

  delete gfx_;
  gfx_ = NULL;
}

ggadget::XMLHttpRequestInterface *GtkViewHost::NewXMLHttpRequest() {
  return ggadget::CreateXMLHttpRequest(gadget_host_, script_context_);
}

void GtkViewHost::QueueDraw() {
  if (gvw_)
    gtk_widget_queue_draw(GTK_WIDGET(gvw_));
}

bool GtkViewHost::GrabKeyboardFocus() {
  if (gvw_) {
    gtk_widget_grab_focus(GTK_WIDGET(gvw_));
    return true;
  }
  return false;
}

#if 0
// TODO: This host should encapsulate all widget creating/switching jobs
// inside of it, so the interface of this method should be revised.
void GtkViewHost::SwitchWidget(GadgetViewWidget *gvw) {
  if (gvw_) {
    gvw_->host = NULL;
  }
  gvw_ = new_gvw;
}
#endif

void GtkViewHost::SetResizeable() {
  // TODO:
}

void GtkViewHost::SetCaption(const char *caption) {
  // TODO:
}

void GtkViewHost::SetShowCaptionAlways(bool always) {
  // TODO:
}

void GtkViewHost::SetCursor(ElementInterface::CursorType type) {
  if (gvw_) {
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
