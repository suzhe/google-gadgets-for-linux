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

#ifndef GGADGET_WINDOWS_LOCALES_H__
#define GGADGET_WINDOWS_LOCALES_H__

#include <string>

namespace ggadget {

/**
 * Gets the corresponding Windows locale identifier for a given locale name.
 *
 * This function is only for historical compatibility. Google desktop gadget
 * library for Windows uses Windows locale identifiers as the names of
 * subdirectories containing localized resources.
 *
 * @param name locale name in format of "lang_TERRITORY", such as "zh_CN".
 * @param[out] id the corresponding Windows locale identifer.
 * @return @c true if @a id found.
 */
bool GetLocaleIDString(const char *name, std::string *id);

} // namespace ggadget

#endif // GGADGET_WINDOWS_LOCALES_H__
