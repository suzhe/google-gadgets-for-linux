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

#ifndef GGADGET_GTK_SINGLE_VIEW_HOST_H__
#define GGADGET_GTK_SINGLE_VIEW_HOST_H__

#include <gtk/gtk.h>
#include <ggadget/view_interface.h>
#include <ggadget/view_host_interface.h>
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
   * @param debug_mode DebugMode when drawing elements.
   */
  SingleViewHost(ViewHostInterface::Type type,
                 double zoom, bool decorated,
                 ViewInterface::DebugMode debug_mode);
  virtual ~SingleViewHost();

  virtual Type GetType() const;
  virtual void Destroy();
  virtual void SetView(ViewInterface *view);
  virtual ViewInterface *GetView() const;
  virtual const GraphicsInterface *GetGraphics() const;
  virtual void *GetNativeWidget() const;
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
  virtual void Alert(const char *message);
  virtual bool Confirm(const char *message);
  virtual std::string Prompt(const char *message,
                             const char *default_value);
  virtual ViewInterface::DebugMode GetDebugMode() const;

 public:
  /** Returns the toplevel GtkWindow that contains the View. */
  GtkWidget *GetToplevelWindow() const;

 private:
  DISALLOW_EVIL_CONSTRUCTORS(SingleViewHost);
  class Impl;
  Impl *impl_;
};

} // namespace gtk
} // namespace ggadget

#endif // GGADGET_GTK_SINGLE_VIEW_HOST_H__