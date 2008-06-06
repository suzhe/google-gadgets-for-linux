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

#include <algorithm>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <cstdlib>
#include <string>
#include <fontconfig/fontconfig.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <ggadget/common.h>
#include <ggadget/logger.h>
#include <ggadget/gadget.h>
#include <ggadget/gadget_consts.h>
#include <ggadget/string_utils.h>
#include <ggadget/file_manager_interface.h>
#include <ggadget/view_interface.h>
#include "utilities.h"

namespace ggadget {
namespace gtk {

void ShowAlertDialog(const char *title, const char *message) {
  GtkWidget *dialog = gtk_message_dialog_new(NULL,
                                             GTK_DIALOG_MODAL,
                                             GTK_MESSAGE_INFO,
                                             GTK_BUTTONS_OK,
                                             "%s", message);
  GdkScreen *screen;
  gdk_display_get_pointer(gdk_display_get_default(), &screen, NULL, NULL, NULL);
  gtk_window_set_screen(GTK_WINDOW(dialog), screen);
  gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);
  gtk_window_set_title(GTK_WINDOW(dialog), title);
  gtk_dialog_run(GTK_DIALOG(dialog));
  gtk_widget_destroy(dialog);
}

bool ShowConfirmDialog(const char *title, const char *message) {
  GtkWidget *dialog = gtk_message_dialog_new(NULL,
                                             GTK_DIALOG_MODAL,
                                             GTK_MESSAGE_QUESTION,
                                             GTK_BUTTONS_YES_NO,
                                             "%s", message);
  GdkScreen *screen;
  gdk_display_get_pointer(gdk_display_get_default(), &screen, NULL, NULL, NULL);
  gtk_window_set_screen(GTK_WINDOW(dialog), screen);
  gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);
  gtk_window_set_title(GTK_WINDOW(dialog), title);
  gint result = gtk_dialog_run(GTK_DIALOG(dialog));
  gtk_widget_destroy(dialog);
  return result == GTK_RESPONSE_YES;
}

std::string ShowPromptDialog(const char *title, const char *message,
                             const char *default_value) {
  GtkWidget *dialog = gtk_dialog_new_with_buttons(
      title, NULL,
      static_cast<GtkDialogFlags>(GTK_DIALOG_MODAL | GTK_DIALOG_NO_SEPARATOR),
      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
      GTK_STOCK_OK, GTK_RESPONSE_OK,
      NULL);
  GdkScreen *screen;
  gdk_display_get_pointer(gdk_display_get_default(), &screen, NULL, NULL, NULL);
  gtk_window_set_screen(GTK_WINDOW(dialog), screen);
  gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);
  gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
  gtk_window_set_skip_taskbar_hint(GTK_WINDOW(dialog), TRUE);
  gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);

  GtkWidget *image = gtk_image_new_from_stock(GTK_STOCK_DIALOG_QUESTION,
                                              GTK_ICON_SIZE_DIALOG);
  GtkWidget *label = gtk_label_new(message);
  gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
  gtk_label_set_selectable(GTK_LABEL(label), TRUE);
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 1.0);
  GtkWidget *entry = gtk_entry_new();
  if (default_value)
    gtk_entry_set_text(GTK_ENTRY(entry), default_value);

  GtkWidget *hbox = gtk_hbox_new(FALSE, 12);
  GtkWidget *vbox = gtk_vbox_new(FALSE, 12);
  gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), entry, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), image, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox, FALSE, FALSE, 0);

  gtk_container_set_border_width(GTK_CONTAINER(hbox), 10);
  gtk_container_set_border_width(
      GTK_CONTAINER(GTK_DIALOG(dialog)->action_area), 10);

  gtk_widget_show_all(dialog);
  gint result = gtk_dialog_run(GTK_DIALOG(dialog));
  std::string text;
  if (result == GTK_RESPONSE_OK)
    text = gtk_entry_get_text(GTK_ENTRY(entry));
  gtk_widget_destroy(dialog);
  return text;
}

