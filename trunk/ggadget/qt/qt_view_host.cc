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

#include <QMessageBox>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QInputDialog>
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

QtViewHost::QtViewHost(ViewHostInterface::Type type,
                       double zoom, bool decorated,
                       ViewInterface::DebugMode debug_mode)
    : view_(NULL),
      type_(type),
      widget_(NULL),
      graphics_(new QtGraphics(zoom)),
      window_(NULL),
      debug_mode_(debug_mode),
      onoptionchanged_connection_(NULL),
      feedback_handler_(NULL) {
//  debug_mode_ = ViewInterface::DEBUG_ALL;
}

QtViewHost::~QtViewHost() {
  if (onoptionchanged_connection_)
    onoptionchanged_connection_->Disconnect();

  delete view_;
  view_ = NULL;

  delete graphics_;
  graphics_ = NULL;
}

void QtViewHost::Destroy() {
  delete this;
}

void QtViewHost::SetView(ViewInterface *view) {
  Detach();
  if (view == NULL) return;

  view_ = view;
  bool no_background = false;
  widget_ = new QGadgetWidget(view_, this, graphics_);
  // Initialize window and widget.
  if (type_ == ViewHostInterface::VIEW_HOST_OPTIONS) {
    window_ = new QDialog();
    QVBoxLayout layout;

    QDialogButtonBox buttons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    window_->connect(&buttons, SIGNAL(accepted()), window_, SLOT(accept()));
    window_->connect(&buttons, SIGNAL(rejected()), window_, SLOT(reject()));

    layout.addWidget(widget_);
    layout.addWidget(&buttons);
    window_->setLayout(&layout);
 /*   int ret = dlg.exec();
    if (ret == QDialog::Accepted) {
      SimpleEvent event(Event::EVENT_OK);
      view_->OnOtherEvent(event, NULL);
    } */
  } else {
    window_ = widget_;
    // Only main view may have transparent background.
    no_background = true;
  }
}

void QtViewHost::ViewCoordToNativeWidgetCoord(
    double x, double y, double *widget_x, double *widget_y) const {
  // TODO
}

void QtViewHost::QueueDraw() {
  widget_->update();
}

void QtViewHost::QueueResize() {
  // TODO
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

void QtViewHost::SetCursor(int type) {
}

void QtViewHost::SetTooltip(const char *tooltip) {
}

bool QtViewHost::ShowView(bool modal, int flags,
                          Slot1<void, int> *feedback_handler) {
  window_->show();
  SimpleEvent e(Event::EVENT_OPEN);
  view_->OnOtherEvent(e);

  return true;
}

void QtViewHost::CloseView() {
  if (window_->isVisible()) {
    // TODO: SaveViewPosition
  }
  window_->hide();
}

bool QtViewHost::ShowContextMenu(int button) {
  return true;
}

void QtViewHost::Alert(const char *message) {
  QMessageBox::information(NULL,
                           view_->GetCaption().c_str(),
                           message);
}

bool QtViewHost::Confirm(const char *message) {
  int ret = QMessageBox::question(NULL,
                                  view_->GetCaption().c_str(),
                                  message,
                                  QMessageBox::Yes| QMessageBox::No,
                                  QMessageBox::Yes);
  return ret == QMessageBox::Yes;
}

std::string QtViewHost::Prompt(const char *message,
                                const char *default_value) {
  QString s= QInputDialog::getText(NULL,
                                   view_->GetCaption().c_str(),
                                   message,
                                   QLineEdit::Normal);
//                                   QMessageBox::Ok| QMessageBox::Cancel);
  return s.toStdString();
}

void QtViewHost::Detach() {
  view_ = NULL;
  if (window_)
    delete window_;
  window_ = widget_ = NULL;
}

} // namespace qt
} // namespace ggadget
