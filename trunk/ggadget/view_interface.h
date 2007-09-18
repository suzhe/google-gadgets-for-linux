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

#include "scriptable_interface.h"
#include "signal.h"

namespace ggadget {

class CanvasInterface;
class ElementInterface;
class HostInterface;
class KeyboardEvent;
class MouseEvent;
class TimerEvent;
class Event;
class Elements;

/**
 * Interface for representing a View in the Gadget API.
 */
class ViewInterface : public ScriptableInterface {
 public:
  CLASS_ID_DECL(0xeb376007cbe64f9f);

  virtual ~ViewInterface() { }

  /** Used in @c SetResizable() and @c GetResizable(). */
  enum ResizableMode {
    RESIZABLE_FALSE,
    RESIZABLE_TRUE,
    /** The user can resize the view while keeping the original aspect ratio. */
    RESIZABLE_ZOOM,
  };

  /** 
   * Attach a view to a host displaying it. A view may only be associated with
   * one host at a time.
   * @param host HostInterface to attach to this view. 
   *   Pass in NULL to detach view from any host.
   * @return true on success, false otherwise. 
   */
  virtual bool AttachHost(HostInterface *host) = 0;
    
  /** Handler of the mouse events. */
  virtual void OnMouseEvent(MouseEvent *event) = 0;
  /** Handler of the keyboard events. */
  virtual void OnKeyEvent(KeyboardEvent *event) = 0;  
  /** Handler for other events. */
  virtual void OnOtherEvent(Event *event) = 0;

  /** 
   * Handler for timer events. 
   * Set event->StopReceivingMore() to cancel the timer. 
   */
  virtual void OnTimerEvent(TimerEvent *event) = 0;
  
  /** Called when any element is added into the view hierarchy. */
  virtual void OnElementAdded(ElementInterface *element) = 0;
  /** Called when any element in the view hierarchy is removed. */
  virtual void OnElementRemoved(ElementInterface *element) = 0;

  /** Any elements should call this method when it need to fire an event. */ 
  virtual void FireEvent(Event *event, const EventSignal &event_signal) = 0;

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

  /**
   * Indicates what happens when the user attempts to resize the gadget using
   * the window border.
   * @see ResizableMode
   */
  virtual void SetResizable(ResizableMode resizable) = 0;
  virtual ResizableMode GetResizable() const = 0;

  /**
   * Caption is the title of the view, by default shown when a gadget is in
   * floating/expanded mode but not shown when the gadget is in the Sidebar.
   * @see SetShowCaptionAlways()
   */
  virtual void SetCaption(const char *caption) = 0;
  virtual const char *GetCaption() const = 0;

  /**
   * When @c true, the Sidebar always shows the caption for this view.
   * By default this value is @c false.
   */
  virtual void SetShowCaptionAlways(bool show_always) = 0;
  virtual bool GetShowCaptionAlways() const = 0;

 public:
  /**
   * Retrieves a collection that contains the immediate children of this
   * view.
   */
  virtual const Elements *GetChildren() const = 0;
  /**
   * Retrieves a collection that contains the immediate children of this
   * view.
   */
  virtual Elements *GetChildren() = 0;
  
  /**
   * Appends an element as the last child of this view.
   * @return the newly created element or @c NULL if this method is not
   *     allowed.
   */
  virtual ElementInterface *AppendElement(const char *tag_name,
                                          const char *name) = 0;
  /**
   * Insert an element immediately before the specified element.
   * If the specified element is not the direct child of this view, the
   * newly created element will be append as the last child of this view.
   * @return the newly created element or @c NULL if this method is not
   *     allowed.
   */
  virtual ElementInterface *InsertElement(const char *tag_name,
                                          const ElementInterface *before,
                                          const char *name) = 0;
  /**
   * Removes the specified child from this view.
   * @param child the element to remove.
   * @return @c true if removed successfully, or @c false if the specified
   *     element doesn't exists or not the direct child of this view.
   */
  virtual bool RemoveElement(ElementInterface *child) = 0;

  /**
   * Looks up an element from all elements directly or indirectly contained
   * in this view by its name.
   * @param name element name.
   * @return the element pointer if found; or @c NULL if not found.
   */
  virtual ElementInterface *GetElementByName(const char *name) = 0;

  /**
   * Constant version of the above GetElementByName();
   */
  virtual const ElementInterface *GetElementByName(const char *name) const = 0;
};

CLASS_ID_IMPL(ViewInterface, ScriptableInterface)

} // namespace ggadget

#endif // GGADGET_VIEW_INTERFACE_H__
