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

#include <sys/time.h>

#include <QtGui/QCursor>
#include <QtGui/QToolTip>
#include <QtGui/QMessageBox>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QVBoxLayout>
#include <QtGui/QInputDialog>
#include <ggadget/file_manager_interface.h>
#include <ggadget/gadget_consts.h>
#include <ggadget/gadget.h>
#include <ggadget/logger.h>
#include <ggadget/messages.h>
#include <ggadget/view.h>
#include <ggadget/options_interface.h>
#include <ggadget/script_context_interface.h>
#include <ggadget/script_runtime_interface.h>
#include <ggadget/script_runtime_manager.h>
#include "utilities.h"
#include "qt_view_host.h"
#include "qt_menu.h"
#include "qt_graphics.h"
#include "qt_view_widget.h"
#include "qt_view_host_internal.h"

namespace ggadget {
namespace qt {

QtViewHost::QtViewHost(ViewHostInterface::Type type,
                       double zoom, bool composite, bool decorated,
                       bool record_states, int debug_mode, QWidget *parent)
  : impl_(new Impl(this, type, zoom, composite,
                   decorated, record_states, debug_mode, parent)) {
}

QtViewHost::~QtViewHost() {
  if (impl_) delete impl_;
}

ViewHostInterface::Type QtViewHost::GetType() const {
  return impl_->type_;
}

ViewInterface *QtViewHost::GetView() const {
  return impl_->view_;
}

GraphicsInterface *QtViewHost::NewGraphics() const {
  return new QtGraphics(impl_->zoom_);
}

void *QtViewHost::GetNativeWidget() const {
  return impl_->widget_;
}

void QtViewHost::Destroy() {
  delete this;
}

void QtViewHost::SetView(ViewInterface *view) {
  if (impl_->view_ == view) return;
  impl_->Detach();
  impl_->view_ = view;
}

void QtViewHost::ViewCoordToNativeWidgetCoord(
    double x, double y, double *widget_x, double *widget_y) const {
  double zoom = impl_->view_->GetGraphics()->GetZoom();
  if (widget_x)
    *widget_x = x * zoom;
  if (widget_y)
    *widget_y = y * zoom;
}

void QtViewHost::NativeWidgetCoordToViewCoord(
    double x, double y, double *view_x, double *view_y) const {
  double zoom = impl_->view_->GetGraphics()->GetZoom();
  if (zoom == 0) return;
  if (view_x) *view_x = x / zoom;
  if (view_y) *view_y = y / zoom;
}

void QtViewHost::QueueDraw() {
  if (impl_->widget_)
    impl_->widget_->update();
}

void QtViewHost::QueueResize() {
  if (impl_->widget_)
    impl_->widget_->AdjustToViewSize();
}

void QtViewHost::EnableInputShapeMask(bool enable) {
  if (impl_->input_shape_mask_ != enable) {
    impl_->input_shape_mask_ = enable;
    if (impl_->widget_) impl_->widget_->EnableInputShapeMask(enable);
  }
}

void QtViewHost::SetResizable(ViewInterface::ResizableMode mode) {
  // TODO:
  DLOG("SetResizable:%d", mode);
}

void QtViewHost::SetCaption(const std::string &caption) {
  impl_->caption_ = QString::fromUtf8(caption.c_str());
  if (impl_->window_)
    impl_->window_->setWindowTitle(impl_->caption_);
}

void QtViewHost::SetShowCaptionAlways(bool always) {
}

void QtViewHost::SetCursor(ViewInterface::CursorType type) {
  QCursor cursor(GetQtCursorShape(type));
  impl_->widget_->setCursor(cursor);
}

void QtViewHost::ShowTooltip(const std::string &tooltip) {
  QToolTip::showText(QCursor::pos(), QString::fromUtf8(tooltip.c_str()));
}

void QtViewHost::ShowTooltipAtPosition(const std::string &tooltip,
                                       double x, double y) {
  ViewCoordToNativeWidgetCoord(x, y, &x, &y);
  QPoint pos(static_cast<int>(x), static_cast<int>(y));
  QToolTip::showText(impl_->widget_->mapToGlobal(pos),
                     QString::fromUtf8(tooltip.c_str()));
}

bool QtViewHost::ShowView(bool modal, int flags,
                          Slot1<bool, int> *feedback_handler) {
  return impl_->ShowView(modal, flags, feedback_handler);
}

void QtViewHost::CloseView() {
  if (impl_->window_) {
    impl_->SaveWindowStates();
    // must set widget_ to NULL before delete window_
    impl_->widget_ = NULL;
    delete impl_->window_;
    impl_->window_ = NULL;
  }
  ASSERT(!impl_->widget_);
}

bool QtViewHost::ShowContextMenu(int button) {
  return impl_->ShowContextMenu(button);
}

void QtViewHost::BeginMoveDrag(int button) {
}

void QtViewHost::Alert(const ViewInterface *view, const char *message) {
  QMessageBox::information(
      NULL,
      QString::fromUtf8(view->GetCaption().c_str()),
      QString::fromUtf8(message));
}

bool QtViewHost::Confirm(const ViewInterface *view, const char *message) {
  int ret = QMessageBox::question(
      NULL,
      QString::fromUtf8(view->GetCaption().c_str()),
      QString::fromUtf8(message),
      QMessageBox::Yes| QMessageBox::No,
      QMessageBox::Yes);
  return ret == QMessageBox::Yes;
}

std::string QtViewHost::Prompt(const ViewInterface *view,
                               const char *message,
                               const char *default_value) {
  QString s= QInputDialog::getText(
      NULL,
      QString::fromUtf8(view->GetCaption().c_str()),
      QString::fromUtf8(message),
      QLineEdit::Normal);
  return s.toStdString();
}

int QtViewHost::GetDebugMode() const {
  return impl_->debug_mode_;
}

QObject *QtViewHost::GetQObject() {
  return impl_;
}

} // namespace qt
} // namespace ggadget

#include "qt_view_host_internal.moc"
