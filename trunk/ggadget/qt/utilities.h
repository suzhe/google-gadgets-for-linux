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

#ifndef GGADGET_QT_UTILITIES_H__
#define GGADGET_QT_UTILITIES_H__

#include <string>
#include <QtGui/QCursor>

namespace ggadget {

class Gadget;
namespace qt {

/**
 * Shows an about dialog for a specified gadget.
 */
void ShowGadgetAboutDialog(Gadget *gadget);

Qt::CursorShape GetQtCursorShape(int type);

int GetMouseButtons(const Qt::MouseButtons buttons);

int GetMouseButton(const Qt::MouseButton button);

int GetModifiers(Qt::KeyboardModifiers state);

unsigned int GetKeyCode(int qt_key);

QWidget *NewGadgetDebugConsole(Gadget *gadget, QWidget **widget);

bool OpenURL(const Gadget *gadget, const char *url);

QPixmap GetGadgetIcon(const Gadget *gadget);

void SetGadgetWindowIcon(QWidget *widget, const Gadget *gadget);

/* Get the proper popup position to show a rectangle of @param size for an existing
 * rectangle with geometry @param rect
 */
QPoint GetPopupPosition(const QRect &rect, const QSize &size);
} // namespace qt
} // namespace ggadget

#endif // GGADGET_QT_UTILITIES_H__
