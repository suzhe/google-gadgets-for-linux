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
 */
class DecoratedViewHost : public ViewHostInterface {
 public:
  DecoratedViewHost(ViewHostInterface *outer_view_host, bool background);
  virtual ~DecoratedViewHost();

  virtual Type GetType() const;
  virtual void Destroy();
  virtual void SetView(ViewInterface *view);
  virtual ViewInterface *GetView() const;
  virtual GraphicsInterface *NewGraphics() const;
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

  virtual void BeginResizeDrag(int button, ViewInterface::HitTest hittest);
  virtual void BeginMoveDrag(int button);
  virtual void Dock();
  virtual void Undock();
  virtual void Expand();
  virtual void Unexpand();

 private:
  DISALLOW_EVIL_CONSTRUCTORS(DecoratedViewHost);
  class Impl;
  Impl *impl_;
};

} // namespace ggadget

#endif  // GGADGET_DECORATED_VIEW_HOST_H__
