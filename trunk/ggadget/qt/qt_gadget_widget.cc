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

#include <QPainter>
#include <QMouseEvent>
#include <ggadget/qt/qt_view_host.h>
#include <ggadget/qt/qt_canvas.h>
#include "qt_gadget_widget.h"
namespace ggadget {
namespace qt {

QGadgetWidget::QGadgetWidget(QtViewHost *host, double zoom,
                             bool composited, bool useshapemask)
    : view_host_(host) {
  view_ = host->GetView();
  setMouseTracking(true);
  setFixedSize(100, 100);
}
void QGadgetWidget::paintEvent(QPaintEvent *event) {
  QPainter p(this);
  width_ = view_->GetWidth();
  height_ = view_->GetHeight();
  setFixedSize(width_, height_);

  QtCanvas *canvas = reinterpret_cast<QtCanvas*>(
      view_host_->GetGraphics()->NewCanvas(width_, height_));
  view_->Draw(canvas);

  p.drawPixmap(0, 0, *canvas->GetPixmap());
}

void QGadgetWidget::mouseDoubleClickEvent(QMouseEvent * event) {
}


void QGadgetWidget::mouseMoveEvent(QMouseEvent* event) {
  if (mouse_just_entered_) {
    LOG("Entered event");
    mouse_just_entered_ = false;
    MouseEvent e(Event::EVENT_MOUSE_OVER,
                 event->x(), event->y(), 0, 0,
                 MouseEvent::BUTTON_NONE, 0);
    if (view_->OnMouseEvent(e) != ggadget::EVENT_RESULT_UNHANDLED)
      event->accept();
  } else {
    MouseEvent e(Event::EVENT_MOUSE_MOVE, event->x(), event->y(), 0, 0,
                 MouseEvent::BUTTON_NONE, 0);
    if (view_->OnMouseEvent(e) != ggadget::EVENT_RESULT_UNHANDLED)
      event->accept(); 
  }
}
void QGadgetWidget::mousePressEvent(QMouseEvent * event ) {
  EventResult handler_result = ggadget::EVENT_RESULT_UNHANDLED;
  int button;
  mouse_down_time_ = GetGlobalMainLoop()->GetCurrentTime();
  if (event->button() == Qt::LeftButton) {
    button = MouseEvent::BUTTON_LEFT;
  } else if (event->button() == Qt::RightButton) {
    button = MouseEvent::BUTTON_RIGHT;
  } else {
    button = MouseEvent::BUTTON_MIDDLE;
  }
  MouseEvent e(Event::EVENT_MOUSE_DOWN,
               event->x(), event->y(), 0, 0, button, 0);
  handler_result = view_->OnMouseEvent(e);
  if (handler_result != ggadget::EVENT_RESULT_UNHANDLED)
    event->accept();
}
static const unsigned int kWindowMoveDelay = 100;

void QGadgetWidget::mouseReleaseEvent(QMouseEvent * event ) {
  EventResult handler_result = ggadget::EVENT_RESULT_UNHANDLED;
  int button;
  if (event->button() == Qt::LeftButton) {
    button = MouseEvent::BUTTON_LEFT;
  } else if (event->button() == Qt::RightButton) {
    button = MouseEvent::BUTTON_RIGHT;
  } else {
    button = MouseEvent::BUTTON_MIDDLE;
  }
  MouseEvent e(Event::EVENT_MOUSE_UP,
               event->x(), event->y(), 0, 0, button, 0);
  handler_result = view_->OnMouseEvent(e);

  if (handler_result != ggadget::EVENT_RESULT_UNHANDLED)
    event->accept();

  MouseEvent e1(Event::EVENT_MOUSE_CLICK,
               event->x(), event->y(), 0, 0, button, 0);
  handler_result = view_->OnMouseEvent(e1);
  
  if (handler_result != ggadget::EVENT_RESULT_UNHANDLED)
    event->accept();
}

void QGadgetWidget::enterEvent(QEvent *event) {
  mouse_just_entered_ = true;
  LOG("Entered");
}

}
}
