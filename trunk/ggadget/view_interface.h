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

#include <ggadget/event.h>
#include <ggadget/scriptable_interface.h>
#include <ggadget/signals.h>

namespace ggadget {

class CanvasInterface;
class FileManagerInterface;
class GraphicsInterface;
class ScriptContextInterface;
class MenuInterface;
class ViewHostInterface;

/**
 * Interface for representing a View in the Gadget API.
 */
class ViewInterface : public ScriptableInterface {
 public:
  CLASS_ID_DECL(0xeb376007cbe64f9f);

  /** Used in @c SetResizable(). */
  enum ResizableMode {
    RESIZABLE_FALSE,
    RESIZABLE_TRUE,
    /** The user can resize the view while keeping the original aspect ratio. */
    RESIZABLE_ZOOM,
  };

  virtual ~ViewInterface() { }

  /**
   * @return the ScriptContextInterface object associated with this view.
   */
  virtual ScriptContextInterface *GetScriptContext() const = 0;

  /**
   * @return the FileManagerInterface object associated with this view's gadget.
   */
  virtual FileManagerInterface *GetFileManager() const = 0;

  /**
   * Init the view from specified XML definition, and start running.
   * @param xml XML definition of the view.
   * @param filename file name of the xml definition, for logging purpose.
   * @return @c true if succeedes.
   */
  virtual bool InitFromXML(const std::string &xml, const char *filename) = 0;

  /**
   * Attaches a view host to this view.
   * Should only be called once, before @c InitFromXML().
   */
  virtual void AttachHost(ViewHostInterface *host) = 0;

  /** Handler of the mouse events. */
  virtual EventResult OnMouseEvent(const MouseEvent &event) = 0;

  /** Handler of the keyboard events. */
  virtual EventResult OnKeyEvent(const KeyboardEvent &event) = 0;

  /**
   * Handler of the drag and drop events.
   * @param event the drag and drop event.
   * @return @c EVENT_RESULT_HANDLED if the dragged contents are accepted by
   *     an element.
   */
  virtual EventResult OnDragEvent(const DragEvent &event) = 0;

  /**
   * Handler of the sizing event.
   * @param event the input event.
   * @param[out] output_event the output event. For @c Event::EVENT_SIZING,
   *     this parameter contains the overriding size set by the handler.
   * @return the result of event handling.
   */
  virtual EventResult OnOtherEvent(const Event &event, Event *output_event) = 0;

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
   * Draws the current view to a canvas.
   * The specified canvas shall already be prepared to be drawn directly
   * without any transformation.
   * @param canvas A canvas for the view to be drawn on. It shall have the same
   * zooming factory as the whole gadget.
   */
  virtual void Draw(CanvasInterface *canvas) = 0;

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
  virtual std::string GetCaption() const = 0;

  /**
   * When @c true, the Sidebar always shows the caption for this view.
   * By default this value is @c false.
   */
  virtual void SetShowCaptionAlways(bool show_always) = 0;
  virtual bool GetShowCaptionAlways() const = 0;

  /**
   * Sets a redraw mark, so that all things of this view will be redrawed
   * during the next draw.
   */
  virtual void MarkRedraw() = 0;
 public:
  /**
   * Called by the global options object when any option changed.
   */
  virtual void OnOptionChanged(const char *name) = 0;

  /**
   * Called by the host to let the view add customized context menu items, and
   * control whether the context menu should be shown.
   * @return @c false if the handler doesn't want the default menu items shown.
   *     If no menu item is added in this handler, and @c false is returned,
   *     the host won't show the whole context menu.
   */
  virtual bool OnAddContextMenuItems(MenuInterface *menu) = 0;
};

CLASS_ID_IMPL(ViewInterface, ScriptableInterface)

} // namespace ggadget

#endif // GGADGET_VIEW_INTERFACE_H__
