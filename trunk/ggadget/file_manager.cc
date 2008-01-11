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

#include <locale.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>

#include <third_party/unzip/unzip.h>
#include "file_manager.h"
#include "gadget_consts.h"
#include "logger.h"
#include "windows_locales.h"
#include "xml_parser_interface.h"

namespace ggadget {

namespace internal {

FileManagerImpl::FileManagerImpl(XMLParserInterface *xml_parser)
    : xml_parser_(xml_parser), is_dir_(false) {
}

FileManagerImpl::~FileManagerImpl() {
}

bool FileManagerImpl::Init(const char *base_path) {
  int error = 0;
  ASSERT_M(base_path_.empty(), ("Don't initialize a FileManager twice"));
  if (!base_path || !base_path[0]) {
    LOG("Base path is empty.");
    return false;
  }

  base_path_ = base_path;
  std::string path;
  std::string filename;
  SplitPathFilename(base_path, &path, &filename);
  if (filename.size() > strlen(kGManifestExt) &&
      strcasecmp(filename.c_str() + filename.size() - strlen(kGManifestExt),
                 kGManifestExt) == 0) {
    base_path_ = path;
  }

  // Remove the trailing slash if any.
  if (*(base_path_.end() - 1) == kDirSeparator)
    base_path_.erase(base_path_.end() - 1);

  struct stat stat_value;
  bzero(&stat_value, sizeof(stat_value));
  error = stat(base_path_.c_str(), &stat_value);
  if (error) {
    LOG("Can't access path: %s", base_path_.c_str());
    return false;
  }

  is_dir_ = S_ISDIR(stat_value.st_mode);
  if (is_dir_) {
    // TODO: Check security?
    if (!ScanDirFilenames(base_path_.c_str())) {
      LOG("Failed to scan dir: %s", base_path_.c_str());
      return false;
    }
  } else {
    // Not dir, so it should be a zip file. Try extracting.
    if (!ScanZipFilenames()) {
      LOG("Failed to scan zip file: %s", base_path_.c_str());
      return false;
    }
  }

  // Init locale strings before loading string table.
  if (!InitLocaleStrings()) {
    LOG("Failed to init locale strings in base path: %s", base_path_.c_str());
    return false;
  }
  // Now Load strings.xml.
  if (!LoadStringTable(kStringsXML)) {
    LOG("Failed to load %s in base path: %s", kStringsXML, base_path_.c_str());
    // Only warn about this error.
  }

  return true;
}

FileManagerImpl::FileMap::const_iterator FileManagerImpl::FindFile(
    const char *file, std::string *normalized_file) {
  ASSERT_M(!base_path_.empty(), ("Not initialized"));
  ASSERT(file);

  normalized_file->assign(file);;
  for (std::string::iterator i = normalized_file->begin();
       i != normalized_file->end(); ++i) {
    // In Linux replace all \ with / for Windows compatibility.
    if ('\\' == *i)
      *i = kDirSeparator;
  }

  // First try non-localized file.
  FileMap::const_iterator iter = files_.find(*normalized_file);
  if (iter == files_.end())
    // Second try localized file.
    iter = FindLocalizedFile(normalized_file->c_str());

  return iter;
}

bool FileManagerImpl::GetFileContents(const char *file,
                                      std::string *data,
                                      std::string *path) {
  std::string normalized_file;
  FileMap::const_iterator iter = FindFile(file, &normalized_file);
  if (iter == files_.end()) {
    LOG("File not found: %s in dir: %s",
        normalized_file.c_str(), base_path_.c_str());
    return false;
  } else {
    *path = base_path_ + kDirSeparator + iter->first;
    return GetFileContentsInternal(iter, data);
  }
}

bool FileManagerImpl::GetXMLFileContents(const char *file,
                                         std::string *data,
                                         std::string *path) {
  ASSERT(data);
  data->clear();

  std::string raw_data;
  if (!GetFileContents(file, &raw_data, path))
    return false;

  // Process text to replace all "&..;" entities with corresponding resources.
  std::string::size_type start = 0;
  std::string::size_type pos = 0;
  for (; pos < raw_data.size(); pos++) {
    char c = raw_data[pos];
    // 1 byte at a time is OK here.
    if ('&' == c) {
      // Append the previous chunk.
      data->append(raw_data, start, pos - start);
      start = pos;

      std::string entity_name;
      while (++pos < raw_data.size() &&
             (c = raw_data[pos]) != ';' && c != '\n')
        entity_name.push_back(c);

      if (c != ';') {
        LOG("Unterminated entity reference: %s in file: %s in dir %s",
            raw_data.substr(start, pos - start).c_str(),
            file, base_path_.c_str());
        return false;
      }

      GadgetStringMap::const_iterator iter = string_table_.find(entity_name);
      if (iter != string_table_.end()) {
        start = pos + 1;  // Reset start for next chunk.
        data->append(xml_parser_->EncodeXMLString(iter->second.c_str()));
      }
      // Else: not a fatal error. Just leave the original entity reference
      // text in the current text chunk and let tinyxml deal with it.
    }
  }

  if (pos - start > 0)  // Append any remaining chars.
    data->append(raw_data, start, pos - start);
  return true;
}

bool FileManagerImpl::ExtractFile(const char *file, std::string *into_file) {
  std::string data;
  std::string real_path;
  if (!GetFileContents(file, &data, &real_path))
    return false;

  // TODO: System dependent.
  ASSERT(into_file);
  char tmp_buffer[] = "/tmp/gglXXXXXX";
  FILE *fp;
  if (into_file->empty()) {
    int fd = mkstemp(tmp_buffer);
    if (fd < 0) {
      LOG("Failed to mkstemp errno=%d", errno);
      return false;
    }
    fp = fdopen(fd, "w");
    if (fp == NULL) {
      LOG("Failed to fdopen %s", tmp_buffer);
      close(fd);
      return false;
    }
    *into_file = tmp_buffer;
  } else {
    fp = fopen(into_file->c_str(), "w");
    if (fp == NULL) {
      LOG("Failed to fopen %s", into_file->c_str());
      return false;
    }
  }

  bool succeeded = true;
  if (fwrite(data.c_str(), 1, data.size(), fp) != data.size()) {
    LOG("Failed writing to file %s", into_file->c_str());
    succeeded = false;
    unlink(into_file->c_str()); // ignore return
  }
  fclose(fp);
  return succeeded;
}

bool FileManagerImpl::InitLocaleStrings() {
  char *locale = setlocale(LC_MESSAGES, NULL);
  if (!locale)
    return false;

  locale_prefix_ = locale;
  std::string::size_type pos = locale_prefix_.find('.');
  if (pos != std::string::npos)
    // Remove '.' and everything after.
    locale_prefix_.erase(pos);

  locale_lang_prefix_ = locale_prefix_;
  pos = locale_lang_prefix_.find('_');
  if (pos != std::string::npos)
    locale_lang_prefix_.erase(pos);

  locale_lang_prefix_ += kDirSeparator;
  if (GetLocaleIDString(locale_prefix_.c_str(), &locale_id_prefix_))
    locale_id_prefix_ += kDirSeparator;
  locale_prefix_ += kDirSeparator;

  return true;
}

void FileManagerImpl::SplitPathFilename(const char *input_path,
                                        std::string *path,
                                        std::string *filename) {
  ASSERT(path);
  *path = input_path;
  if (filename)
    *filename = "";

  std::string::size_type pos = path->rfind(kDirSeparator);
  if (pos != std::string::npos) {
    if (filename)
      *filename = path->substr(pos + 1);
    // Preserve the starting '/' there is only one '/'.
    path->erase(pos ? pos : 1);
  }
}

bool FileManagerImpl::LoadStringTable(const char *string_table) {
  // Don't use GetXMLFileContents() because it tries to do string
  // resource substitution on the file, which is inappropiate here.
  std::string data, filename;
  if (!GetFileContents(string_table, &data, &filename))
    return false;

  bool result = xml_parser_->ParseXMLIntoXPathMap(data, filename.c_str(),
                                                  kStringsTag, NULL,
                                                  &string_table_);

  if (!result) {
    // For compatibility with some Windows gadget files that use ISO8859-1
    // encoding without declaration.
    result = xml_parser_->ParseXMLIntoXPathMap(data, filename.c_str(),
                                               kStringsTag, "ISO8859-1",
                                               &string_table_);
  }

  return result;
}

bool FileManagerImpl::ScanDirFilenames(const char *dir_path) {
  DIR *pdir = opendir(dir_path);
  if (!pdir) {
    LOG("Can't read directory: %s", dir_path);
    return false;
  }

  struct dirent *pfile = NULL;
  while ((pfile = readdir(pdir)) != NULL) {
    if (strcmp(pfile->d_name, ".") != 0 &&
        strcmp(pfile->d_name, "..") != 0) {

      struct stat stat_value;
      bzero(&stat_value, sizeof(stat_value));

      std::string abs_path = dir_path;
      abs_path += kDirSeparator;
      abs_path += pfile->d_name;
      if (stat(abs_path.c_str(), &stat_value) != 0) {
        LOG("File not accessable: %s", abs_path.c_str());
        // Only a warning, continue to scan other files.
      } else if (S_ISDIR(stat_value.st_mode)) {
        // Ignore the result, continue to scan other files even on error.
        ScanDirFilenames(abs_path.c_str());
      } else {
        unz_file_pos dummy_unz_pos = { 0, 0 };
        files_[abs_path.substr(base_path_.size() + 1)] = dummy_unz_pos;
      }
    }
  }

  closedir(pdir);
  return true;
}

bool FileManagerImpl::ScanZipFilenames() {
  unzFile zip = unzOpen(base_path_.c_str());
  if (!zip) {
    LOG("Failed to open zip file: %s", base_path_.c_str());
    return false;
  }

  int error;
  for (error = unzGoToFirstFile(zip);
       error == UNZ_OK;
       error = unzGoToNextFile(zip)) {
    unz_file_info file_info;
    bzero(&file_info, sizeof(file_info));

    char filename[PATH_MAX];
    error = unzGetCurrentFileInfo(zip,
                                  &file_info,
                                  filename,
                                  sizeof(filename),
                                  NULL, 0, NULL, 0);
    if (UNZ_OK == error && filename[strlen(filename) - 1] != kDirSeparator) {
      unz_file_pos pos;
      error = unzGetFilePos(zip, &pos);
      if (UNZ_OK == error)
        files_[filename] = pos;
    }
  }
  unzClose(zip);
  return error == UNZ_END_OF_LIST_OF_FILE;
}

FileManagerImpl::FileMap::const_iterator FileManagerImpl::FindLocalizedFile(
    const char *file) const {
  // Try lang_TERRITORY/file, e.g. zh_CN/myfile.
  FileMap::const_iterator iter = files_.find(locale_prefix_ + file);
  if (iter == files_.end()) {
    // Try lang/file, e.g. zh/myfile.
    iter = files_.find(locale_lang_prefix_ + file);
    if (iter == files_.end()) {
      // Try default en_US and en locale.
      iter = files_.find(std::string("en_US/") + file);
      if (iter == files_.end()) {
        iter = files_.find(std::string("en/") + file);
        // Windows compatible, using locale ids.
        // Try locale_id/file, e.g. 2052/myfile.
        if (iter == files_.end() &&
            (locale_id_prefix_.empty() ||
             (iter = files_.find(locale_id_prefix_ + file)) == files_.end())) {
          // Try default locale_id, e.g. 1033/myfile.
          iter = files_.find(std::string("1033/") + file);
        }
      }
    }
  }
  return iter;
}

bool FileManagerImpl::GetFileContentsInternal(FileMap::const_iterator iter,
                                              std::string *data) {
  if (is_dir_)
    return GetDirFileContents(iter, data);
  else
    return GetZipFileContents(iter, data);
}

bool FileManagerImpl::GetDirFileContents(FileMap::const_iterator iter,
                                         std::string *data) {
  ASSERT(data);
  data->clear();
  // TODO: check security?
  std::string real_name = base_path_ + kDirSeparator + iter->first;
  // Try to read from disk.
  // The approach below doesn't really work for large files, but files
  // in the gadget should be small so this should be OK.
  // Also, reading from a dir is used for gadget testing/design scenarios
  // only, which is not a common scenario.
  // A memory-mapped file scheme might be better here.
  FILE *datafile = fopen(real_name.c_str(), "r");
  if (!datafile) {
    LOG("Failed to open file: %s", real_name.c_str());
    return false;
  }

  const size_t kChunkSize = 2048;
  char buffer[kChunkSize];
  while (true) {
    size_t read_size = fread(buffer, 1, kChunkSize, datafile);
    data->append(buffer, read_size);
    if (read_size < kChunkSize)
      break;
  }

  if (ferror(datafile)) {
    LOG("Error when reading file: %s", real_name.c_str());
    data->clear();
    fclose(datafile);
    return false;
  }

  fclose(datafile);
  return true;
}

bool FileManagerImpl::GetZipFileContents(FileMap::const_iterator iter,
                                         std::string *data) {
  ASSERT(data);
  data->clear();

  unzFile zip = unzOpen(base_path_.c_str());
  if (!zip) {
    LOG("Failed to open zip file: %s", base_path_.c_str());
    return false;
  }

  bool status = true;
  const char *filename = iter->first.c_str();
  if (unzGoToFilePos(zip, const_cast<unz_file_pos *>(&iter->second)) == UNZ_OK
      && unzOpenCurrentFile(zip) == UNZ_OK) {
    const int kChunkSize = 2048;
    char buffer[kChunkSize];
    while (true) {
      int read_size = unzReadCurrentFile(zip, buffer, kChunkSize);
      if (read_size > 0)
        data->append(buffer, read_size);
      else if (read_size < 0) {
        LOG("Error reading file: %s in zip file: %s",
            filename, base_path_.c_str());
        data->clear();
        status = false;
        break;
      } else {
        break;
      }
    }
  } else {
    LOG("Failed to open file: %s in zip file: %s",
        filename, base_path_.c_str());
    status = false;
  }

  if (unzCloseCurrentFile(zip) != UNZ_OK) {
    LOG("CRC error in file: %s in zip file: %s",
        filename, base_path_.c_str());
    data->clear();
    status = false;
  }
  unzClose(zip);
  return status;
}

bool FileManagerImpl::FileExists(const char *file) {
  std::string normalized_file;
  FileMap::const_iterator iter = FindFile(file, &normalized_file);
  return iter != files_.end();
}

} // namespace internal

FileManager::FileManager(XMLParserInterface *xml_parser)
    : impl_(new internal::FileManagerImpl(xml_parser)) { }

FileManager::~FileManager() {
  delete impl_;
}

bool FileManager::Init(const char *base_path) {
  return impl_->Init(base_path);
}

bool FileManager::GetFileContents(const char *file,
                                  std::string *data,
                                  std::string *path) {
  return impl_->GetFileContents(file, data, path);
}

bool FileManager::GetXMLFileContents(const char *file,
                                     std::string *data,
                                     std::string *path) {
  return impl_->GetXMLFileContents(file, data, path);
}

bool FileManager::ExtractFile(const char *file, std::string *into_file) {
  return impl_->ExtractFile(file, into_file);
}

const GadgetStringMap *FileManager::GetStringTable() const {
  return &impl_->string_table_;
}

bool FileManager::FileExists(const char *file) {
  return impl_->FileExists(file);
}

} // namespace ggadget
