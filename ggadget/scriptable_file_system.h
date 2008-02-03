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

#ifndef GGADGET_SCRIPTABLE_FILE_SYSTEM_H__
#define GGADGET_SCRIPTABLE_FILE_SYSTEM_H__

#include <ggadget/scriptable_helper.h>

namespace ggadget {

namespace framework {
class FileSystemInterface;

class ScriptableFileSystem : public ScriptableHelperNativeOwnedDefault {
 public:
  DEFINE_CLASS_ID(0x881b7d66c6bf4ca5, ScriptableInterface)

  explicit ScriptableFileSystem(FileSystemInterface *filesystem);
  virtual ~ScriptableFileSystem();

 private:
  DISALLOW_EVIL_CONSTRUCTORS(ScriptableFileSystem);
  class Impl;
  Impl *impl_;
};

} // namespace framework
} // namespace ggadget

#endif  // GGADGET_SCRIPTABLE_FILE_SYSTEM_H__
