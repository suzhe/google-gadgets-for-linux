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

#ifndef HOSTS_SIMPLE_GTK_GADGET_HOST_H__
#define HOSTS_SIMPLE_GTK_GADGET_HOST_H__

#include <set>

#include <gtk/gtk.h>
#include "ggadget/ggadget.h"

/**
 * An implementation of @c GadgetHostInterface for the simple gadget host.
 */
class GtkGadgetHost : public ggadget::GadgetHostInterface {
 public:
  GtkGadgetHost();
  virtual ~GtkGadgetHost();

  virtual ggadget::ScriptRuntimeInterface *GetScriptRuntime(
      ScriptRuntimeType type);
  virtual ggadget::ElementFactoryInterface *GetElementFactory();
  virtual ggadget::FileManagerInterface *GetGlobalFileManager();
  virtual ggadget::XMLHttpRequestInterface *NewXMLHttpRequest();
  virtual ggadget::ViewHostInterface *NewViewHost(
      ViewType type,
      ggadget::ScriptableInterface *prototype,
      ggadget::OptionsInterface *options);

  virtual void DebugOutput(DebugLevel level, const char *message);
  virtual uint64_t GetCurrentTime() const;
  virtual int RegisterTimer(unsigned ms, TimerCallback *callback);
  virtual bool RemoveTimer(int token);
  virtual int RegisterReadWatch(int fd, IOWatchCallback *callback);
  virtual int RegisterWriteWatch(int fd, IOWatchCallback *callback);
  virtual bool RemoveIOWatch(int token);

  /**
   * Load a gadget from file system.
   * @param base_path the base path of this gadget. It can be a directory or
   *     path to a .gg file.
   * @param zoom zoom factor of this gadget.
   * @param debug_mode 0: no debug; 1: debugs container elements by drawing
   *     a bounding box for each container element; 2: debugs all elements.
   * @return the loaded gadget if succeeded, or @c NULL otherwise.
   */
  // TODO: store zoom (and debug_mode?) into options repository.
  ggadget::GadgetInterface *LoadGadget(const char *base_path,
                                       double zoom, int debug_mode);

 private:
  void ReportScriptError(const char *message);

  class CallbackData;
  int RegisterIOWatch(bool read_or_write, int fd, IOWatchCallback *callback);
  bool RemoveCallback(int token);

  static gboolean DispatchTimer(gpointer data);
  static gboolean DispatchIOWatch(GIOChannel *source,
                                  GIOCondition cond,
                                  gpointer data);

  ggadget::ScriptRuntimeInterface *script_runtime_;
  ggadget::ElementFactoryInterface *element_factory_;
  ggadget::FileManagerInterface *global_file_manager_;

  typedef std::map<int, CallbackData *> CallbackMap;
  CallbackMap callbacks_;

  DISALLOW_EVIL_CONSTRUCTORS(GtkGadgetHost);
};

#endif // HOSTS_SIMPLE_GTK_GADGET_HOST_H__
