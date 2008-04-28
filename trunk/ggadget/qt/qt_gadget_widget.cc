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

#include <QtCore/QUrl>
#include <QtGui/QPainter>
#include <QtGui/QMouseEvent>
#include <ggadget/gadget.h>
#include <ggadget/qt/qt_view_host.h>
#include <ggadget/qt/qt_canvas.h>
#include <ggadget/qt/utilities.h>
#include <ggadget/qt/qt_menu.h>
#include "qt_gadget_widget.h"
namespace ggadget {
namespace qt {

QGadgetWidget::QGadgetWidget(ViewInterface *view,
                             ViewHostInterface *host,
                             bool composite)
     : canvas_(NULL), graphics_(NULL), view_(view), view_host_(host),
       width_(0), height_(0),
       drag_files_(NULL),
       composite_(composite) {
  graphics_ = host->NewGraphics();
  zoom_ = graphics_->GetZoom();
  setMouseTracking(true);
  setAcceptDrops(true);
  setWindowFlags(Qt::FramelessWindowHint);
  setAttribute(Qt::WA_InputMethodEnabled);
}

QGadgetWidget::~QGadgetWidget() {
  if (graphics_) delete graphics_;
}

void QGadgetWidget::paintEvent(QPaintEvent *event) {
  double old_width = width_, old_height = height_;
  width_ = view_->GetWidth();
  height_ = view_->GetHeight();

  if (old_width != width_ || old_height != height_) {
    setFixedSize(D2I(width_ * zoom_), D2I(height_ * zoom_));
    setMinimumSize(0, 0);
    setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
  }

  QPainter p(this);
  p.setRenderHint(QPainter::Antialiasing);
  p.setClipRect(event->rect());

  if (composite_) {
    p.save();
    p.setCompositionMode(QPainter::CompositionMode_Source);
    p.fillRect(rect(), Qt::transparent);
    p.restore();
  }
  QtCanvas canvas(D2I(width_ * zoom_), D2I(height_ * zoom_), &p);
  view_->Draw(&canvas);
}

void QGadgetWidget::mouseDoubleClickEvent(QMouseEvent * event) {
}

void QGadgetWidget::mouseMoveEvent(QMouseEvent* event) {
  int buttons = GetMouseButtons(event->buttons());
  MouseEvent e(Event::EVENT_MOUSE_MOVE,
               event->x() / zoom_, event->y() / zoom_, 0, 0,
               buttons, 0);
  if (view_->OnMouseEvent(e) != ggadget::EVENT_RESULT_UNHANDLED)
    event->accept();
}

void QGadgetWidget::mousePressEvent(QMouseEvent * event ) {
  setFocus(Qt::MouseFocusReason);
  EventResult handler_result = EVENT_RESULT_UNHANDLED;
  int button = GetMouseButton(event->button());

  MouseEvent e(Event::EVENT_MOUSE_DOWN,
               event->x() / zoom_, event->y() / zoom_, 0, 0, button, 0);
  handler_result = view_->OnMouseEvent(e);

  if (handler_result != ggadget::EVENT_RESULT_UNHANDLED) {
    event->accept();
  }
}

static const unsigned int kWindowMoveDelay = 100;

void QGadgetWidget::mouseReleaseEvent(QMouseEvent * event ) {
  EventResult handler_result = ggadget::EVENT_RESULT_UNHANDLED;
  int button = GetMouseButton(event->button());

  MouseEvent e(Event::EVENT_MOUSE_UP,
               event->x() / zoom_, event->y() / zoom_, 0, 0, button, 0);
  handler_result = view_->OnMouseEvent(e);

  if (handler_result != ggadget::EVENT_RESULT_UNHANDLED)
    event->accept();

  MouseEvent e1(Event::EVENT_MOUSE_CLICK,
               event->x() / zoom_, event->y() / zoom_, 0, 0, button, 0);
  handler_result = view_->OnMouseEvent(e1);

  if (handler_result != ggadget::EVENT_RESULT_UNHANDLED)
    event->accept();
}

void QGadgetWidget::enterEvent(QEvent *event) {
  MouseEvent e(Event::EVENT_MOUSE_OVER,
      0, 0, 0, 0,
      MouseEvent::BUTTON_NONE, 0);
  if (view_->OnMouseEvent(e) != ggadget::EVENT_RESULT_UNHANDLED)
    event->accept();
}

void QGadgetWidget::leaveEvent(QEvent *event) {
  MouseEvent e(Event::EVENT_MOUSE_OUT,
               0, 0, 0, 0,
               MouseEvent::BUTTON_NONE, 0);
  if (view_->OnMouseEvent(e) != ggadget::EVENT_RESULT_UNHANDLED)
    event->accept();
}

void QGadgetWidget::keyPressEvent(QKeyEvent *event) {
  // For key down event
  EventResult handler_result = ggadget::EVENT_RESULT_UNHANDLED;
  // For key press event
  EventResult handler_result2 = ggadget::EVENT_RESULT_UNHANDLED;

  // Key down event
  int mod = GetModifiers(event->modifiers());
  unsigned int key_code = GetKeyCode(event->key());
  if (key_code) {
    KeyboardEvent e(Event::EVENT_KEY_DOWN, key_code, mod, event);
    handler_result = view_->OnKeyEvent(e);
  } else {
    LOG("Unknown key: 0x%x", event->key());
  }

  // Key press event
  QString text = event->text();
  if (!text.isEmpty() && !text.isNull()) {
    KeyboardEvent e2(Event::EVENT_KEY_PRESS, text[0].unicode(), mod, event);
    handler_result2 = view_->OnKeyEvent(e2);
  }

  if (handler_result != ggadget::EVENT_RESULT_UNHANDLED ||
      handler_result2 != ggadget::EVENT_RESULT_UNHANDLED)
    event->accept();
}

void QGadgetWidget::keyReleaseEvent(QKeyEvent *event) {
  EventResult handler_result = ggadget::EVENT_RESULT_UNHANDLED;

  int mod = GetModifiers(event->modifiers());
  unsigned int key_code = GetKeyCode(event->key());
  if (key_code) {
    KeyboardEvent e(Event::EVENT_KEY_UP, key_code, mod, event);
    handler_result = view_->OnKeyEvent(e);
  } else {
    LOG("Unknown key: 0x%x", event->key());
  }

  if (handler_result != ggadget::EVENT_RESULT_UNHANDLED)
    event->accept();
}

void QGadgetWidget::dragEnterEvent(QDragEnterEvent *event) {
  LOG("drag enter");
  if (event->mimeData()->hasUrls()) {
    drag_urls_.clear();
    if (drag_files_)
      delete [] drag_files_;

    QList<QUrl> urls = event->mimeData()->urls();
    drag_files_ = new const char *[urls.size() + 1];
    if (!drag_files_) return;

    for (int i = 0; i < urls.size(); i++) {
      drag_urls_.push_back(urls[i].toString().toStdString());
      drag_files_[i] = drag_urls_[i].c_str();
    }
    drag_files_[urls.size()] = NULL;
    event->acceptProposedAction();
  }
}

void QGadgetWidget::dragLeaveEvent(QDragLeaveEvent *event) {
  LOG("drag leave");
  DragEvent drag_event(Event::EVENT_DRAG_OUT, 0, 0, drag_files_);
  view_->OnDragEvent(drag_event);
}

void QGadgetWidget::dragMoveEvent(QDragMoveEvent *event) {
  DragEvent drag_event(Event::EVENT_DRAG_MOTION,
                       event->pos().x(), event->pos().y(),
                       drag_files_);
  if (view_->OnDragEvent(drag_event) != EVENT_RESULT_UNHANDLED)
    event->acceptProposedAction();
  else
    event->ignore();
}

void QGadgetWidget::dropEvent(QDropEvent *event) {
  LOG("drag drop");
  DragEvent drag_event(Event::EVENT_DRAG_DROP,
                       event->pos().x(), event->pos().y(),
                       drag_files_);
  if (view_->OnDragEvent(drag_event) == EVENT_RESULT_UNHANDLED) {
    event->ignore();
  }
}

void QGadgetWidget::resizeEvent(QResizeEvent *event) {
  if (width_ == 0) return;
  ViewInterface::ResizableMode mode = view_->GetResizable();
  if (mode == ViewInterface::RESIZABLE_ZOOM) {
    double x_ratio =
        static_cast<double>(event->size().width()) /
        static_cast<double>(width_);
    double y_ratio =
        static_cast<double>(event->size().height()) /
        static_cast<double>(height_);
    zoom_ = x_ratio > y_ratio ? y_ratio : x_ratio;
    graphics_->SetZoom(zoom_);
    view_->MarkRedraw();
    repaint();
  } else if (mode == ViewInterface::RESIZABLE_TRUE) {
    double width = event->size().width() / zoom_;
    double height = event->size().height() / zoom_;
    if (width != view_->GetWidth() || height != view_->GetHeight()) {
      if (view_->OnSizing(&width, &height)) {
        view_->SetSize(width, height);
      } else {
        view_host_->QueueResize();
      }
    }
  } else {
    view_host_->QueueResize();
  }
}

void QGadgetWidget::closeEvent(QCloseEvent *event) {
  event->accept();
  emit closed();
}
#include "qt_gadget_widget.moc"
}
}
