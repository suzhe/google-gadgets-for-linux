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

#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fontconfig/fontconfig.h>

#include <ggadget/file_manager.h>
#include <ggadget/element_factory.h>
#include <ggadget/gadget.h>
#include <ggadget/common.h>
#include <ggadget/smjs/js_script_runtime.h>

#include <ggadget/button_element.h>
#include <ggadget/div_element.h>
#include <ggadget/img_element.h>
#include <ggadget/scrollbar_element.h>
#include <ggadget/label_element.h>
#include <ggadget/anchor_element.h>
#include <ggadget/checkbox_element.h>
#include <ggadget/progressbar_element.h>

#include "gtk_gadget_host.h"
#include "gtk_menu_impl.h"
#include "gtk_view_host.h"
#include "options.h"
#include "simplehost_file_manager.h"

class GtkGadgetHost::CallbackData {
 public:
  CallbackData(ggadget::Slot *s, GtkGadgetHost *h)
      : callback(s), host(h) { }
  virtual ~CallbackData() {
    delete callback;
  }

  int id;
  ggadget::Slot *callback;
  GtkGadgetHost *host;
};

GtkGadgetHost::GtkGadgetHost()
    : script_runtime_(new ggadget::JSScriptRuntime()),
      element_factory_(NULL),
      file_manager_(new ggadget::FileManager()),
      global_file_manager_(new SimpleHostFileManager()),
      options_(new Options()),
      gadget_(NULL),
      plugin_flags_(0),
      toolbox_(NULL), menu_button_(NULL), back_button_(NULL),
      forward_button_(NULL), details_button_(NULL),
      menu_(NULL) {
  ggadget::ElementFactory *factory = new ggadget::ElementFactory();
  factory->RegisterElementClass("button",
                                &ggadget::ButtonElement::CreateInstance);
  factory->RegisterElementClass("div",
                                &ggadget::DivElement::CreateInstance);
  factory->RegisterElementClass("img",
                                &ggadget::ImgElement::CreateInstance);
  factory->RegisterElementClass("progressbar",
                                &ggadget::ProgressBarElement::CreateInstance);
  factory->RegisterElementClass("scrollbar",
                                &ggadget::ScrollBarElement::CreateInstance);
  factory->RegisterElementClass("label",
                                &ggadget::LabelElement::CreateInstance);
  factory->RegisterElementClass("a", &ggadget::AnchorElement::CreateInstance);
  factory->RegisterElementClass("checkbox",
                                &ggadget::CheckBoxElement::CreateCheckBoxInstance);
  factory->RegisterElementClass("radio",
                                &ggadget::CheckBoxElement::CreateRadioInstance);
  element_factory_ = factory;

  global_file_manager_->Init(NULL);

  script_runtime_->ConnectErrorReporter(
      NewSlot(this, &GtkGadgetHost::ReportScriptError));

  FcInit(); // Just in case this hasn't been done.
}

GtkGadgetHost::~GtkGadgetHost() {
  CallbackMap::iterator i = callbacks_.begin();
  while (i != callbacks_.end()) {
    CallbackMap::iterator next = i;
    ++next;
    RemoveCallback(i->first);
    i = next;
  }
  ASSERT(callbacks_.empty());

  delete gadget_;
  gadget_ = NULL;
  delete options_;
  options_ = NULL;
  delete element_factory_;
  element_factory_ = NULL;
  delete script_runtime_;
  script_runtime_ = NULL;
  delete file_manager_;
  file_manager_ = NULL;
  delete global_file_manager_;
  global_file_manager_ = NULL;
  delete menu_;
  menu_ = NULL;
}

ggadget::ScriptRuntimeInterface *GtkGadgetHost::GetScriptRuntime(
    ScriptRuntimeType type) {
  return script_runtime_;
}

ggadget::ElementFactoryInterface *GtkGadgetHost::GetElementFactory() {
  return element_factory_;
}

ggadget::FileManagerInterface *GtkGadgetHost::GetFileManager() {
  return file_manager_;  
}

ggadget::FileManagerInterface *GtkGadgetHost::GetGlobalFileManager() {
  return global_file_manager_;  
}

ggadget::OptionsInterface *GtkGadgetHost::GetOptions() {
  return options_;
}

ggadget::GadgetInterface *GtkGadgetHost::GetGadget() {
  return gadget_;
}

ggadget::ViewHostInterface *GtkGadgetHost::NewViewHost(
    ViewType type, ggadget::ScriptableInterface *prototype) {
  return new GtkViewHost(this, type, prototype);
}

