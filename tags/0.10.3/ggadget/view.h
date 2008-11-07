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

#ifndef GGADGET_VIEW_H__
#define GGADGET_VIEW_H__

#include <ggadget/common.h>
#include <ggadget/slot.h>
#include <ggadget/variant.h>
#include <ggadget/view_interface.h>

namespace ggadget {

class BasicElement;
class Elements;
class ContentAreaElement;
class ElementFactory;
class GraphicsInterface;
class ScriptableInterface;
class ScriptContextInterface;
class RegisterableInterface;
class MainLoopInterface;
class ViewHostInterface;
class ImageInterface;
class ScriptableEvent;
class Texture;
class Gadget;
class Rectangle;
class MenuInterface;

/**
 * The default View implementation.
 */
class View : public ViewInterface {
 public:
  /**
   * Constructor.
   *
   * @param host the ViewHost instance that is associated with the View
   *        instance. It's not owned by the View, and shall be destroyed after
   *        destroying the View.
   * @param gadget the Gadget instance that owns this view. For debug purpose,
   *        it could be NULL.
   * @param element_factory the ElementFactory instance that shall be used by
   *        this view. It won't be destroyed by the view.
   * @param script_context the ScriptContext instance that is used when
   *        creating new elements. (Only used in Elements::InsertElementFromXML)
   *        It's not owned by the View and shall be destroyed after destroying
   *        the View.
   */
  View(ViewHostInterface *host,
       Gadget *gadget,
       ElementFactory *element_factory,
       ScriptContextInterface *script_context);

  /** Destructor. */
  virtual ~View();

  /**
   * @return the ScriptContextInterface object associated with this view.
   */
  ScriptContextInterface *GetScriptContext() const;

  /**
   * @return the FileManagerInterface object associated with this view's gadget.
   */
  FileManagerInterface *GetFileManager() const;

  /**
   * Force layout its children.
   *
   * It'll only be called by ViewElement to propagate layout request.
   */
  void Layout();

  /**
   * Registers all properties of the View instance to specified Scriptable
   * instance./
   */
  virtual void RegisterProperties(RegisterableInterface *obj) const;

  /**
   * Sets the corresponding ScriptableView object of this View.
   * This View doesn't own the reference of specified ScriptableView object.
   *
   * RegisterProperties(obj->GetRegisterable()) will be called automatically.
   */
  void SetScriptable(ScriptableInterface *obj);

  /** Gets the corresponding ScriptableView object of this View. */
  ScriptableInterface *GetScriptable() const;

 public: // ViewInterface methods.
  virtual Gadget *GetGadget() const;
  virtual GraphicsInterface *GetGraphics() const;
  virtual ViewHostInterface *SwitchViewHost(ViewHostInterface *new_host);
  virtual ViewHostInterface *GetViewHost() const;
  virtual void SetWidth(double width);
  virtual void SetHeight(double height);
  virtual void SetSize(double width, double height);
  virtual double GetWidth() const;
  virtual double GetHeight() const;
  virtual void GetDefaultSize(double *width, double *height) const;

  virtual void SetResizable(ResizableMode resizable);
  virtual ResizableMode GetResizable() const;
  virtual void SetCaption(const std::string &caption);
  virtual std::string GetCaption() const;
  virtual void SetShowCaptionAlways(bool show_always);
  virtual bool GetShowCaptionAlways() const;
  virtual void SetResizeBorder(double left, double top,
                               double right, double bottom);
  virtual bool GetResizeBorder(double *left, double *top,
                               double *right, double *bottom) const;
  virtual void MarkRedraw();

  virtual void Draw(CanvasInterface *canvas);

  virtual EventResult OnMouseEvent(const MouseEvent &event);
  virtual EventResult OnKeyEvent(const KeyboardEvent &event);
  virtual EventResult OnDragEvent(const DragEvent &event);
  virtual EventResult OnOtherEvent(const Event &event);

  virtual HitTest GetHitTest() const;

  virtual bool OnAddContextMenuItems(MenuInterface *menu);
  virtual bool OnSizing(double *width, double *height);

 public: // Additional event handling methods
  /**
   * Any elements should call this method when it need to fire an event.
   * @param event the event to be fired. The caller can choose to allocate the
   *     event object on stack or heap.
   * @param event_signal
   */
  void FireEvent(ScriptableEvent *event, const EventSignal &event_signal);

