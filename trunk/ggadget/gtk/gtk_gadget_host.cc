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

#include <ggadget/ggadget.h>
#include <ggadget/element_factory.h>
#include <ggadget/file_manager.h>
#include <ggadget/gadget.h>

#include <ggadget/anchor_element.h>
#include <ggadget/button_element.h>
#include <ggadget/checkbox_element.h>
#include <ggadget/combobox_element.h>
#include <ggadget/contentarea_element.h>
#include <ggadget/div_element.h>
#include <ggadget/edit_element.h>
#include <ggadget/img_element.h>
#include <ggadget/item_element.h>
#include <ggadget/label_element.h>
#include <ggadget/listbox_element.h>
#include <ggadget/progressbar_element.h>
#include <ggadget/scrollbar_element.h>

#include "gtk_gadget_host.h"
#include "cairo_graphics.h"
#include "global_file_manager.h"
#include "gtk_menu_impl.h"
#include "gtk_view_host.h"
#include "options.h"

namespace ggadget {

class GtkGadgetHost::CallbackData {
 public:
  CallbackData(Slot *s, GtkGadgetHost *h)
      : callback(s), host(h) { }
  virtual ~CallbackData() {
    delete callback;
  }

  int id;
  Slot *callback;
  GtkGadgetHost *host;
};

GtkGadgetHost::GtkGadgetHost(ScriptRuntimeInterface *script_runtime,
                             FrameworkInterface *framework,
                             bool composited, bool useshapemask,
                             double zoom, int debug_mode)
    : script_runtime_(script_runtime),
      element_factory_(NULL),
      global_file_manager_(new GlobalFileManager()),
      file_manager_(new FileManager(global_file_manager_)),
      options_(new Options()),
      framework_(framework),
      gadget_(NULL),
      plugin_flags_(0), composited_(composited), useshapemask_(useshapemask),
      zoom_(zoom), debug_mode_(debug_mode),
      toolbox_(NULL), menu_button_(NULL), back_button_(NULL),
      forward_button_(NULL), details_button_(NULL),
      menu_(NULL) {
  ElementFactory *factory = new ElementFactory();
  factory->RegisterElementClass("a", &ggadget::AnchorElement::CreateInstance);
  factory->RegisterElementClass("button",
                                &ggadget::ButtonElement::CreateInstance);
  factory->RegisterElementClass("checkbox",
                             &ggadget::CheckBoxElement::CreateCheckBoxInstance);
  factory->RegisterElementClass("combobox",
                                &ggadget::ComboBoxElement::CreateInstance);
  factory->RegisterElementClass("contentarea",
                                &ggadget::ContentAreaElement::CreateInstance);
  factory->RegisterElementClass("div",
                                &ggadget::DivElement::CreateInstance);
  factory->RegisterElementClass("edit",
                                &ggadget::EditElement::CreateInstance);
  factory->RegisterElementClass("img",
                                &ggadget::ImgElement::CreateInstance);
  factory->RegisterElementClass("item",
                                &ggadget::ItemElement::CreateInstance);
  factory->RegisterElementClass("label",
                                &ggadget::LabelElement::CreateInstance);
  factory->RegisterElementClass("listbox",
                                &ggadget::ListBoxElement::CreateInstance);
  factory->RegisterElementClass("listitem",
                                &ggadget::ItemElement::CreateListItemInstance);
  factory->RegisterElementClass("progressbar",
                                &ggadget::ProgressBarElement::CreateInstance);
  factory->RegisterElementClass("radio",
                                &ggadget::CheckBoxElement::CreateRadioInstance);
  factory->RegisterElementClass("scrollbar",
                                &ggadget::ScrollBarElement::CreateInstance);
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
  delete framework_;
  framework_ = NULL;
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

ScriptRuntimeInterface *GtkGadgetHost::GetScriptRuntime(
    ScriptRuntimeType type) {
  return script_runtime_;
}

ElementFactoryInterface *GtkGadgetHost::GetElementFactory() {
  return element_factory_;
}

FileManagerInterface *GtkGadgetHost::GetFileManager() {
  return file_manager_;
}

FileManagerInterface *GtkGadgetHost::GetGlobalFileManager() {
  return global_file_manager_;
}

OptionsInterface *GtkGadgetHost::GetOptions() {
  return options_;
}

FrameworkInterface *GtkGadgetHost::GetFramework() {
  return framework_;
}

GadgetInterface *GtkGadgetHost::GetGadget() {
  return gadget_;
}

ViewHostInterface *GtkGadgetHost::NewViewHost(
    ViewType type, ScriptableInterface *prototype) {
  return new GtkViewHost(this, type, prototype, 
                         composited_, useshapemask_, zoom_, debug_mode_);
}

void GtkGadgetHost::SetPluginFlags(int plugin_flags) {
  if (plugin_flags & PLUGIN_FLAG_TOOLBAR_BACK)
    gtk_widget_show(back_button_);
  else
    gtk_widget_hide(back_button_);

  if (plugin_flags & PLUGIN_FLAG_TOOLBAR_FORWARD)
    gtk_widget_show(forward_button_);
  else
    gtk_widget_hide(forward_button_);
}

void GtkGadgetHost::RemoveMe(bool save_data) {
}

void GtkGadgetHost::DebugOutput(DebugLevel level, const char *message) const {
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
  return static_cast<uint64_t>(tv.tv_sec) * 1000 + tv.tv_usec / 1000;
}

gboolean GtkGadgetHost::DispatchTimer(gpointer data) {
  CallbackData *tmdata = static_cast<CallbackData *>(data);
  // view->OnTimerEvent may call RemoveTimer, causing tmdata pointer invalid,
  // so save the valid host pointer first.
  GtkGadgetHost *host = tmdata->host;
  int id = tmdata->id;
  // DLOG("DispatchTimer id=%d", id);

  Variant param(id);
  Variant result = tmdata->callback->Call(1, &param);
  if (!VariantValue<bool>()(result)) {
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
  Variant param(g_io_channel_unix_get_fd(source));
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

bool GtkGadgetHost::LoadFont(const char *filename) {
  std::string fontfile;
  if (!file_manager_->ExtractFile(filename, &fontfile)) {
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
                               const char *base_path) {
  gadget_ = new Gadget(this);
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
  gtk_widget_set_no_show_all(forward_button_, TRUE);
  back_button_ = gtk_button_new_with_label(" < ");
  gtk_box_pack_end(toolbox_, back_button_, FALSE, FALSE, 0);
  g_signal_connect(back_button_, "clicked",
                   G_CALLBACK(OnBackClicked), this);
  gtk_widget_set_no_show_all(back_button_, TRUE);
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
  gtk_widget_set_sensitive(GTK_WIDGET(item), gadget_->HasOptionsDialog());
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
  GtkGadgetHost *this_p = static_cast<GtkGadgetHost *>(user_data);
  this_p->gadget_->OnCommand(GadgetInterface::CMD_TOOLBAR_BACK);
}

void GtkGadgetHost::OnForwardClicked(GtkButton *button, gpointer user_data) {
  GtkGadgetHost *this_p = static_cast<GtkGadgetHost *>(user_data);
  this_p->gadget_->OnCommand(GadgetInterface::CMD_TOOLBAR_FORWARD);
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
  DLOG("OptionsActivate");
  GtkGadgetHost *this_p = static_cast<GtkGadgetHost *>(user_data);
  this_p->gadget_->ShowOptionsDialog();
}

void GtkGadgetHost::OnAboutActivate(GtkMenuItem *menu_item,
                                    gpointer user_data) {
  GtkGadgetHost *this_p = static_cast<GtkGadgetHost *>(user_data);
  const char *about_text = this_p->gadget_->GetManifestInfo(
      kManifestAboutText);
  if (!about_text || !*about_text) {
    this_p->gadget_->OnCommand(GadgetInterface::CMD_ABOUT_DIALOG);
  } else {
    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        this_p->gadget_->GetManifestInfo(kManifestName), NULL,
        static_cast<GtkDialogFlags>(GTK_DIALOG_MODAL | GTK_DIALOG_NO_SEPARATOR),
        GTK_STOCK_OK, GTK_RESPONSE_OK,
        NULL);
    gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
    gtk_window_set_skip_taskbar_hint(GTK_WINDOW(dialog), TRUE);
    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);

    std::string about_text(TrimString(
        this_p->gadget_->GetManifestInfo(kManifestAboutText)));
    std::string title_text;
    std::string copyright_text;
    if (!SplitString(about_text, "\n", &title_text, &about_text)) {
      about_text = title_text; 
      title_text = this_p->gadget_->GetManifestInfo(kManifestName);
    }
    title_text = TrimString(title_text);
    about_text = TrimString(about_text);

    if (!SplitString(about_text, "\n", &copyright_text, &about_text)) {
      about_text = copyright_text;
      copyright_text = this_p->gadget_->GetManifestInfo(kManifestCopyright);
    }
    copyright_text = TrimString(copyright_text);
    about_text = TrimString(about_text);

    GtkWidget *title = gtk_label_new("");
    gchar *gadget_name_markup = g_markup_printf_escaped(
        "<b><big>%s</big></b>", title_text.c_str());
    gtk_label_set_markup(GTK_LABEL(title), gadget_name_markup);
    g_free(gadget_name_markup);
    gtk_label_set_line_wrap(GTK_LABEL(title), TRUE);
    gtk_misc_set_alignment(GTK_MISC(title), 0.0, 0.0);

    GtkWidget *copyright = gtk_label_new(copyright_text.c_str());
    gtk_label_set_line_wrap(GTK_LABEL(copyright), TRUE);
    gtk_misc_set_alignment(GTK_MISC(copyright), 0.0, 0.0);

    GtkWidget *about = gtk_label_new(about_text.c_str());
    gtk_label_set_line_wrap(GTK_LABEL(about), TRUE);
    gtk_label_set_selectable(GTK_LABEL(about), TRUE);
    gtk_misc_set_alignment(GTK_MISC(about), 0.0, 0.0);
    GtkWidget *about_box = gtk_vbox_new(FALSE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(about_box), 10);
    gtk_box_pack_start(GTK_BOX(about_box), about, FALSE, FALSE, 0);

    GtkWidget *image = NULL;
    const char *icon_name = this_p->gadget_->GetManifestInfo(kManifestIcon);
    std::string data;
    std::string real_path;
    if (this_p->file_manager_->GetFileContents(icon_name, &data, &real_path)) {
      GdkPixbuf *pixbuf = CairoGraphics::LoadPixbufFromData(data.c_str(),
                                                            data.size());
      if (pixbuf) {
        image = gtk_image_new_from_pixbuf(pixbuf);
        g_object_unref(pixbuf);
      }
    }

    GtkWidget *hbox = gtk_hbox_new(FALSE, 12);
    GtkWidget *vbox = gtk_vbox_new(FALSE, 12);
    gtk_box_pack_start(GTK_BOX(vbox), title, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), copyright, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), image, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox,
                       FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), about_box,
                       FALSE, FALSE, 0);

    gtk_container_set_border_width(GTK_CONTAINER(hbox), 10);
    gtk_container_set_border_width(
        GTK_CONTAINER(GTK_DIALOG(dialog)->action_area), 10);

    gtk_widget_show_all(dialog);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
  }
}

