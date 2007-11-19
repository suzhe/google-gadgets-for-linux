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
#include <cstring>
#include <string>
#include <ggadget/common.h>
#include <ggadget/file_manager_interface.h>
#include <ggadget/string_utils.h>
#include <third_party/unzip/unzip.h>

class TiXmlDocument;  // TinyXML DOM Document.

namespace ggadget {

namespace internal {

/** Declared here only for unit testing. */
class FileManagerImpl {
 public:
  FileManagerImpl(FileManagerInterface *global_file_manager);
  ~FileManagerImpl();

  typedef std::map<std::string, unz_file_pos, GadgetStringComparator> FileMap;

  bool Init(const char *base_path);
  bool GetFileContents(const char *file, std::string *data, std::string *path);
  bool GetXMLFileContents(const char *file,
                          std::string *data, std::string *path);
  bool ExtractFile(const char *file, std::string *into_file);
  bool FileExists(const char *file);

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
  FileMap::const_iterator FindFile(const char *file,
                                   std::string *normalized_file);

  FileManagerInterface *global_file_manager_;
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

/**
 * Handles all file resources and file access used by a gadget.
 * This is a single-use container for file objects; once initialized,
 * it should not be reused with a different base path.
 */
class FileManager : public FileManagerInterface {
 public:
  /**
   * @param global_file_manager the file manager used to access global files
   *     in the file system. Can be NULL if the file manager is not allowed to
   *     access files in the file system.
   */
  FileManager(FileManagerInterface *global_file_manager);
  virtual ~FileManager();

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
  virtual GadgetStringMap *GetStringTable();
  /** @see FileManagerInterface::FileExists() */
  virtual bool FileExists(const char *file);

 private:
  internal::FileManagerImpl *impl_;
  DISALLOW_EVIL_CONSTRUCTORS(FileManager);
};

} // namespace ggadget

#endif  // GGADGET_FILE_MANAGER_H__
