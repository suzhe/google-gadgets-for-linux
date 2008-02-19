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
#include <ggadget/script_runtime_manager.h>
#include "qt_view_host.h"
#include "qt_graphics.h"
#include "qt_gadget_widget.h"

namespace ggadget {
namespace qt {

QtViewHost::QtViewHost(QtGadgetHost *gadget_host,
                       GadgetHostInterface::ViewType type,
                       ViewInterface *view,
                       bool composited, bool useshapemask,
                       double zoom)
    : gadget_host_(gadget_host),
      view_(view),
      script_context_(NULL),
      widget_(NULL),
      graphics_(NULL),
      onoptionchanged_connection_(NULL),
      details_feedback_handler_(NULL) {
  if (type != GadgetHostInterface::VIEW_OLD_OPTIONS) {
    // Only xml based views have standalone script context.
    script_context_ = ScriptRuntimeManager::get()->CreateScriptContext("js");
    VERIFY_M(script_context_, ("Failed to create ScriptContext instance."));
  }

  view_->AttachHost(this);

  if (type != GadgetHostInterface::VIEW_OLD_OPTIONS) {
    OptionsInterface *options = gadget_host->GetOptions();
    // Continue to initialize the script context.
    onoptionchanged_connection_ = options->ConnectOnOptionChanged(
        NewSlot(view_, &ViewInterface::OnOptionChanged));
  }

  widget_ = new QGadgetWidget(this, zoom, composited, useshapemask);
  graphics_ = new QtGraphics(zoom);
}

QtViewHost::~QtViewHost() {
  if (onoptionchanged_connection_)
    onoptionchanged_connection_->Disconnect();

  delete view_;
  view_ = NULL;

  if (script_context_) {
    script_context_->Destroy();
    script_context_ = NULL;
  }

  delete graphics_;
  graphics_ = NULL;
}

void QtViewHost::GetNativeWidgetInfo(void **native_widget, int *x, int *y) {
  ASSERT(native_widget && x && y);
  *native_widget = widget_;
  *x = 0;
  *y = 0;
}

void QtViewHost::QueueDraw() {
  widget_->update();
}

bool QtViewHost::GrabKeyboardFocus() {
  return false;
}

void QtViewHost::SetResizable(ViewInterface::ResizableMode mode) {
  // TODO:
}

void QtViewHost::SetCaption(const char *caption) {
  // TODO:
}

void QtViewHost::SetShowCaptionAlways(bool always) {
  // TODO:
}

void QtViewHost::SetCursor(CursorType type) {
}

void QtViewHost::RunDialog() {
}

void QtViewHost::ShowInDetailsView(const char *title, int flags,
                                    Slot1<void, int> *feedback_handler) {
}

void QtViewHost::CloseDetailsView() {
}

void QtViewHost::Alert(const char *message) {
}

bool QtViewHost::Confirm(const char *message) {
  return false;
}

std::string QtViewHost::Prompt(const char *message,
                                const char *default_value) {
  return "nothing";
}

void QtViewHost::SetTooltip(const char *tooltip) {
}

void QtViewHost::ChangeZoom(double zoom) {
  graphics_->SetZoom(zoom);
  view_->MarkRedraw();
}

} // namespace qt
} // namespace ggadget
