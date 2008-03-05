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
#include <QGLWidget>
#include <ggadget/view_interface.h>
#include "qt_canvas.h"

namespace ggadget {
namespace qt {
class QtViewHost;

class QGadgetWidget : public QWidget {
 public:
  QGadgetWidget(QtViewHost* host, double zoom,
                bool composited, bool useshapemask);
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
  QtViewHost *view_host_;
  QtCanvas *canvas_;
  ViewInterface *view_;
  size_t width_, height_;
  uint64_t mouse_down_time_;
};


} // end of namespace qt
} // end of namespace ggadget

#endif // GGADGET_QT_QT_GADGET_VIEW_WIDGET_H__
