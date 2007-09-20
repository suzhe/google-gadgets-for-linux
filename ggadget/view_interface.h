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
class GraphicsInterface;

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
  virtual void OnElementAdd(ElementInterface *element) = 0;
  /** Called when any element in the view hierarchy is about to be removed. */
  virtual void OnElementRemove(ElementInterface *element) = 0;

  /** Any elements should call this method when it need to fire an event. */ 
  virtual void FireEvent(Event *event, const EventSignal &event_signal) = 0;

  /**
   * These methods are provided to the event handlers of native gadgets to
   * retrieve the current fired event.
   */
  virtual Event *GetEvent() = 0;
  virtual const Event *GetEvent() const = 0;

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
   * @return A canvas suitable for drawing. This should never be NULL.
   */
  virtual const CanvasInterface *Draw(bool *changed) = 0;
  
  /** Asks the host to redraw the given view. */
  virtual void QueueDraw() = 0;
  
  /**
   * @return The current graphics interface used for drawing elements.
   */
  virtual const GraphicsInterface *GetGraphics() const = 0;
  
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

 public:  // Element management functions.
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

 public: // Timer, interval and animation functions.
  /**
   * Starts an animation timer. The @a slot is called periodically during 
   * @a duration with a value between @a start_value and @a end_value according
   * to the progress.
   * 
   * The value parameter of the slot is calculated as
   * (where progress is a float number between 0 and 1): <code>
   * value = start_value + (int)((end_value - start_value) * progress).</code>
   *
   * Note: The number of times the slot is called depends on the system
   * performance and current load of the system. It may be as high as 100 fps.
   * 
   * @param slot the call target of the timer. This @c ViewInterface instance
   *     becomes the owner of this slot after this call.
   * @param start_value start value of the animation.
   * @param end_value end value of the animation.
   * @param duration the duration of the whole animation in milliseconds.
   * @return the animation token that can be used in @c CancelAnimation().
   */
  virtual int BeginAnimation(Slot1<void, int> *slot,
                             int start_value,
                             int end_value,
                             unsigned int duration) = 0;

  /**
   * Cancels a currently running animation.
   * @param token the token returned by BeginAnimation().
   */
  virtual void CancelAnimation(int token) = 0;

  /**
   * Creates a run-once timer.
   * @param slot the call target of the timer. This @c ViewInterface instance
   *     becomes the owner of this slot after this call.
   * @param duration the duration of the timer in milliseconds.
   * @return the timeout token that can be used in @c ClearTimeout().
   */
  virtual int SetTimeout(Slot0<void> *slot, unsigned int duration) = 0;

  /**
   * Cancels a run-once timer.
   * @param token the token returned by SetTimeout().
   */
  virtual void ClearTimeout(int token) = 0;

  /**
   * Creates a run-forever timer.
   * @param slot the call target of the timer. This @c ViewInterface instance
   *     becomes the owner of this slot after this call.
   * @param duration the period between calls in milliseconds.
   * @return the interval token than can be used in @c ClearInterval().
   */
  virtual int SetInterval(Slot0<void> *slot, unsigned int duration) = 0;

  /**
   * Cancels a run-forever timer.
   * @param token the token returned by SetInterval().
   */
  virtual void ClearInterval(int token) = 0;
};

CLASS_ID_IMPL(ViewInterface, ScriptableInterface)

} // namespace ggadget

#endif // GGADGET_VIEW_INTERFACE_H__
