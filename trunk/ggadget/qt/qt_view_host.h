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

#include <ggadget/view_interface.h>
#include <ggadget/view_host_interface.h>
#include <ggadget/qt/qt_gadget_widget.h>
#include <ggadget/qt/qt_graphics.h>
#include <ggadget/qt/qt_gadget_host.h>

namespace ggadget {

class DOMDocumentInterface;
class View;

namespace qt {

/**
 * An implementation of @c ViewHostInterface for the simple gadget host.
 * In this implementation, there is one instance of @c GtkViewHost per view,
 * and one instance of GraphicsInterface per @c GtkViewHost.
 */
class QtViewHost : public ViewHostInterface {
 public:
  QtViewHost(QtGadgetHost *gadget_host,
             GadgetHostInterface::ViewType type,
             ViewInterface *view,
             bool composited, bool useshapemask,
             double zoom);
  virtual ~QtViewHost();

#if 0
  // TODO: This host should encapsulate all widget creating/switching jobs
  // inside of it, so the interface of this method should be revised.
  /**
   * Switches the GadgetViewWidget associated with this host.
   * When this is done, the original GadgetViewWidget will no longer have a
   * valid GtkCairoHost object. The new GadgetViewWidget is responsible for
   * freeing this host.
   */
  void SwitchWidget(GadgetViewWidget *new_gvw);
#endif

  virtual GadgetHostInterface *GetGadgetHost() const {
    return gadget_host_;
  }
  virtual ViewInterface *GetView() { return view_; }
  virtual const ViewInterface *GetView() const { return view_; }
  virtual ScriptContextInterface *GetScriptContext() const {
    return script_context_;
  }
  virtual const GraphicsInterface *GetGraphics() const { return graphics_; }

  virtual void GetNativeWidgetInfo(void **native_widget, int *x, int *y);
  virtual void QueueDraw();
  virtual bool GrabKeyboardFocus();

  virtual void SetResizable(ViewInterface::ResizableMode mode);
  virtual void SetCaption(const char *caption);
  virtual void SetShowCaptionAlways(bool always);
  virtual void SetCursor(CursorType type);
  virtual void SetTooltip(const char *tooltip);
  virtual void RunDialog();
  virtual void ShowInDetailsView(const char *title, int flags,
                                 Slot1<void, int> *feedback_handler);
  virtual void CloseDetailsView();

  virtual void Alert(const char *message);
  virtual bool Confirm(const char *message);
  virtual std::string Prompt(const char *message, const char *default_value);
  virtual void *GetNativeWidget() {  return widget_;  }
  virtual void ViewCoordToNativeWidgetCoord(double x, double y,
                                            double *widget_x, double *widget_y) {
  }

  QGadgetWidget *GetWidget() { return widget_; }

  QtGadgetHost *GetQtGadgetHost() const {
    return gadget_host_;
  }

  void ChangeZoom(double zoom);

 private:
  QtGadgetHost *gadget_host_;
  ViewInterface *view_;
  ScriptContextInterface *script_context_;
  QGadgetWidget *widget_;
  QtGraphics *graphics_;
  Connection *onoptionchanged_connection_;

  static const unsigned int kShowTooltipDelay = 500;
  static const unsigned int kHideTooltipDelay = 4000;
  std::string tooltip_;
  int tooltip_timer_;

  Slot1<void, int> *details_feedback_handler_;

  DISALLOW_EVIL_CONSTRUCTORS(QtViewHost);
};

} // namespace qt
} // namespace ggadget

#endif // GGADGET_QT_QT_VIEW_HOST_H__
