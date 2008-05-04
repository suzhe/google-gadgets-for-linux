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

#ifndef GGADGET_QT_QT_VIEW_HOST_H__
#define GGADGET_QT_QT_VIEW_HOST_H__

#include <set>

#include <QtGui/QWidget>
#include <QtCore/QTimer>
#include <ggadget/view_interface.h>
#include <ggadget/view_host_interface.h>
#include <ggadget/qt/qt_gadget_widget.h>
#include <ggadget/qt/qt_graphics.h>

namespace ggadget {
namespace qt {

class QtViewHostObject;
class QtViewHost : public ViewHostInterface {
 public:
  QtViewHost(ViewHostInterface::Type type,
             double zoom, bool decorated,
             int debug_mode);
  virtual ~QtViewHost();

  virtual Type GetType() const { return type_; }
  virtual void Destroy();
  virtual void SetView(ViewInterface *view);
  virtual ViewInterface *GetView() const { return view_; }
  virtual GraphicsInterface *NewGraphics() const { return new QtGraphics(zoom_); }
  virtual void *GetNativeWidget() const { return widget_; }
  virtual void ViewCoordToNativeWidgetCoord(
      double x, double y, double *widget_x, double *widget_y) const;
  virtual void QueueDraw();
  virtual void QueueResize();
  virtual void SetResizable(ViewInterface::ResizableMode mode);
  virtual void SetCaption(const char *caption);
  virtual void SetShowCaptionAlways(bool always);
  virtual void SetCursor(int type);
  virtual void SetTooltip(const char *tooltip);
  virtual bool ShowView(bool modal, int flags,
                        Slot1<void, int> *feedback_handler);
  virtual void CloseView();
  virtual bool ShowContextMenu(int button);
  virtual void BeginResizeDrag(int button, ViewInterface::HitTest hittest) {}
  virtual void BeginMoveDrag(int button) {}

  virtual void Alert(const char *message);
  virtual bool Confirm(const char *message);
  virtual std::string Prompt(const char *message,
                             const char *default_value);
  virtual int GetDebugMode() const { return debug_mode_; }
  void HandleOptionViewResponse(ViewInterface::OptionsViewFlags flag);
  void HandleDetailsViewClose();

 private:
  ViewInterface *view_;
  ViewHostInterface::Type type_;
  QGadgetWidget *widget_;
  QWidget *window_;     // Top level window of the view
  QDialog *dialog_;     // Top level window of the view
  int debug_mode_;
  double zoom_;
  Connection *onoptionchanged_connection_;

  static const unsigned int kShowTooltipDelay = 500;
  static const unsigned int kHideTooltipDelay = 4000;

  Slot1<void, int> *feedback_handler_;

  bool composite_;
  QtViewHostObject *qt_obj_;    // used for handling qt signal

  void Detach();

  DISALLOW_EVIL_CONSTRUCTORS(QtViewHost);
};

class QtViewHostObject : public QObject {
  Q_OBJECT
 public:
  QtViewHostObject(QtViewHost* owner) : owner_(owner) {}

 public slots:
  void OnOptionViewOK() {
    owner_->HandleOptionViewResponse(ViewInterface::OPTIONS_VIEW_FLAG_OK);
  }
  void OnOptionViewCancel() {
    owner_->HandleOptionViewResponse(ViewInterface::OPTIONS_VIEW_FLAG_CANCEL);
  }
  void OnDetailsViewClose() {
    owner_->HandleDetailsViewClose();
  }
 private:
  QtViewHost* owner_;
};

} // namespace qt
} // namespace ggadget

#endif // GGADGET_QT_QT_VIEW_HOST_H__
