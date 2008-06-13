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

#ifndef HOSTS_QT_HOST_H__
#define HOSTS_QT_HOST_H__

#include <string>
#include <ggadget/host_interface.h>
#include <ggadget/gadget_manager_interface.h>
#include <ggadget/decorated_view_host.h>

namespace hosts {
namespace qt {

using namespace ggadget;
using ggadget::Gadget;
using ggadget::HostInterface;
using ggadget::ViewHostInterface;;
using ggadget::ViewInterface;

class QtHost : public ggadget::HostInterface {
 public:
  QtHost(int view_debug_mode);
  virtual ~QtHost();
  virtual ViewHostInterface *NewViewHost(Gadget *gadget,
                                         ViewHostInterface::Type type);
  virtual void RemoveGadget(Gadget *gadget, bool save_data);
  virtual void DebugOutput(DebugLevel level, const char *message) const;
  virtual bool OpenURL(const char *url) const;
  virtual bool LoadFont(const char *filename);
  virtual void Run();
  virtual void ShowGadgetAboutDialog(Gadget *gadget);

 private:
  class Impl;
  Impl *impl_;
};

} // namespace qt
} // namespace hosts

#endif // HOSTS_QT_HOST_H__
