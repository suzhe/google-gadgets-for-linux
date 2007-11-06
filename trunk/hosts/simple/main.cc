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

#include <cstdlib>
#include <gtk/gtk.h>
#include <locale.h>

#include <ggadget/ggadget.h>
#include "gadget_view_widget.h"
#include "gtk_gadget_host.h"
#include "gtk_view_host.h"

static double g_zoom = 1.;
static int g_debug_mode = 0;
static GtkGadgetHost *g_gadget_host = NULL;
static ggadget::GadgetInterface *g_gadget = NULL;

static gboolean DeleteEventHandler(GtkWidget *widget,
                                   GdkEvent *event,
                                   gpointer data) {
  return FALSE;
}

static gboolean DestroyHandler(GtkWidget *widget,
                               gpointer data) {
  gtk_main_quit();
  return FALSE;
}

static bool CreateGadgetUI(GtkWindow *window, GtkBox *box,
                           const char *base_path) {
  g_gadget_host = new GtkGadgetHost();
  g_gadget = g_gadget_host->LoadGadget(base_path, g_zoom, g_debug_mode);
  if (!g_gadget) {
    LOG("Error: unable to load gadget from %s", base_path);
    return false;
  }

  GtkViewHost *view_host = ggadget::down_cast<GtkViewHost *>(
      g_gadget->GetMainViewHost());
  GadgetViewWidget *gvw = view_host->GetWidget();
  gtk_box_pack_start(box, GTK_WIDGET(gvw), TRUE, TRUE, 0);

  // Setting min size here allows the window to resize below the size
  // request of the gadget view.
  GdkGeometry geometry;
  geometry.min_width = geometry.min_height = 100;
  gtk_window_set_geometry_hints(window, GTK_WIDGET(gvw),
                                &geometry, GDK_HINT_MIN_SIZE);
  return true;
}

static bool CreateGTKUI(const char *base_path) {
  GtkWidget *window;
  GtkWidget *exit_button;
  GtkWidget *separator;
  GtkWidget *label;
  GtkBox *vbox;

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(window), "Google Gadgets");
  g_signal_connect(G_OBJECT(window), "delete_event",
                   G_CALLBACK(DeleteEventHandler), NULL);
  g_signal_connect(G_OBJECT(window), "destroy",
                   G_CALLBACK(DestroyHandler), NULL);

  vbox = GTK_BOX(gtk_vbox_new(FALSE, 0));
  gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(vbox));

  exit_button = gtk_button_new_with_label("Exit");
  gtk_box_pack_end(vbox, exit_button, FALSE, FALSE, 0);
  g_signal_connect_swapped(G_OBJECT(exit_button), "clicked",
                           G_CALLBACK(gtk_widget_destroy), G_OBJECT(window));

  separator = gtk_hseparator_new();
  gtk_box_pack_end(vbox, separator, TRUE, TRUE, 5);

  label = gtk_label_new("Gadget Main View:");
  gtk_box_pack_start(vbox, label, FALSE, FALSE, 0);

  if (!CreateGadgetUI(GTK_WINDOW(window), vbox, base_path)) {
    return false;
  }

  gtk_widget_show_all(window);

  return true;
}

static void DestroyUI() {
  delete g_gadget;
  g_gadget = NULL;
  delete g_gadget_host;
  g_gadget_host = NULL;
}

int main(int argc, char* argv[]) {
  gtk_init(&argc, &argv);

  // set locale according to env vars
  setlocale(LC_ALL, "");

  if (argc < 2) {
    LOG("Error: not enough arguments. Gadget base path required.");
    return -1;
  }

  if (argc >= 3) {
    sscanf(argv[2], "%lg", &g_zoom);
    if (g_zoom <= 0 || g_zoom > 5) {
      LOG("Zoom level invalid, resetting to 1");
      g_zoom = 1.;
    }
  }

  if (argc == 4) {
    sscanf(argv[3], "%d", &g_debug_mode);
    if (g_debug_mode < 0 || g_debug_mode > 2) {
      LOG("Debug mode invalid, resetting to 0");
      g_debug_mode = 0;
    }
  }

  if (!CreateGTKUI(argv[1])) {
    LOG("Error: unable to create UI");
    return -1;
  }

  gtk_main();

  DestroyUI();

  return 0;
}
