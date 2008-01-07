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

#ifndef GGADGET_FILE_MANAGER_INTERFACE_H__
#define GGADGET_FILE_MANAGER_INTERFACE_H__

#include <string>
#include "string_utils.h"

namespace ggadget {

/**
 * Handles all file resources and file access used by a gadget.
 */
class FileManagerInterface {
 public:
  virtual ~FileManagerInterface() { }

  /**
   * Initialize the @c FileManagerInterface instance.
   *
   * A @c FileManager instance must be initialized before use.
   * @param base_path the base path of this @c FileManager.  All file names
   *     in subsequent operations are relative to this base path.
   * @return @c true if succeeded.
   */
  virtual bool Init(const char *base_path) = 0;

  /**
   * Gets the raw contents in a file without translating it.
   * The file is searched in the paths in the following order:
   *   - @c file (in the @c base_path),
   *   - @c lang_TERRITORY/file (e.g. @c zh_CN/myfile),
   *   - @c lang/file (e.g. @c zh/myfile),
   *   - @c locale_id/file (for Windows compatibility, e.g. 2052/myfile),
   *   - @c en_US/file,
   *   - @c en/file,
   *   - @c 1033/file (for Windows compatibility).
   *
   * @param file the file name relative to the base path or an absolute
   *     filename in the file system.
   * @param[out] data returns the file contents.
   * @param[out] path the actual path name of the file, for logging only.
   * @return @c true if succeeded.
   */
  virtual bool GetFileContents(const char *file,
                               std::string *data,
                               std::string *path) = 0;

  /**
   * Gets the contents of an XML file. The file is searched in the same
   * sequence as in @c GetFileContents().  Entities defined in @c strings.xml
   * are replaced with localized strings.
   *
   * @param file the file name relative to the base path.
   * @param[out] data returns the file contents.
   * @param[out] path the actual path name of the file, for logging only.
   * @return @c true if succeeded.
   */
  virtual bool GetXMLFileContents(const char *file,
                                  std::string *data,
                                  std::string *path) = 0;

  /**
   * Extracts the contents of a file into a given file name or into a
   * temporary file.
   *
   * @param file the file name relative to the base path, or an absolute
   *     filename in the file system.
   * @param[in, out] into_file if the input value is empty, the file manager
   *     generates a unique temporary file name and save the contents into
   *     that file and return the filename in this parameter on return;
   *     Otherwise the file manager will save into the given file name.
   * @return @c true if succeeded.
   */
  virtual bool ExtractFile(const char *file, std::string *into_file) = 0;

  /**
   * Gets the content of strings.xml by returning a string map.
   */
  virtual const GadgetStringMap *GetStringTable() const = 0;

  /**
   * Check if a file with the given name exists under the base_path of this
   * file manager. Returns @c false if the filename is absolute.
   */
  virtual bool FileExists(const char *file) = 0;

};

} // namespace ggadget

#endif  // GGADGET_FILE_MANAGER_INTERFACE_H__
