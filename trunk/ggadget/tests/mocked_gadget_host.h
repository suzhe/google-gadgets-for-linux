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

#ifndef GGADGET_TESTS_MOCKED_GADGET_HOST_H__
#define GGADGET_TESTS_MOCKED_GADGET_HOST_H__

#include "ggadget/gadget_host_interface.h"

class MockedGadgetHost : public ggadget::GadgetHostInterface {
 public:
  virtual ggadget::ScriptRuntimeInterface *GetScriptRuntime(
      ScriptRuntimeType type) { return NULL; }
  virtual ggadget::ElementFactoryInterface *GetElementFactory() {
    return NULL;
  }
  virtual ggadget::FileManagerInterface *GetFileManager() { return NULL; }
  virtual ggadget::FileManagerInterface *GetGlobalFileManager() { return NULL; }
  virtual ggadget::FrameworkInterface *GetFramework() { return NULL; }
  virtual ggadget::OptionsInterface *GetOptions() { return NULL; }
  virtual ggadget::GadgetInterface *GetGadget() { return NULL; }
  virtual ggadget::ViewHostInterface *NewViewHost(
      ViewType type, ggadget::ScriptableInterface *prototype) { return NULL; }
  virtual void SetPluginFlags(int plugin_flags) { }
  virtual void RemoveMe(bool save_data) { }
  virtual void ShowDetailsView(ggadget::DetailsViewInterface *details_view,
                               const char *title, int flags,
                               ggadget::Slot1<void, int> *feedback_handler) { }
  virtual void CloseDetailsView() { }
  virtual void DebugOutput(DebugLevel level, const char *message) const { }
  virtual uint64_t GetCurrentTime() const { return 0; }
  virtual int RegisterTimer(unsigned ms, TimerCallback *callback) { return 0; }
  virtual bool RemoveTimer(int token) { return true; }
  virtual int RegisterReadWatch(int fd, IOWatchCallback *callback) { return 0; }
  virtual int RegisterWriteWatch(int fd, IOWatchCallback *callback) {
    return 0;
  }
  virtual bool RemoveIOWatch(int token) { return true; }
  virtual bool OpenURL(const char *url) const { return true; }

  virtual bool LoadFont(const char *filename) { return true; }
  virtual bool UnloadFont(const char *filename) { return true; }
  virtual const char *BrowseForFile(const char *filter) { return ""; }
  virtual ggadget::GadgetHostInterface::FilesInterface *BrowseForFiles(
      const char *filter) { return NULL; }
  virtual void GetCursorPos(int *x, int *y) const { }
  virtual void GetScreenSize(int *width, int *height) const { }
  virtual const char *GetFileIcon(const char *filename) const { return ""; }
  virtual ggadget::AudioclipInterface *CreateAudioclip(const char *filename) {
    return NULL;
  }
};

#endif // GGADGET_TESTS_MOCKED_GADGET_HOST_H__
