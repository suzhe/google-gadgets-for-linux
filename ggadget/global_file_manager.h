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

#ifndef GGADGET_GTK_GLOBAL_FILE_MANAGER_H__
#define GGADGET_GTK_GLOBAL_FILE_MANAGER_H__

#include <string>
#include <ggadget/common.h>
#include <ggadget/file_manager_interface.h>
#include <ggadget/string_utils.h>

namespace ggadget {

/**
 * Handles global file resources on disk.
 */
class GlobalFileManager : public FileManagerInterface {
 public:
  GlobalFileManager();
  ~GlobalFileManager();

  /** @see FileManagerInterface::Init() */
  virtual bool Init(const char *base_path);
  /** @see FileManagerInterface::GetFileContents() */
  virtual bool GetFileContents(const char *file,
                               std::string *data,
                               std::string *path);
  /** @see FileManagerInterface::GetXMLFileContents() */
  virtual bool GetXMLFileContents(const char *file,
                                  std::string *data,
                                  std::string *path);
  /** @see FileManagerInterface::ExtractFile() */
  virtual bool ExtractFile(const char *file, std::string *into_file);
  /** @see FileManagerInterface::GetStringTable() */
  virtual const GadgetStringMap *GetStringTable() const;
  /** @see FileManagerInterface::FileExists() */
  virtual bool FileExists(const char *file, std::string *path);

 private:
  std::string base_path_;

  DISALLOW_EVIL_CONSTRUCTORS(GlobalFileManager);
};

} // namespace ggadget

#endif  // GGADGET_GTK_GLOBAL_FILE_MANAGER_H__
