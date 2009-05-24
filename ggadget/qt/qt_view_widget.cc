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
#include <QtGui/QPalette>
#include <QtGui/QPainter>
#include <QtGui/QLayout>
#include <QtGui/QMouseEvent>
#include <ggadget/graphics_interface.h>
#include <ggadget/main_loop_interface.h>
#include <ggadget/gadget.h>
#include <ggadget/clip_region.h>
#include <ggadget/math_utils.h>
#include <ggadget/string_utils.h>
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
#ifdef _DEBUG
static const uint64_t kFPSCountDuration = 5000;
#endif

// update input mask once per second.
static const uint64_t kUpdateMaskInterval = 1000;

QtViewWidget::QtViewWidget(ViewInterface *view, QtViewWidget::Flags flags)
    : view_(view),
      drag_files_(NULL),
      drag_urls_(NULL),
      composite_(flags.testFlag(QtViewWidget::FLAG_COMPOSITE)),
      movable_(flags.testFlag(QtViewWidget::FLAG_MOVABLE)),
      enable_input_mask_(false),
      support_input_mask_(flags.testFlag(QtViewWidget::FLAG_INPUT_MASK) &&
                          flags.testFlag(QtViewWidget::FLAG_COMPOSITE)),
      offscreen_pixmap_(NULL),
      mouse_drag_moved_(false),
      child_(NULL),
      zoom_(view->GetGraphics()->GetZoom()),
      last_mask_time_(0),
#ifdef _DEBUG
      last_fps_time_(0),
      draw_count_(0),
#endif
      mouse_down_hittest_(ViewInterface::HT_CLIENT),
      resize_drag_(false),
      old_width_(0),
      old_height_(0) {
  setMouseTracking(true);
  setAcceptDrops(true);
  AdjustToViewSize();
  if (!flags.testFlag(QtViewWidget::FLAG_WM_DECORATED)) {
    setWindowFlags(Qt::FramelessWindowHint);
    SetUndecoratedWMProperties();
  }
  setAttribute(Qt::WA_InputMethodEnabled);
  setAttribute(Qt::WA_OpaquePaintEvent);
  setAttribute(Qt::WA_NoSystemBackground);
  setAutoFillBackground(false);
}

QtViewWidget::~QtViewWidget() {
  DLOG("Widget freed");
  if (child_) {
    // Because I don't own child_, just let it go
    child_->setParent(NULL);
  }
  if (offscreen_pixmap_) delete offscreen_pixmap_;
  if (drag_files_) delete [] drag_files_;
  if (drag_urls_) delete [] drag_urls_;
}

static QRegion CreateClipRegion(const ClipRegion *view_region, double zoom) {
  QRegion qregion;
  QRect qrect;
  size_t count = view_region->GetRectangleCount();
  if (count) {
    Rectangle rect;
    for (size_t i = 0; i < count; ++i) {
      rect = view_region->GetRectangle(i);
      if (zoom != 1.0) {
        rect.Zoom(zoom);
        rect.Integerize(true);
      }
      qrect.setX(static_cast<int>(rect.x));
      qrect.setY(static_cast<int>(rect.y));
      qrect.setWidth(static_cast<int>(rect.w));
      qrect.setHeight(static_cast<int>(rect.h));
      qregion += qrect;
    }
  }
  return qregion;
}

