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
#include <gtk/gtk.h>
#include <ggadget/common.h>
#include <ggadget/gadget_consts.h>
#include <ggadget/framework_interface.h>
#include <ggadget/registerable_interface.h>
#include <ggadget/scriptable_interface.h>
#include <ggadget/scriptable_framework.h>
#include <ggadget/scriptable_array.h>
#include <ggadget/string_utils.h>
#include <ggadget/gadget.h>

#define Initialize gtk_system_framework_LTX_Initialize
#define Finalize gtk_system_framework_LTX_Finalize
#define RegisterFrameworkExtension \
    gtk_system_framework_LTX_RegisterFrameworkExtension

namespace ggadget {
namespace framework {

// To avoid naming conflicts.
namespace gtk_system_framework {

class GtkSystemCursor : public CursorInterface {
 public:
  virtual void GetPosition(int *x, int *y)  {
    gint px, py;
    gdk_display_get_pointer(gdk_display_get_default(), NULL, &px, &py, NULL);
    if (x) *x = px;
    if (y) *y = py;
  }
};

class GtkSystemScreen : public ScreenInterface {
 public:
  virtual void GetSize(int *width, int *height) {
    GdkScreen *screen = NULL;
    gdk_display_get_pointer(gdk_display_get_default(), &screen, NULL, NULL, NULL);

    if (width) *width = gdk_screen_get_width(screen);
    if (height) *height = gdk_screen_get_height(screen);
  }
};

class GtkSystemBrowseForFileHelper {
 public:
  GtkSystemBrowseForFileHelper(ScriptableInterface *framework, Gadget *gadget)
    : gadget_(gadget) {
    framework->ConnectOnReferenceChange(
      NewSlot(this, &GtkSystemBrowseForFileHelper::OnFrameworkRefChange));
  }

  // Function to destroy the helper object when framework is destroyed.
  void OnFrameworkRefChange(int ref, int change) {
    if (change == 0) {
      DLOG("Framework destroyed, delete GtkSystemBrowseForFileHelper object.");
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

    GtkWidget *dialog = gtk_file_chooser_dialog_new(
        gadget_->GetManifestInfo(kManifestName).c_str(), NULL,
        GTK_FILE_CHOOSER_ACTION_OPEN,
        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
        GTK_STOCK_OK, GTK_RESPONSE_OK,
        NULL);

    gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), multiple);
    if (filter && *filter) {
      std::string filter_str(filter);
      std::string filter_name, patterns, pattern;
      while (!filter_str.empty()) {
        if (SplitString(filter_str, "|", &filter_name, &filter_str))
          SplitString(filter_str, "|", &patterns, &filter_str);
        else
          patterns = filter_name;

        GtkFileFilter *file_filter = gtk_file_filter_new();
        gtk_file_filter_set_name(file_filter, filter_name.c_str());
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

    if (!selected_files)
      return false;

    while (selected_files) {
      result->push_back(static_cast<const char *>(selected_files->data));
      selected_files = g_slist_next(selected_files);
    }
    return true;
  }

  Gadget *gadget_;
};


static GtkSystemCursor g_cursor_;
static GtkSystemScreen g_screen_;
static ScriptableCursor g_script_cursor_(&g_cursor_);
static ScriptableScreen g_script_screen_(&g_screen_);

} // namespace gtk_system_framework
} // namespace framework
} // namespace ggadget

using namespace ggadget;
using namespace ggadget::framework;
using namespace ggadget::framework::gtk_system_framework;

extern "C" {
  bool Initialize() {
    LOGI("Initialize gtk_system_framework extension.");
    return true;
  }

  void Finalize() {
    LOGI("Finalize gtk_system_framework extension.");
  }

  bool RegisterFrameworkExtension(ScriptableInterface *framework,
                                  Gadget *gadget) {
    LOGI("Register gtk_system_framework extension.");
    ASSERT(framework && gadget);

    if (!framework)
      return false;

    RegisterableInterface *reg_framework = framework->GetRegisterable();

    if (!reg_framework) {
      LOG("Specified framework is not registerable.");
      return false;
    }

    GtkSystemBrowseForFileHelper *helper =
        new GtkSystemBrowseForFileHelper(framework, gadget);

    reg_framework->RegisterMethod("BrowseForFile",
        NewSlot(helper, &GtkSystemBrowseForFileHelper::BrowseForFile));
    reg_framework->RegisterMethod("BrowseForFiles",
        NewSlot(helper, &GtkSystemBrowseForFileHelper::BrowseForFiles));

    ScriptableInterface *system = NULL;
    // Gets or adds system object.
    ResultVariant prop = framework->GetProperty("system");
    if (prop.v().type() != Variant::TYPE_SCRIPTABLE) {
      // property "system" is not available or have wrong type, then add one
      // with correct type.
      // Using SharedScriptable here, so that it can be destroyed correctly
      // when framework is destroyed.
      system = new SharedScriptable<UINT64_C(0xdf78c12fc974489c)>();
      reg_framework->RegisterVariantConstant("system", Variant(system));
    } else {
      system = VariantValue<ScriptableInterface *>()(prop.v());
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
