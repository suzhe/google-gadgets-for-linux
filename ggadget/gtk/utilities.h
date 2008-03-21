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

#ifndef GGADGET_GTK_UTILITIES_H__
#define GGADGET_GTK_UTILITIES_H__

#include <string>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

namespace ggadget {

class Gadget;
namespace gtk {

/**
 * Displays a message box containing the message string.
 *
 * @param title tile of the alert window.
 * @param message the alert message.
 */
void ShowAlertDialog(const char *title, const char *message);

/**
 * Displays a dialog containing the message string and Yes and No buttons.
 *
 * @param title tile of the alert window.
 * @param message the message string.
 * @return @c true if Yes button is pressed, @c false if not.
 */
bool ShowConfirmDialog(const char *title, const char *message);

/**
 * Displays a dialog asking the user to enter text.
 *
 * @param title tile of the alert window.
 * @param message the message string displayed before the edit box.
 * @param default_value the initial default value dispalyed in the edit box.
 * @return the user inputted text, or an empty string if user canceled the
 *     dialog.
 */
std::string ShowPromptDialog(const char *title, const char *message,
                             const char *default_value);

/**
 * Shows an about dialog for a specified gadget.
 */
void ShowGadgetAboutDialog(Gadget *gadget);

/** Open the given URL in the user's default web browser. */
bool OpenURL(const char *url);

/** Load a given font into the application. */
bool LoadFont(const char *filename);

/**
 * Loads a GdkPixbuf object from raw image data.
 * @param data A string object containing the raw image data.
 * @return NULL on failure, a GdkPixbuf object otherwise.
 */
GdkPixbuf *LoadPixbufFromData(const std::string &data);

/**
 * Creates a GdkCursor for a specified cursor type.
 *
 * @param type the cursor type, see @c ViewInterface::CursorType.
 * @return a new  GdkCursor instance when succeeded, otherwize NULL.
 */
GdkCursor *CreateCursor(int type);

/**
 * Disables the background of a widget.
 *
 * This function can only take effect when the Window system supports RGBA
 * visual. In another word, a window manager that supports composition must be
 * available.
 *
 * @param widget the GtkWidget of which the background to be disabled.
 * @return true if succeeded.
 */
bool DisableWidgetBackground(GtkWidget *widget);

} // namespace gtk
} // namespace ggadget

#endif // GGADGET_GTK_UTILITIES_H__
