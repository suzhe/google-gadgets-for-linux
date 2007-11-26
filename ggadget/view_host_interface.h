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

#include <ggadget/element_interface.h>

namespace ggadget {

template <typename R, typename P1> class Slot1;
class GadgetHostInterface;
class GraphicsInterface;
class ScriptContextInterface;
class ViewInterface;
class XMLHttpRequestInterface;

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

  /** Creates a new @c XMLHttpRequestInterface instance. */
  virtual XMLHttpRequestInterface *NewXMLHttpRequest() = 0;

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

  /** Sets the current mouse cursor. */
  virtual void SetCursor(ElementInterface::CursorType type) = 0;

  // TODO: Add other methods about menu, tooltip, etc.

  /** Run the view in a dialog with OK and Cancel buttons. */
  virtual void RunDialog() = 0;

  enum DetailsViewFlags {
    DETAILS_VIEW_FLAG_NONE = 0,
    /** Makes the details view title clickable like a button. */
    DETAILS_VIEW_FLAG_TOOLBAR_OPEN = 1,
    /** Adds a negative feedback button in the details view. */
    DETAILS_VIEW_FLAG_NEGATIVE_FEEDBACK = 2,
    /** Adds a "Remove" button in the details view. */
    DETAILS_VIEW_FLAG_REMOVE_BUTTON = 4,
    /** Adds a button to display the friends list. */
    DETAILS_VIEW_FLAG_SHARE_WITH_BUTTON = 8,
  };

  /**
   * Show the view in a details view.
   * @param title the title of the details view.
   * @param flags combination of @c DetailsViewFlags.
   * @param feedback_handler called when user clicks on feedback buttons. The
   *     handler has one parameter, which specifies @c DetailsViewFlags.
   */
  virtual void ShowInDetailsView(const char *title, int flags,
                                 Slot1<void, int> *feedback_handler) = 0;

  /**
   * Close the details view if it is opened.
   */
  virtual void CloseDetailsView() = 0;

};

} // namespace ggadget

#endif // GGADGET_VIEW_HOST_INTERFACE_H__
