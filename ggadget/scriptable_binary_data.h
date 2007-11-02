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

#ifndef GGADGETS_SCRIPTABLE_BINARY_DATA_H__
#define GGADGETS_SCRIPTABLE_BINARY_DATA_H__

#include <string>
#include "scriptable_helper.h"

namespace ggadget {

/**
 * This class is used to transfer native binary data opaquely through
 * script code. It doesn't expose any property or method to script.
 * Its ownership policy is transferrable, that is, the script engine owns
 * it when it is passed from native code to the script engine.
 */
class ScriptableBinaryData : public ScriptableHelper<ScriptableInterface> {
 public:
  DEFINE_CLASS_ID(0x381e0cd617734500, ScriptableInterface);

  ScriptableBinaryData(const char *data, size_t size)
      : data_(data ? data : "", data ? size : 0) { }

  explicit ScriptableBinaryData(const std::string &data)
      : data_(data) { }

  ScriptableBinaryData(const ScriptableBinaryData &data)
      : data_(data.data_) { }

  const char *data() { return data_.c_str(); }
  size_t size() { return data_.size(); }

  ScriptableBinaryData &operator =(const ScriptableBinaryData &data) {
    data_ = data.data_;
    return *this;
  }

  virtual OwnershipPolicy Attach() { return OWNERSHIP_TRANSFERRABLE; }
  virtual bool Detach() { delete this; return true; }

 private:
  std::string data_;
};

} // namespace ggadget

#endif // GGADGETS_SCRIPTABLE_BINARY_DATA_H__
