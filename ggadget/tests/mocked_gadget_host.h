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
      ScriptRuntimeType type) {
    return NULL;
  }
  virtual ggadget::ElementFactoryInterface *GetElementFactory() {
    return NULL;
  }
  virtual ggadget::FileManagerInterface *GetGlobalFileManager() {
    return NULL;
  }  
  virtual ggadget::XMLHttpRequestInterface *NewXMLHttpRequest() {
    return NULL;
  }
  virtual ggadget::ViewHostInterface *NewViewHost(
      ViewType type, ggadget::ScriptableInterface *prototype,
      ggadget::OptionsInterface *options) {
    return NULL;
  }
  virtual void DebugOutput(DebugLevel level, const char *message) { }
  virtual uint64_t GetCurrentTime() const { return 0; }
  virtual int RegisterTimer(unsigned ms, TimerCallback *callback) { return 0; }
  virtual bool RemoveTimer(int token) { return true; }
  virtual int RegisterReadWatch(int fd, IOWatchCallback *callback) { return 0; }
  virtual int RegisterWriteWatch(int fd, IOWatchCallback *callback) {
    return 0;
  }
  virtual bool RemoveIOWatch(int token) { return true; }
  virtual bool OpenURL(const char *url) const { return true; }
};

#endif // GGADGET_TESTS_MOCKED_GADGET_HOST_H__
