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

#ifndef GGADGET_QT_QT_VIEW_WIDGET_H__
#define GGADGET_QT_QT_VIEW_WIDGET_H__

#include <string>
#include <vector>
#include <QtGui/QWidget>
#include <ggadget/view_interface.h>
#include <ggadget/view_host_interface.h>
#include "qt_canvas.h"

namespace ggadget {
namespace qt {

class QtViewWidget : public QWidget {
 public:
  QtViewWidget(ViewInterface* view,
                bool composite, bool decorated);
  ~QtViewWidget();
  void EnableInputShapeMask(bool enable);
  void SetChild(QWidget *widget);
  void SkipTaskBar();
  void SetKeepAbove(bool above);

 protected:
  virtual void paintEvent(QPaintEvent *event);
  virtual void mouseDoubleClickEvent(QMouseEvent *event);
  virtual void mouseMoveEvent(QMouseEvent *event);
  virtual void mousePressEvent(QMouseEvent *event);
  virtual void mouseReleaseEvent(QMouseEvent *event);
  virtual void enterEvent(QEvent *event);
  virtual void leaveEvent(QEvent *event);
  virtual void wheelEvent(QWheelEvent * event);
  virtual void keyPressEvent(QKeyEvent *event);
  virtual void keyReleaseEvent(QKeyEvent *event);
  virtual void dragEnterEvent(QDragEnterEvent *event);
  virtual void dragLeaveEvent(QDragLeaveEvent *event);
  virtual void dragMoveEvent(QDragMoveEvent *event);
  virtual void dropEvent(QDropEvent *event);

  void SetInputMask(QPixmap *pixmap);
  void SetSize(int width, int height);
  ViewInterface *view_;
  const char **drag_files_;
  std::vector<std::string> drag_urls_;
  bool composite_;
  bool enable_input_mask_;
  QPixmap *offscreen_pixmap_;
  QPoint mouse_pos_;
  bool mouse_drag_moved_;
  QWidget *child_;
  double zoom_;
  ViewInterface::HitTest mouse_down_hittest_;
  bool resize_drag_;
  QRect origi_geometry_;
  // used as coefficient of mouse move in window resize
  int top_, bottom_, left_, right_;
};


} // end of namespace qt
} // end of namespace ggadget

#endif // GGADGET_QT_QT_VIEW_WIDGET_H__
