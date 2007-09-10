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

#ifndef GGADGET_VIEW_INTERFACE_H__
#define GGADGET_VIEW_INTERFACE_H__

#include "host_interface.h"

namespace ggadget {

class CanvasInterface;
class KeyboardEvent;
class MouseEvent;
class Event;

/**
 * Interface for representing a View in the Gadget API.
 */
class ViewInterface {
 public:
  /** 
   * Attach a view to a host displaying it. A view may only be associated with
   * one host at a time.
   * @param host HostInterface to attach to this view. 
   *   Pass in NULL to detach view from any host.
   * @return true on success, false otherwise. 
   */
  virtual bool AttachHost(HostInterface *host) = 0;
    
  /** Handler of the mouse button down event. */
  virtual void OnMouseDown(const MouseEvent *event) = 0;
  /** Handler of the mouse button up event. */
  virtual void OnMouseUp(const MouseEvent *event) = 0;
  /** Handler of the mouse button click event. */
  virtual void OnMouseClick(const MouseEvent *event) = 0;
  /** Handler of the mouse button double click event. */
  virtual void OnMouseDblClick(const MouseEvent *event) = 0;
  /** Handler of the mouse move event. */
  virtual void OnMouseMove(const MouseEvent *event) = 0;
  /** Handler of the mouse out event. */
  virtual void OnMouseOut(const MouseEvent *event) = 0;
  /** Handler of the mouse over event. */
  virtual void OnMouseOver(const MouseEvent *event) = 0;
  /** Handler of the mouse wheel event. */
  virtual void OnMouseWheel(const MouseEvent *event) = 0;

  /** Handler of the keyboard key down event. */
  virtual void OnKeyDown(const KeyboardEvent *event) = 0;
  /** Handler of the keyboard key up event. */
  virtual void OnKeyUp(const KeyboardEvent *event) = 0;  
  /** Handler of the keyboard key press event. */
  virtual void OnKeyPress(const KeyboardEvent *event) = 0;
  
  /** Indicate that the view is now receiving keyboard events. */
  virtual void OnFocusIn(const Event *event) = 0;
  /** Indicate that focus has lost. */
  virtual void OnFocusOut(const Event *event) = 0;

  /** 
   * Set the width of the view.
   * @return true if new size is allowed, false otherwise. 
   * */
  virtual bool SetWidth(int width) = 0;
  /** 
   * Set the height of the view. 
   * @return true if new size is allowed, false otherwise.
   */
  virtual bool SetHeight(int height) = 0;
  /** 
   * Set the size of the view. Use this when setting both height and width 
   * to prevent two invocations of the sizing event. 
   * @return true if new size is allowed, false otherwise.
   * */
  virtual bool SetSize(int width, int height) = 0;
    
  /** Retrieves the width of the view in pixels. */
  virtual int GetWidth() const = 0;
  /** Retrieves the height of view in pixels. */
  virtual int GetHeight() const = 0;
  
  /**
   * Draws the current view to a canvas. The caller does NOT own this canvas
   * and should not free it.
   * @param[out] changed True if the returned canvas is different from that 
   *   of the last call, false otherwise.
   * @return A canvas suitable for drawing.
   */
  virtual const CanvasInterface *Draw(bool *changed) = 0; 
    
  virtual ~ViewInterface() {}
};

} // namespace ggadget

#endif // GGADGET_VIEW_INTERFACE_H__
