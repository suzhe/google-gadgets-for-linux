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

#ifndef GGADGET_ENCRYPTOR_H__
#define GGADGET_ENCRYPTOR_H__

#include <ggadget/encryptor_interface.h>

namespace ggadget {

/**
 * Gets the EncryptorInterface instance.
 *
 * The returned instance is a singleton provided by a Encryptor
 * extension module, which is loaded into the global ExtensionManager in
 * advance; or a default simple EncryptorInterface instance if no such
 * Encryptor extension module exists.
 */
EncryptorInterface *GetEncryptor();

} // namespace ggadget

#endif // GGADGET_ENCRYPTOR_H__