void QtViewWidget::paintEvent(QPaintEvent *event) {
  if (!view_) return;
  QPainter p(this);
  view_->Layout();

  DLOG("paint: %p, ow:%d, oh:%d, w:%d, h:%d, vw:%f, vh:%f, uw:%d, uh:%d",
       view_, old_width_, old_height_, width(), height(),
       view_->GetWidth(), view_->GetHeight(),
       event->rect().width(), event->rect().height());

  if (old_width_ != width() || old_height_ != height() ||
      rect() != event->rect()) {
    view_->AddRectangleToClipRegion(
        Rectangle(0, 0, view_->GetWidth(), view_->GetHeight()));
    old_width_ = width();
    old_height_ = height();
    delete offscreen_pixmap_;
    offscreen_pixmap_ = NULL;
    p.setClipRect(rect());
  }

  uint64_t current_time = GetGlobalMainLoop()->GetCurrentTime();
  if (enable_input_mask_ &&
      (current_time - last_mask_time_ > kUpdateMaskInterval)) {
    last_mask_time_ = current_time;
    // Only update input mask once per second.
    if (!offscreen_pixmap_) {
      offscreen_pixmap_ = new QPixmap(width(), height());
      offscreen_pixmap_->fill(Qt::transparent);
      view_->AddRectangleToClipRegion(
          Rectangle(0, 0, view_->GetWidth(), view_->GetHeight()));
    }

    QRegion clip_region = CreateClipRegion(view_->GetClipRegion(), zoom_);
    QPainter poff(offscreen_pixmap_);
    poff.setClipRegion(clip_region);
    poff.setCompositionMode(QPainter::CompositionMode_Clear);
    if (composite_) {
      poff.fillRect(rect(), Qt::transparent);
    } else {
      poff.fillRect(rect(), palette().window());
    }
    poff.scale(zoom_, zoom_);

    QtCanvas canvas(width(), height(), &poff);
    view_->Draw(&canvas);
    SetInputMask(offscreen_pixmap_);

    p.setClipRegion(clip_region, Qt::UniteClip);
    p.setCompositionMode(QPainter::CompositionMode_Source);
    p.drawPixmap(0, 0, *offscreen_pixmap_);
  } else {
    QRegion clip_region = CreateClipRegion(view_->GetClipRegion(), zoom_);
    p.setClipRegion(clip_region, Qt::UniteClip);
    p.setCompositionMode(QPainter::CompositionMode_Source);
    if (composite_) {
      p.fillRect(rect(), Qt::transparent);
    } else {
      p.fillRect(rect(), palette().window());
    }
    p.scale(zoom_, zoom_);
    QtCanvas canvas(width(), height(), &p);
    view_->Draw(&canvas);
  }

#ifdef _DEBUG
  ++draw_count_;
  uint64_t duration = current_time - last_fps_time_;
  if (duration >= kFPSCountDuration) {
    last_fps_time_ = current_time;
    DLOG("FPS of View %s: %f", view_->GetCaption().c_str(),
         static_cast<double>(draw_count_ * 1000) /
         static_cast<double>(duration));
    draw_count_ = 0;
  }
#endif
}

void QtViewWidget::mouseDoubleClickEvent(QMouseEvent * event) {
  if (!view_) return;
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
  if (!view_) return;
  int buttons = GetMouseButtons(event->buttons());
  if (buttons != MouseEvent::BUTTON_NONE) {
    grabMouse();

    if (!mouse_drag_moved_) {
      // Ignore tiny movement of mouse.
      QPoint offset = QCursor::pos() - mouse_pos_;
      if (std::abs(static_cast<double>(offset.x())) < kDragThreshold &&
          std::abs(static_cast<double>(offset.y())) < kDragThreshold)
        return;
    }
  }

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
      mouse_drag_moved_ = true;
      MouseEvent e(Event::EVENT_MOUSE_UP,
                   event->x() / zoom_, event->y() / zoom_,
                   0, 0, buttons, 0);
      // Ignore the result of this fake event.
      view_->OnMouseEvent(e);

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
      rect.setTop(rect.top() + top_ * p.y());
      rect.setBottom(rect.bottom() + bottom_ * p.y());
      rect.setLeft(rect.left() + left_ * p.x());
      rect.setRight(rect.right() + right_ * p.x());
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
  if (!view_) return;
  if (!hasFocus()) {
    setFocus(Qt::MouseFocusReason);
    SimpleEvent e(Event::EVENT_FOCUS_IN);
    view_->OnOtherEvent(e);
  }

  mouse_down_hittest_ = view_->GetHitTest();
  mouse_drag_moved_ = false;
  resize_drag_ = false;
  // Remember the position of mouse, it may be used to move the view or
  // resize the view
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
  if (!view_ || mouse_drag_moved_)
    return;

  EventResult handler_result = ggadget::EVENT_RESULT_UNHANDLED;
  int button = GetMouseButton(event->button());

  MouseEvent e(Event::EVENT_MOUSE_UP,
               event->x() / zoom_, event->y() / zoom_, 0, 0, button, 0);
  handler_result = view_->OnMouseEvent(e);
  if (handler_result != ggadget::EVENT_RESULT_UNHANDLED)
    event->accept();

  MouseEvent e1(event->button() == Qt::LeftButton ? Event::EVENT_MOUSE_CLICK :
                                                    Event::EVENT_MOUSE_RCLICK,
               event->x() / zoom_, event->y() / zoom_, 0, 0, button, 0);
  handler_result = view_->OnMouseEvent(e1);

  if (handler_result != ggadget::EVENT_RESULT_UNHANDLED)
    event->accept();
}

