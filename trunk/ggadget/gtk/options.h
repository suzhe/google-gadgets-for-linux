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

#ifndef GGADGET_GTK_OPTIONS_H__
#define GGADGET_GTK_OPTIONS_H__

#include <map>
#include <ggadget/ggadget.h>

namespace ggadget {
namespace gtk {

class Options : public ggadget::OptionsInterface {
 public:
  Options();
  virtual ~Options();

  virtual ggadget::Connection *ConnectOnOptionChanged(
      ggadget::Slot1<void, const char *> *handler);
  virtual size_t GetCount();
  virtual void Add(const char *name, const ggadget::Variant &value);
  virtual bool Exists(const char *name);
  virtual ggadget::Variant GetDefaultValue(const char *name);
  virtual void PutDefaultValue(const char *name, const ggadget::Variant &value);
  virtual ggadget::Variant GetValue(const char *name);
  virtual void PutValue(const char *name, const ggadget::Variant &value);
  virtual void Remove(const char *name);
  virtual void RemoveAll();

  virtual ggadget::Variant GetInternalValue(const char *name);
  virtual void PutInternalValue(const char *name,
                                const ggadget::Variant &value);

 private:
  DISALLOW_EVIL_CONSTRUCTORS(Options);

  void FireChangedEvent(const char *name);
  typedef std::map<std::string, ggadget::Variant,
                   ggadget::GadgetStringComparator> OptionsMap;
  OptionsMap values_;
  OptionsMap defaults_;
  OptionsMap internal_values_;
  ggadget::Signal1<void, const char *> onoptionchanged_signal_;
};

} // namespace gtk
} // namespace ggadget

#endif  // GGADGET_GTK_OPTIONS_H__
