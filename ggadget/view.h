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

#ifndef GGADGET_VIEW_H__
#define GGADGET_VIEW_H__

#include <ggadget/common.h>
#include <ggadget/scriptable_helper.h>
#include <ggadget/view_interface.h>

namespace ggadget {

template <typename R> class Slot0;
class BasicElement;
class Elements;
class ContentAreaElement;
class DetailsView;
class ElementFactory;
class GadgetHostInterface;
class MenuInterface;
class ScriptContextInterface;
class Slot;
class MainLoopInterface;
class ViewHostInterface;
class ImageInterface;
class ScriptableEvent;
class Texture;

/**
 * Main View implementation.
 */
class View : public ScriptableHelperNativeOwned<ViewInterface> {
 public:
  DEFINE_CLASS_ID(0xc4ee4a622fbc4b7a, ViewInterface)

  View(ScriptableInterface *prototype,
       ElementFactory *element_factory,
       int debug_mode);
  virtual ~View();

 public: // ViewInterface methods.
  virtual ScriptContextInterface *GetScriptContext() const;
  virtual FileManagerInterface *GetFileManager() const;
  virtual bool InitFromFile(const char *filename);
  virtual void AttachHost(ViewHostInterface *host);

  virtual EventResult OnMouseEvent(const MouseEvent &event);
  virtual EventResult OnKeyEvent(const KeyboardEvent &event);
  virtual EventResult OnDragEvent(const DragEvent &event);
  virtual EventResult OnOtherEvent(const Event &event, Event *output_event);

  virtual bool SetWidth(int width);
  virtual bool SetHeight(int height);
  virtual bool SetSize(int width, int height);
  virtual int GetWidth() const;
  virtual int GetHeight() const;

  virtual void Draw(CanvasInterface *canvas);

  virtual void SetResizable(ViewInterface::ResizableMode resizable);
  virtual ViewInterface::ResizableMode GetResizable() const;
  virtual void SetCaption(const char *caption);
  virtual std::string GetCaption() const;
  virtual void SetShowCaptionAlways(bool show_always);
  virtual bool GetShowCaptionAlways() const;
  virtual void MarkRedraw();
  virtual void OnOptionChanged(const char *name);
  virtual bool OnAddContextMenuItems(MenuInterface *menu);

 public:  // Element management functions.
  /**
   * Retrieves the ElementFactory used to create elements in this
   * view.
   */
  ElementFactory *GetElementFactory() const;

  /**
   * Retrieves a collection that contains the immediate children of this
   * view.
   */
  const Elements *GetChildren() const;
  /**
   * Retrieves a collection that contains the immediate children of this
   * view.
   */
  Elements *GetChildren();

  /**
   * Looks up an element from all elements directly or indirectly contained
   * in this view by its name.
   * @param name element name.
   * @return the element pointer if found; or @c NULL if not found.
   */
  BasicElement *GetElementByName(const char *name);

  /**
   * Constant version of the above GetElementByName();
   */
  const BasicElement *GetElementByName(const char *name) const;

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
  int BeginAnimation(Slot0<void> *slot,
                     int start_value, int end_value, unsigned int duration);

  /**
   * Cancels a currently running animation.
   * @param token the token returned by BeginAnimation().
   */
  void CancelAnimation(int token);

  /**
   * Creates a run-once timer.
   * @param slot the call target of the timer. This @c ViewInterface instance
   *     becomes the owner of this slot after this call.
   * @param duration the duration of the timer in milliseconds.
   * @return the timeout token that can be used in @c ClearTimeout().
   */
  int SetTimeout(Slot0<void> *slot, unsigned int duration);

  /**
   * Cancels a run-once timer.
   * @param token the token returned by SetTimeout().
   */
  void ClearTimeout(int token);

  /**
   * Creates a run-forever timer.
   * @param slot the call target of the timer. This @c ViewInterface instance
   *     becomes the owner of this slot after this call.
   * @param duration the period between calls in milliseconds.
   * @return the interval token than can be used in @c ClearInterval().
   */
  int SetInterval(Slot0<void> *slot, unsigned int duration);

