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
#include <ggadget/ggadget.h>

class GtkMenuImpl;

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
  virtual ggadget::FileManagerInterface *GetFileManager();
  virtual ggadget::FileManagerInterface *GetGlobalFileManager();
  virtual ggadget::OptionsInterface *GetOptions();
  virtual ggadget::GadgetInterface *GetGadget();
  virtual ggadget::ViewHostInterface *NewViewHost(
      ViewType type, ggadget::ScriptableInterface *prototype);

  virtual void SetPluginFlags(int plugin_flags);
  virtual void RemoveMe(bool save_data);
  virtual void ShowDetailsView(ggadget::DetailsViewInterface *details_view,
                               const char *title, int flags,
                               ggadget::Slot1<void, int> *feedback_handler);
  virtual void CloseDetailsView();
  virtual void ShowOptionsDialog();

  virtual void DebugOutput(DebugLevel level, const char *message);
  virtual uint64_t GetCurrentTime() const;
  virtual int RegisterTimer(unsigned ms, TimerCallback *callback);
  virtual bool RemoveTimer(int token);
  virtual int RegisterReadWatch(int fd, IOWatchCallback *callback);
  virtual int RegisterWriteWatch(int fd, IOWatchCallback *callback);
  virtual bool RemoveIOWatch(int token);
  virtual bool OpenURL(const char *url) const;
  virtual bool LoadFont(const char *filename, 
                        ggadget::FileManagerInterface *fm);
  virtual bool UnloadFont(const char *filename);

  /**
   * Loads a gadget from file system and hosts it.
   * @param container the gtk widget to contain this gadget.
   * @param base_path the base path of this gadget. It can be a directory or
   *     path to a .gg file.
   * @param zoom zoom factor of this gadget.
   * @param debug_mode 0: no debug; 1: debugs container elements by drawing
   *     a bounding box for each container element; 2: debugs all elements.
   * @return the loaded gadget if succeeded, or @c NULL otherwise.
   */
  // TODO: store zoom (and debug_mode?) into options repository.
  bool LoadGadget(GtkBox *container,
                  const char *base_path, double zoom, int debug_mode);

 private:
  void ReportScriptError(const char *message);

  class CallbackData;
  int RegisterIOWatch(bool read_or_write, int fd, IOWatchCallback *callback);
  bool RemoveCallback(int token);

  static gboolean DispatchTimer(gpointer data);
  static gboolean DispatchIOWatch(GIOChannel *source,
                                  GIOCondition cond,
                                  gpointer data);

  static void OnMenuClicked(GtkButton *button, gpointer user_data);
  void PopupMenu();
  static void OnBackClicked(GtkButton *button, gpointer user_data);
  static void OnForwardClicked(GtkButton *button, gpointer user_data);
  static void OnDetailsClicked(GtkButton *button, gpointer user_data);
  static void OnCollapseActivate(GtkMenuItem *menu_item, gpointer user_data);
  static void OnOptionsActivate(GtkMenuItem *menu_item, gpointer user_data);
  static void OnAboutActivate(GtkMenuItem *menu_item, gpointer user_data);
  static void OnDockActivate(GtkMenuItem *menu_item, gpointer user_data);

  ggadget::ScriptRuntimeInterface *script_runtime_;
  ggadget::ElementFactoryInterface *element_factory_;
  ggadget::FileManagerInterface *global_file_manager_;
  ggadget::FileManagerInterface *file_manager_;
  ggadget::OptionsInterface *options_;
  ggadget::GadgetInterface *gadget_;

  int plugin_flags_;
  typedef std::map<int, CallbackData *> CallbackMap;
  CallbackMap callbacks_;

  // Maps original font filename to temp font filename
  std::map<std::string, std::string> loaded_fonts_;

  GtkBox *toolbox_;
  GtkWidget *menu_button_, *back_button_, *forward_button_, *details_button_;
  GtkMenuImpl *menu_;

  DISALLOW_EVIL_CONSTRUCTORS(GtkGadgetHost);
};

#endif // HOSTS_SIMPLE_GTK_GADGET_HOST_H__
