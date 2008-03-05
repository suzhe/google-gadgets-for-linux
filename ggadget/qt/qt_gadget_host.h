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

#ifndef GGADGET_QT_QT_GADGET_HOST_H__
#define GGADGET_QT_QT_GADGET_HOST_H__

#include <set>
#include <map>
#include <ggadget/gadget_host_interface.h>
#include <ggadget/qt/qt_main_loop.h>

namespace ggadget {
namespace qt {

/**
 * An implementation of @c GadgetHostInterface for the qt gadget host.
 */
class QtGadgetHost : public GadgetHostInterface {
 public:
  QtGadgetHost(bool composited, bool useshapemask,
               double zoom, int debug_mode);
  virtual ~QtGadgetHost();

  virtual OptionsInterface *GetOptions();
  virtual GadgetInterface *GetGadget();
  virtual ViewHostInterface *NewViewHost(ViewType type, ViewInterface *view);

  virtual void SetPluginFlags(int plugin_flags);
  virtual void RemoveMe(bool save_data);

  virtual void DebugOutput(DebugLevel level, const char *message) const;
  virtual bool OpenURL(const char *url) const;
  virtual bool LoadFont(const char *filename);

  bool LoadGadget(const char *base_path);

 private:
  void ReportScriptError(const char *message);

  /*static gboolean DispatchTimer(gpointer data);
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
  static void OnDockActivate(GtkMenuItem *menu_item, gpointer user_data); */

  OptionsInterface *options_;
  GadgetInterface *gadget_;

  int plugin_flags_;

  // Maps original font filename to temp font filename
  std::map<std::string, std::string> loaded_fonts_;
  bool composited_;
  bool useshapemask_;
  double zoom_;
  int debug_mode_;

  DISALLOW_EVIL_CONSTRUCTORS(QtGadgetHost);
};

} // namespace qt
} // namespace ggadget

#endif // GGADGET_QT_QT_GADGET_HOST_H__
