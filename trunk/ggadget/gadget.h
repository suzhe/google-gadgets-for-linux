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

#ifndef GGADGET_GADGET_H__
#define GGADGET_GADGET_H__

#include <ggadget/common.h>
#include <ggadget/gadget_interface.h>

namespace ggadget {

class GadgetHostInterface;

class Gadget: public GadgetInterface {
 public:
  Gadget(GadgetHostInterface *host);
  virtual ~Gadget();

  virtual bool Init();
  virtual ViewHostInterface *GetMainViewHost();
  virtual const char *GetManifestInfo(const char *key) const;
  virtual bool HasOptionsDialog() const;
  virtual bool ShowOptionsDialog();
  virtual bool ShowDetailsView(DetailsView *details_view,
                               const char *title, int flags,
                               Slot1<void, int> *feedback_handler);
  virtual void CloseDetailsView();
  virtual void OnAddCustomMenuItems(MenuInterface *menu);
  virtual void OnCommand(Command command);
  virtual void OnDisplayStateChange(DisplayState display_state);
  virtual void OnDisplayTargetChange(DisplayTarget display_target);

 private:
  class Impl;
  Impl *impl_;
  DISALLOW_EVIL_CONSTRUCTORS(Gadget);
};

} // namespace ggadget

#endif // GGADGET_GADGET_H__