void GtkGadgetHost::SetPluginFlags(int plugin_flags) {
  if (plugin_flags & gddPluginFlagToolbarBack)
    gtk_widget_show(back_button_);
  else
    gtk_widget_hide(back_button_);

  if (plugin_flags & gddPluginFlagToolbarForward)
    gtk_widget_show(forward_button_);
  else
    gtk_widget_hide(forward_button_);
}

void GtkGadgetHost::RemoveMe(bool save_data) {
}

void GtkGadgetHost::ShowDetailsView(
    ggadget::DetailsViewInterface *details_view,
    const char *title, int flags,
    ggadget::Slot1<void, int> *feedback_handler) {
}

void GtkGadgetHost::CloseDetailsView() {
}

void GtkGadgetHost::ShowOptionsDialog() {
}

void GtkGadgetHost::DebugOutput(DebugLevel level, const char *message) {
  const char *str_level = "";
  switch (level) {
    case DEBUG_TRACE: str_level = "TRACE: "; break;
    case DEBUG_WARNING: str_level = "WARNING: "; break;
    case DEBUG_ERROR: str_level = "ERROR: "; break;
    default: break;
  }
  // TODO: actual debug console.
  printf("%s%s\n", str_level, message);
}

uint64_t GtkGadgetHost::GetCurrentTime() const {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return static_cast<uint64_t>(tv.tv_sec) * 1000000 + tv.tv_usec;
}

gboolean GtkGadgetHost::DispatchTimer(gpointer data) {
  CallbackData *tmdata = static_cast<CallbackData *>(data);
  // view->OnTimerEvent may call RemoveTimer, causing tmdata pointer invalid,
  // so save the valid host pointer first.
  GtkGadgetHost *host = tmdata->host;
  int id = tmdata->id;
  // DLOG("DispatchTimer id=%d", id);

  ggadget::Variant param(id);
  ggadget::Variant result = tmdata->callback->Call(1, &param);
  if (!ggadget::VariantValue<bool>()(result)) {
    // Event receiver has indicated that this timer should be removed.
    host->RemoveCallback(id);
    return FALSE;
  }
  return TRUE;
}

int GtkGadgetHost::RegisterTimer(unsigned ms, TimerCallback *callback) {
  ASSERT(callback);

  CallbackData *tmdata = new CallbackData(callback, this);
  tmdata->id = static_cast<int>(g_timeout_add(ms, DispatchTimer, tmdata));
  callbacks_[tmdata->id] = tmdata;
  // DLOG("RegisterTimer id=%d", tmdata->id);
  return tmdata->id;
}

bool GtkGadgetHost::RemoveTimer(int token) {
  // DLOG("RemoveTimer id=%d", token);
  return RemoveCallback(token);
}

gboolean GtkGadgetHost::DispatchIOWatch(GIOChannel *source,
                                        GIOCondition cond,
                                        gpointer data) {
  CallbackData *iodata = static_cast<CallbackData *>(data);
  ggadget::Variant param(g_io_channel_unix_get_fd(source));
  iodata->callback->Call(1, &param);
  return TRUE;
}

int GtkGadgetHost::RegisterIOWatch(bool read_or_write, int fd,
                                  IOWatchCallback *callback) {
  ASSERT(callback);

  GIOCondition cond = read_or_write ? G_IO_IN : G_IO_OUT;
  GIOChannel *channel = g_io_channel_unix_new(fd);
  CallbackData *iodata = new CallbackData(callback, this);
  iodata->id = static_cast<int>(g_io_add_watch(channel, cond,
                                               DispatchIOWatch, iodata));
  callbacks_[iodata->id] = iodata;
  g_io_channel_unref(channel);
  return iodata->id;
}

int GtkGadgetHost::RegisterReadWatch(int fd, IOWatchCallback *callback) {
  return RegisterIOWatch(true, fd, callback);
}

int GtkGadgetHost::RegisterWriteWatch(int fd, IOWatchCallback *callback) {
  return RegisterIOWatch(false, fd, callback);
}

bool GtkGadgetHost::RemoveIOWatch(int token) {
  return RemoveCallback(token);
}

bool GtkGadgetHost::RemoveCallback(int token) {
  ASSERT(token);

  CallbackMap::iterator i = callbacks_.find(token);
  if (i == callbacks_.end())
    // This data may be a stale pointer.
    return false;

  if (!g_source_remove(token))
    return false;

  delete i->second;
  callbacks_.erase(i);
  return true;
}

