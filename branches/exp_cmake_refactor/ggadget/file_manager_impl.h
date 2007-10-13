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

#ifndef GGADGET_FILE_MANAGER_IMPL_H__
#define GGADGET_FILE_MANAGER_IMPL_H__

#include <cstddef>
#include <string>
#include <cstring>
#include <map>

#include "string_utils.h"
#include "third_party/unzip/unzip.h"

class TiXmlDocument;  // TinyXML DOM Document.

namespace ggadget {

namespace internal {

class Gadget;

class FileManagerImpl {
 public:
  FileManagerImpl() { }
  ~FileManagerImpl();

  typedef std::map<std::string, unz_file_pos, GadgetStringComparator> FileMap;

  bool Init(const char *base_path);
  bool GetFileContents(const char *file,
                       std::string *data,
                       std::string *path);
  bool GetXMLFileContents(const char *file,
                          std::string *data,
                          std::string *path);
  bool ExtractFile(const char *file, std::string *into_file);

  bool InitLocaleStrings();
  static void SplitPathFilename(const char *input_path,
                                std::string *path,
                                std::string *filename);

  bool LoadStringTable(const char *stringfile);

  // Generate files_ recursively starting at dir_path.
  bool ScanDirFilenames(const char *dir_path);
  // Generate files_ based on the directory info of a zip file.
  bool ScanZipFilenames();

  // This function will not check file in the current base_path directory.
  // Returns iterator to entry in files_. returns files_.end() if not found.
  FileMap::const_iterator FindLocalizedFile(const char *file) const;

  bool GetFileContentsInternal(FileMap::const_iterator iter, std::string *data);
  bool GetDirFileContents(FileMap::const_iterator iter, std::string *data);
  bool GetZipFileContents(FileMap::const_iterator iter, std::string *data);

  // base path must in correct case (case sensitive),
  // but files in base path need not to be
  std::string base_path_;
  bool is_dir_;
  std::string locale_prefix_;
  std::string locale_lang_prefix_;
  std::string locale_id_prefix_;

  // Map filenames to data in a zip file.
  // Also used as cache for files if base_path is a dir.
  FileMap files_;

  // Maps resource names to string resources from strings.xml.
  GadgetStringMap string_table_;
};

} // namespace internal

} // namespace ggadget

#endif  // GGADGET_FILE_MANAGER_IMPL_H__