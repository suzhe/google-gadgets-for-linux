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

#ifndef GGADGET_GADGET_INTERFACE_H__
#define GGADGET_GADGET_INTERFACE_H__

namespace ggadget {

template <typename R, typename P1> class Slot1;
class DetailsView;
class FileManagerInterface;
class GDDisplayWindowInterface;
class MenuInterface;
class ViewHostInterface;

/**
 * Interface for representing a Gadget in the Gadget API.
 */
class GadgetInterface {
 public:
  virtual ~GadgetInterface() { }

  /**
   * Init the gadget by loading the gadget definitions.
   * @return @c true if succeeded, @c false otherwise.
   */
  virtual bool Init() = 0;

  /** @return the host of the main view */
  virtual ViewHostInterface *GetMainViewHost() = 0;

  /**
   * Get a value configured in the gadget manifest file.
   * @param key the value key like a simple XPath expression. See
   *     gadget_consts.h for available keys, and @c ParseXMLIntoXPathMap() in
   *     xml_utils.h for details of the XPath expression.
   * @return the configured value. @c NULL if not found.
   */
  virtual const char *GetManifestInfo(const char *key) const = 0;

  /** Checks whether this gadget has options dialog. */
  virtual bool HasOptionsDialog() const = 0;

  /**
   * Show the options dialog, either old @c GDDisplayWindowInterface style or
   * XML view style, depending on whether @c options.xml exists.
   * @return @c true if succeeded.
   */
  virtual bool ShowOptionsDialog() = 0;

  /**
   * Close the details view if it is opened.
   */
  virtual void CloseDetailsView() = 0;

  /**
   * Displays a details view containing the specified details control and the
   * specified title.  If there is already details view opened, it will be
   * closed first.
   * @param details_view
   * @param title the title of the details view.
   * @param flags combination of @c ViewHostInterface::DetailsViewFlags.
   * @param feedback_handler called when user clicks on feedback buttons. The
   *     handler has one parameter, which specifies @c DetailsViewFlags.
   */
  virtual bool ShowDetailsView(DetailsView *details_view,
                               const char *title, int flags,
                               Slot1<void, int> *feedback_handler) = 0;

  /**
   * Fires just before the gadget's menu is displayed. Handle this event to
   * customize the menu.
   */
  virtual void OnAddCustomMenuItems(MenuInterface *menu) = 0;

  enum Command {
    /** Show About dialog. */
    CMD_ABOUT_DIALOG = 1,
    /** User clicked the 'back' button. */
    CMD_TOOLBAR_BACK = 2,
    /** User clicked the 'forward' button. */
    CMD_TOOLBAR_FORWARD = 3,
  };
  virtual void OnCommand(Command command) = 0;

  enum DisplayState {
    /** Tile is not visible. */
    TILE_DISPLAY_STATE_HIDDEN = 0,
    /** Tile is restored from being minimized or popped out states. */
    TILE_DISPLAY_STATE_RESTORED = 1,
    /** Tile is minimized and only the title bar is visible. */
    TILE_DISPLAY_STATE_MINIMIZED = 2,
    /** Tile is 'popped-out' of the sidebar in a separate window. */
    TILE_DISPLAY_STATE_POPPED_OUT = 3,
    /** Tile is resized. */
    TILE_DISPLAY_STATE_RESIZED = 4,
  };

  /**
   * Fires after a gadget's display state changes -- for example, when it's
   * resized or minimized.
   */
  virtual void OnDisplayStateChange(DisplayState display_state) = 0;

  enum DisplayTarget {
    /** Item is being displayed/drawn in the Sidebar. */
    TARGET_SIDEBAR = 0,
    /** Item is being displayed/drawn in the notification window. */
    TARGET_NOTIFIER = 1,
    /** Item is being displayed in its own window floating on the desktop */
    TARGET_FLOATING_VIEW = 2,
  };

  /**
   * Fires just before the gadget's display location changes, such as from the
   * Sidebar to a floating desktop window.
   */
  virtual void OnDisplayTargetChange(DisplayTarget display_target) = 0;
};

} // namespace ggadget

#endif // GGADGET_GADGET_INTERFACE_H__
