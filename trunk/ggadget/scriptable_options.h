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

#ifndef GGADGET_SCRIPTABLE_OPTIONS_H__
#define GGADGET_SCRIPTABLE_OPTIONS_H__

#include "scriptable_helper.h"
#include "scriptable_interface.h"

namespace ggadget {

class OptionsInterface;

class ScriptableOptions : public ScriptableInterface {
 public:
  DEFINE_CLASS_ID(0x1a7bc9215ef74743, ScriptableInterface)

  explicit ScriptableOptions(OptionsInterface *options); 
  virtual ~ScriptableOptions();

  DEFAULT_OWNERSHIP_POLICY
  DELEGATE_SCRIPTABLE_INTERFACE(scriptable_helper_)
  virtual bool IsStrict() const { return true; }

  OptionsInterface *GetOptions() const { return options_; }

 private:
  DISALLOW_EVIL_CONSTRUCTORS(ScriptableOptions);
  DELEGATE_SCRIPTABLE_REGISTER(scriptable_helper_)
  ScriptableHelper scriptable_helper_;
  OptionsInterface *options_;
};

} // namespace ggadget

#endif  // GGADGET_SCRIPTABLE_OPTIONS_H__