  /**
   * Post a onsize event into the event queue for an element. The event will
   * be fired after the current @c Layout() if this method is called from
   * @c Layout(), or the next @c Layout().
   * @param element the element posting the event.
   * @param signal the onsize signal of the element.
   */
  void PostElementSizeEvent(BasicElement *element, const EventSignal &signal);

  /**
   * This method is provided to the event handlers of native gadgets to
   * retrieve the current fired event.
   */
  ScriptableEvent *GetEvent() const;

  /**
   * Enables or disables firing events.
   * Because onchange events should not be fired during XML setup, events
   * should be disabled until the view is ready to run.
   */
  void EnableEvents(bool enable_events);

  /**
   * Enables or disables the canvas cache in the view object.
   * The cache is enabled by default.
   */
  void EnableCanvasCache(bool enable_cache);

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
  Elements *GetChildren() const;

  /**
   * Looks up an element from all elements directly or indirectly contained
   * in this view by its name.
   * @param name element name.
   * @return the element pointer if found; or @c NULL if not found.
   */
  BasicElement *GetElementByName(const char *name) const;

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
   * Sets the element to be shown as a popup, above all other elements on
   * the view.
   */
  void SetPopupElement(BasicElement *element);

  /** Gets the element to be shown as a popup. */
  BasicElement *GetPopupElement() const;

  /** Gets the element currently having the input focus. */
  BasicElement *GetFocusedElement() const;

  /** Gets the element which the mouse is currently over. */
  BasicElement *GetMouseOverElement() const;

  /** Returns the content area element if there is one, or @c NULL. */
  ContentAreaElement *GetContentAreaElement() const;

  /**
   * Return if the element is in the clip region to decide if it is need to
   * redraw.
   */
  bool IsElementInClipRegion(const BasicElement *element) const;

  /**
   * Add an element (part or whole) to the clip region when it is changed and
   * need to redraw.
   *
   * @param element The element to be added into clip region.
   * @param rect If it's not NULL then only add this rectangle, which is in
   *        element's coordinates. If it's NULL then add the whole element to
   *        the clip region.
   */
  void AddElementToClipRegion(BasicElement *element, const Rectangle *rect);

  /**
   * Enables or disables current clip regions.
   * When clip region is disabled then IsElementInClipRegion() will always
   * return true and all elements will be drawn when drawing View.
   *
   * It's useful when draw an element into off-screen buffer.
   */
  void EnableClipRegion(bool enable);

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
  int BeginAnimation(Slot0<void> *slot, int start_value, int end_value,
                     int duration);

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
  int SetTimeout(Slot0<void> *slot, int duration);

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
  int SetInterval(Slot0<void> *slot, int duration);

  /**
   * Cancels a run-forever timer.
   * @param token the token returned by SetInterval().
   */
  void ClearInterval(int token);

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
  ImageInterface *LoadImage(const Variant &src, bool is_mask) const;

  /**
   * Load an image from the global file manager.
   * @param name the name within the gadget base path.
   * @param is_mask if the image is used as a mask.
   * @return the loaded image (may lazy initialized) if succeeds, or @c NULL.
   */
  ImageInterface *LoadImageFromGlobal(const char *name, bool is_mask) const;

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
  Texture *LoadTexture(const Variant &src) const;

 public: // Delegate to Gadget or ViewHost.
  /** Gets pointer to the native widget holding this view. */
  void *GetNativeWidget() const;

  /**
   * Converts coordinates in the view's space to coordinates in the native
   * widget which holds the view.
   */
  void ViewCoordToNativeWidgetCoord(double x, double y,
                                    double *widget_x, double *widget_y) const;

  /**
   * Converts coordinates in the native widget which holds the view to
   * coordinates in the view's space.
   */
  void NativeWidgetCoordToViewCoord(double x, double y,
                                    double *view_x, double *view_y) const;

  /**
   * Asks the host to redraw the given view.
   * @param changed_element the pointer to the basic element that file this
   * request.
   */
  void QueueDraw();

  /**
   * Gets the current debug mode.
   */
  int GetDebugMode() const;

  /**
   * Open the given URL in the user's default web brower.
   * Only HTTP and HTTPS are supported.
   */
  bool OpenURL(const char *url) const;

  /** Displays a message box containing the message string. */
  void Alert(const char *message) const;