void GtkGadgetHost::OnDockActivate(GtkMenuItem *menu_item,
                                   gpointer user_data) {
  // GtkGadgetHost *this_p = static_cast<GtkGadgetHost *>(user_data);
  DLOG("DockActivate");
}

class GSListFiles : public GadgetHostInterface::FilesInterface {
 public:
  GSListFiles(GSList *list) : list_(list) { }
  virtual ~GSListFiles() {
    for (GSList *list = list_; list; list = g_slist_next(list)) {
      g_free(list->data);
    }
    g_slist_free(list_);
  }
  virtual void Destroy() { delete this; }
  virtual int GetCount() const { return g_slist_length(list_); }
  virtual const char *GetItem(int index) const {
    return static_cast<const char *>(g_slist_nth_data(list_, index));
  }
 private:
  DISALLOW_EVIL_CONSTRUCTORS(GSListFiles);
  GSList *list_;
};

GadgetHostInterface::FilesInterface *GtkGadgetHost::BrowseForFiles(
    const char *filter, bool multiple) {
  GtkWidget *dialog = gtk_file_chooser_dialog_new(
      gadget_->GetManifestInfo(kManifestName), NULL,
      GTK_FILE_CHOOSER_ACTION_OPEN,
      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
      GTK_STOCK_OK, GTK_RESPONSE_OK,
      NULL);

  gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), multiple);
  if (filter) {
    std::string filter_str(filter);
    std::string filter_name, patterns, pattern;
    while (!filter_str.empty()) {
      if (!SplitString(filter_str, "|", &filter_name, &filter_str)) {
        LOG("Invalid filter string: %s", filter_str.c_str());
        break;
      }
      GtkFileFilter *file_filter = gtk_file_filter_new();
      gtk_file_filter_set_name(file_filter, filter_name.c_str());
      SplitString(filter_str, "|", &patterns, &filter_str);
      while (!patterns.empty()) {
        SplitString(patterns, ";", &pattern, &patterns);
        gtk_file_filter_add_pattern(file_filter, pattern.c_str());
      }
      gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), file_filter);
    }
  }

  GSList *selected_files = NULL;
  if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK)
    selected_files = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(dialog));
  gtk_widget_destroy(dialog);

  return new GSListFiles(selected_files);
}

