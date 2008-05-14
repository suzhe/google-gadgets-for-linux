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
#include <ggadget/graphics_interface.h>
#include <ggadget/gadget.h>
#include <ggadget/qt/qt_canvas.h>
#include <ggadget/qt/utilities.h>
#include <ggadget/qt/qt_menu.h>
#include "qt_view_widget.h"

#ifdef GGL_USE_X11
#include <QtGui/QX11Info>
#include <QtGui/QBitmap>
#include <X11/extensions/shape.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#endif

namespace ggadget {
namespace qt {

QtViewWidget::QtViewWidget(ViewInterface *view,
                             bool composite,
                             bool decorated)
     : view_(view),
       drag_files_(NULL),
       composite_(composite),
       enable_input_mask_(true),
       mouse_drag_moved_(false),
       child_(NULL) {
  setMouseTracking(true);
  SetSize(2, 2);
  setAcceptDrops(true);
  if (!decorated) {
    setWindowFlags(Qt::FramelessWindowHint);
    SkipTaskBar();
  }
  setAttribute(Qt::WA_InputMethodEnabled);
}

QtViewWidget::~QtViewWidget() {
  LOG("Widget freed");
  if (child_) {
    child_->setParent(NULL);
  }
}

void QtViewWidget::paintEvent(QPaintEvent *event) {
  zoom_ = view_->GetGraphics()->GetZoom();
  int int_width = D2I(view_->GetWidth() * zoom_);
  int int_height = D2I(view_->GetHeight() * zoom_);

  if (width() != int_width || height() != int_height
      || offscreen_pixmap_.isNull()) {
    SetSize(int_width, int_height);
    QPixmap pixmap(int_width, int_height);
    offscreen_pixmap_ = pixmap;
  }
  // Draw view on offscreen pixmap
  {
    QPainter p(&offscreen_pixmap_);
    p.scale(zoom_, zoom_);
    p.setCompositionMode(QPainter::CompositionMode_Source);
    p.fillRect(offscreen_pixmap_.rect(), Qt::transparent);
    QtCanvas canvas(int_width, int_height, &p);
    view_->Draw(&canvas);
    // view_'s size may be changed after Draw
    int_width = D2I(view_->GetWidth() * zoom_);
    int_height = D2I(view_->GetHeight() * zoom_);
    if (width() != int_width || height() != int_height) {
      update();
      return;
    }
  }

  if (enable_input_mask_ && composite_) {
    SetInputMask(&offscreen_pixmap_);
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
  p.drawPixmap(0, 0, offscreen_pixmap_);
}

void QtViewWidget::mouseDoubleClickEvent(QMouseEvent * event) {
}

void QtViewWidget::mouseMoveEvent(QMouseEvent* event) {
  int buttons = GetMouseButtons(event->buttons());
  if (buttons != MouseEvent::BUTTON_NONE)
    grabMouse();
  MouseEvent e(Event::EVENT_MOUSE_MOVE,
               event->x() / zoom_, event->y() / zoom_, 0, 0,
               buttons, 0);

  if (view_->OnMouseEvent(e) != ggadget::EVENT_RESULT_UNHANDLED)
    event->accept();
  else if (buttons != MouseEvent::BUTTON_NONE) {
    // Send fake mouse up event to the view so that we can start to drag
    // the window. Note: no mouse click event is sent in this case, to prevent
    // unwanted action after window move.
    if (!mouse_drag_moved_) {
      MouseEvent e(Event::EVENT_MOUSE_UP,
                   event->x() / zoom_, event->y() / zoom_,
                   0, 0, buttons, 0);
      // Ignore the result of this fake event.
      view_->OnMouseEvent(e);
      mouse_drag_moved_ = true;
    }

    window()->move(window()->pos() + QCursor::pos() - mouse_pos_);
    mouse_pos_ = QCursor::pos();
  }
}

void QtViewWidget::mousePressEvent(QMouseEvent * event ) {
  setFocus(Qt::MouseFocusReason);
  mouse_drag_moved_ = false;
  // Remember the position of mouse, it may be used to move the gadget
  mouse_pos_ = QCursor::pos();
  EventResult handler_result = EVENT_RESULT_UNHANDLED;
  int button = GetMouseButton(event->button());

  MouseEvent e(Event::EVENT_MOUSE_DOWN,
               event->x() / zoom_, event->y() / zoom_, 0, 0, button, 0);
  handler_result = view_->OnMouseEvent(e);

  if (handler_result != ggadget::EVENT_RESULT_UNHANDLED) {
    event->accept();
  }
}

void QtViewWidget::mouseReleaseEvent(QMouseEvent * event ) {
  releaseMouse();
  EventResult handler_result = ggadget::EVENT_RESULT_UNHANDLED;
  int button = GetMouseButton(event->button());

  if (mouse_drag_moved_) return;

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

void QtViewWidget::enterEvent(QEvent *event) {
  MouseEvent e(Event::EVENT_MOUSE_OVER,
      0, 0, 0, 0,
      MouseEvent::BUTTON_NONE, 0);
  if (view_->OnMouseEvent(e) != ggadget::EVENT_RESULT_UNHANDLED)
    event->accept();
}

void QtViewWidget::leaveEvent(QEvent *event) {
  MouseEvent e(Event::EVENT_MOUSE_OUT,
               0, 0, 0, 0,
               MouseEvent::BUTTON_NONE, 0);
  if (view_->OnMouseEvent(e) != ggadget::EVENT_RESULT_UNHANDLED)
    event->accept();
}

void QtViewWidget::wheelEvent(QWheelEvent * event) {
  int delta_x = 0, delta_y = 0;
  if (event->orientation() == Qt::Horizontal) {
    delta_x = -event->delta();
  } else {
    delta_y = -event->delta();
  }
  MouseEvent e(Event::EVENT_MOUSE_WHEEL,
               event->x() / zoom_, event->y() / zoom_,
               delta_x, delta_y,
               GetMouseButtons(event->buttons()),
               0);

  if (view_->OnMouseEvent(e) != EVENT_RESULT_UNHANDLED)
    event->accept();
}

void QtViewWidget::keyPressEvent(QKeyEvent *event) {
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

void QtViewWidget::keyReleaseEvent(QKeyEvent *event) {
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

void QtViewWidget::dragEnterEvent(QDragEnterEvent *event) {
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

void QtViewWidget::dragLeaveEvent(QDragLeaveEvent *event) {
  LOG("drag leave");
  DragEvent drag_event(Event::EVENT_DRAG_OUT, 0, 0, drag_files_);
  view_->OnDragEvent(drag_event);
}

void QtViewWidget::dragMoveEvent(QDragMoveEvent *event) {
  DragEvent drag_event(Event::EVENT_DRAG_MOTION,
                       event->pos().x(), event->pos().y(),
                       drag_files_);
  if (view_->OnDragEvent(drag_event) != EVENT_RESULT_UNHANDLED)
    event->acceptProposedAction();
  else
    event->ignore();
}

void QtViewWidget::dropEvent(QDropEvent *event) {
  LOG("drag drop");
  DragEvent drag_event(Event::EVENT_DRAG_DROP,
                       event->pos().x(), event->pos().y(),
                       drag_files_);
  if (view_->OnDragEvent(drag_event) == EVENT_RESULT_UNHANDLED) {
    event->ignore();
  }
}

#if 0
void QtViewWidget::resizeEvent(QResizeEvent *event) {
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
#endif

void QtViewWidget::EnableInputShapeMask(bool enable) {
  if (enable_input_mask_ != enable) {
    enable_input_mask_ = enable;
    if (!enable) SetInputMask(NULL);
  }
}

void QtViewWidget::SetInputMask(QPixmap *pixmap) {
#ifdef GGL_USE_X11
  if (!pixmap) {
    XShapeCombineMask(QX11Info::display(),
                      winId(),
                      ShapeInput,
                      0, 0,
                      None,
                      ShapeSet);
    return;
  }
  QBitmap bm = pixmap->createMaskFromColor(QColor(0, 0, 0, 0), Qt::MaskInColor);
  XShapeCombineMask(QX11Info::display(),
                    winId(),
                    ShapeInput,
                    0, 0,
                    bm.handle(),
                    ShapeSet);
#endif
}

void QtViewWidget::SkipTaskBar() {
#ifdef GGL_USE_X11
  Display *dpy = QX11Info::display();
  Atom net_wm_state_skip_taskbar=XInternAtom(dpy, "_NET_WM_STATE_SKIP_TASKBAR",
                                             False);
  Atom net_wm_state = XInternAtom(dpy, "_NET_WM_STATE", False);
  XChangeProperty(dpy, winId(), net_wm_state,
                  XA_ATOM, 32, PropModeAppend,
                  (unsigned char *)&net_wm_state_skip_taskbar, 1);
#endif
}

void QtViewWidget::SetSize(int width, int height) {
  setFixedSize(width, height);
  setMinimumSize(0, 0);
  setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
}

void QtViewWidget::SetChild(QWidget *widget) {
  child_ = widget;
  widget->setParent(this);
  // this will expose parent widget so its paintEvent will be triggered.
  widget->move(0, 10);
}
}
}