void ShowGadgetAboutDialog(Gadget *gadget) {
  ASSERT(gadget);

  std::string about_text =
      TrimString(gadget->GetManifestInfo(kManifestAboutText));

  if (about_text.empty()) {
    gadget->OnCommand(Gadget::CMD_ABOUT_DIALOG);
    return;
  }

  GtkWidget *dialog = gtk_dialog_new_with_buttons(
      gadget->GetManifestInfo(kManifestName).c_str(), NULL,
      static_cast<GtkDialogFlags>(GTK_DIALOG_MODAL | GTK_DIALOG_NO_SEPARATOR),
      GTK_STOCK_OK, GTK_RESPONSE_OK,
      NULL);
  GdkScreen *screen;
  gdk_display_get_pointer(gdk_display_get_default(), &screen, NULL, NULL, NULL);
  gtk_window_set_screen(GTK_WINDOW(dialog), screen);
  gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);
  gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
  gtk_window_set_skip_taskbar_hint(GTK_WINDOW(dialog), TRUE);
  gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);

  std::string title_text;
  std::string copyright_text;
  if (!SplitString(about_text, "\n", &title_text, &about_text)) {
    about_text = title_text;
    title_text = gadget->GetManifestInfo(kManifestName);
  }
  title_text = TrimString(title_text);
  about_text = TrimString(about_text);

  if (!SplitString(about_text, "\n", &copyright_text, &about_text)) {
    about_text = copyright_text;
    copyright_text = gadget->GetManifestInfo(kManifestCopyright);
  }
  copyright_text = TrimString(copyright_text);
  about_text = TrimString(about_text);

  // Remove HTML tags from the text because this dialog can't render them.
  if (ContainsHTML(title_text.c_str()))
    title_text = ExtractTextFromHTML(title_text.c_str());
  if (ContainsHTML(copyright_text.c_str()))
    copyright_text = ExtractTextFromHTML(copyright_text.c_str());
  if (ContainsHTML(about_text.c_str()))
    about_text = ExtractTextFromHTML(about_text.c_str());

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
  std::string icon_name = gadget->GetManifestInfo(kManifestIcon);
  std::string data;
  if (gadget->GetFileManager()->ReadFile(icon_name.c_str(), &data)) {
    GdkPixbuf *pixbuf = LoadPixbufFromData(data);
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

  gtk_window_set_title(GTK_WINDOW(dialog), title_text.c_str());
  gtk_widget_show_all(dialog);
  gtk_dialog_run(GTK_DIALOG(dialog));
  gtk_widget_destroy(dialog);
}

/**
 * Taken from GDLinux.
 * May move this function elsewhere if other classes use it too.
 */
#ifdef GGL_HOST_LINUX
static std::string GetFullPathOfSysCommand(const std::string &command) {
  const char *env_path_value = getenv("PATH");
  if (env_path_value == NULL)
    return "";

  std::string all_path = std::string(env_path_value);
  size_t cur_colon_pos = 0;
  size_t next_colon_pos = 0;
  // Iterator through all the parts in env value.
  while ((next_colon_pos = all_path.find(":", cur_colon_pos)) !=
         std::string::npos) {
    std::string path =
        all_path.substr(cur_colon_pos, next_colon_pos - cur_colon_pos);
    path += "/";
    path += command;
    if (access(path.c_str(), X_OK) == 0) {
      return path;
    }
    cur_colon_pos = next_colon_pos + 1;
  }
  return "";
}
#endif

bool OpenURL(const char *url) {
#ifdef GGL_HOST_LINUX
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

    execl(xdg_open.c_str(), xdg_open.c_str(), url, NULL);

    DLOG("Failed to exec command: %s", xdg_open.c_str());
    _exit(-1);
  }

  int status = 0;
  waitpid(pid, &status, 0);

  // Assume xdg-open will always succeed.
  return true;
#else
  LOG("Don't know how to open an url.");
  return false;
#endif
}

bool LoadFont(const char *filename) {
  FcConfig *config = FcConfigGetCurrent();
  bool success = FcConfigAppFontAddFile(config,
                   reinterpret_cast<const FcChar8 *>(filename));
  DLOG("LoadFont: %s %s", filename, success ? "success" : "fail");
  return success;
}

GdkPixbuf *LoadPixbufFromData(const std::string &data) {
  GdkPixbuf *pixbuf = NULL;
  GdkPixbufLoader *loader = NULL;
  GError *error = NULL;

  loader = gdk_pixbuf_loader_new();

  const guchar *ptr = reinterpret_cast<const guchar *>(data.c_str());
  if (gdk_pixbuf_loader_write(loader, ptr, data.size(), &error) &&
      gdk_pixbuf_loader_close(loader, &error)) {
    pixbuf = gdk_pixbuf_loader_get_pixbuf(loader);
    if (pixbuf) g_object_ref(pixbuf);
  }

  if (error) g_error_free(error);
  if (loader) g_object_unref(loader);

  return pixbuf;
}