/** 
 * Taken from GDLinux. 
 * May move this function elsewhere if other classes use it too.
 */
std::string GetFullPathOfSysCommand(const std::string &command) {
  const char *env_path_value = getenv("PATH");
  if (env_path_value == NULL)
    return "";

  std::string all_path = std::string(env_path_value);
  size_t cur_colon_pos = 0;
  size_t next_colon_pos = 0;
  // Iterator through all the parts in env value.
  while ((next_colon_pos = all_path.find(":", cur_colon_pos)) != std::string::npos) {
    std::string path = all_path.substr(cur_colon_pos, next_colon_pos - cur_colon_pos);
    path += "/";
    path += command;
    if (access(path.c_str(), X_OK) == 0) {
      return path;
    }
    cur_colon_pos = next_colon_pos + 1;
  }
  return "";
}

bool GtkGadgetHost::OpenURL(const char *url) const {
  std::string xdg_open = GetFullPathOfSysCommand("xdg-open");
  if (xdg_open.empty()) {
    xdg_open = GetFullPathOfSysCommand("gnome-open");
    if (xdg_open.empty()) {
      LOG("Couldn't find xdg-open or gnome-open.");
      return false;
    }
  }

  DLOG("Launching URL: %s", url);

  pid_t pid;
  // fork and run the command.
  if ((pid = fork()) == 0) {
    if (fork() != 0)
      _exit(0);

    // Restore original LD_LIBRARY_PATH, to prevent external applications
    // from loading our private libraries.
    //char *ld_path_old = getenv("LD_LIBRARY_PATH_GDL_BACKUP");

    //if (ld_path_old)
    //  setenv("LD_LIBRARY_PATH", ld_path_old, 1);
    //else
    //  unsetenv("LD_LIBRARY_PATH");

    execl(xdg_open.c_str(), xdg_open.c_str(), url, NULL);

    DLOG("Failed to exec command: %s", xdg_open.c_str());
    _exit(-1);
  }

  int status = 0;
  waitpid(pid, &status, 0);

  // Assume xdg-open will always succeed.
  return true;
}

void GtkGadgetHost::ReportScriptError(const char *message) {
  DebugOutput(DEBUG_ERROR, (std::string("Script error: " ) + message).c_str());
}

bool GtkGadgetHost::LoadFont(const char *filename,
                             ggadget::FileManagerInterface *fm) {
  std::string fontfile;
  if (!fm->ExtractFile(filename, &fontfile)) {
    return false;
  }

  loaded_fonts_[filename] = fontfile;

  FcConfig *config = FcConfigGetCurrent();
  bool success = FcConfigAppFontAddFile(config,
                   reinterpret_cast<const FcChar8 *>(fontfile.c_str()));
  DLOG("LoadFont: %s %s", filename, fontfile.c_str());
  return success;
}

bool GtkGadgetHost::UnloadFont(const char *filename) {
  // FontConfig doesn't actually allow dynamic removal of App Fonts, so
  // just remove the file.
  std::map<std::string, std::string>::iterator i = loaded_fonts_.find(filename);
  if (i == loaded_fonts_.end()) {
    return false;
  }

  unlink((i->second).c_str()); // ignore return
  loaded_fonts_.erase(i);

  return true;
}

bool GtkGadgetHost::LoadGadget(GtkBox *container,
                               const char *base_path,
                               double zoom, int debug_mode) {
  // TODO: store zoom (and debug_mode?) into options repository.
  options_->PutValue(ggadget::kOptionZoom, ggadget::Variant(zoom));
  options_->PutValue(ggadget::kOptionDebugMode, ggadget::Variant(debug_mode));
  gadget_ = new ggadget::Gadget(this);
  if (!file_manager_->Init(base_path) || !gadget_->Init())
    return false;

  toolbox_ = GTK_BOX(gtk_hbox_new(FALSE, 0));
  gtk_box_pack_start(container, GTK_WIDGET(toolbox_), FALSE, FALSE, 0);

  menu_button_ = gtk_button_new_with_label("Menu");
  gtk_box_pack_end(toolbox_, menu_button_, FALSE, FALSE, 0);
  g_signal_connect(menu_button_, "clicked",
                   G_CALLBACK(OnMenuClicked), this);
  forward_button_ = gtk_button_new_with_label(" > ");
  gtk_box_pack_end(toolbox_, forward_button_, FALSE, FALSE, 0);
  g_signal_connect(forward_button_, "clicked",
                   G_CALLBACK(OnForwardClicked), this);
  back_button_ = gtk_button_new_with_label(" < ");
  gtk_box_pack_end(toolbox_, back_button_, FALSE, FALSE, 0);
  g_signal_connect(back_button_, "clicked",
                   G_CALLBACK(OnBackClicked), this);
  details_button_ = gtk_button_new_with_label("<<");
  gtk_box_pack_end(toolbox_, details_button_, FALSE, FALSE, 0);
  g_signal_connect(details_button_, "clicked",
                   G_CALLBACK(OnDetailsClicked), this);

  SetPluginFlags(0);
  return true;
}

