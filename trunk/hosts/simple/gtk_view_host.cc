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

#include "gtk_view_host.h"
#include "gadget_view_widget.h"
#include "ggadget/graphics/cairo_graphics.h"
#include "ggadget/view.h"

GtkViewHost::GtkViewHost(ggadget::GadgetHostInterface *gadget_host,
                         ggadget::GadgetHostInterface::ViewType type,
                         ggadget::OptionsInterface *options,
                         ggadget::ScriptableInterface *prototype)
    : gadget_host_(gadget_host),
      view_(NULL),
      script_context_(NULL),
      gvw_(NULL),
      gfx_(NULL) {
  ggadget::ScriptRuntimeInterface *script_runtime =
      gadget_host->GetScriptRuntime(ggadget::GadgetHostInterface::JAVASCRIPT);
  script_context_ = script_runtime->CreateContext();

  int debug_mode = ggadget::VariantValue<int>()(options->GetValue(
      ggadget::kOptionDebugMode));
  view_ = new ggadget::View(this, prototype, gadget_host->GetElementFactory(),
                            debug_mode);

  double zoom = ggadget::VariantValue<double>()(options->GetValue(
      ggadget::kOptionZoom));
  gvw_ = GADGETVIEWWIDGET(GadgetViewWidget_new(this, zoom));
  gfx_ = new ggadget::CairoGraphics(zoom);
}

GtkViewHost::~GtkViewHost() {
  delete view_;
  view_ = NULL;

  if (script_context_) {
    script_context_->Destroy();
    script_context_ = NULL;
  }

  delete gfx_;
  gfx_ = NULL;
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
