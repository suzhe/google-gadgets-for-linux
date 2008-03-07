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

#include <QUrl>
#include <QPainter>
#include <QMouseEvent>
#include <ggadget/gadget.h>
#include <ggadget/qt/qt_view_host.h>
#include <ggadget/qt/qt_canvas.h>
#include <ggadget/qt/qt_menu.h>
#include "qt_gadget_widget.h"
namespace ggadget {
namespace qt {

QGadgetWidget::QGadgetWidget(QtViewHost *host, double zoom,
                             bool composited, bool useshapemask)
//    : QGLWidget(QGLFormat(QGL::SampleBuffers)),
     : view_host_(host), canvas_(NULL), drag_files_(NULL) {
  view_ = host->GetView();
  setMouseTracking(true);
  setAcceptDrops(true);
  setFixedSize(100, 100);
  setAttribute(Qt::WA_InputMethodEnabled);
}

void QGadgetWidget::paintEvent(QPaintEvent *event) {
  QPainter p(this);
  size_t old_width = width_, old_height = height_;
  width_ = view_->GetWidth();
  height_ = view_->GetHeight();

  if (old_width != width_ || old_height != height_)
    setFixedSize(width_, height_);

  QtCanvas canvas(NULL, width_, height_, &p);
  canvas.ClearCanvas();
  view_->Draw(&canvas);
}

void QGadgetWidget::mouseDoubleClickEvent(QMouseEvent * event) {
}
// Check out qt document to get more information about MouseButtons and
// MouseButton
static int GetMouseButtons(const Qt::MouseButtons buttons){
  int ret = 0;
  if (buttons & Qt::LeftButton) ret |= MouseEvent::BUTTON_LEFT;
  if (buttons & Qt::RightButton) ret |= MouseEvent::BUTTON_RIGHT;
  if (buttons & Qt::MidButton) ret |= MouseEvent::BUTTON_MIDDLE;
  return ret;
}

static int GetMouseButton(const Qt::MouseButton button) {
  if (button == Qt::LeftButton) return MouseEvent::BUTTON_LEFT;
  if (button == Qt::RightButton) return MouseEvent::BUTTON_RIGHT;
  if (button == Qt::MidButton) return MouseEvent::BUTTON_MIDDLE;
  return 0;
}

void QGadgetWidget::mouseMoveEvent(QMouseEvent* event) {
  int buttons = GetMouseButtons(event->buttons());
  MouseEvent e(Event::EVENT_MOUSE_MOVE, event->x(), event->y(), 0, 0,
      buttons, 0);
  if (view_->OnMouseEvent(e) != ggadget::EVENT_RESULT_UNHANDLED)
    event->accept();
}

void QGadgetWidget::mousePressEvent(QMouseEvent * event ) {
  setFocus(Qt::MouseFocusReason);
  EventResult handler_result = ggadget::EVENT_RESULT_UNHANDLED;
  int button = GetMouseButton(event->button());
  mouse_down_time_ = GetGlobalMainLoop()->GetCurrentTime();

  MouseEvent e(Event::EVENT_MOUSE_DOWN,
               event->x(), event->y(), 0, 0, button, 0);
  handler_result = view_->OnMouseEvent(e);

  if (handler_result != ggadget::EVENT_RESULT_UNHANDLED) {
    event->accept();
    return;
  }
  if (event->button() == Qt::RightButton) {
    // Handle context menu
    QMenu qmenu;
    QtMenu menu(&qmenu);
    GadgetInterface *gadget = view_host_->GetGadgetHost()->GetGadget();
    gadget->OnAddCustomMenuItems(&menu);
    if (!qmenu.isEmpty()) {
      qmenu.exec(event->globalPos());
    }
  }
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

static int GetModifier(Qt::KeyboardModifiers state) {
  int mod = Event::MOD_NONE;
  if (state & Qt::ShiftModifier) mod |= Event::MOD_SHIFT;
  if (state & Qt::ControlModifier)  mod |= Event::MOD_CONTROL;
  if (state & Qt::AltModifier) mod |= Event::MOD_ALT;
  return mod;
}

static unsigned int GetKeyCode(int qt_key) {
  return qt_key;
}

void QGadgetWidget::keyPressEvent(QKeyEvent *event) {
  EventResult handler_result = ggadget::EVENT_RESULT_UNHANDLED;

  int mod = GetModifier(event->modifiers());
  unsigned int key_code = GetKeyCode(event->key());
  if (key_code) {
    KeyboardEvent e(Event::EVENT_KEY_DOWN, key_code, mod, event);
    handler_result = view_->OnKeyEvent(e);
  } else {
    LOG("Unknown key: 0x%x", event->key());
  }

 if (handler_result != ggadget::EVENT_RESULT_UNHANDLED)
    event->accept();
}

void QGadgetWidget::keyReleaseEvent(QKeyEvent *event) {
  EventResult handler_result = ggadget::EVENT_RESULT_UNHANDLED;

  int mod = GetModifier(event->modifiers());
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

}
}
