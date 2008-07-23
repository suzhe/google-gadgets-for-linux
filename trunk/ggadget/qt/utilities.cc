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

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string>
#include <QtGui/QMessageBox>
#include <QtGui/QPixmap>
#include <ggadget/common.h>
#include <ggadget/logger.h>
#include <ggadget/gadget.h>
#include <ggadget/gadget_consts.h>
#include <ggadget/string_utils.h>
#include <ggadget/file_manager_interface.h>
#include <ggadget/view_interface.h>
#include "utilities.h"
#include "utilities_internal.h"

namespace ggadget {
namespace qt {

struct CursorTypeMapping {
  int type;
  Qt::CursorShape qt_type;
};

static const CursorTypeMapping kCursorTypeMappings[] = {
  { ViewInterface::CURSOR_ARROW, Qt::ArrowCursor }, // FIXME
  { ViewInterface::CURSOR_IBEAM, Qt::IBeamCursor },
  { ViewInterface::CURSOR_WAIT, Qt::WaitCursor },
  { ViewInterface::CURSOR_CROSS, Qt::CrossCursor },
  { ViewInterface::CURSOR_UPARROW, Qt::UpArrowCursor },
  { ViewInterface::CURSOR_SIZE, Qt::SizeAllCursor },
  { ViewInterface::CURSOR_SIZENWSE, Qt::SizeFDiagCursor },
  { ViewInterface::CURSOR_SIZENESW, Qt::SizeBDiagCursor },
  { ViewInterface::CURSOR_SIZEWE, Qt::SizeHorCursor },
  { ViewInterface::CURSOR_SIZENS, Qt::SizeVerCursor },
  { ViewInterface::CURSOR_SIZEALL, Qt::SizeAllCursor },
  { ViewInterface::CURSOR_NO, Qt::ForbiddenCursor },
  { ViewInterface::CURSOR_HAND, Qt::OpenHandCursor },
  { ViewInterface::CURSOR_BUSY, Qt::BusyCursor }, // FIXME
  { ViewInterface::CURSOR_HELP, Qt::WhatsThisCursor }
};

Qt::CursorShape GetQtCursorShape(int type) {
  for (size_t i = 0; i < arraysize(kCursorTypeMappings); ++i) {
    if (kCursorTypeMappings[i].type == type)
      return kCursorTypeMappings[i].qt_type;
  }
  return Qt::ArrowCursor;
}

// Check out qt document to get more information about MouseButtons and
// MouseButton
int GetMouseButtons(const Qt::MouseButtons buttons){
  int ret = 0;
  if (buttons & Qt::LeftButton) ret |= MouseEvent::BUTTON_LEFT;
  if (buttons & Qt::RightButton) ret |= MouseEvent::BUTTON_RIGHT;
  if (buttons & Qt::MidButton) ret |= MouseEvent::BUTTON_MIDDLE;
  return ret;
}

int GetMouseButton(const Qt::MouseButton button) {
  if (button == Qt::LeftButton) return MouseEvent::BUTTON_LEFT;
  if (button == Qt::RightButton) return MouseEvent::BUTTON_RIGHT;
  if (button == Qt::MidButton) return MouseEvent::BUTTON_MIDDLE;
  return 0;
}

int GetModifiers(Qt::KeyboardModifiers state) {
  int mod = Event::MOD_NONE;
  if (state & Qt::ShiftModifier) mod |= Event::MOD_SHIFT;
  if (state & Qt::ControlModifier)  mod |= Event::MOD_CONTROL;
  if (state & Qt::AltModifier) mod |= Event::MOD_ALT;
  return mod;
}

unsigned int GetKeyCode(int qt_key) {
  return qt_key;
}

void ShowGadgetAboutDialog(Gadget *gadget) {
  ASSERT(gadget);

  // About text
  std::string about_text =
      TrimString(gadget->GetManifestInfo(kManifestAboutText));

  if (about_text.empty()) {
    gadget->OnCommand(Gadget::CMD_ABOUT_DIALOG);
    return;
  }

  // Title and Copyright
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

  // Remove HTML tags from the text.
  if (ContainsHTML(title_text.c_str()))
    title_text = ExtractTextFromHTML(title_text.c_str());
  if (ContainsHTML(copyright_text.c_str()))
    copyright_text = ExtractTextFromHTML(copyright_text.c_str());
  if (ContainsHTML(about_text.c_str()))
    about_text = ExtractTextFromHTML(about_text.c_str());

  std::string title_copyright = "<b>";
  title_copyright.append(title_text);
  title_copyright.append("</b><br>");
  title_copyright.append(copyright_text);

  // Load icon
  std::string icon_name = gadget->GetManifestInfo(kManifestIcon);
  std::string data;
  QPixmap icon;
  if (gadget->GetFileManager()->ReadFile(icon_name.c_str(), &data)) {
    icon.loadFromData(reinterpret_cast<const uchar *>(data.c_str()),
                      static_cast<int>(data.length()));
  }

  QMessageBox box(QMessageBox::NoIcon,
                  QString::fromUtf8(title_text.c_str()),
                  QString::fromUtf8(title_copyright.c_str()),
                  QMessageBox::Ok);
  box.setInformativeText(QString::fromUtf8(about_text.c_str()));

  box.setIconPixmap(icon);
  box.exec();
}

QWidget *NewGadgetDebugConsole(Gadget *gadget, QWidget** widget) {
  DebugConsole *console = new DebugConsole(gadget, widget);
  console->show();
  return console;
}
} // namespace qt
} // namespace ggadget
#include "utilities_internal.moc"
