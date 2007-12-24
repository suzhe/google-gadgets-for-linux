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

#ifndef GGADGET_MEMORY_OPTIONS_H__
#define GGADGET_MEMORY_OPTIONS_H__

#include <map>
#include <ggadget/common.h>
#include <ggadget/signals.h>
#include <ggadget/options_interface.h>

namespace ggadget {

class MemoryOptions : public OptionsInterface {
 public:
  MemoryOptions();
  virtual ~MemoryOptions();

  virtual Connection *ConnectOnOptionChanged(
      Slot1<void, const char *> *handler);
  virtual size_t GetCount();
  virtual void Add(const char *name, const Variant &value);
  virtual bool Exists(const char *name);
  virtual Variant GetDefaultValue(const char *name);
  virtual void PutDefaultValue(const char *name, const Variant &value);
  virtual Variant GetValue(const char *name);
  virtual void PutValue(const char *name, const Variant &value);
  virtual void Remove(const char *name);
  virtual void RemoveAll();

  virtual Variant GetInternalValue(const char *name);
  virtual void PutInternalValue(const char *name, const Variant &value);

 private:
  DISALLOW_EVIL_CONSTRUCTORS(MemoryOptions);
  class Impl;
  Impl *impl_;
};

} // namespace ggadget

#endif  // GGADGET_MEMORY_OPTIONS_H__