struct CursorTypeMapping {
  int type;
  GdkCursorType gdk_type;
};

// Ordering in this array must match the declaration in
// ViewInterface::CursorType.
static const CursorTypeMapping kCursorTypeMappings[] = {
  { ViewInterface::CURSOR_ARROW, GDK_LEFT_PTR}, // FIXME
  { ViewInterface::CURSOR_IBEAM, GDK_XTERM },
  { ViewInterface::CURSOR_WAIT, GDK_WATCH },
  { ViewInterface::CURSOR_CROSS, GDK_CROSS },
  { ViewInterface::CURSOR_UPARROW, GDK_CENTER_PTR },
  { ViewInterface::CURSOR_SIZE, GDK_SIZING },
  { ViewInterface::CURSOR_SIZENWSE, GDK_ARROW }, // FIXME
  { ViewInterface::CURSOR_SIZENESW, GDK_ARROW }, // FIXME
  { ViewInterface::CURSOR_SIZEWE, GDK_SB_H_DOUBLE_ARROW }, // FIXME
  { ViewInterface::CURSOR_SIZENS, GDK_SB_V_DOUBLE_ARROW }, // FIXME
  { ViewInterface::CURSOR_SIZEALL, GDK_SIZING }, // FIXME
  { ViewInterface::CURSOR_NO, GDK_X_CURSOR },
  { ViewInterface::CURSOR_HAND, GDK_HAND1 },
  { ViewInterface::CURSOR_BUSY, GDK_WATCH }, // FIXME
  { ViewInterface::CURSOR_HELP, GDK_QUESTION_ARROW }
};

struct HitTestCursorTypeMapping {
  ViewInterface::HitTest hittest;
  GdkCursorType gdk_type;
};

static const HitTestCursorTypeMapping kHitTestCursorTypeMappings[] = {
  { ViewInterface::HT_LEFT, GDK_LEFT_SIDE },
  { ViewInterface::HT_RIGHT, GDK_RIGHT_SIDE },
  { ViewInterface::HT_TOP, GDK_TOP_SIDE },
  { ViewInterface::HT_BOTTOM, GDK_BOTTOM_SIDE },
  { ViewInterface::HT_TOPLEFT, GDK_TOP_LEFT_CORNER },
  { ViewInterface::HT_TOPRIGHT, GDK_TOP_RIGHT_CORNER },
  { ViewInterface::HT_BOTTOMLEFT, GDK_BOTTOM_LEFT_CORNER },
  { ViewInterface::HT_BOTTOMRIGHT, GDK_BOTTOM_RIGHT_CORNER }
};

GdkCursor *CreateCursor(int type, ViewInterface::HitTest hittest) {
  if (type < 0) return NULL;

  GdkCursorType gdk_type = GDK_ARROW;
  for (size_t i = 0; i < arraysize(kCursorTypeMappings); ++i) {
    if (kCursorTypeMappings[i].type == type) {
      gdk_type = kCursorTypeMappings[i].gdk_type;
      break;
    }
  }

  // No suitable mapping, try matching with hittest.
  if (gdk_type == GDK_ARROW) {
    for (size_t i = 0; i < arraysize(kHitTestCursorTypeMappings); ++i) {
      if (kHitTestCursorTypeMappings[i].hittest == hittest) {
        gdk_type = kHitTestCursorTypeMappings[i].gdk_type;
        break;
      }
    }
  }

  return gdk_cursor_new(gdk_type);
}

bool DisableWidgetBackground(GtkWidget *widget) {
  bool result = false;
  if (GTK_IS_WIDGET(widget) && SupportsComposite(widget)) {
    GdkScreen *screen = gtk_widget_get_screen(widget);
    GdkColormap *colormap = gdk_screen_get_rgba_colormap(screen);

    if (colormap) {
      if (GTK_WIDGET_REALIZED(widget))
        gtk_widget_unrealize(widget);
      gtk_widget_set_colormap(widget, colormap);
      gtk_widget_realize(widget);
      gdk_window_set_back_pixmap (widget->window, NULL, FALSE);
      result = true;
    }
  }
  return result;
}

