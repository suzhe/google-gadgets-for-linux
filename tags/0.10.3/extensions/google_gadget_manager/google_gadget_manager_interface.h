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

#ifndef GGADGET_GOOGLE_GADGET_MANAGER_INTERFACE_H__
#define GGADGET_GOOGLE_GADGET_MANAGER_INTERFACE_H__

#include <ggadget/gadget_manager_interface.h>

namespace ggadget {

class ScriptContextInterface;

namespace google {

const char kGoogleGadgetManagerTag[] = "GoogleGadgetManager";

/**
 * This interface is for the local extension module in the gadget browser
 * gadget to access google specific gadget manager functionalities.
 */
class GoogleGadgetManagerInterface : public GadgetManagerInterface {
 protected:
  virtual ~GoogleGadgetManagerInterface() { }

 public:
  /**
   * Registers "gadgetBrowserUtils" object into the script context.
   */
  virtual bool RegisterGadgetBrowserScriptUtils(
      ScriptContextInterface *script_context) = 0;
};

} // namespace google
} // namespace ggadget

#endif // GGADGET_GOOGLE_GADGET_MANAGER_INTERFACE_H__
