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

#ifndef GGADGET_TESTS_MOCKED_HOST_H__
#define GGADGET_TESTS_MOCKED_HOST_H__

#include "ggadget/host_interface.h"
#include "ggadget/view_host_interface.h"
#include "mocked_view_host.h"

class MockedHost : public ggadget::HostInterface {
 public:
  MockedHost() { }
  virtual ~MockedHost() { }
  virtual ggadget::ViewHostInterface *
      NewViewHost(ggadget::ViewHostInterface::Type type)
  { return new MockedViewHost(type); }
  virtual void RemoveGadget(int id, bool save_data) { }
  virtual void DebugOutput(DebugLevel level, const char *message) const { }
  virtual bool OpenURL(const char *url) const { return true; }

  virtual bool LoadFont(const char *filename) { return true; }
  virtual void ShowGadgetAboutDialog(ggadget::Gadget *gadget) { }
};

#endif // GGADGET_TESTS_MOCKED_HOST_H__
