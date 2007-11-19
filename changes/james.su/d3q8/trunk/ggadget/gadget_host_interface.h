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

#ifndef GGADGET_GADGET_HOST_INTERFACE_H__
#define GGADGET_GADGET_HOST_INTERFACE_H__

namespace ggadget {

template <typename R, typename P1> class Slot1;
class AudioclipInterface;
class DetailsViewInterface;
class ElementFactoryInterface;
class FileManagerInterface;
class FrameworkInterface;
class GadgetInterface;
class OptionsInterface;
class ScriptableInterface;
class ScriptRuntimeInterface;
class Signal;
class ViewHostInterface;

/**
 * Interface for providing host services to the gadgets.
 * The @c GadgetHostInterface implementation should depend on the host.
 */
class GadgetHostInterface {
 public:
  virtual ~GadgetHostInterface() { }

  // TODO: Add a method to return the framework.

  enum ScriptRuntimeType {
    JAVASCRIPT,
  };

  /** Returns the global @c ScriptRuntimeInterface instance. */
  virtual ScriptRuntimeInterface *GetScriptRuntime(ScriptRuntimeType type) = 0;

  /** Returns the global @c ElementFactoryInterface instance. */
  virtual ElementFactoryInterface *GetElementFactory() = 0;

  /** Get the file manager used to load this gadget. */
  virtual FileManagerInterface *GetFileManager() = 0;

  /** Returns the @c FileManagerInterface used to load global resources. */
  virtual FileManagerInterface *GetGlobalFileManager() = 0;

  /** Returns the @c OptionsInterface instance for this gadget. */
  virtual OptionsInterface *GetOptions() = 0;

  /** Returns the global @c FrameworkInterface instance. */
  virtual FrameworkInterface *GetFramework() = 0;

  /** Returns the hosted gadget. */
  virtual GadgetInterface *GetGadget() = 0;

  enum ViewType {
    VIEW_MAIN,
    VIEW_OPTIONS,
  };

  /**
   * Creates a new @c ViewHostInterface for a view
   * @param type type of view.
   * @param prototype the scriptable prototype that can be shared among views.
   */
  virtual ViewHostInterface *NewViewHost(ViewType type,
                                         ScriptableInterface *prototype) = 0;

  enum PluginFlags {
    PLUGIN_FLAG_NONE = 0,
    /** Adds a "back" button in the plugin toolbar. */
    PLUGIN_FLAG_TOOLBAR_BACK = 1,
    /** Adds a "forward" button in the plugin toolbar. */
    PLUGIN_FLAG_TOOLBAR_FORWARD = 2,
  };

  /**
   * @param plugin_flags combination of PluginFlags.
   */
  virtual void SetPluginFlags(int plugin_flags) = 0;

  /**
   * Requests that the gadget be removed from the container (e.g. sidebar).
   * @param save_data if @c true, the gadget's state is saved before the gadget
   *     is removed.
   */
  virtual void RemoveMe(bool save_data) = 0;

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
   * Displays a details view containing the specified details control and the
   * specified title.  If there is already details view opened, it will be
   * closed first.
   * @param details_view
   * @param title the title of the details view.
   * @param flags combination of @c DetailsViewFlags.
   * @param feedback_handler called when user clicks on feedback buttons. The
   *     handler has one parameter, which specifies @c DetailsViewFlags.
   */
  virtual void ShowDetailsView(DetailsViewInterface *details_view,
                               const char *title, int flags,
                               Slot1<void, int> *feedback_handler) = 0;

  /**
   * Hides and destroys the details view that is being shown for this gadget.
   */
  virtual void CloseDetailsView() = 0;

  /** Shows the options dialog. */
  virtual void ShowOptionsDialog() = 0;

  enum DebugLevel {
    DEBUG_TRACE,
    DEBUG_WARNING,
    DEBUG_ERROR,
  };

  /** Output a debug string to the debug console or other places. */
  virtual void DebugOutput(DebugLevel level, const char *message) const = 0;

  /**
   * Returns the current time in microsecond units. This should only
   * be used for relative time comparisons, to compute elapsed time.
   */
  virtual uint64_t GetCurrentTime() const = 0;