  /**
   * Cancels a run-forever timer.
   * @param token the token returned by SetInterval().
   */
  void ClearInterval(int token);

 public:
  /** Gets pointer to the native widget holding this view. */
  void *GetNativeWidget();

  /**
   * Converts coordinates in the view's space to coordinates in the native
   * widget which holds the view.
   */
  void ViewCoordToNativeWidgetCoord(double x, double y,
                                    double *widget_x, double *widget_y);

  /** Asks the host to redraw the given view. */
  void QueueDraw();

  /** @return the current graphics interface used for drawing elements. */
  const GraphicsInterface *GetGraphics() const;

  /**
   * Called when any element is about to be added into the view hierarchy.
   * @return @c true if the element is allowed to be added into the view.
   */
  bool OnElementAdd(BasicElement *element);
  /** Called when any element in the view hierarchy is about to be removed. */
  void OnElementRemove(BasicElement *element);

  /**
   * Sets current input focus to the @a element. If some element has the focus,
   * removes the focus first.
   * @param element the element to put focus on. If it is @c NULL, only remove
   *     the focus from the current focused element and thus no element has
   *     the focus.
   */
  void SetFocus(BasicElement *element);

  /**
   * Sets and gets the element to be shown as a popup, above all 
   * other elements on the view.
   */
  void SetPopupElement(BasicElement *element);
  BasicElement *GetPopupElement();

  /**
   * Any elements should call this method when it need to fire an event.
   * @param event the event to be fired. The caller can choose to allocate the
   *     event object on stack or heap.
   * @param event_signal
   */
  void FireEvent(ScriptableEvent *event, const EventSignal &event_signal);

  /**
   * Post an event into the event queue.  The event will be fired in the next
   * event loop.
   * @param event the event to be fired. The caller must allocate the
   *     @c ScriptableEvent and the @c Event objects on heap. They will be
   *     deleted by this view.
   * @param event_signal
   */
  void PostEvent(ScriptableEvent *event, const EventSignal &event_signal);

  /**
   * These methods are provided to the event handlers of native gadgets to
   * retrieve the current fired event.
   */
  ScriptableEvent *GetEvent();
  const ScriptableEvent *GetEvent() const;

  /**
   * Gets the current debug mode.
   * 0: no debug; 1: debug container elements only; 2: debug all elements.
   */
  int GetDebugMode() const;

  /** Gets the element currently having the input focus. */  
  BasicElement *GetFocusedElement();
  /** Gets the element which the mouse is currently over. */
  BasicElement *GetMouseOverElement();

  /**
   * Enables or disables firing events.
   * Because onchange events should not be fired during XML setup, events
   * should be disabled until the view is ready to run. 
   */
  void EnableEvents(bool enable_events);

 public:  // Other utilities.
  /**
   * Load an image from the gadget file.
   * @param src the image source, can be of the following types:
   *     - @c Variant::TYPE_STRING: the name within the gadget base path;
   *     - @c Variant::TYPE_SCRIPTABLE or @c Variant::TYPE_CONST_SCRIPTABLE and
   *       the scriptable object's type is ScriptableBinaryData: the binary
   *       data of the image.
   * @param is_mask if the image is used as a mask.
   * @return the loaded image (may lazy initialized) if succeeds, or @c NULL.
   */
  ImageInterface *LoadImage(const Variant &src, bool is_mask);

  /**
   * Load an image from the global file manager.
   * @param name the name within the gadget base path.
   * @param is_mask if the image is used as a mask.
   * @return the loaded image (may lazy initialized) if succeeds, or @c NULL.
   */
  ImageInterface *LoadImageFromGlobal(const char *name, bool is_mask);

