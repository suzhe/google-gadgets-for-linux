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

#ifndef GGADGET_DECORATED_VIEW_HOST_H__
#define GGADGET_DECORATED_VIEW_HOST_H__

#include <ggadget/common.h>
#include <ggadget/view_interface.h>
#include <ggadget/view_host_interface.h>

namespace ggadget {

/**
 * DecoratedViewHost shows a view with the appropiate decorations. Internally,
 * it creates another view that draws the decorations along with the given view.
 *
 * Only main and details view can have decorator.
 */
class DecoratedViewHost : public ViewHostInterface {
 protected:
  virtual ~DecoratedViewHost();

 public:
  enum DecoratorType {
    MAIN_DOCKED,     /**< For main view in the Sidebar. */
    MAIN_STANDALONE, /**< For main view in standalone window. */
    MAIN_EXPANDED,   /**< For main view in expanded window. */
    DETAILS,         /**< For details view. */
  };

  /**
   * Constructor.
   *
   * @param view_host The ViewHost to contain the decorator view.
   * @param decorator_type Type of the decorator, must match the type of
   *        outer view host.
   * @param transparent If it's true then transparent background will be used.
   */
  DecoratedViewHost(ViewHostInterface *view_host,
                    DecoratorType decorator_type,
                    bool transparent);

  /** Gets the decorator type. */
  DecoratorType GetDecoratorType() const;

  /**
   * Gets the view which contains the decoration and the child view.
   * The caller shall not destroy the returned view.
   */
  ViewInterface *GetDecoratedView() const;

  /**
   * Connects a handler to OnDock signal.
   * This signal will be emitted when dock menu item is activated by user.
   * Host shall connect to this signal and perform the real dock action.
   */
  Connection *ConnectOnDock(Slot0<void> *slot);

  /**
   * Connects a handler to OnUndock signal.
   * This signal will be emitted when undock menu item is activated by user.
   * Host shall connect to this signal and perform the real dock action.
   */
  Connection *ConnectOnUndock(Slot0<void> *slot);

  /**
   * Connects a handler to OnPopOut signal.
   * This signal will be emitted when popout button is clicked by user.
   * Host shall connect to this signal and perform the real popout action.
   */
  Connection *ConnectOnPopOut(Slot0<void> *slot);

  /**
   * Connects a handler to OnPopIn signal.
   * This signal will be emitted when popin or close button is clicked by user.
   * Host shall connect to this signal and perform the real popin action.
   */
  Connection *ConnectOnPopIn(Slot0<void> *slot);

  /**
   * Connects a handler to OnClose signal.
   * This signal will be emitted when close button is clicked by user.
   * Host shall connect to this signal and perform the real close action.
   */
  Connection *ConnectOnClose(Slot0<void> *slot);

  /**
   * Sets screen edge which is current snapping the docked decorator.
   *
   * It's only valid for DecoratedViewHost with MAIN_DOCKED type.
   * Only left and right are supported.
   *
   * @param right true to select right edge, otherwise choose left edge.
   */
  void SetDockEdge(bool right);

  /**
   * Lets the view decorator to restore previously stored child view size.
   */
  void RestoreViewSize();

  /**
   * Enables or disables automatically restore child view size.
   *
   * If it's enabled, then the view decorator will restore previously stored
   * child view size automatically when the child view is attached to the
   * decorator or it's shown the first time.
   */
  void EnableAutoRestoreViewSize(bool enable);

  /**
   * Gets or sets minimized state.
   *
   * It's only applicable for MainView decorator. It should be called after
   * attaching the view.
   */
  bool IsMinimized() const;
  void SetMinimized(bool minimized);

 public:

  /** Returns the ViewHost type. */
  virtual Type GetType() const;
  virtual void Destroy();

  /** Sets the inner view which will be hosted in the decorator. */
  virtual void SetView(ViewInterface *view);

  /** Gets the inner view hosted in the decorator. */
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
                        Slot1<void, int> *feedback_handler);
  virtual void CloseView();
  virtual bool ShowContextMenu(int button);
  virtual void Alert(const ViewInterface *view, const char *message);
  virtual bool Confirm(const ViewInterface *view, const char *message);
  virtual std::string Prompt(const ViewInterface *view, const char *message,
                             const char *default_value);
  virtual int GetDebugMode() const;

  virtual void BeginResizeDrag(int button, ViewInterface::HitTest hittest);
  virtual void BeginMoveDrag(int button);

 private:
  DISALLOW_EVIL_CONSTRUCTORS(DecoratedViewHost);
  class Impl;
  Impl *impl_;
};

} // namespace ggadget

#endif  // GGADGET_DECORATED_VIEW_HOST_H__
