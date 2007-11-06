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

#include <ggadget/graphics/cairo_graphics.h>
#include <ggadget/view.h>
#include <ggadget/xml_dom.h>
#include <ggadget/xml_http_request.h>
#include "gtk_view_host.h"
#include "gadget_view_widget.h"

GtkViewHost::GtkViewHost(ggadget::GadgetHostInterface *gadget_host,
                         ggadget::GadgetHostInterface::ViewType type,
                         ggadget::OptionsInterface *options,
                         ggadget::ScriptableInterface *prototype)
    : gadget_host_(gadget_host),
      view_(NULL),
      script_context_(NULL),
      gvw_(NULL),
      gfx_(NULL),
      onoptionchanged_connection_(NULL) {
  ggadget::ScriptRuntimeInterface *script_runtime =
      gadget_host->GetScriptRuntime(ggadget::GadgetHostInterface::JAVASCRIPT);
  script_context_ = script_runtime->CreateContext();

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
  gvw_ = GADGETVIEWWIDGET(GadgetViewWidget_new(this, zoom));
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