  /**
   * Load a texture from image file or create a colored texture.
   * @param src the source of the texture image, can be of the following types:
   *     - @c Variant::TYPE_STRING: the name within the gadget base path, or a
   *       color description with HTML-style color ("#rrggbb"), or HTML-style
   *       color with alpha ("#aarrggbb").
   *     - @c Variant::TYPE_SCRIPTABLE or @c Variant::TYPE_CONST_SCRIPTABLE and
   *       the scriptable object's type is ScriptableBinaryData: the binary
   *       data of the image.
   * @return the created texture ifsucceeds, or @c NULL.
   */
  Texture *LoadTexture(const Variant &src);

  /**
   * Open the given URL in the user's default web brower.
   * Only HTTP, HTTPS, and FTP URLs are supported.
   */
  bool OpenURL(const char *url) const;

  /** Displays a message box containing the message string. */
  void Alert(const char *message);

  /**
   * Displays a dialog containing the message string and Yes and No buttons.
   * @param message the message string.
   * @return @c true if Yes button is pressed, @c false if not.
   */
  bool Confirm(const char *message);

  /**
   * Displays a dialog asking the user to enter text.
   * @param message the message string displayed before the edit box.
   * @param default_value the initial default value dispalyed in the edit box.
   * @return the user inputted text, or an empty string if user canceled the
   *     dialog.
   */
  std::string Prompt(const char *message, const char *default_value);

  /**
   * Gets the current time.
   * Delegated to @c MainLoopInterface::GetCurrentTime().
   */
  uint64_t GetCurrentTime();

  /** Returns the content area element if there is one, or @c NULL. */
  ContentAreaElement *GetContentAreaElement();

  /**
   * Display tooltip at the current cursor position. The tooltip will be
   * automatically hidden when appropriate.
   */
  void SetTooltip(const char *tooltip);

  /** Delegates to GadgetInterface::ShowDetailsView(). */
  bool ShowDetailsView(DetailsView *details_view, const char *title, int flags,
                       Slot1<void, int> *feedback_handler);

 public: // Event connection methods.
  Connection *ConnectOnCancelEvent(Slot0<void> *handler);
  Connection *ConnectOnClickEvent(Slot0<void> *handler);
  Connection *ConnectOnCloseEvent(Slot0<void> *handler);
  Connection *ConnectOnDblClickEvent(Slot0<void> *handler);
  Connection *ConnectOnRClickEvent(Slot0<void> *handler);
  Connection *ConnectOnRDblClickCancelEvent(Slot0<void> *handler);
  Connection *ConnectOnDockEvent(Slot0<void> *handler);
  Connection *ConnectOnKeyDownEvent(Slot0<void> *handler);
  Connection *ConnectOnPressEvent(Slot0<void> *handler);
  Connection *ConnectOnKeyUpEvent(Slot0<void> *handler);
  Connection *ConnectOnMinimizeEvent(Slot0<void> *handler);
  Connection *ConnectOnMouseDownEvent(Slot0<void> *handler);
  Connection *ConnectOnMouseOverEvent(Slot0<void> *handler);
  Connection *ConnectOnMouseOutEvent(Slot0<void> *handler);
  Connection *ConnectOnMouseUpEvent(Slot0<void> *handler);
  Connection *ConnectOnOkEvent(Slot0<void> *handler);
  Connection *ConnectOnOpenEvent(Slot0<void> *handler);
  Connection *ConnectOnOptionChangedEvent(Slot0<void> *handler);
  Connection *ConnectOnPopInEvent(Slot0<void> *handler);
  Connection *ConnectOnPopOutEvent(Slot0<void> *handler);
  Connection *ConnectOnRestoreEvent(Slot0<void> *handler);
  Connection *ConnectOnSizeEvent(Slot0<void> *handler);
  Connection *ConnectOnSizingEvent(Slot0<void> *handler);
  Connection *ConnectOnUndockEvent(Slot0<void> *handler);

 private:
  class Impl;
  Impl *impl_;
  DISALLOW_EVIL_CONSTRUCTORS(View);
};

} // namespace ggadget

#endif // GGADGET_VIEW_H__
