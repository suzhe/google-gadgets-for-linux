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
#include <algorithm>

#include <QtCore/QUrl>
#include <QtGui/QDesktopWidget>
#include <QtGui/QPainter>
#include <QtGui/QMouseEvent>
#include <ggadget/graphics_interface.h>
#include <ggadget/gadget.h>
#include <ggadget/qt/qt_canvas.h>
#include <ggadget/qt/utilities.h>
#include <ggadget/qt/qt_menu.h>
#include "qt_view_widget.h"

#if defined(Q_WS_X11) && defined(HAVE_X11)
#include <QtGui/QX11Info>
#include <QtGui/QBitmap>
#include <X11/extensions/shape.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#endif

namespace ggadget {
namespace qt {

static const double kDragThreshold = 3;

QtViewWidget::QtViewWidget(ViewInterface *view,
                           bool composite,
                           bool decorated,
                           bool movable,
                           bool support_input_mask)
    : view_(view),
      drag_files_(NULL),
      composite_(composite),
      movable_(movable),
      enable_input_mask_(composite && support_input_mask),
      support_input_mask_(support_input_mask && composite),
      offscreen_pixmap_(NULL),
      mouse_drag_moved_(false),
      child_(NULL),
      zoom_(view->GetGraphics()->GetZoom()),
      mouse_down_hittest_(ViewInterface::HT_CLIENT),
      resize_drag_(false) {
  setMouseTracking(true);
  setAcceptDrops(true);
  AdjustToViewSize();
  if (!decorated) {
    setWindowFlags(Qt::FramelessWindowHint);
    SkipTaskBar();
  }
  setAttribute(Qt::WA_InputMethodEnabled);
}

QtViewWidget::~QtViewWidget() {
  DLOG("Widget freed");
  if (child_) {
    child_->setParent(NULL);
  }
  if (offscreen_pixmap_) delete offscreen_pixmap_;
  if (drag_files_) delete [] drag_files_;
}

void QtViewWidget::paintEvent(QPaintEvent *event) {
  QPainter p(this);
  p.setRenderHint(QPainter::Antialiasing);
  p.setClipRect(event->rect());

  if (composite_) {
    p.save();
    p.setCompositionMode(QPainter::CompositionMode_Source);
    p.fillRect(rect(), Qt::transparent);
    p.restore();
  }

  if (enable_input_mask_) {
    if (!offscreen_pixmap_
        || offscreen_pixmap_->width() != width()
        || offscreen_pixmap_->height() != height()) {
      if (offscreen_pixmap_) delete offscreen_pixmap_;
      offscreen_pixmap_ = new QPixmap(width(), height());
    }
    // Draw view on offscreen pixmap
    QPainter poff(offscreen_pixmap_);
    poff.scale(zoom_, zoom_);
    poff.setCompositionMode(QPainter::CompositionMode_Source);
    poff.fillRect(offscreen_pixmap_->rect(), Qt::transparent);
    QtCanvas canvas(width(), height(), &poff);
    view_->Draw(&canvas);
    SetInputMask(offscreen_pixmap_);
    p.drawPixmap(0, 0, *offscreen_pixmap_);
 } else {
    // Draw directly on widget
    QtCanvas canvas(width(), height(), &p);
    view_->Draw(&canvas);
  }
}

void QtViewWidget::mouseDoubleClickEvent(QMouseEvent * event) {
  Event::Type type;
  if (Qt::LeftButton == event->button())
    type = Event::EVENT_MOUSE_DBLCLICK;
  else
    type = Event::EVENT_MOUSE_RDBLCLICK;
  MouseEvent e(type, event->x() / zoom_, event->y() / zoom_, 0, 0, 0, 0);
  if (view_->OnMouseEvent(e) != ggadget::EVENT_RESULT_UNHANDLED)
    event->accept();
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
      resize_drag_ = true;
      origi_geometry_ = window()->geometry();
      top_ = bottom_ = left_ = right_ = 0;
      if (mouse_down_hittest_ == ViewInterface::HT_LEFT)
        left_ = 1;
      else if (mouse_down_hittest_ == ViewInterface::HT_RIGHT)
        right_ = 1;
      else if (mouse_down_hittest_ == ViewInterface::HT_TOP)
        top_ = 1;
      else if (mouse_down_hittest_ == ViewInterface::HT_BOTTOM)
        bottom_ = 1;
      else if (mouse_down_hittest_ == ViewInterface::HT_TOPLEFT)
        top_ = 1, left_ = 1;
      else if (mouse_down_hittest_ == ViewInterface::HT_TOPRIGHT)
        top_ = 1, right_ = 1;
      else if (mouse_down_hittest_ == ViewInterface::HT_BOTTOMLEFT)
        bottom_ = 1, left_ = 1;
      else if (mouse_down_hittest_ == ViewInterface::HT_BOTTOMRIGHT)
        bottom_ = 1, right_ = 1;
      else
        resize_drag_ = false;
    }