void GtkGadgetHost::GetCursorPos(int *x, int *y) const {
  gdk_display_get_pointer(gdk_display_get_default(), NULL, x, y, NULL);
}

void GtkGadgetHost::GetScreenSize(int *width, int *height) const {
  GdkScreen *screen;
  gdk_display_get_pointer(gdk_display_get_default(), &screen, NULL, NULL, NULL);
  *width = gdk_screen_get_width(screen);
  *height = gdk_screen_get_height(screen);
}

const char *GtkGadgetHost::GetFileIcon(const char *filename) const {
  // TODO:
  return "/usr/share/icons/application-default-icon.png";
}

// TODO:
class TemporaryAudioclip : public AudioclipInterface {
 public:
  virtual void Destroy() { delete this; }
  virtual int GetBalance() const { return 0; }
  virtual void SetBalance(int balance) { }
  virtual int GetCurrentPosition() const { return 0; }
  virtual void SetCurrentPosition(int position) { }
  virtual int GetDuration() const { return 100; }
  virtual ErrorCode GetError() const { return SOUND_ERROR_NO_ERROR; }
  virtual const char *GetSrc() const { return "src"; }
  virtual void SetSrc(const char *src) { }
  virtual State GetState() const { return SOUND_STATE_PLAYING; }
  virtual int GetVolume() const { return 100; }
  virtual void SetVolume(int volume) const { }
  virtual void Play() { }
  virtual void Pause() { }
  virtual void Stop() { }
  virtual OnStateChangeHandler *GetOnStateChange() const { return NULL; }
  virtual void SetOnStateChange(OnStateChangeHandler *handler) {
    delete handler;
  }
};


AudioclipInterface *GtkGadgetHost::CreateAudioclip(const char *src) {
  return new TemporaryAudioclip();
}

} // namespace ggadget
