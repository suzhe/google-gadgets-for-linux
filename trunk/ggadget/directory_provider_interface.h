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

#ifndef GGADGET_DIRECTORY_PROVIDER_INTERFACE_H__
#define GGADGET_DIRECTORY_PROVIDER_INTERFACE_H__

#include <string>

namespace ggadget {

/**
 * Simulates the Microsoft IFileSystem3 interface.
 * Used for framework.filesystem.
 *
 * NOTE: if a method returns <code>const char *</code>, the pointer must be
 * used transiently or made a copy. The pointer may become invalid after
 * another call to a method also returns <code>const char *</code> in some
 * implementations.
 */
class DirectoryProviderInterface {
 protected:
  virtual ~DirectoryProviderInterface() { }

 public:
  /** Gets the directory where the user local data resides. */
  virtual std::string GetProfileDirectory() = 0;

  /** Gets the directory where readonly resources resides. */
  virtual std::string GetResourceDirectory() = 0;
};

/**
 * Sets a directory provider as the global provider, which can be used by any
 * components.
 *
 * This function must be called in main program at very beginning, and can only
 * be called once.
 */
bool SetDirectoryProvider(DirectoryProviderInterface *directory_provider);

/**
 * Gets the global directory provider set by calling SetDirectoryProvider()
 * previously.
 */
DirectoryProviderInterface *GetDirectoryProvider();

} // namespace ggadget

#endif // GGADGET_DIRECTORY_PROVIDER_INTERFACE_H__
