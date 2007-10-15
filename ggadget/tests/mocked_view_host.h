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

#ifndef GGADGET_TESTS_MOCKED_VIEW_HOST_H__
#define GGADGET_TESTS_MOCKED_VIEW_HOST_H__

#include "ggadget/view_host_interface.h"

class MockedViewHost : public ggadget::ViewHostInterface {
 public:
  virtual ggadget::GadgetHostInterface *GetGadgetHost() const { return NULL; }
  virtual ggadget::ViewInterface *GetView() { return NULL; }
  virtual const ggadget::ViewInterface *GetView() const { return NULL; }
  virtual ggadget::ScriptContextInterface *GetScriptContext() const {
    return NULL;
  }
  virtual const ggadget::GraphicsInterface *GetGraphics() const { return NULL; }
  virtual void QueueDraw() { }
  virtual bool GrabKeyboardFocus() { return false; }
  virtual void SetResizeable() { }
  virtual void SetCaption(const char *caption) { }
  virtual void SetShowCaptionAlways(bool always) { }
};

#endif // GGADGET_TESTS_MOCKED_VIEW_HOST_H__
