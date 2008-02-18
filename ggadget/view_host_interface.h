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

#include <ggadget/view_interface.h>

namespace ggadget {

template <typename R, typename P1> class Slot1;
class GadgetHostInterface;
class GraphicsInterface;
class ScriptContextInterface;

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

  /**
   * Gets information about the native widget.
   * @param[out] native_widget the native widget handle of this view host.
   * @param[out] x the horizontal offset of this view host in the native widget.
   * @param[out] y the vertical offset of this view host in the native widget.
   */
  virtual void *GetNativeWidget() = 0;

  /**
   * Converts coordinates in the view's space to coordinates in the native
   * widget which holds the view.
   *
   * @param x x-coordinate in the view's space to convert.
   * @param y y-coordinate in the view's space to convert.
   * @param[out] widget_x parameter to store the converted widget x-coordinate.
   * @param[out] widget_y parameter to store the converted widget y-coordinate.
   */
  virtual void ViewCoordToNativeWidgetCoord(
      double x, double y, double *widget_x, double *widget_y) = 0;

  /** Asks the host to redraw the given view. */
  virtual void QueueDraw() = 0;

  /** Asks the host to deliver keyboard events to the view. */
  virtual bool GrabKeyboardFocus() = 0;

  /**
   * When the resizable field on the view is updated, the host needs to be
   * alerted of this change.
   */
  virtual void SetResizable(ViewInterface::ResizableMode mode) = 0;

  /**
   * Sets a caption to be shown when the View is in floating or expanded
   * mode.
   */
  virtual void SetCaption(const char *caption) = 0;

  /** Sets whether to always show the caption for this view. */
  virtual void SetShowCaptionAlways(bool always) = 0;

  enum CursorType {
    CURSOR_ARROW,
    CURSOR_IBEAM,
    CURSOR_WAIT,
    CURSOR_CROSS,
    CURSOR_UPARROW,
    CURSOR_SIZE,
    CURSOR_SIZENWSE,
    CURSOR_SIZENESW,
    CURSOR_SIZEWE,
    CURSOR_SIZENS,
    CURSOR_SIZEALL,
    CURSOR_NO,
    CURSOR_HAND,
    CURSOR_BUSY,
    CURSOR_HELP,
  };

  /** Sets the current mouse cursor. */
  virtual void SetCursor(CursorType type) = 0;

  /**
   * Shows a tooltip popup after certain initial delay at the current mouse
   * position . The implementation should handle tooltip auto-hiding.
   * @param tooltip the tooltip to display. If @c NULL or blank, currently
   *     displayed tooltip will be hidden.
   */
  virtual void SetTooltip(const char *tooltip) = 0;

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

  /** Displays a message box containing the message string. */
  virtual void Alert(const char *message) = 0;

  /**
   * Displays a dialog containing the message string and Yes and No buttons.
   * @param message the message string.
   * @return @c true if Yes button is pressed, @c false if not.
   */
  virtual bool Confirm(const char *message) = 0;

  /**
   * Displays a dialog asking the user to enter text.
   * @param message the message string displayed before the edit box.
   * @param default_value the initial default value dispalyed in the edit box.
   * @return the user inputted text, or an empty string if user canceled the
   *     dialog.
   */
  virtual std::string Prompt(const char *message,
                             const char *default_value) = 0;
};

} // namespace ggadget

#endif // GGADGET_VIEW_HOST_INTERFACE_H__
