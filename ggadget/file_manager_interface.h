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

#include <cstddef>
#include <string>

class TiXmlDocument;  // TinyXML DOM Document.

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
   *   - @c en_US/file,
   *   - @c en/file,
   *   - @c locale_id/file (for Windows compatibility, e.g. 2052/myfile),
   *   - @c 1033/file (for Windows compatibility).
   *
   * @param file the file name relative to the base path.
   * @param[out] data returns the file contents.
   * @return @c true if succeeded.
   */
  virtual bool GetFileContents(const char *file,
                               std::string *data) = 0;

  /**
   * Parses an XML file.
   * Entities defined in @c string.xml are replaced with localized strings.
   *
   * @param file the file name relative to the base path.
   * @return the parsed document if succeeded, or @c NULL on any error.
   *     The caller must delete the returned @c TiXmlDocument after use.
   */
  virtual TiXmlDocument *ParseXMLFile(const char *file) = 0;

};

} // namespace ggadget

#endif  // GGADGET_FILE_MANAGER_INTERFACE_H__
