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

#include <vector>
#include <QtGui/QCursor>
#include <QtGui/QFileDialog>
#include <QtGui/QDesktopWidget>
#include <ggadget/common.h>
#include <ggadget/gadget_consts.h>
#include <ggadget/framework_interface.h>
#include <ggadget/registerable_interface.h>
#include <ggadget/scriptable_interface.h>
#include <ggadget/scriptable_framework.h>
#include <ggadget/scriptable_array.h>
#include <ggadget/string_utils.h>
#include <ggadget/gadget.h>

#define Initialize qt_system_framework_LTX_Initialize
#define Finalize qt_system_framework_LTX_Finalize
#define RegisterFrameworkExtension \
    qt_system_framework_LTX_RegisterFrameworkExtension

namespace ggadget {
namespace framework {

// To avoid naming conflicts.
namespace qt_system_framework {

class QtSystemCursor : public CursorInterface {
 public:
  virtual void GetPosition(int *x, int *y)  {
    QPoint p = QCursor::pos();
    if (x) *x = p.x();
    if (y) *y = p.y();
  }
};

class QtSystemScreen : public ScreenInterface {
 public:
  virtual void GetSize(int *width, int *height) {
    QDesktopWidget w;
    QRect r = w.screenGeometry();
    if (width) *width = r.width();
    if (height) *height = r.height();
  }
};

class QtSystemBrowseForFileHelper {
 public:
  QtSystemBrowseForFileHelper(ScriptableInterface *framework, Gadget *gadget)
    : gadget_(gadget) {
    framework->ConnectOnReferenceChange(
      NewSlot(this, &QtSystemBrowseForFileHelper::OnFrameworkRefChange));
  }

  // Function to destroy the helper object when framework is destroyed.
  void OnFrameworkRefChange(int ref, int change) {
    if (!ref || !change) {
      DLOG("Framework destroyed, delete QtSystemBrowseForFileHelper object.");
      delete this;
    }
  }

  std::string BrowseForFile(const char *filter) {
    std::string result;
    std::vector<std::string> files;
    if (BrowseForFilesImpl(filter, false, &files) && files.size() > 0)
      result = files[0];
    return result;
  }

  ScriptableArray *BrowseForFiles(const char *filter) {
    std::vector<std::string> files;
    BrowseForFilesImpl(filter, true, &files);
    return ScriptableArray::Create(files.begin(), files.size());
  }

  bool BrowseForFilesImpl(const char *filter,
      bool multiple,
      std::vector<std::string> *result) {
    ASSERT(result);
    result->clear();

    QStringList filters;
    QFileDialog dialog;
    if (multiple) dialog.setFileMode(QFileDialog::ExistingFiles);
    if (filter && *filter) {
      size_t len = strlen(filter);
      char *copy = new char[len + 2];
      memcpy(copy, filter, len + 1);
      copy[len] = '|';
      copy[len + 1] = '\0';
      char *str = copy;
      int i = 0;
      int t = 0;
      while (str[i] != '\0') {
        if (str[i] == '|') {
          t++;
          if (t == 1) str[i] = '(';
          if (t == 2) {
            str[i] = ')';
            char bak = str[i + 1];
            str[i + 1] = '\0';
            filters << str;
            str[i + 1] = bak;
            str = &str[i+1];
            i = 0;
            t = 0;
            continue;
          }
        } else if (str[i] == ';' && t == 1) {
          str[i] = ' ';
        }
        i++;
      }
      delete [] copy;
      dialog.setFilters(filters);
    }
    if (dialog.exec()) {
      QStringList fnames = dialog.selectedFiles();
      for (int i = 0; i < fnames.size(); ++i)
        result->push_back(fnames.at(i).toStdString());
      return true;
    }
    return false;
  }

  Gadget *gadget_;
};


static QtSystemCursor g_cursor_;
static QtSystemScreen g_screen_;
static ScriptableCursor g_script_cursor_(&g_cursor_);
static ScriptableScreen g_script_screen_(&g_screen_);

} // namespace qt_system_framework
} // namespace framework
} // namespace ggadget

using namespace ggadget;
using namespace ggadget::framework;
using namespace ggadget::framework::qt_system_framework;

extern "C" {
  bool Initialize() {
    LOG("Initialize qt_system_framework extension.");
    return true;
  }

  void Finalize() {
    LOG("Finalize qt_system_framework extension.");
  }

  bool RegisterFrameworkExtension(ScriptableInterface *framework,
                                  Gadget *gadget) {
    LOG("Register qt_system_framework extension.");
    ASSERT(framework && gadget);

    if (!framework)
      return false;

    RegisterableInterface *reg_framework = framework->GetRegisterable();

    if (!reg_framework) {
      LOG("Specified framework is not registerable.");
      return false;
    }

    QtSystemBrowseForFileHelper *helper =
        new QtSystemBrowseForFileHelper(framework, gadget);

    reg_framework->RegisterMethod("BrowseForFile",
        NewSlot(helper, &QtSystemBrowseForFileHelper::BrowseForFile));
    reg_framework->RegisterMethod("BrowseForFiles",
        NewSlot(helper, &QtSystemBrowseForFileHelper::BrowseForFiles));

    ScriptableInterface *system = NULL;
    // Gets or adds system object.
    Variant prop = GetPropertyByName(framework, "system");
    if (prop.type() != Variant::TYPE_SCRIPTABLE) {
      // property "system" is not available or have wrong type, then add one
      // with correct type.
      // Using SharedScriptable here, so that it can be destroyed correctly
      // when framework is destroyed.
      system = new SharedScriptable();
      reg_framework->RegisterVariantConstant("system", Variant(system));
    } else {
      system = VariantValue<ScriptableInterface *>()(prop);
    }

    if (!system) {
      LOG("Failed to retrieve or add framework.system object.");
      return false;
    }

    RegisterableInterface *reg_system = system->GetRegisterable();
    if (!reg_system) {
      LOG("framework.system object is not registerable.");
      return false;
    }

    reg_system->RegisterVariantConstant("cursor",
                                        Variant(&g_script_cursor_));
    reg_system->RegisterVariantConstant("screen",
                                        Variant(&g_script_screen_));
    return true;
  }
}
