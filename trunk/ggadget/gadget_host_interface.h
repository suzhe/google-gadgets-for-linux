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
class ElementFactoryInterface;
class FileManagerInterface;
class OptionsInterface;
class ScriptableInterface;
class ScriptRuntimeInterface;
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

  /** Returns the @c FileManagerInterface used to load global resources. */ 
  virtual FileManagerInterface *GetGlobalFileManager() = 0;

  enum ViewType {
    VIEW_MAIN,
    VIEW_OPTIONS,
  };

  /**
   * Creates a new @c ViewHostInterface for a view
   * @param type type of view.
   * @param prototype the scriptable prototype that can be shared among views.
   * @param options the options of the gadget containing the view.
   */
  virtual ViewHostInterface *NewViewHost(ViewType type,
                                         ScriptableInterface *prototype,
                                         OptionsInterface *options) = 0;

  enum DebugLevel {
    DEBUG_TRACE,
    DEBUG_WARNING,
    DEBUG_ERROR,
  };

  /** Output a debug string to the debug console or other places. */
  virtual void DebugOutput(DebugLevel level, const char *message) = 0;

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
  virtual bool LoadFont(const char *filename, FileManagerInterface *fm) = 0;

  /** Remove a previously installed font. */
  virtual bool UnloadFont(const char *filename) = 0;
};

} // namespace ggadget

#endif // GGADGET_GADGET_HOST_INTERFACE_H__
