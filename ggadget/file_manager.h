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

#ifndef GGADGET_FILE_MANAGER_H__
#define GGADGET_FILE_MANAGER_H__

#include <cstddef>
#include <string>
#include "common.h"
#include "file_manager_interface.h"

class TiXmlDocument;  // TinyXML DOM Document.

namespace ggadget {

namespace internal {
class FileManagerImpl;
}

/**
 * Handles all file resources and file access used by a gadget.
 * This is a single-use container for file objects; once initialized,
 * it should not be reused with a different base path.
 */
class FileManager : public FileManagerInterface {
 public:
  FileManager();
  ~FileManager();

  /** @see FileManagerInterface::Init() */
  virtual bool Init(const char *base_path);
  /** @see FileManagerInterface::GetFileContents() */
  virtual bool GetFileContents(const char *file,
                               std::string *data);
  /** @see FileManagerInterface::ParseXMLFile() */
  virtual TiXmlDocument *ParseXMLFile(const char *file);

 private:
  internal::FileManagerImpl *impl_;
  DISALLOW_EVIL_CONSTRUCTORS(FileManager);
};

} // namespace ggadget

#endif  // GGADGET_FILE_MANAGER_H__
