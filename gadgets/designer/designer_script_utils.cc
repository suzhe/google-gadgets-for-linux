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

#include <cstring>
#include <string>
#include <vector>
#include <ggadget/basic_element.h> 
#include <ggadget/file_manager_factory.h>
#include <ggadget/file_manager_wrapper.h>
#include <ggadget/gadget.h>
#include <ggadget/gadget_consts.h>
#include <ggadget/host_interface.h>
#include <ggadget/logger.h>
#include <ggadget/scriptable_array.h>
#include <ggadget/scriptable_helper.h>
#include <ggadget/scriptable_menu.h>
#include <ggadget/scriptable_view.h>
#include <ggadget/script_context_interface.h>
#include <ggadget/string_utils.h>
#include <ggadget/system_utils.h>
#include <ggadget/view_interface.h>

namespace ggadget {
namespace designer {

static const char kGadgetFileManagerPrefix[] = "gadget://";
static FileManagerInterface *g_gadget_file_manager = NULL;
static FileManagerWrapper *g_designer_file_manager = NULL;

class ScriptableFileManager : public ScriptableHelperDefault {
 public:
  DEFINE_CLASS_ID(0x5a03aafca3094f1c, ScriptableInterface);

  ScriptableFileManager(FileManagerInterface *fm)
      : fm_(fm) {
  }

  virtual void DoRegister() {
    RegisterMethod("read",
                   NewSlot(this, &ScriptableFileManager::ReadFile));
    RegisterMethod("write",
                   NewSlot(fm_, &FileManagerInterface::WriteFile));
    RegisterMethod("remove",
                   NewSlot(fm_, &FileManagerInterface::RemoveFile));
    RegisterMethod("extract",
                   NewSlot(this, &ScriptableFileManager::ExtractFile));
    RegisterMethod("exists",
                   NewSlot(this, &ScriptableFileManager::FileExists));
    RegisterMethod("isDirectlyAccessible",
                   NewSlot(this, &ScriptableFileManager::IsDirectlyAccessible));
    RegisterMethod("getFullPath",
                   NewSlot(fm_, &FileManagerInterface::GetFullPath));
    RegisterMethod("getLastModifiedTime",
                   NewSlot(this, &ScriptableFileManager::GetLastModifiedTime));
    RegisterMethod("getAllFiles",
                   NewSlot(this, &ScriptableFileManager::GetAllFiles));
  }

  std::string ReadFile(const char *file) {
    std::string result;
    fm_->ReadFile(file, &result);
    return result;
  }

  std::string ExtractFile(const char *file) {
    std::string into_file;
    fm_->ExtractFile(file, &into_file);
    return into_file;
  }

  bool FileExists(const char *file) {
    return fm_->FileExists(file, NULL);
  }

  bool IsDirectlyAccessible(const char *file) {
    return fm_->IsDirectlyAccessible(file, NULL);
  }

  Date GetLastModifiedTime(const char *file) {
    return Date(fm_->GetLastModifiedTime(file));
  }

  static bool GetFile(const char *file, std::vector<std::string> *files) {
    files->push_back(file);
    return true;
  }

  ScriptableArray *GetAllFiles() {
    std::vector<std::string> files;
    fm_->EnumerateFiles("", NewSlot(GetFile, &files));
    return ScriptableArray::Create(files.begin(), files.size());
  }

  FileManagerInterface *fm_;
};

class DesignerUtils : public ScriptableHelperNativeOwnedDefault {
 public:
  DEFINE_CLASS_ID(0xd83de55b392c4d56, ScriptableInterface);

  virtual void DoRegister() {
    RegisterMethod("elementCoordToAncestor",
                   NewSlot(this, &DesignerUtils::ElementCoordToAncestor));
    RegisterMethod("ancestorCoordToElement",
                   NewSlot(this, &DesignerUtils::AncestorCoordToElement));
    RegisterMethod("isPointIn",
                   NewSlot(this, &DesignerUtils::IsPointIn));
    RegisterMethod("getOffsetPinX",
                   NewSlot(this, &DesignerUtils::GetOffsetPinX));
    RegisterMethod("getOffsetPinY",
                   NewSlot(this, &DesignerUtils::GetOffsetPinY));
    RegisterMethod("initGadgetFileManager",
                   NewSlot(this, &DesignerUtils::InitGadgetFileManager));
    RegisterMethod("getGlobalFileManager",
                   NewSlot(this, &DesignerUtils::GetGlobalFileManager));
    RegisterMethod("connectViewMenuHandler",
                   NewSlot(this, &DesignerUtils::ConnectViewMenuHandler));
    RegisterMethod("connectElementMenuHandler",
                   NewSlot(this, &DesignerUtils::ConnectElementMenuHandler));
    RegisterMethod("showXMLOptionsDialog",
                   NewSlot(this, &DesignerUtils::ShowXMLOptionsDialog));
    RegisterMethod("setDesignerMode",
                   NewSlot(this, &DesignerUtils::SetDesignerMode));
    RegisterMethod("systemOpenFile",
                   NewSlot(this, &DesignerUtils::SystemOpenFile));
    RegisterMethod("systemOpenFileWith",
                   NewSlot(this, &DesignerUtils::SystemOpenFileWith));
  }