  /**
   * Displays a dialog containing the message string and Yes and No buttons.
   * @param message the message string.
   * @return @c true if Yes button is pressed, @c false if not.
   */
  bool Confirm(const char *message) const;

  /**
   * Displays a dialog asking the user to enter text.
   * @param message the message string displayed before the edit box.
   * @param default_value the initial default value dispalyed in the edit box.
   * @return the user inputted text, or an empty string if user canceled the
   *     dialog.
   */
  std::string Prompt(const char *message, const char *default_value) const;

  /**
   * Gets the current time.
   * Delegated to @c MainLoopInterface::GetCurrentTime().
   */
  uint64_t GetCurrentTime() const;

  /**
   * Shows an element's tooltip at current cursor position. The tooltip will be
   * automatically hidden when appropriate.
   */
  void ShowElementTooltip(BasicElement *element);

  /**
   * Shows an element's tooltip at specific position in element's coordinates.
   * @param element The element for which tooltip will be shown. Must belong to
   *     this view.
   * @param x, y Position to show the tooltip, in element's coordinates.
   */
  void ShowElementTooltipAtPosition(BasicElement *element, double x, double y);

  /**
   * Sets the current mouse cursor.
   *
   * @param type the cursor type, see @c ViewInterface::CursorType.
   *        -1 means the default type.
   */
  void SetCursor(int type);

  /**
   * Shows the associated View by proper method according to type of the View.
   *
   * The behavior of this function will be different for different types of
   * view:
   * - For main view, all parameters will be ignored. The feedback_handler will
   *   just be deleted if it's not NULL.
   * - For options view, the flags is combination of @c OptionsViewFlags,
   *   feedback_handler will be called when closing the options view, with one
   *   of OptionsViewFlags as the parameter.
   * - For details view, the flags is combination of DetailsViewFlags,
   *   feedback_handler will be called when closing the details view, with one
   *   of DetailsViewFlags as the parameter.
   *
   * @param modal if it's true then the view will be displayed in modal mode,
   *        and this function won't return until the view is closed.
   * @param flags for options view, it's combination of OptionsViewFlags,
   *        for details view, it's combination of DetailsViewFlags.
   * @param feedback_handler a callback that will be called when the view is
   *        closed. It has no effect for main view.
   * @return true if the View is shown correctly. Otherwise returns false.
   */
  bool ShowView(bool modal, int flags, Slot1<bool, int> *feedback_handler);

  /** Closes the view if it's opened by calling ShowView(). */
  void CloseView();

  /**
   * Gets the default font size, which can be customized by the user.
   * @return the default font point size.
   */
  int GetDefaultFontSize() const;

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
  Connection *ConnectOnMouseMoveEvent(Slot0<void> *handler);
  Connection *ConnectOnMouseOverEvent(Slot0<void> *handler);
  Connection *ConnectOnMouseOutEvent(Slot0<void> *handler);
  Connection *ConnectOnMouseUpEvent(Slot0<void> *handler);
  Connection *ConnectOnMouseWheelEvent(Slot0<void> *handler);
  Connection *ConnectOnOkEvent(Slot0<void> *handler);
  Connection *ConnectOnOpenEvent(Slot0<void> *handler);
  Connection *ConnectOnOptionChangedEvent(Slot0<void> *handler);
  Connection *ConnectOnPopInEvent(Slot0<void> *handler);
  Connection *ConnectOnPopOutEvent(Slot0<void> *handler);
  Connection *ConnectOnRestoreEvent(Slot0<void> *handler);
  Connection *ConnectOnSizeEvent(Slot0<void> *handler);
  Connection *ConnectOnSizingEvent(Slot0<void> *handler);
  Connection *ConnectOnUndockEvent(Slot0<void> *handler);
  Connection *ConnectOnContextMenuEvent(Slot0<void> *handler);
  Connection *ConnectOnThemeChangedEvent(Slot0<void> *handler);

 public:
  /** For performance testing. */
  void IncreaseDrawCount();

 private:
  class Impl;
  Impl *impl_;
  DISALLOW_EVIL_CONSTRUCTORS(View);
};

/**
 * Make sure that View pointer can be transfered through signal-slot.
 * Some code depends on it.
 */
DECLARE_VARIANT_PTR_TYPE(View);

} // namespace ggadget

#endif // GGADGET_VIEW_H__
