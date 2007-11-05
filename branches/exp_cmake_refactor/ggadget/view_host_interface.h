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

#ifndef GGADGET_VIEW_HOST_INTERFACE_H__
#define GGADGET_VIEW_HOST_INTERFACE_H__

namespace ggadget {

class GadgetHostInterface;
class GraphicsInterface;
class ScriptContextInterface;
class ViewInterface;

/**
 * Interface for providing host services to views.. Each view contains a
 * pointer to a @c ViewHostInterface object which is dedicated to the view.
 * The @c ViewHostInterface implementation should depend on the host.
 * The services provided by @c ViewHostInterface are bi-directional.
 * The view calls methods in the @c ViewHostInterface, and the host callback
 * to the view's event handler methods.
 */
class ViewHostInterface {
 public:
  virtual ~ViewHostInterface() { }

  /** Get the @c GadgetHostInterface which owns this view host. */
  virtual GadgetHostInterface *GetGadgetHost() const = 0;

  /** Get the associated view. */
  virtual ViewInterface *GetView() = 0;

  /** Get the associated view. */
  virtual const ViewInterface *GetView() const = 0;

  /** Get the associated @c ScriptContextInterface instance. */
  virtual ScriptContextInterface *GetScriptContext() const = 0;

  /** Returns the @c GraphicsInterface associated with this host. */
  virtual const GraphicsInterface *GetGraphics() const = 0;

  /** Asks the host to redraw the given view. */
  virtual void QueueDraw() = 0;

  /** Asks the host to deliver keyboard events to the view. */
  virtual bool GrabKeyboardFocus() = 0;

  /**
   * When the resizable field on the view is updated, the host needs to be
   * alerted of this change.
   */
  virtual void SetResizeable() = 0;

  /**
   * Sets a caption to be shown when the View is in floating or expanded
   * mode.
   */
  virtual void SetCaption(const char *caption) = 0;

  /** Sets whether to always show the caption for this view. */
  virtual void SetShowCaptionAlways(bool always) = 0;

  // TODO: Add other methods about menu, tooltip, etc.
};

} // namespace ggadget

#endif // GGADGET_VIEW_HOST_INTERFACE_H__
