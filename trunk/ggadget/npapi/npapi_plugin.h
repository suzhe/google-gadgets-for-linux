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

#ifndef GGADGET_NPAPI_NPAPI_PLUGIN_H__
#define GGADGET_NPAPI_NPAPI_PLUGIN_H__

#include <sys/types.h>
#include <X11/Xlib.h>
#include <string>

#include <ggadget/basic_element.h>
#include <ggadget/scriptable_interface.h>
#include <ggadget/signals.h>
#include <ggadget/slot.h>

namespace ggadget {
namespace npapi {

/**
 * The type of toolkit the widgets use.
 */
enum ToolkitType {
  GTK12,
  GTK2,
};

/**
 * Window or windowless enumerations.
 */
enum WindowType {
  WindowTypeWindowed = 1,
  WindowTypeWindowless,
};

/**
 * Definition of window info structure about the plugin's window environment.
 */
struct WindowInfoStruct {
  /** Don't set this field by yourself. */
  int32_t type_;
  Display *display_;
  Visual *visual_;
  Colormap colormap_;
  unsigned int depth_;
};

/**
 * Definition of clip rectangle type.
 */
struct ClipRect {
  uint16_t top_;
  uint16_t left_;
  uint16_t bottom_;
  uint16_t right_;
};

/**
 * Definition of Wndow structure used to set up the plugin window.
 */
struct Window {
  /** Window mode: X Window ID (if X toolkit is used), or the window id of
   *    socket widget (if XEmbed is used).
   *  Windowless mode: never set this field. */
  void *window_;
  /** Coordinates of the drawing area. Relative to the element's rectangle. */
  uint32_t x_;
  uint32_t y_;
  uint32_t width_;
  uint32_t height_;
  /** Clipping rectangle coordinates. Relative to the element's rectangle. */
  ClipRect cliprect_;
  /** Contains information about the plugin's window environment. */
  WindowInfoStruct *ws_info_;
  /** Window or windowless. */
  WindowType type_;
};

class NPPlugin {
 public:
  /** Get the name of the plugin. */
  std::string GetName();

  /** Get the description of the plugin. */
  std::string GetDescription();

  /**
   * Get the window type the plugin uses, i.e. window or windowless.
   * The Host should call this first to determine the window type before
   * calling SetWindow.
   */
  WindowType GetWindowType();

  /**
   * Setup the plugin window.
   * The host should reset the window if window meta changes, such as resize,
   * changing view, etc.. This class will make a copy of the window object
   * passed in.
   */
  bool SetWindow(Window *window);

  /** Set URL of the stream that will consumed by the plugin. */
  bool SetURL(const char *url);

  /**
   * Delegate an X event to the plugin. Only use this interface for windowless
   * mode, as X server sends events to the plugin directly if the plugin has
   * its own window (Exactly speaking, some events will be sent to the parent
   * socket window but not plugin window when XEmbed is used).
   * Supported event types:
   * GraphicsExpose, FocusIn, FocusOut, EnterNotify, LeaveNotify, MotionNotify,
   * ButtonPress, ButtonRelease, KeyPress, KeyRelease
   * Note:
   * -The host element should invoke GraphicsExpose event in its DoDraw
   *  method. This class will set coordinate fields for this event. The host
   *  only needs to set three fields: type, display and drawable.
   * -Key events don't need have a position.
   * -Mouse events have x and y coordinates relative to the top-left corner
   *  of the plugin rectangle.
   */
  EventResult HandleEvent(XEvent event);

  /**
   * Returns true if the plugin is in transparent mode, otherwise returns false.
   */
  bool IsTransparent();

  /**
   * Set handler that will be called when plugin wants to show some
   * status message.
   */
  Connection *ConnectOnNewMessage(Slot1<void, const char *> *handler);

  /**
   * Scriptable entry for the plugin. The host should register this
   * root object as a constant that can be accessed from script such
   * as Javascript.
   */
  ScriptableInterface *GetScriptablePlugin();

 private:
  friend class NPContainer;
  friend class NPAPIImpl;

  // Can only create plugin instance through plugin container.
  NPPlugin(const std::string &mime_type, BasicElement *element,
           const std::string &name, const std::string &description,
           void *instance, void *plugin_funcs,
           bool xembed, ToolkitType toolkit);
  ~NPPlugin();

  class Impl;
  Impl *impl_;
};

} // namespace npapi
} // namespace ggadget

#endif // GGADGET_NPAPI_NPAPI_PLUGIN_H__
