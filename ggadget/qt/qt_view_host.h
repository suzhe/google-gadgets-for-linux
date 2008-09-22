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

#ifndef GGADGET_QT_QT_VIEW_HOST_H__
#define GGADGET_QT_QT_VIEW_HOST_H__

#include <set>

#include <QtGui/QWidget>
#include <QtCore/QTimer>
#include <ggadget/view_interface.h>
#include <ggadget/view_host_interface.h>
#include <ggadget/qt/qt_view_widget.h>
#include <ggadget/qt/qt_graphics.h>

namespace ggadget {
namespace qt {

class QtViewHostObject;
class QtViewHost : public ViewHostInterface {
 public:
  /**
   * @param parent If not null, this view host will be shown at the popup
   *               position of parent
   */
  QtViewHost(ViewHostInterface::Type type,
             double zoom, bool composite, bool decorated,
             bool record_states, int debug_mode, QWidget* parent);
  virtual ~QtViewHost();

  virtual Type GetType() const;
  virtual void Destroy();
  virtual void SetView(ViewInterface *view);
  virtual ViewInterface *GetView() const;
  virtual GraphicsInterface *NewGraphics() const;
  virtual void *GetNativeWidget() const;
  virtual void ViewCoordToNativeWidgetCoord(
      double x, double y, double *widget_x, double *widget_y) const;
  virtual void NativeWidgetCoordToViewCoord(
      double x, double y, double *view_x, double *view_y) const;
  virtual void QueueDraw();
  virtual void QueueResize();
  virtual void EnableInputShapeMask(bool enable);
  virtual void SetResizable(ViewInterface::ResizableMode mode);
  virtual void SetCaption(const char *caption);
  virtual void SetShowCaptionAlways(bool always);
  virtual void SetCursor(int type);
  virtual void SetTooltip(const char *tooltip);
  virtual bool ShowView(bool modal, int flags,
                        Slot1<bool, int> *feedback_handler);
  virtual void CloseView();
  virtual bool ShowContextMenu(int button);
  virtual void BeginResizeDrag(int button, ViewInterface::HitTest hittest) {}
  virtual void BeginMoveDrag(int button);

  virtual void Alert(const ViewInterface *view, const char *message);
  virtual bool Confirm(const ViewInterface *view, const char *message);
  virtual std::string Prompt(const ViewInterface *view,
                             const char *message,
                             const char *default_value);
  virtual int GetDebugMode() const;

  friend class QtViewHostObject;
  QtViewHostObject *GetQObject();

 private:
  class Impl;
  Impl *impl_;
  DISALLOW_EVIL_CONSTRUCTORS(QtViewHost);
};

class QtViewHostObject : public QObject {
  Q_OBJECT
 public:
  QtViewHostObject(QtViewHost::Impl* owner) : owner_(owner) {}

 public slots:
  void OnOptionViewOK();
  void OnOptionViewCancel();
  void OnViewWidgetClose(QObject*);
  void OnShow(bool);

 private:
  QtViewHost::Impl* owner_;
};

} // namespace qt
} // namespace ggadget

#endif // GGADGET_QT_QT_VIEW_HOST_H__
