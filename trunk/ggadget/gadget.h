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

#ifndef GGADGET_GADGET_H__
#define GGADGET_GADGET_H__

#include <string>
#include <ggadget/common.h>
#include <ggadget/string_utils.h>

namespace ggadget {

template <typename R, typename P1> class Slot1;
class HostInterface;
class DetailsViewData;
class FileManagerInterface;
class MenuInterface;
class View;
class Connection;
class OptionsInterface;
class DOMDocumentInterface;

/**
 * A class to hold a gadget instance.
 */
class Gadget {
 public:
  /** special commands that can be executed by gadget. */
  enum Command {
    /** Show About dialog. */
    CMD_ABOUT_DIALOG = 1,
    /** User clicked the 'back' button. */
    CMD_TOOLBAR_BACK = 2,
    /** User clicked the 'forward' button. */
    CMD_TOOLBAR_FORWARD = 3,
  };

  /** Display states of the gadget's main view. */
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

  /** Display targets of the gadget's main view. */
  enum DisplayTarget {
    /** Item is being displayed/drawn in the Sidebar. */
    TARGET_SIDEBAR = 0,
    /** Item is being displayed/drawn in the notification window. */
    TARGET_NOTIFIER = 1,
    /** Item is being displayed in its own window floating on the desktop */
    TARGET_FLOATING_VIEW = 2,
  };

  /**
   * Special flags that can be changed by gadget.
   * These flags shall be handled by VewHost or ViewDecorator to determine how
   * to show the decorator of the gadget's main view.
   */
  enum PluginFlags {
    PLUGIN_FLAG_NONE = 0,
    /** Adds a "back" button in the plugin toolbar. */
    PLUGIN_FLAG_TOOLBAR_BACK = 1,
    /** Adds a "forward" button in the plugin toolbar. */
    PLUGIN_FLAG_TOOLBAR_FORWARD = 2,
  };

 public:
  /**
   * Constructor.
   *
   * The gadget will be loaded and initialized, if failed then IsValid() method
   * will return false.
   *
   * @param host the host of this gadget.
   * @param base_path the base path of this gadget. It can be a directory,
   *     path to a .gg file, or path to a gadget.gmanifest file.
   * @param instance_id An unique id to identify this Gadget instance. It can
   *        be used to remove this Gadget instance by calling
   *        HostInteface::RemoveGadget() method.
   */
  Gadget(HostInterface *host,
         const char *base_path,
         const char *options_name,
         int instance_id);

  /** Destructor */
  ~Gadget();

  /**
   * Asks the Host to remove the Gadget instance.
   *
   * Unlike just delete the Gadget instance, this method will remove the Gadget
   * instance from GadgetManager, so that it won't be displayed anymore.
   *
   * @param save_data if save the options data of this Gadget instance.
   */
  void RemoveMe(bool save_data);

  /** Returns the HostInterface instance used by this gadget. */
  HostInterface *GetHost() const;

  /**
   * Checks if the gadget is valid or not.
   */
  bool IsValid() const;

  /** Returns the instance id of this gadget instance. */
  int GetInstanceID() const;

  /** Returns current plugin flags of the gadget. */
  int GetPluginFlags() const;

  /**
   * Gets the FileManager instance used by this gadget.
   * Caller shall not destroy the returned FileManager instance.
   */
  FileManagerInterface *GetFileManager() const;

  /**
   * Gets the Options instance used by this gadget.
   * Caller shall not destroy the returned Options instance.
   */
  OptionsInterface *GetOptions() const;

  /** Gets the main view of this gadget. */
  View *GetMainView() const;

  /**
   * Get a value configured in the gadget manifest file.
   * @param key the value key like a simple XPath expression. See
   *     gadget_consts.h for available keys, and @c ParseXMLIntoXPathMap() in
   *     xml_utils.h for details of the XPath expression.
   * @return the configured value. @c NULL if not found.
   */
  std::string GetManifestInfo(const char *key) const;

  /**
   * Parse XML into DOM, using the entities defined in strings.xml.
   */
  bool ParseLocalizedXML(const std::string &xml,
                         const char *filename,
                         DOMDocumentInterface *xmldoc) const;

  /** Checks whether this gadget has options dialog. */
  bool HasOptionsDialog() const;

  /** Shows main view of the gadget. */
  bool ShowMainView();

  /** Closes main view of the gadget. */
  void CloseMainView();

  /**
   * Show the options dialog, either old @c GDDisplayWindowInterface style or
   * XML view style, depending on whether @c options.xml exists.
   * @return @c true if succeeded.
   */
  bool ShowOptionsDialog();

  /**
   * Displays a details view containing the specified details control and the
   * specified title.  If there is already details view opened, it will be
   * closed first.
   * @param details_view
   * @param title the title of the details view.
   * @param flags combination of @c ViewInterface::DetailsViewFlags.
   * @param feedback_handler called when user clicks on feedback buttons. The
   *     handler has one parameter, which specifies @c DetailsViewFlags.
   */
  bool ShowDetailsView(DetailsViewData *details_view_data,
                       const char *title, int flags,
                       Slot1<void, int> *feedback_handler);

  /**
   * Close the details view if it is opened.
   */
  void CloseDetailsView();

  /**
   * Fires just before the gadget's menu is displayed. Handle this event to
   * customize the menu.
   */
  void OnAddCustomMenuItems(MenuInterface *menu);

  /** Execute a gadget specific command. */
  void OnCommand(Command command);

  /**
   * Fires after a gadget's display state changes -- for example, when it's
   * resized or minimized.
   */
  void OnDisplayStateChange(DisplayState display_state);

  /**
   * Fires just before the gadget's display location changes, such as from the
   * Sidebar to a floating desktop window.
   */
  void OnDisplayTargetChange(DisplayTarget display_target);

  /**
   * Connects a slot to the PluginFlagsChanged signal. The specified slot will
   * be called when the gadget's plugin flags is changed.
   *
   * Either ViewHost or ViewDecorator of the gadget's main view shall connect
   * to this signal to monitor the changes of plugin flags and update the view
   * decorator UI accordingly.
   *
   * The slot accepts one int parameter which is the new plugin flags value.
   */
  Connection *ConnectOnPluginFlagsChanged(Slot1<void, int> *handler);

 public:
  /**
   * A utility to get the manifest infomation of a gadget without
   * constructing a Gadget object.
   * @param base_path see document for Gadget constructor.
   * @param[out] data receive the manifest data.
   * @return @c true if succeeds.
   */
  static bool GetGadgetManifest(const char *base_path, StringMap *data);

 private:
  class Impl;
  Impl *impl_;
  DISALLOW_EVIL_CONSTRUCTORS(Gadget);
};

} // namespace ggadget

#endif // GGADGET_GADGET_H__
