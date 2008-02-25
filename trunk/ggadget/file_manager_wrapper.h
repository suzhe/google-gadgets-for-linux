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

#ifndef GGADGET_FILE_MANAGER_WRAPPER_H__
#define GGADGET_FILE_MANAGER_WRAPPER_H__

#include <string>

#include "file_manager.h"
#include "string_utils.h"

namespace ggadget {

class XMLParserInterface;

/**
 * Parses filemanager paths and dispatches commands to the appropiate
 * filemanager.
 */
class FileManagerWrapper : public FileManagerInterface {
 public:
  FileManagerWrapper(XMLParserInterface *xml_parser);

  virtual ~FileManagerWrapper();

  /**
   * Registers a new file manager and an associated prefix to be handled
   * by this wrapper. The prefixes associated with this wrapper do not have to
   * be unique. If a prefix is shared by more than one filemanager,
   * the filemanagers will be called in the order in which they were registered.
   * The filemanagers must already be initialized before registration. The
   * filemanager wrapper will not free those filemanagers.
   *
   * Note: One registered prefix must not be the prefix of another (e.g. "abc"
   * and "abcdef"). The "" prefix is reserved for use by the default gadget
   * filemanager.
   */
  bool RegisterFileManager(const char *prefix, FileManagerInterface *fm);

  /**
   * Init() will initialize the filemanager wrapper with a default filemanager
   * associated with a given gadget basepath and the "" prefix.
   */
  virtual bool Init(const char *base_path);

  virtual bool GetFileContents(const char *file,
                               std::string *data,
                               std::string *path);

  virtual bool GetXMLFileContents(const char *file,
                                  std::string *data,
                                  std::string *path);

  virtual bool ExtractFile(const char *file, std::string *into_file);

  virtual const GadgetStringMap *GetStringTable() const;

  virtual bool FileExists(const char *file, std::string *path);

 private:
  DISALLOW_EVIL_CONSTRUCTORS(FileManagerWrapper);

  class Impl;
  Impl *impl_;
};

} // namespace ggadget

#endif  // GGADGET_FILE_MANAGER_WRAPPER_H__