bool SupportsComposite(GtkWidget *widget) {
  GdkScreen *screen = NULL;
  if (GTK_IS_WINDOW(widget))
    screen = gtk_widget_get_screen(widget);
  if (!screen)
    screen = gdk_screen_get_default();
#if GTK_CHECK_VERSION(2,10,0)
  return gdk_screen_is_composited(screen);
#else
  return gdk_screen_get_rgba_colormap(screen) != NULL;
#endif
}

#ifdef GDK_WINDOWING_X11
static bool MaximizeXWindow(GtkWidget *window,
                            bool maximize_vert, bool maximize_horz) {
  GdkDisplay *display = gtk_widget_get_display(window);
  Display *xd = gdk_x11_display_get_xdisplay(display);
  XClientMessageEvent xclient;
  memset(&xclient, 0, sizeof(xclient));
  xclient.type = ClientMessage;
  xclient.window = GDK_WINDOW_XWINDOW(window->window);
  xclient.message_type = XInternAtom(xd, "_NET_WM_STATE", False);
  xclient.format = 32;
  xclient.data.l[0] = 1;
  if (maximize_vert)
    xclient.data.l[1] = XInternAtom(xd, "_NET_WM_STATE_MAXIMIZED_VERT", False);
  if (maximize_horz)
    xclient.data.l[2] = XInternAtom(xd, "_NET_WM_STATE_MAXIMIZED_HORZ", False);

  gdk_error_trap_push();
  Status s = XSendEvent(xd, gdk_x11_get_default_root_xwindow(), False,
      SubstructureRedirectMask | SubstructureNotifyMask, (XEvent *)&xclient);
  gdk_error_trap_pop();
  DLOG("MaximizeXWindow result: %d", s);
  return !s;
}
#endif

bool MaximizeWindow(GtkWidget *window,
                    bool maximize_vert, bool maximize_horz) {
// This method is based on xlib, changed to gdk in the future if possible
#ifdef GDK_WINDOWING_X11
  return MaximizeXWindow(window, maximize_vert, maximize_horz);
#else
  return false;
#endif
}

void GetWorkAreaGeometry(GtkWidget *window, GdkRectangle *workarea) {
  ASSERT(GTK_IS_WINDOW(window));
  ASSERT(workarea);

#ifdef GDK_WINDOWING_X11
  static GdkAtom net_current_desktop_atom = GDK_NONE;
  static GdkAtom net_workarea_atom = GDK_NONE;

  if (net_current_desktop_atom == GDK_NONE)
    net_current_desktop_atom = gdk_atom_intern ("_NET_CURRENT_DESKTOP", TRUE);;
  if (net_workarea_atom == GDK_NONE)
    net_workarea_atom = gdk_atom_intern ("_NET_WORKAREA", TRUE);
#endif

  GdkScreen *screen = gtk_window_get_screen(GTK_WINDOW(window));
  GdkWindow *root = NULL;
  gint screen_width = 0;
  gint screen_height = 0;
  if (!screen)
    screen = gdk_screen_get_default();
  if (screen) {
    screen_width = gdk_screen_get_width(screen);
    screen_height = gdk_screen_get_height(screen);
    root = gdk_screen_get_root_window(screen);
  }
  if (!root)
    root = gdk_get_default_root_window();

  workarea->x = 0;
  workarea->y = 0;
  workarea->width = screen_width;
  workarea->height = screen_height;

  if (root) {
#ifdef GDK_WINDOWING_X11
    GdkAtom atom_ret;
    gint format = 0;
    gint length = 0;
    gint cur = 0;
    guchar *data = NULL;
    gboolean found = FALSE;

    found = gdk_property_get(root, net_current_desktop_atom,
                             GDK_NONE, 0, G_MAXLONG, FALSE,
                             &atom_ret, &format, &length, &data);
    if (found && format == 32 && length / sizeof(glong) > 0)
      cur = static_cast<gint>(reinterpret_cast<glong*>(data)[0]);
    if (found)
      g_free (data);

    found = gdk_property_get(root, net_workarea_atom,
                             GDK_NONE, 0, G_MAXLONG, FALSE,
                             &atom_ret, &format, &length, &data);
    if (found && format == 32 &&
        static_cast<gint>(length / sizeof(glong)) >= (cur + 1) * 4) {
      workarea->x = std::max(
            static_cast<int>(reinterpret_cast<glong*>(data)[cur * 4]), 0);
      workarea->y = std::max(
            static_cast<int>(reinterpret_cast<glong*>(data)[cur * 4 + 1]), 0);
      workarea->width = std::min(
            static_cast<int>(reinterpret_cast<glong*>(data)[cur * 4 + 2]),
            screen_width);
      workarea->height = std::min(
            static_cast<int>(reinterpret_cast<glong*>(data)[cur * 4 + 3]),
            screen_height);
    }
    if (found)
      g_free (data);
#endif
  }
}