void GtkGadgetHost::PopupMenu() {
  if (menu_) {
    gtk_widget_destroy(GTK_WIDGET(menu_->menu()));
    delete menu_;
  }

  GtkMenu *menu = GTK_MENU(gtk_menu_new());
  gtk_menu_attach_to_widget(menu, menu_button_, NULL);
  menu_ = new GtkMenuImpl(GTK_MENU(menu));

  gadget_->OnAddCustomMenuItems(menu_);

  GtkMenuShell *menu_shell = GTK_MENU_SHELL(menu);
  if (g_list_length(gtk_container_get_children(GTK_CONTAINER(menu))) > 0)
    gtk_menu_shell_append(menu_shell,
                          GTK_WIDGET(gtk_separator_menu_item_new()));

  GtkWidget *item = gtk_menu_item_new_with_label("Collapse");
  gtk_menu_shell_append(menu_shell, item);
  g_signal_connect(item, "activate", G_CALLBACK(OnCollapseActivate), this);
  item = gtk_menu_item_new_with_label("Options...");
  gtk_menu_shell_append(menu_shell, item);
  g_signal_connect(item, "activate", G_CALLBACK(OnOptionsActivate), this);
  gtk_menu_shell_append(menu_shell,
                        GTK_WIDGET(gtk_separator_menu_item_new()));
  item = gtk_menu_item_new_with_label("About...");
  gtk_menu_shell_append(menu_shell, item);
  g_signal_connect(item, "activate", G_CALLBACK(OnAboutActivate), this);
  item = gtk_menu_item_new_with_label("Undock from Sidebar");
  gtk_menu_shell_append(menu_shell, item);
  g_signal_connect(item, "activate", G_CALLBACK(OnDockActivate), this);

  gtk_widget_show_all(GTK_WIDGET(menu));
  gtk_menu_popup(menu_->menu(), NULL, NULL, NULL, menu_, 0, 0);
}

void GtkGadgetHost::OnMenuClicked(GtkButton *button, gpointer user_data) {
  GtkGadgetHost *this_p = static_cast<GtkGadgetHost *>(user_data);
  this_p->PopupMenu();
}

void GtkGadgetHost::OnBackClicked(GtkButton *button, gpointer user_data) {
  // GtkGadgetHost *this_p = static_cast<GtkGadgetHost *>(user_data);
  DLOG("Back");
}

void GtkGadgetHost::OnForwardClicked(GtkButton *button, gpointer user_data) {
  // GtkGadgetHost *this_p = static_cast<GtkGadgetHost *>(user_data);
  DLOG("Forward");
}

void GtkGadgetHost::OnDetailsClicked(GtkButton *button, gpointer user_data) {
  // GtkGadgetHost *this_p = static_cast<GtkGadgetHost *>(user_data);
  DLOG("Details");
}

void GtkGadgetHost::OnCollapseActivate(GtkMenuItem *menu_item,
                                       gpointer user_data) {
  // GtkGadgetHost *this_p = static_cast<GtkGadgetHost *>(user_data);
  DLOG("CollapseActivate");
}

void GtkGadgetHost::OnOptionsActivate(GtkMenuItem *menu_item,
                                      gpointer user_data) {
  // GtkGadgetHost *this_p = static_cast<GtkGadgetHost *>(user_data);
  DLOG("OptionsActivate");
}

void GtkGadgetHost::OnAboutActivate(GtkMenuItem *menu_item,
                                    gpointer user_data) {
  // GtkGadgetHost *this_p = static_cast<GtkGadgetHost *>(user_data);
  DLOG("AboutActivate");
}

void GtkGadgetHost::OnDockActivate(GtkMenuItem *menu_item,
                                   gpointer user_data) {
  // GtkGadgetHost *this_p = static_cast<GtkGadgetHost *>(user_data);
  DLOG("DockActivate");
}
