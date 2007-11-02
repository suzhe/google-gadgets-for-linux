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

#ifndef HOSTS_SIMPLE_SIMPLEHOST_FILE_MANAGER_H__
#define HOSTS_SIMPLE_SIMPLEHOST_FILE_MANAGER_H__

#include <cstddef>
#include <string>
#include "ggadget/common.h"
#include "ggadget/file_manager_interface.h"

/**
 * Handles all global file resources (i.e. resources not belonging to a 
 * gadget package).
 */
class SimpleHostFileManager : public ggadget::FileManagerInterface {
 public:
  SimpleHostFileManager();
  ~SimpleHostFileManager();

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
  virtual ggadget::GadgetStringMap *GetStringTable();

 private:
  DISALLOW_EVIL_CONSTRUCTORS(SimpleHostFileManager);
};

#endif  // HOSTS_SIMPLE_SIMPLEHOST_FILE_MANAGER_H__