#ifdef GDK_WINDOWING_X11
static const char kWorkAreaChangeSlotTag[] = "workarea-change-slot";
static const char kWorkAreaChangeSelfTag[] = "workarea-change-self";

static GdkFilterReturn WorkAreaPropertyNotifyFilter(
    GdkXEvent *gdk_xevent, GdkEvent *event, gpointer data) {
  g_return_val_if_fail(gdk_xevent, GDK_FILTER_CONTINUE);

  Slot0<void> *slot = static_cast<Slot0<void> *>(
      g_object_get_data(G_OBJECT(data), kWorkAreaChangeSlotTag));

  if (slot) {
    XEvent *xev = (XEvent*)gdk_xevent;
    if (xev->type == PropertyNotify &&
        (xev->xproperty.atom ==
         gdk_x11_get_xatom_by_name ("_NET_WORKAREA") ||
         xev->xproperty.atom ==
         gdk_x11_get_xatom_by_name ("_NET_CURRENT_DESKTOP"))) {
      DLOG("Work area changed, call slot.");
      (*slot)();
    }
  }

  return GDK_FILTER_CONTINUE;
}

static void WorkAreaChangeDestroySlotNotify(gpointer data) {
  delete static_cast<Slot0<void> *>(data);
}

static void WorkAreaChangeDestroySelfNotify(gpointer data) {
  GtkWidget *widget = GTK_WIDGET(data);
  if (widget) {
    GdkScreen *screen = gtk_widget_get_screen(widget);
    if (screen) {
      GdkWindow *root = gdk_screen_get_root_window(screen);
      if (root)
        gdk_window_remove_filter(root, WorkAreaPropertyNotifyFilter, widget);
    }
  }
}

static void WorkAreaScreenChangedCallback(GtkWidget *widget, GdkScreen *prev,
                                          gpointer data) {
  if (prev) {
    GdkWindow *root = gdk_screen_get_root_window(prev);
    if (root)
      gdk_window_remove_filter(root, WorkAreaPropertyNotifyFilter, widget);
  }

  GdkScreen *cur = gtk_widget_get_screen(widget);
  if (cur) {
    GdkWindow *root = gdk_screen_get_root_window(cur);
    if (root) {
      gdk_window_set_events(root, static_cast<GdkEventMask>(
          gdk_window_get_events(root) | GDK_PROPERTY_NOTIFY));
      gdk_window_add_filter(root, WorkAreaPropertyNotifyFilter, widget);
    }
  }
}
#endif

bool MonitorWorkAreaChange(GtkWidget *window, Slot0<void> *slot) {
  ASSERT(GTK_IS_WINDOW(window));

  // Only supports X11 window system.
#ifdef GDK_WINDOWING_X11
  if (window) {
    // If it's the first time to set the monitor, setup necessary signal
    // handlers.
    if (!g_object_get_data(G_OBJECT(window), kWorkAreaChangeSelfTag)) {
      g_signal_connect(G_OBJECT(window), "screen-changed",
                       G_CALLBACK(WorkAreaScreenChangedCallback), NULL);
      g_object_set_data_full(G_OBJECT(window), kWorkAreaChangeSelfTag,
                             window, WorkAreaChangeDestroySelfNotify);
      WorkAreaScreenChangedCallback(window, NULL, NULL);
    }

    // Attach the slot to the widget, the old one will be destroyed
    // automatically.
    g_object_set_data_full(G_OBJECT(window), kWorkAreaChangeSlotTag,
                           slot, WorkAreaChangeDestroySlotNotify);
    return true;
  }
#endif
  delete slot;
  return false;
}

} // namespace gtk
} // namespace ggadget
