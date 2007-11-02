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

namespace ggadget {

class OptionsInterface;

class ScriptableOptions : public ScriptableHelper<ScriptableInterface> {
 public:
  DEFINE_CLASS_ID(0x1a7bc9215ef74743, ScriptableInterface)

  explicit ScriptableOptions(OptionsInterface *options);
  virtual ~ScriptableOptions();

  virtual OwnershipPolicy Attach() { return NATIVE_PERMANENT; }

  OptionsInterface *GetOptions() const { return options_; }

 private:
  DISALLOW_EVIL_CONSTRUCTORS(ScriptableOptions);
  OptionsInterface *options_;
  class Impl;
  Impl *impl_;
};

} // namespace ggadget

#endif  // GGADGET_SCRIPTABLE_OPTIONS_H__
