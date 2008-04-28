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

#include <QtGui/QCursor>
#include <QtGui/QToolTip>
#include <QtGui/QMessageBox>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QVBoxLayout>
#include <QtGui/QInputDialog>
#include <ggadget/file_manager_interface.h>
#include <ggadget/gadget_consts.h>
#include <ggadget/logger.h>
#include <ggadget/options_interface.h>
#include <ggadget/script_context_interface.h>
#include <ggadget/script_runtime_interface.h>
#include <ggadget/script_runtime_manager.h>
#include "utilities.h"
#include "qt_view_host.h"
#include "qt_menu.h"
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
      window_(NULL),
      dialog_(NULL),
      debug_mode_(debug_mode),
      zoom_(zoom),
      onoptionchanged_connection_(NULL),
      feedback_handler_(NULL),
      composite_(false),
      qt_obj_(new QtViewHostObject(this)) {
  if (type_ == ViewHostInterface::VIEW_HOST_MAIN)
    composite_ = true;
//  debug_mode_ = ViewInterface::DEBUG_ALL;
}

QtViewHost::~QtViewHost() {
  if (onoptionchanged_connection_)
    onoptionchanged_connection_->Disconnect();

  if (view_) {
    delete view_;
    view_ = NULL;
  }

  delete qt_obj_;
}

void QtViewHost::Destroy() {
  delete this;
}

void QtViewHost::SetView(ViewInterface *view) {
  Detach();
  if (view == NULL) return;
  view_ = view;
  widget_ = new QGadgetWidget(view_, this, composite_);
}

void QtViewHost::ViewCoordToNativeWidgetCoord(
    double x, double y, double *widget_x, double *widget_y) const {
  double zoom = view_->GetGraphics()->GetZoom();
  if (widget_x)
    *widget_x = x * zoom;
  if (widget_y)
    *widget_y = y * zoom;
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
}

void QtViewHost::SetCursor(int type) {
  QCursor cursor(GetQtCursorShape(type));
  widget_->setCursor(cursor);
}

void QtViewHost::SetTooltip(const char *tooltip) {
  QToolTip::showText(QCursor::pos(), QString::fromUtf8(tooltip));
}

bool QtViewHost::ShowView(bool modal, int flags,
                          Slot1<void, int> *feedback_handler) {
  ASSERT(view_);
  if (feedback_handler_)
    delete feedback_handler_;
  feedback_handler_ = feedback_handler;

  // Initialize window and widget.
  if (type_ == ViewHostInterface::VIEW_HOST_OPTIONS) {
    QVBoxLayout *layout = new QVBoxLayout();
    widget_->setFixedSize(D2I(view_->GetWidth()), D2I(view_->GetHeight()));
    layout->addWidget(widget_);
    dialog_ = new QDialog();

    QDialogButtonBox::StandardButtons what_buttons = 0;
    if (flags & ViewInterface::OPTIONS_VIEW_FLAG_OK)
      what_buttons |= QDialogButtonBox::Ok;

    if (flags & ViewInterface::OPTIONS_VIEW_FLAG_CANCEL)
      what_buttons |= QDialogButtonBox::Cancel;

    if (what_buttons != 0) {
      QDialogButtonBox *buttons = new QDialogButtonBox(what_buttons);
      if (flags & ViewInterface::OPTIONS_VIEW_FLAG_OK)
        dialog_->connect(buttons, SIGNAL(accepted()),
                         qt_obj_, SLOT(OnOptionViewOK()));
      if (flags & ViewInterface::OPTIONS_VIEW_FLAG_CANCEL)
        dialog_->connect(buttons, SIGNAL(rejected()),
                         qt_obj_, SLOT(OnOptionViewCancel()));
      layout->addWidget(buttons);
    }

    dialog_->setLayout(layout);

    if (modal) {
      dialog_->exec();
    } else {
      dialog_->show();
    }
  } else if (type_ == ViewHostInterface::VIEW_HOST_DETAILS) {
    window_ = widget_;
    window_->setAttribute(Qt::WA_DeleteOnClose, false);
    widget_->connect(widget_, SIGNAL(closed()),
                     qt_obj_, SLOT(OnDetailsViewClose()));
    window_->show();
  } else {
    window_ = widget_;
    window_->show();
  }

  return true;
}

void QtViewHost::CloseView() {
  // DetailsView will be only hiden here since it may be reused later.
  // window_ will be freed when SetView is called
  if (window_) {
    window_->close();
  }
}

bool QtViewHost::ShowContextMenu(int button) {
  ASSERT(view_);
  QMenu menu;
  QtMenu qt_menu(&menu);
  view_->OnAddContextMenuItems(&qt_menu);
  if (!menu.isEmpty()) {
    menu.exec(QCursor::pos());
    return true;
  } else {
    return false;
  }
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
  return s.toStdString();
}

void QtViewHost::Detach() {
  view_ = NULL;
  if (window_)
    delete window_;
  if (dialog_)
    delete dialog_;
  window_ = widget_ = NULL;
  dialog_ = NULL;
}

void QtViewHost::HandleOptionViewResponse(ViewInterface::OptionsViewFlags flag) {
  if (feedback_handler_) {
    (*feedback_handler_)(flag);
    delete feedback_handler_;
    feedback_handler_ = NULL;
  }
  dialog_->hide();
}

void QtViewHost::HandleDetailsViewClose() {
  if (feedback_handler_) {
    (*feedback_handler_)(ViewInterface::DETAILS_VIEW_FLAG_NONE);
    delete feedback_handler_;
    feedback_handler_ = NULL;
  }
}

#include "qt_view_host.moc"

} // namespace qt
} // namespace ggadget