  typedef Slot1<bool, int> TimerCallback;

  /**
   * Registers a timer with the host. The host will call the callback with the
   * timer token parameter when the interval hits. The first call will occur
   * after the first interval passes. The callback function returns @c true
   * if it wants to be called again. If not, returning @c false will unregister
   * the timer from the host.
   * @param ms timer interval in milliseconds.
   * @param callback the target to callback to.
   * @return token to timer (<code>!= 0</code>) if set, @c 0 otherwise.
   */
  virtual int RegisterTimer(unsigned ms, TimerCallback *callback) = 0;

  /**
   * Unregisters a timer.
   * @param token timer token.
   * @return @c true on success, @c false otherwise.
   */
  virtual bool RemoveTimer(int token) = 0;

  typedef ggadget::Slot1<void, int> IOWatchCallback;

  /**
   * Registers an IO watch with the host. The host will call the callback with
   * the fd parameter when the file descriptor have data to read.
   * @param fd the file descriptor to watch upon.
   * @param callback the target to callback to.
   * @return token to the watch (<code>!= 0</code> if set, @c 0 otherwise.
   */
  virtual int RegisterReadWatch(int fd, IOWatchCallback *callback) = 0;

  /**
   * Registers an IO watch with the host. The host will call the callback with
   * the fd parameter when the file descriptor have can be write non-blocking.
   * @param fd the file descriptor to watch upon.
   * @param callback the target to callback to.
   * @return token to the watch (<code>!= 0</code> if set, @c 0 otherwise.
   */
  virtual int RegisterWriteWatch(int fd, IOWatchCallback *callback) = 0;

  /**
   * Unregisters an IO watch.
   * @param token IO watch token.
   * @return @c true on success, @c false otherwise.
   */
  virtual bool RemoveIOWatch(int token) = 0;

  /** Open the given URL in the user's default web brower. */
  virtual bool OpenURL(const char *url) const = 0;

  /** Temporarily install a given font on the system. */
  virtual bool LoadFont(const char *filename) = 0;

  /** Remove a previously installed font. */
  virtual bool UnloadFont(const char *filename) = 0;

  /**
   * Displays the standard browse for file dialog and returns the name.
   * @param filter in the form "Display Name|List of Types", and multiple
   *     entries can be added to it. For example:
   *     "Music Files|*.mp3;*.wma|All Files|*.*".
   * @return the selected file or an empty string if the dialog is cancelled.
   *     The caller should not free the pointer it returned.
   */
  virtual const char *BrowseForFile(const char *filter) = 0;

  /** Interface for enumerating the files returned @c BrowseForFiles(). */
  class FilesInterface {
   protected:
    virtual ~FilesInterface() {}

   public:
    virtual void Destroy() = 0;

   public:
    /** Get the number of files. */
    virtual int GetCount() const = 0;
    /**
     * Get the file name according to the index.
     * The caller should not free the pointer this method returned,
     * and the returned pointer may be freed next time when calling to the
     * method in some implementations.
     */
    virtual const char *GetItem(int index) const = 0;
  };

  /**
   * Displays the standard browse for file dialog and returns a collection
   * containing the names of the selected files.
   * @param filter in the form "Display Name|List of Types", and multiple
   *     entries can be added to it. For example:
   *     "Music Files|*.mp3;*.wma|All Files|*.*".
   * @return the selected files or an empty collection if the dialog is
   *     cancelled. The caller should call @c Destroy() to the returned
   *     pointer after use.
   */
  virtual FilesInterface *BrowseForFiles(const char *filter) = 0;

  /** Retrieves the position of the cursor. */
  virtual void GetCursorPos(int *x, int *y) const = 0;

  /** Retrieves the screen size. */
  virtual void GetScreenSize(int *width, int *height) const = 0;

  /** Returns the path to the icon associated with the specified file. */
  virtual const char *GetFileIcon(const char *filename) const = 0;

  /** Creates an audio clip from given file or url. */
  virtual AudioclipInterface *CreateAudioclip(const char *src) = 0;
};

} // namespace ggadget

#endif // GGADGET_GADGET_HOST_INTERFACE_H__