void QtViewWidget::enterEvent(QEvent *event) {
  if (!view_) return;
  MouseEvent e(Event::EVENT_MOUSE_OVER,
      0, 0, 0, 0,
      MouseEvent::BUTTON_NONE, 0);
  if (view_->OnMouseEvent(e) != ggadget::EVENT_RESULT_UNHANDLED)
    event->accept();
}

void QtViewWidget::leaveEvent(QEvent *event) {
  if (!view_) return;
  MouseEvent e(Event::EVENT_MOUSE_OUT,
               0, 0, 0, 0,
               MouseEvent::BUTTON_NONE, 0);
  if (view_->OnMouseEvent(e) != ggadget::EVENT_RESULT_UNHANDLED)
    event->accept();
}

void QtViewWidget::wheelEvent(QWheelEvent * event) {
  if (!view_) return;

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
  if (!view_) return;

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
  if (!view_) return;

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

// We treat inputMethodEvent as special KeyboardEvent.
void QtViewWidget::inputMethodEvent(QInputMethodEvent *event) {
  if (!view_) return;
  KeyboardEvent e(Event::EVENT_KEY_DOWN, 0, 0, event);
  if (view_->OnKeyEvent(e) != ggadget::EVENT_RESULT_UNHANDLED)
    event->accept();
}

void QtViewWidget::dragEnterEvent(QDragEnterEvent *event) {
  if (!view_) return;
  DLOG("drag enter");

  bool accept = false;
  delete [] drag_files_;
  delete [] drag_urls_;
  drag_files_ = drag_urls_ = NULL;
  drag_text_ = std::string();
  drag_files_and_urls_.clear();

  if (event->mimeData()->hasText()) {
    drag_text_ = event->mimeData()->text().toUtf8().data();
    accept = true;
  }
  if (event->mimeData()->hasUrls()) {
    QList<QUrl> urls = event->mimeData()->urls();
    drag_files_ = new const char *[urls.size() + 1];
    drag_urls_ = new const char *[urls.size() + 1];
    if (!drag_files_ || !drag_urls_) {
      delete [] drag_files_;
      delete [] drag_urls_;
      drag_files_ = drag_urls_ = NULL;
      return;
    }

    size_t files_count = 0;
    size_t urls_count = 0;
    for (int i = 0; i < urls.size(); i++) {
      std::string url = urls[i].toEncoded().data();
      if (url.length()) {
        if (IsValidFileURL(url.c_str())) {
          std::string path = GetPathFromFileURL(url.c_str());
          if (path.length()) {
            drag_files_and_urls_.push_back(path);
            drag_files_[files_count++] =
                drag_files_and_urls_[drag_files_and_urls_.size() - 1].c_str();
          }
        } else if (IsValidURL(url.c_str())) {
          drag_files_and_urls_.push_back(url);
          drag_urls_[urls_count++] =
              drag_files_and_urls_[drag_files_and_urls_.size() - 1].c_str();
        }
      }
    }

    drag_files_[files_count] = NULL;
    drag_urls_[urls_count] = NULL;
    accept = accept || files_count || urls_count;
  }

  if (accept)
    event->acceptProposedAction();
}

void QtViewWidget::dragLeaveEvent(QDragLeaveEvent *event) {
  if (!view_) return;
  DLOG("drag leave");
  DragEvent drag_event(Event::EVENT_DRAG_OUT, 0, 0);
  drag_event.SetDragFiles(drag_files_);
  drag_event.SetDragUrls(drag_urls_);
  drag_event.SetDragText(drag_text_.length() ? drag_text_.c_str() : NULL);
  view_->OnDragEvent(drag_event);
}

void QtViewWidget::dragMoveEvent(QDragMoveEvent *event) {
  if (!view_) return;
  DragEvent drag_event(Event::EVENT_DRAG_MOTION,
                       event->pos().x(), event->pos().y());
  drag_event.SetDragFiles(drag_files_);
  drag_event.SetDragUrls(drag_urls_);
  drag_event.SetDragText(drag_text_.length() ? drag_text_.c_str() : NULL);
  if (view_->OnDragEvent(drag_event) != EVENT_RESULT_UNHANDLED)
    event->acceptProposedAction();
  else
    event->ignore();
}

void QtViewWidget::dropEvent(QDropEvent *event) {
  if (!view_) return;
  LOG("drag drop");
  DragEvent drag_event(Event::EVENT_DRAG_DROP,
                       event->pos().x(), event->pos().y());
  drag_event.SetDragFiles(drag_files_);
  drag_event.SetDragUrls(drag_urls_);
  drag_event.SetDragText(drag_text_.length() ? drag_text_.c_str() : NULL);
  if (view_->OnDragEvent(drag_event) == EVENT_RESULT_UNHANDLED) {
    event->ignore();
  }
}

void QtViewWidget::resizeEvent(QResizeEvent *event) {
  /**
   * TODO: Only after ggl-plasma gets the layout issue fixed can we apply this
   * procession to view host other than options.
   */
  if (!view_ ||
      view_->GetViewHost()->GetType() != ViewHostInterface::VIEW_HOST_OPTIONS)
    return;
  QSize s = event->size();
  DLOG("resizeEvent: %d, %d", s.width(), s.height());
  double w = s.width();
  double h = s.height();
  if (w == view_->GetWidth() && h == view_->GetHeight())
    return;
  if (view_->OnSizing(&w, &h)) {
    view_->SetSize(w, h);
  }
}

void QtViewWidget::focusInEvent(QFocusEvent *event) {
  if (!view_) return;
  SimpleEvent e(Event::EVENT_FOCUS_IN);
  view_->OnOtherEvent(e);
}

void QtViewWidget::focusOutEvent(QFocusEvent *event) {
  if (!view_) return;
  SimpleEvent e(Event::EVENT_FOCUS_OUT);
  view_->OnOtherEvent(e);
}

QSize QtViewWidget::sizeHint() const {
  if (!view_) return QWidget::sizeHint();
  int w = D2I(view_->GetWidth() * zoom_);
  int h = D2I(view_->GetHeight() * zoom_);
  if (w == 0 || h == 0) {
    double dw, dh;
    view_->GetDefaultSize(&dw, &dh);
    w = D2I(dw * zoom_);
    h = D2I(dh * zoom_);
  }

  DLOG("sizeHint: %d, %d", w, h);
  return QSize(w, h);
}

void QtViewWidget::EnableInputShapeMask(bool enable) {
  if (!support_input_mask_) return;
  if (enable_input_mask_ != enable) {
    enable_input_mask_ = enable;
    if (!enable) {
      SetInputMask(NULL);
      delete offscreen_pixmap_;
      offscreen_pixmap_ = NULL;
    }
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
  if (!view_) return;
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
  SetUndecoratedWMProperties();
  show();
}

void QtViewWidget::SetView(ViewInterface *view) {
  if (view_ != view) {
    view_ = view;
    if (view) {
      zoom_ = view->GetGraphics()->GetZoom();
      AdjustToViewSize();
    }
  }
}

#if defined(Q_WS_X11) && defined(HAVE_X11)
static void SetWMState(Window w, const char *property_name) {
  Display *dpy = QX11Info::display();
  Atom property = XInternAtom(dpy, property_name, False);
  Atom net_wm_state = XInternAtom(dpy, "_NET_WM_STATE", False);
  XChangeProperty(dpy, w, net_wm_state, XA_ATOM, 32, PropModeAppend,
                  (unsigned char *)&property, 1);
}
#endif

void QtViewWidget::SetUndecoratedWMProperties() {
#if defined(Q_WS_X11) && defined(HAVE_X11)
  SetWMState(winId(), "_NET_WM_STATE_SKIP_TASKBAR");
  SetWMState(winId(), "_NET_WM_STATE_SKIP_PAGER");

  // Shows on all desktops
  Display *dpy = QX11Info::display();
  int32_t desktop = -1;
  Atom property = XInternAtom(dpy, "_NET_WM_DESKTOP", False);
  XChangeProperty(dpy, winId(), property, XA_CARDINAL, 32, PropModeReplace,
                  (unsigned char *)&desktop, 1);
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

void QtViewWidget::UnsetMinimumSizeHint() {
  setMinimumSize(0, 0);
}

}
}
#include "qt_view_widget.moc"
