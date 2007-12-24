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

#include "memory_options.h"
#include "logger.h"
#include "string_utils.h"

namespace ggadget {

class MemoryOptions::Impl {
 public:
  void FireChangedEvent(const char *name, const Variant &value) {
    DLOG("option %s changed to %s", name, value.Print().c_str());
    onoptionchanged_signal_(name);
  }

  typedef std::map<std::string, Variant, GadgetStringComparator> OptionsMap;
  OptionsMap values_;
  OptionsMap defaults_;
  OptionsMap internal_values_;
  Signal1<void, const char *> onoptionchanged_signal_;
};

MemoryOptions::MemoryOptions() : impl_(new Impl()) {
}

MemoryOptions::~MemoryOptions() {
  delete impl_;
}

Connection *MemoryOptions::ConnectOnOptionChanged(
    Slot1<void, const char *> *handler) {
  return impl_->onoptionchanged_signal_.Connect(handler);
}

size_t MemoryOptions::GetCount() {
  return impl_->values_.size();
}

void MemoryOptions::Add(const char *name, const Variant &value) {
  if (impl_->values_.find(name) == impl_->values_.end()) {
    impl_->values_[name] = value;
    impl_->FireChangedEvent(name, value);
  }
}

bool MemoryOptions::Exists(const char *name) {
  return impl_->values_.find(name) != impl_->values_.end();
}

Variant MemoryOptions::GetDefaultValue(const char *name) {
  Impl::OptionsMap::const_iterator it = impl_->defaults_.find(name);
  return it == impl_->defaults_.end() ? Variant() : it->second;
}

void MemoryOptions::PutDefaultValue(const char *name, const Variant &value) {
  impl_->defaults_[name] = value;
}

Variant MemoryOptions::GetValue(const char *name) {
  Impl::OptionsMap::const_iterator it = impl_->values_.find(name);
  return it == impl_->values_.end() ? GetDefaultValue(name) : it->second;
}

void MemoryOptions::PutValue(const char *name, const Variant &value) {
  impl_->values_[name] = value;
  impl_->FireChangedEvent(name, value);
}

void MemoryOptions::Remove(const char *name) {
  Impl::OptionsMap::iterator it = impl_->values_.find(name);
  if (it != impl_->values_.end()) {
    impl_->values_.erase(it);
    impl_->FireChangedEvent(name, Variant());
  }
}

void MemoryOptions::RemoveAll() {
  while (!impl_->values_.empty()) {
    Impl::OptionsMap::iterator it = impl_->values_.begin();
    std::string name(it->first);
    impl_->values_.erase(it);
    impl_->FireChangedEvent(name.c_str(), Variant());
  }
}

Variant MemoryOptions::GetInternalValue(const char *name) {
  Impl::OptionsMap::const_iterator it = impl_->internal_values_.find(name);
  return it == impl_->internal_values_.end() ? Variant() : it->second;
}

void MemoryOptions::PutInternalValue(const char *name, const Variant &value) {
  impl_->internal_values_[name] = value;
}

} // namespace ggadget
