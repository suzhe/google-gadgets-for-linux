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

#ifndef GGADGET_QT_QT_GADGET_WIDGET_H__
#define GGADGET_QT_QT_GADGET_WIDGET_H__

#include <string>
#include <vector>
// #include <QtOpenGL/QGLWidget>
#include <QtGui/QWidget>
#include <ggadget/view_interface.h>
#include <ggadget/view_host_interface.h>
#include "qt_canvas.h"

namespace ggadget {
namespace qt {

class QGadgetWidget : public QWidget {
 public:
  QGadgetWidget(ViewInterface* view, ViewHostInterface *host, QtGraphics *g);
 protected:
  virtual void paintEvent(QPaintEvent *event);
  virtual void mouseDoubleClickEvent(QMouseEvent *event);
  virtual void mouseMoveEvent(QMouseEvent *event);
  virtual void mousePressEvent(QMouseEvent *event);
  virtual void mouseReleaseEvent(QMouseEvent *event);
  virtual void enterEvent(QEvent *event);
  virtual void leaveEvent(QEvent *event);
  virtual void keyPressEvent(QKeyEvent *event);
  virtual void keyReleaseEvent(QKeyEvent *event);
  virtual void dragEnterEvent(QDragEnterEvent *event);
  virtual void dragLeaveEvent(QDragLeaveEvent *event);
  virtual void dragMoveEvent(QDragMoveEvent *event);
  virtual void dropEvent(QDropEvent *event);
  virtual void resizeEvent(QResizeEvent *event);
  QtCanvas *canvas_;
  QtGraphics *graphics_;
  ViewInterface *view_;
  ViewHostInterface *view_host_;
  double width_, height_;  // The view size
  const char **drag_files_;
  std::vector<std::string> drag_urls_;
  uint64_t mouse_down_time_;
  double zoom_;
};


} // end of namespace qt
} // end of namespace ggadget

#endif // GGADGET_QT_QT_GADGET_VIEW_WIDGET_H__
