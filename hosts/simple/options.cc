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

#include "options.h"

Options::Options() {
}

Options::~Options() {
}

size_t Options::GetCount() {
  return values_.size();
}

void Options::Add(const char *name, const Variant &value) {
  if (values_.find(name) == values_.end()) {
    values_[name] = value;
    FireChangedEvent(name);
  }
}

bool Options::Exists(const char *name) {
  return values_.find(name) != values_.end();
}
  
Variant Options::GetDefaultValue(const char *name) {
  OptionsMap::const_iterator it = defaults_.find(name);
  return it == defaults_.end() ? Variant() : it->second;
}

void Options::PutDefaultValue(const char *name, const Variant &value) {
  defaults_[name] = value;
}

Variant Options::GetValue(const char *name) {
  OptionsMap::const_iterator it = values_.find(name);
  return it == values_.end() ? GetDefaultValue(name) : it->second;
}

void Options::PutValue(const char *name, const Variant &value) {
  values_[name] = value;
  FireChangedEvent(name);
}

void Options::Remove(const char *name) {
  OptionsMap::iterator it = values_.find(name);
  if (it != values_.end()) {
    values_.erase(it);
    FireChangedEvent(name);
  }
}

void Options::RemoveAll() {
  while (!values_.empty()) {
    OptionsMap::iterator it = values_.begin();
    std::string name(it->first);
    values_.erase(it);
    FireChangedEvent(name.c_str());
  }
}

void Options::FireChangedEvent(const char *name) {
  DLOG("option changed: %s", name);
}