  JSONString ElementCoordToAncestor(const BasicElement *element,
                                    const BasicElement *ancestor,
                                    double x, double y) {
    while (element && element != ancestor) {
      element->SelfCoordToParentCoord(x, y, &x, &y);
      element = element->GetParentElement();
    }
    return JSONString(StringPrintf("{\"x\":%f,\"y\":%f}", x, y));
  }

  JSONString AncestorCoordToElement(const BasicElement *ancestor,
                                    const BasicElement *element,
                                    double x, double y) {
    std::vector<const BasicElement *> path;
    while (element && element != ancestor) {
      path.push_back(element);
      element = element->GetParentElement();
    }
    for (size_t i = path.size(); i > 0; i--) {
      path[i - 1]->ParentCoordToSelfCoord(x, y, &x, &y);
    }
    return JSONString(StringPrintf("{\"x\":%f,\"y\":%f}", x, y));
  }

  bool IsPointIn(const BasicElement *element, double x, double y) {
    return element->IsPointIn(x, y);
  }

  double GetOffsetPinX(const BasicElement *element) {
    return element->GetPixelPinX();
  }

  double GetOffsetPinY(const BasicElement *element) {
    return element->GetPixelPinY();
  }

  ScriptableFileManager *InitGadgetFileManager(const char *gadget_path) {
    if (g_gadget_file_manager) {
      g_designer_file_manager->UnregisterFileManager(kGadgetFileManagerPrefix,
                                                     g_gadget_file_manager);
      delete g_gadget_file_manager;
    }
    std::string path, filename;
    SplitFilePath(gadget_path, &path, &filename);

    // Uses the parent path of base_path if it refers to a manifest file.
    if (filename != kGadgetGManifest)
      path = gadget_path;

    g_gadget_file_manager = CreateFileManager(path.c_str());
    if (g_gadget_file_manager) {
      g_designer_file_manager->RegisterFileManager(kGadgetFileManagerPrefix,
                                                   g_gadget_file_manager);
      return new ScriptableFileManager(g_gadget_file_manager);
    }
    return NULL;
  }

  ScriptableFileManager *GetGlobalFileManager() {
    return new ScriptableFileManager(::ggadget::GetGlobalFileManager());
  }

  static bool ProxyMenuHandler(MenuInterface *menu, Slot *handler) {
    ScriptableMenu scriptable_menu(menu);
    Variant arg(&scriptable_menu);
    return VariantValue<bool>()(handler->Call(NULL, 1, &arg).v());
  }

  void ConnectViewMenuHandler(ScriptableView *view, Slot *handler) {
    view->view()->ConnectOnAddContextMenuItems(NewSlot(ProxyMenuHandler,
                                                       handler));
  }

  void ConnectElementMenuHandler(BasicElement *element, Slot *handler) {
    element->ConnectOnAddContextMenuItems(NewSlot(ProxyMenuHandler, handler));
  }

  void ShowXMLOptionsDialog(ScriptableView *view, const char *xml_file,
                            ScriptableInterface *param) {
    view->view()->GetGadget()->ShowXMLOptionsDialog(
        ViewInterface::OPTIONS_VIEW_FLAG_OK |
        ViewInterface::OPTIONS_VIEW_FLAG_CANCEL,
        xml_file, param);
  }

  void SetDesignerMode(BasicElement *element) {
    element->SetDesignerMode(true);
  }

  void SystemOpenFile(ScriptableView *view, const char *file) {
    std::string url = "file://";
    url += file;
    view->view()->GetGadget()->GetHost()->OpenURL(url.c_str());
  }

  void SystemOpenFileWith(const char *command, const char *file) {
    pid_t pid;
    // fork and run the command.
    if ((pid = fork()) == 0) {
      if (fork() != 0)
        _exit(0);
      execlp(command, command, file, NULL);
      DLOG("Failed to exec command: %s", command);
      _exit(-1);
    }
  }
};

static DesignerUtils g_designer_utils;

} // namespace designer
} // namespace ggadget

#define Initialize designer_script_utils_LTX_Initialize
#define Finalize designer_script_utils_LTX_Finalize
#define RegisterScriptExtension \
    designer_script_utils_LTX_RegisterScriptExtension
#define RegisterFileManagerExtension \
    designer_script_utils_LTX_RegisterFileManagerExtension

extern "C" {
  bool Initialize() {
    LOGI("Initialize designer_script_utils extension.");
    return true;
  }

  void Finalize() {
    LOGI("Finalize designer_script_utils extension.");
  }

  bool RegisterScriptExtension(ggadget::ScriptContextInterface *context) {
    ASSERT(context);
    if (context) {
      if (!context->AssignFromNative(
          NULL, NULL, "designerUtils",
          ggadget::Variant(&ggadget::designer::g_designer_utils))) {
        LOG("Failed to register designerUtils.");
        return false;
      }
      return true;
    }
    return false;
  }

  bool RegisterFileManagerExtension(ggadget::FileManagerWrapper *fm) {
    LOGI("Register designer_script_utils file manager extension.");
    ASSERT(fm);
    ggadget::designer::g_designer_file_manager = fm;
    return true;
  }

}

