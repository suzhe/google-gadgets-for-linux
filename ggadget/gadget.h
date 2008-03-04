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
#include <ggadget/string_utils.h>

namespace ggadget {

class GadgetHostInterface;

class Gadget: public GadgetInterface {
 public:
  /**
   * @param host the host of this gadget.
   * @param base_path the base path of this gadget. It can be a directory,
   *     path to a .gg file, or path to a gadget.gmanifest file.
   * @param debug_mode 0: no debug; 1: debugs container elements by drawing
   *     a bounding box for each container element; 2: debugs all elements.
   */
  Gadget(GadgetHostInterface *host, const char *base_path, int debug_mode);
  virtual ~Gadget();

  virtual bool Init();
  virtual ViewHostInterface *GetMainViewHost();
  virtual FileManagerInterface *GetFileManager();
  virtual std::string GetManifestInfo(const char *key) const;
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

  /**
   * A utility to get the manifest infomation of a gadget without
   * constructing a Gadget object.
   * @param base_path see document for Gadget constructor.
   * @param[out] data receive the manifest data.
   * @return @c true if succeeds.
   */
  static bool GetGadgetManifest(const char *base_path, GadgetStringMap *data);

 private:
  class Impl;
  Impl *impl_;
  DISALLOW_EVIL_CONSTRUCTORS(Gadget);
};

} // namespace ggadget

#endif // GGADGET_GADGET_H__
