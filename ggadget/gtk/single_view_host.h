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

#ifndef GGADGET_GTK_SINGLE_VIEW_HOST_H__
#define GGADGET_GTK_SINGLE_VIEW_HOST_H__

#include <gtk/gtk.h>
#include <ggadget/view_interface.h>
#include <ggadget/view_host_interface.h>
#include <ggadget/slot.h>
#include <ggadget/signals.h>
#include <ggadget/gtk/cairo_graphics.h>

namespace ggadget {
namespace gtk {

/**
 * An implementation of @c ViewHostInterface for simple gtk host.
 *
 * This host can only show one View in single GtkWindow.
 *
 * Following View events are not implemented:
 * - ondock
 * - onminimize
 * - onpopin
 * - onpopout
 * - onrestore
 * - onundock
 */
class SingleViewHost : public ViewHostInterface {
 public:
  /**
   * @param view The View instance associated to this host.
   * @param zoom Zoom factor used by the Graphics object.
   * @param remove_on_close remove the gadget when the main view is closed.
   * @param record_states records the window states (eg. position),
   *        so that they can be restored next time.
   * @param debug_mode DebugMode when drawing elements.
   */
  SingleViewHost(ViewHostInterface::Type type,
                 double zoom,
                 bool decorated,
                 bool remove_on_close,
                 bool record_states,
                 int debug_mode);
  virtual ~SingleViewHost();

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
  virtual void BeginResizeDrag(int button, ViewInterface::HitTest hittest);

  virtual void BeginMoveDrag(int button);

  virtual void Alert(const ViewInterface *view, const char *message);
  virtual bool Confirm(const ViewInterface *view, const char *message);
  virtual std::string Prompt(const ViewInterface *view, const char *message,
                             const char *default_value);
  virtual int GetDebugMode() const;

 public:
  /** Gets the top level gtk window. */
  GtkWidget *GetWindow() const;

  /** Gets and sets position of the top level window. */
  void GetWindowPosition(int *x, int *y) const;
  void SetWindowPosition(int x, int y);

  /** Gets size of the top level window. */
  void GetWindowSize(int *width, int *height) const;

  /** Gets and sets keep-above state. */
  bool IsKeepAbove() const;
  void SetKeepAbove(bool keep_above);

  /** Checks if the top level window is visible or not. */
  bool IsVisible() const;

  /** Sets the gtk window type hint. */
  void SetWindowType(GdkWindowTypeHint type);

 public:
  /**
   * Connects a slot to OnViewChanged signal.
   *
   * The slot will be called when the attached view has been changed.
   */
  Connection *ConnectOnViewChanged(Slot0<void> *slot);

  /**
   * Connects a slot to OnShowHide signal.
   *
   * The slot will be called when the show/hide state of the top level window
   * has been changed. The first parameter of the slot indicates the new
   * show/hide state, true means the top level window has been shown.
   */
  Connection *ConnectOnShowHide(Slot1<void, bool> *slot);

  /**
   * Connects a slot to OnBeginResizeDrag signal.
   *
   * The slot will be called when BeginResizeDrag() method is called, the first
   * parameter of the slot is the mouse button initiated the drag. See
   * @MouseEvent::Button for definition of mouse button. The second parameter
   * is the hittest value representing the border or corner to be dragged.
   *
   * If the slot returns @false then the default resize drag operation will be
   * performed for the topleve GtkWindow, otherwise no other action will be
   * performed.
   */
  Connection *ConnectOnBeginResizeDrag(Slot2<bool, int, int> *slot);

  /**
   * Connects a slot to OnResized signal.
   *
   * The slot will be called when the top level window size is changed.
   * The two parameters are the new width and height of the window.
   */
  Connection *ConnectOnResized(Slot2<void, int, int> *slot);

  /**
   * Connects a slot to OnEndResizeDrag signal.
   *
   * The slot will be called when the resize drag has been finished.
   */
  Connection *ConnectOnEndResizeDrag(Slot0<void> *slot);

  /**
   * Connects a slot to OnBeginMoveDrag signal.
   *
   * The slot will be called when BeginMoveDrag() method is called, the first
   * parameter of the slot is the mouse button initiated the drag. @See
   * @MouseEvent::Button for definition of mouse button.
   *
   * If the slot returns @false then the default move drag operation will be
   * performed for the topleve GtkWindow, otherwise no other action will be
   * performed.
   */
  Connection *ConnectOnBeginMoveDrag(Slot1<bool, int> *slot);

  /**
   * Connects a slot to OnMoved signal.
   *
   * The slot will be called when the top level window position is changed.
   * The two parameters are the new x and y position of the top level window's
   * top left corner, related to the screen.
   */
  Connection *ConnectOnMoved(Slot2<void, int, int> *slot);

  /**
   * Connects a slot to OnEndMoveDrag signal.
   *
   * The slot will be called when the move drag has been finished.
   */
  Connection *ConnectOnEndMoveDrag(Slot0<void> *slot);

 private:
  DISALLOW_EVIL_CONSTRUCTORS(SingleViewHost);
  class Impl;
  Impl *impl_;
};

} // namespace gtk
} // namespace ggadget

#endif // GGADGET_GTK_SINGLE_VIEW_HOST_H__