    if (resize_drag_) {
      QPoint p = QCursor::pos() - mouse_pos_;
      QRect rect = origi_geometry_;
      rect.setTop(rect.top() + top_*p.y());
      rect.setBottom(rect.bottom() + bottom_*p.y());
      rect.setLeft(rect.left() + left_*p.x());
      rect.setRight(rect.right() + right_*p.x());
      double w = rect.width();
      double h = rect.height();
      if (w != view_->GetWidth() || h != view_->GetHeight()) {
        if (view_->OnSizing(&w, &h))
          view_->SetSize(w, h);
      }
    }  else {
      QPoint offset = QCursor::pos() - mouse_pos_;
      if (movable_) window()->move(window()->pos() + offset);
      mouse_pos_ = QCursor::pos();
      emit moved(offset.x(), offset.y());
    }
  }
}

void QtViewWidget::mousePressEvent(QMouseEvent * event ) {
  if (!hasFocus()) {
    setFocus(Qt::MouseFocusReason);
    SimpleEvent e(Event::EVENT_FOCUS_IN);
    view_->OnOtherEvent(e);
  }

  mouse_down_hittest_ = view_->GetHitTest();
  mouse_drag_moved_ = false;
  resize_drag_ = false;
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

  if (mouse_drag_moved_) {
    QPoint offset = QCursor::pos() - mouse_pos_;
    if (std::abs(static_cast<double>(offset.x())) > kDragThreshold ||
    	std::abs(static_cast<double>(offset.y())) > kDragThreshold)
    return;
  } else {
    MouseEvent e(Event::EVENT_MOUSE_UP,
                 event->x() / zoom_, event->y() / zoom_, 0, 0, button, 0);
    handler_result = view_->OnMouseEvent(e);
    if (handler_result != ggadget::EVENT_RESULT_UNHANDLED)
      event->accept();
  }

  MouseEvent e1(event->button() == Qt::LeftButton ? Event::EVENT_MOUSE_CLICK :
                                                    Event::EVENT_MOUSE_RCLICK,
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
    delta_x = event->delta();
  } else {
    delta_y = event->delta();
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
  DLOG("drag enter");
  if (event->mimeData()->hasUrls()) {
    drag_urls_.clear();
    if (drag_files_)
      delete [] drag_files_;

    QList<QUrl> urls = event->mimeData()->urls();
    drag_files_ = new const char *[urls.size() + 1];
    if (!drag_files_) return;

    for (int i = 0; i < urls.size(); i++) {
      std::string url = urls[i].toString().toUtf8().data();
      if (url.substr(0, 7) == "file://")
        url = url.substr(7);
      drag_urls_.push_back(url);
      drag_files_[i] = drag_urls_[i].c_str();
    }
    drag_files_[urls.size()] = NULL;
    event->acceptProposedAction();
  }
}

void QtViewWidget::dragLeaveEvent(QDragLeaveEvent *event) {
  DLOG("drag leave");
  DragEvent drag_event(Event::EVENT_DRAG_OUT, 0, 0);
  drag_event.SetDragFiles(drag_files_);
  view_->OnDragEvent(drag_event);
}

void QtViewWidget::dragMoveEvent(QDragMoveEvent *event) {
  DragEvent drag_event(Event::EVENT_DRAG_MOTION,
                       event->pos().x(), event->pos().y());
  drag_event.SetDragFiles(drag_files_);
  if (view_->OnDragEvent(drag_event) != EVENT_RESULT_UNHANDLED)
    event->acceptProposedAction();
  else
    event->ignore();
}

void QtViewWidget::dropEvent(QDropEvent *event) {
  LOG("drag drop");
  DragEvent drag_event(Event::EVENT_DRAG_DROP,
                       event->pos().x(), event->pos().y());
  drag_event.SetDragFiles(drag_files_);
  if (view_->OnDragEvent(drag_event) == EVENT_RESULT_UNHANDLED) {
    event->ignore();
  }
}

void QtViewWidget::resizeEvent(QResizeEvent *event) {
  
}

QSize QtViewWidget::sizeHint() const {
  return minimumSizeHint();
}

QSize QtViewWidget::minimumSizeHint() const {
  int w = D2I(view_->GetWidth() * zoom_);
  int h = D2I(view_->GetHeight() * zoom_);
  if (w == 0 || h == 0) {
    double dw, dh;
    view_->GetDefaultSize(&dw, &dh);
    w = D2I(dw * zoom_);
    h = D2I(dh * zoom_);
  }
  return QSize(w > 0 ? w : 1, h > 0 ? h : 1);
}

void QtViewWidget::EnableInputShapeMask(bool enable) {
  if (!support_input_mask_) return;
  if (enable_input_mask_ != enable) {
    enable_input_mask_ = enable;
    if (!enable) SetInputMask(NULL);
  }
}

void QtViewWidget::SetInputMask(QPixmap *pixmap) {
#if defined(Q_WS_X11) && defined(HAVE_X11)
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

void QtViewWidget::AdjustToViewSize() {
  int w = D2I(view_->GetWidth() * zoom_);
  int h = D2I(view_->GetHeight() * zoom_);
  if (resize_drag_) {
    int dtop, dleft, dw, dh;
    dw = w - origi_geometry_.width();
    dh = h - origi_geometry_.height();
    dtop = dleft = 0;
    if (top_) {
      dtop = -dh;
      dh = 0;
    }
    if (left_) {
      dleft = -dw;
      dw = 0;
    }

    DLOG("offset: (%d, %d, %d, %d)", dleft, dtop, dw, dh);
    origi_geometry_.adjust(dleft, dtop, dw, dh);
    mouse_pos_ = QCursor::pos();
    if (movable_) {
      window()->setGeometry(origi_geometry_);  //.adjusted(dleft, dtop, dw, dh));
    } else {
      emit geometryChanged(dleft, dtop, dw, dh);
    }
    return;
  }
  resize(w, h);
}

void QtViewWidget::SetKeepAbove(bool above) {
  Qt::WindowFlags f = windowFlags();
  if (above)
    f |= Qt::WindowStaysOnTopHint;
  else
    f &= ~Qt::WindowStaysOnTopHint;
  setWindowFlags(f);
  show();
}

void QtViewWidget::SkipTaskBar() {
#if defined(Q_WS_X11) && defined(HAVE_X11)
  Display *dpy = QX11Info::display();
  Atom net_wm_state_skip_taskbar=XInternAtom(dpy, "_NET_WM_STATE_SKIP_TASKBAR",
                                             False);
  Atom net_wm_state = XInternAtom(dpy, "_NET_WM_STATE", False);
  XChangeProperty(dpy, winId(), net_wm_state,
                  XA_ATOM, 32, PropModeAppend,
                  (unsigned char *)&net_wm_state_skip_taskbar, 1);
#endif
}

void QtViewWidget::SetChild(QWidget *widget) {
  if (child_) {
    child_->setParent(NULL);
  }
  child_ = widget;
  if (widget) {
    widget->setParent(this);
    // this will expose parent widget so its paintEvent will be triggered.
    widget->move(0, 10);
  }
}

}
}
#include "qt_view_widget.moc"
