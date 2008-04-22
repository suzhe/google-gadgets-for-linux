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

#ifndef GGADGET_SIDEBAR_H__
#define GGADGET_SIDEBAR_H__

#include <ggadget/basic_element.h>
#include <ggadget/view_host_interface.h>

namespace ggadget {

template <typename R> class Slot0;
class HostInterface;
class ViewElement;
class CanvasInterface;

class SideBar {
 public:
  SideBar(HostInterface *host, ViewHostInterface *view_host);
  virtual ~SideBar();
 public:
  ViewHostInterface *NewViewHost(ViewHostInterface::Type type);
  ViewHostInterface *GetViewHost() const;
  void InsertNullElement(int y, View *view);
  void ClearNullElement();
  bool Dock(int insert_point, View *view, bool force_insert);
  bool Undock(View *view);
  void Expand(View *view);
  void Unexpand(View *view);

  Gadget *GetMouseOverGadget() const;
  double GetGadgetHeight(const Gadget *gadget) const;

  void SetAddGadgetSlot(Slot0<void> *slot);
  void SetMenuSlot(Slot0<void> *slot);
  void SetCloseSlot(Slot0<void> *slot);
 private:
  class Impl;
  Impl *impl_;
};

}  // namespace ggadget

#endif  // GGADGET_SIDEBAR_H__
