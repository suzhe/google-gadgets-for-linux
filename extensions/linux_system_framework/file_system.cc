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

#include <string>
#include <sys/time.h>
#include <sys/stat.h>
#include <dirent.h>
#include <vector>
#include <iterator>

#include "ggadget/string_utils.h"

#include "file_system.h"

namespace ggadget {
namespace framework {
namespace linux_system {


//******************************
std::string ToUpper(const std::string &s) {
  std::string result(s);
  std::transform(result.begin(), result.end(), result.begin(), ::toupper);
  return result;
}
//******************************


// utility function for replace all char1 to char2
static void ReplaceAll(std::string *str_ptr,
                       const char char1,
                       const char char2) {
  for (size_t i = 0; i < str_ptr->size(); ++i)
    if ((*str_ptr)[i] == char1)
      (*str_ptr)[i] = char2;
}

// utility function for initializing the file path
static void InitFilePath(const char *filename,
                         std::string *base_ptr,
                         std::string *name_ptr,
                         std::string *path_ptr) {
  if (!filename || !strlen(filename)) {
    *base_ptr = *name_ptr = *path_ptr = "";
    return;
  }

  std::string base, name, path;
  std::string str_path(filename);
  ReplaceAll(&str_path, '\\', '/');

  int last_index = str_path.find_last_of('/');

  if (last_index == -1) {
    // if filename uses relative path, then use current working directory
    // as base path
    
    char current_dir[PATH_MAX + 1];
    
    // get current working directory
    if (!getcwd(current_dir, PATH_MAX)) {
      path = base = name = "";
    } else {
      path = base = name = "";
      name = str_path;
      base = std::string(current_dir);
      if (base.size() && base[base.size() - 1] == '/')
        path = base + name;
      else
        path = base + "/" + name;
    }
  } else {
    // filename is in absolute path
    name = str_path.substr(last_index + 1,
                            str_path.size() - last_index - 1);
    base = str_path.substr(0, last_index + 1);
    path = str_path;
  }

  *name_ptr = name;
  *base_ptr = base;
  *path_ptr = path;
}

class Drives : public DrivesInterface {
 public:
  virtual void Destroy() { }

 public:
  virtual int GetCount() const {
    return 0;
  }

  virtual DriveInterface *GetItem(int index) {
    return NULL;
  }
};

class Drive : public DriveInterface {
 public:
  Drive(const char *drive_spec) {
  }

  virtual void Destroy() {
    delete this;
  }

 public:
  virtual std::string GetPath() {
    return "";
  }

  virtual std::string GetDriveLetter() {
    return "";
  }

  virtual std::string GetShareName() {
    return "";
  }

  virtual DriveType GetDriveType() {
    return DRIVE_TYPE_UNKNOWN;
  }

  virtual FolderInterface *GetRootFolder() {
    return NULL;
  }

  virtual int64_t GetAvailableSpace() {
    return 0;
  }

  virtual int64_t GetFreeSpace() {
    return 0;
  }

  virtual int64_t GetTotalSize() {
    return 0;
  }

  virtual std::string GetVolumnName() {
    return "";
  }

  virtual bool SetVolumnName(const char *name) {
    return false;
  }

  virtual std::string GetFileSystem() {
    return "";
  }

  virtual int64_t GetSerialNumber() {
    return 0;
  }

  virtual bool IsReady() {
    return false;
  }
};

class Files : public FilesInterface {
 public:
  Files() {
  }

  virtual void Destroy() {
    for (size_t i = 0; i < files_.size(); ++i) {
      files_[i]->Destroy();
      files_[i] = NULL;
    }
    files_.clear();
    delete this;
  }

 public:
  virtual int GetCount() const {
    return files_.size();
  }

  virtual FileInterface *GetItem(int index) {
    if ((size_t)index < 0 || (size_t)index >= files_.size())
      return NULL;
    FileSystem filesytem;
    return filesytem.GetFile(files_[index]->GetPath().c_str());
  }

  void AddItem(FileInterface *file_ptr) {
    if (file_ptr)
      files_.push_back(file_ptr);
  }

  void AddItems(FilesInterface *files_ptr) {
    if (!files_ptr)
      return;

    for (int i = 0; i < files_ptr->GetCount(); ++i) {
      files_.push_back(files_ptr->GetItem(i));
    }
  }

 private:
  std::vector<FileInterface *> files_;
};

class File : public FileInterface {
 public:
  explicit File(const char *filename) {
    InitFilePath(filename, &base_, &name_, &path_);
  }

  virtual void Destroy() {
    delete this;
  }

 public:
  virtual std::string GetPath() {
    return path_;
  }

  virtual std::string GetName() {
    return name_;
  }

  virtual bool SetName(const char *name) {
    if (!name || !strlen(name))
      return false;
    if (name_ == "" || base_ == "" || path_ == "")
      return false;

    if (!strcmp(name, name_.c_str()))
      return true;

    std::string oldpath = path_;
    name_ = std::string(name);
    path_ = base_ + name_;
    return !rename(oldpath.c_str(), path_.c_str());
  }

  virtual std::string GetShortPath() {
    if (base_ == "/")
      return path_;
    if (name_ == "" || base_ == "" || path_ == "")
      return "";

    return filesystem_.BuildPath(base_.c_str(), GetShortName().c_str());
  }

  virtual std::string GetShortName() {
    if (name_ == "" || base_ == "" || path_ == "")
      return "";

    std::string extension = ToUpper(filesystem_.GetExtensionName(
                                                   name_.c_str()).substr(0, 3));

    std::string name = name_.size() > 8
                     ? ToUpper(name_.substr(0, 6)) + "~1"
                     : ToUpper(name_);

    if (extension == "")
      return name;
    else
      return name + "." + extension;
  }

  virtual DriveInterface *GetDrive() {
    return NULL;
  }

  virtual FolderInterface *GetParentFolder();
  
  virtual FileAttribute GetAttributes() {
    if (name_ == "" || base_ == "" || path_ == "")
      return FILE_ATTR_NORMAL;

    FileAttribute attribute = FILE_ATTR_NORMAL;

    if (name_.size() && name_[0] == '.') {
      attribute = (FileAttribute) (attribute | FILE_ATTR_HIDDEN);
    }
    
    struct stat statbuf;
    if (stat(path_.c_str(), &statbuf) == -1) {
      return attribute;
    }
    
    int mode = statbuf.st_mode;

    if (S_ISLNK(mode)) {
      // it is a symbolic link
      attribute = (FileAttribute) (attribute | FILE_ATTR_ALIAS);
    }

    if (!(mode & S_IWUSR) && mode & S_IRUSR) {
      // it is read only by owner
      attribute = (FileAttribute) (attribute | FILE_ATTR_READONLY); 
    }

    return attribute;
  }

  virtual bool SetAttributes(FileAttribute attributes) {
    if (name_ == "" || base_ == "" || path_ == "")
      return false;

    // only accept FILE_ATTR_READONLY and FILE_ATTR_HIDDEN

    if (attributes & FILE_ATTR_READONLY) {
      struct stat statbuf;
      if (stat(path_.c_str(), &statbuf) == -1) {
        return false;
      }
      int mode = statbuf.st_mode;

      // modify all the attributes to read only
      mode = (FileAttribute) ((mode | S_IRUSR) & ~S_IWUSR);
      mode = (FileAttribute) ((mode | S_IRGRP) & ~S_IWGRP);
      mode = (FileAttribute) ((mode | S_IROTH) & ~S_IWOTH);

      chmod(path_.c_str(), mode);
    }

    if (attributes & FILE_ATTR_HIDDEN) {
      if (!SetName(("." + name_).c_str()))
        return false;
    }

    return true;
  }

  virtual Date GetDateCreated() {
    // can't determine the created date under linux os
    return Date(0);
  }

  virtual Date GetDateLastModified() {
    if (name_ == "" || base_ == "" || path_ == "")
      return Date(0);

    struct stat statbuf;
    if (stat(path_.c_str(), &statbuf))
      return Date(0);

    return Date(statbuf.st_mtim.tv_sec * 1000
                + statbuf.st_mtim.tv_nsec / 1000000);
  }

  virtual Date GetDateLastAccessed() {
    if (name_ == "" || base_ == "" || path_ == "")
      return Date(0);

    struct stat statbuf;
    if (stat(path_.c_str(), &statbuf))
      return Date(0);

    return Date(statbuf.st_atim.tv_sec * 1000
                + statbuf.st_atim.tv_nsec / 1000000);
  }

  virtual int64_t GetSize() {
    if (name_ == "" || base_ == "" || path_ == "")
      return 0;

    FILE * pFile = fopen (path_.c_str(), "rb");

    if (!pFile)
      return 0;

    fseek(pFile, 0, SEEK_END);
    int64_t count = ftell(pFile);

    fclose (pFile);
    return count;
  }

  virtual std::string GetType() {
    if (name_ == "" || base_ == "" || path_ == "")
      return "";

    return filesystem_.GetExtensionName(path_.c_str());
  }

  virtual bool Delete(bool force) {
    if (name_ == "" || base_ == "" || path_ == "")
      return false;

    FileSystem filesystem;
    return filesystem_.DeleteFile(path_.c_str(), force);
  }

  virtual bool Copy(const char *dest, bool overwrite) {
    if (name_ == "" || base_ == "" || path_ == "")
      return false;

    return filesystem_.CopyFile(path_.c_str(), dest, overwrite);
  }

  virtual bool Move(const char *dest) {
    if (name_ == "" || base_ == "" || path_ == "")
      return false;

    return filesystem_.MoveFile(path_.c_str(), dest);
  }

  virtual TextStreamInterface *OpenAsTextStream(IOMode IOMode,
                                                Tristate Format) {
    if (name_ == "" || base_ == "" || path_ == "")
      return NULL;

    return filesystem_.CreateTextFile(path_.c_str(), true, true);
  }

 private:
  std::string path_;
  std::string base_;
  std::string name_;

  FileSystem filesystem_;
};

/** IFolderCollection. */
class Folders : public FoldersInterface {
 public:
   Folders() {
   }

   virtual void Destroy() {
     for (size_t i = 0; i < folders_.size(); ++i) {
       folders_[i]->Destroy();
       folders_[i] = NULL;
     }
     folders_.clear();
     delete this;
   }

 public:
  virtual int GetCount() const {
    return folders_.size();
  }

  virtual FolderInterface *GetItem(int index) {
    if (index < 0 || (size_t)index >= folders_.size())
      return NULL;
    FileSystem filesytem;
    return filesytem.GetFolder(folders_[index]->GetPath().c_str());
  }

  void AddItem(FolderInterface *folder_ptr) {
    if (folder_ptr) {
      FileSystem filesytem;
      // the added item will be destroyed in destructor
      folders_.push_back(filesytem.GetFolder(folder_ptr->GetPath().c_str()));
    }
  }

  void AddItems(FoldersInterface *folders_ptr) {
    if (!folders_ptr)
      return;

    for (int i = 0; i < folders_ptr->GetCount(); ++i) {
      // the added item will be destroyed in destructor
      folders_.push_back(folders_ptr->GetItem(i));
    }
  }

 private:
  std::vector<FolderInterface *> folders_;
};

static void InitFolder(const char *filename,
                       std::string *base,
                       std::string *name,
                       std::string *path) {
  if (!filename || !strlen(filename))
    return;

  std::string str_path(filename);
  ReplaceAll(&str_path, '\\', '/');

  FileSystem filesystem;

  if (filesystem.FileExists(str_path.c_str())) {
    // it is a file
    *base = *name = *path = "";
    return;
  }

  if (!filesystem.FolderExists(str_path.c_str())) {
    // if not exist, create this folder with permission "drwxrwxr-x"
    if (mkdir(str_path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH))
      // if error, just return
      return;
  }
}

class Folder : public FolderInterface {
 public:
  explicit Folder(const char *filename) {
    InitFilePath(filename, &base_, &name_, &path_);
    InitFolder(filename, &base_, &name_, &path_);
  }

  virtual void Destroy() {
    delete this;
  }

 public:
  virtual std::string GetPath() {
    return path_;
  }

  virtual std::string GetName() {
    return name_;
  }

  virtual bool SetName(const char *name) {
    if (!name || !strlen(name))
      return false;
    if (name_ == "" || base_ == "" || path_ == "")
      return false;
    if (!strcmp(name, name_.c_str()))
      return true;
    
    std::string oldpath = path_;
    name_ = std::string(name);
    ReplaceAll(&name_, '\\', '/');
    path_ = base_ + name_;
    return !rename(oldpath.c_str(), path_.c_str());
  }

  virtual std::string GetShortPath() {
    if (name_ == "" || base_ == "" || path_ == "")
      return "";
    if (base_ == "/")
      return path_;

    return filesystem_.BuildPath(base_.c_str(), GetShortName().c_str());
  }

  virtual std::string GetShortName() {
    if (name_ == "" || base_ == "" || path_ == "")
      return "";

    if (name_.size() > 8)
      return ToUpper(name_.substr(0, 6)) + "~1";
    return ToUpper(name_);
  }

  virtual DriveInterface *GetDrive() {
    return NULL;
  }

  virtual FolderInterface *GetParentFolder() {
    if (name_ == "" || base_ == "" || path_ == "")
      return NULL;

    int length = base_.find_last_of('/');
    if (length == -1)
      return NULL;

    if (!length)
      return new Folder("/");

    return new Folder(base_.substr(0, length).c_str());
  }

  virtual FileAttribute GetAttributes() {
    if (name_ == "" || base_ == "" || path_ == "")
      return FILE_ATTR_DIRECTORY;

    FileAttribute attribute = FILE_ATTR_DIRECTORY;

    if (name_.size() && name_[0] == '.') {
      attribute = (FileAttribute) (attribute | FILE_ATTR_HIDDEN);
    }

    struct stat statbuf;
    if (stat(path_.c_str(), &statbuf) == -1) {
      return attribute;
    }

    int mode = statbuf.st_mode;

    if (!(mode & S_IWUSR) && mode & S_IRUSR) {
      // it is read only by owner
      attribute = (FileAttribute) (attribute | FILE_ATTR_READONLY); 
    }

    return attribute;
  }

  virtual bool SetAttributes(FileAttribute attributes) {
    if (name_ == "" || base_ == "" || path_ == "")
      return false;

    // only accept FILE_ATTR_READONLY and FILE_ATTR_HIDDEN

    if (attributes & FILE_ATTR_READONLY) {
      struct stat statbuf;
      if (stat(path_.c_str(), &statbuf) == -1) {
        return false;
      }
      int mode = statbuf.st_mode;

      // modify all the attributes to read only
      mode = (FileAttribute) ((mode | S_IRUSR) & ~S_IWUSR);
      mode = (FileAttribute) ((mode | S_IRGRP) & ~S_IWGRP);
      mode = (FileAttribute) ((mode | S_IROTH) & ~S_IWOTH);

      chmod(path_.c_str(), mode);
    }

    if (attributes & FILE_ATTR_HIDDEN) {
      if (!SetName(("." + name_).c_str()))
        return false;
    }

    return true;
  }

  virtual Date GetDateCreated() {
    // can't determine the created date under linux os
    return Date(0);
  }

  virtual Date GetDateLastModified() {
    if (name_ == "" || base_ == "" || path_ == "")
      return Date(0);

    struct stat statbuf;
    if (stat(path_.c_str(), &statbuf))
      return Date(0);

    return Date(statbuf.st_mtim.tv_sec * 1000
                + statbuf.st_mtim.tv_nsec / 1000000);
  }

  virtual Date GetDateLastAccessed() {
    if (name_ == "" || base_ == "" || path_ == "")
      return Date(0);

    struct stat statbuf;
    if (stat(path_.c_str(), &statbuf))
      return Date(0);

    return Date(statbuf.st_atim.tv_sec * 1000
                + statbuf.st_atim.tv_nsec / 1000000);
  }

  virtual std::string GetType() {
    return "FOLDER";
  }

  virtual bool Delete(bool force) {
    if (name_ == "" || base_ == "" || path_ == "")
      return false;
    return filesystem_.DeleteFolder(path_.c_str(), force);
  }

  virtual bool Copy(const char *dest, bool overwrite) {
    if (name_ == "" || base_ == "" || path_ == "")
      return false;
    return filesystem_.CopyFile(path_.c_str(), dest, overwrite);
  }

  virtual bool Move(const char *dest) {
    if (name_ == "" || base_ == "" || path_ == "")
      return false;
    return filesystem_.MoveFile(path_.c_str(), dest);
  }

  virtual bool IsRootFolder() {
    return path_ == "/" ? true : false;
  }

  /** Sum of files and subfolders. */
  virtual int64_t GetSize() {
    if (name_ == "" || base_ == "" || path_ == "")
      return 0;

    if (!filesystem_.FolderExists(path_.c_str()))
      return 0;

    // get the init-size of folder.  In Goobuntu, it is 4096
    struct stat statbuf;
    if (stat(path_.c_str(), &statbuf))
      return 0;
    int64_t size = statbuf.st_size;

    DIR *dir = NULL;
    struct dirent *entry = NULL;

    dir = opendir(path_.c_str());
    if (!dir)
      return false;

    while ((entry = readdir(dir)) != NULL) {
      if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
        continue;

      if (entry->d_type == DT_DIR) {
        // sum up the folder's size
        std::string foldername = filesystem_.BuildPath(path_.c_str(),
                                                       entry->d_name);
        FolderInterface *folder = filesystem_.GetFolder(foldername.c_str());
        size += folder->GetSize();
        folder->Destroy();
      } else {
        // sum up the file's size
        std::string filename = filesystem_.BuildPath(path_.c_str(),
                                                     entry->d_name);
        FileInterface *file = filesystem_.GetFile(filename.c_str());
        size += file->GetSize();
        file->Destroy();
      }
    }

    closedir(dir);

    return size;
  }

  virtual FoldersInterface *GetSubFolders() {
    if (name_ == "" || base_ == "" || path_ == "")
      return NULL;

    if (!filesystem_.FolderExists(path_.c_str()))
      return NULL;

    // creates the Folders instance
    Folders* folders_ptr = new Folders();

    DIR *dir = NULL;
    struct dirent *entry = NULL;

    dir = opendir(path_.c_str());
    if (!dir)
      return false;

    while ((entry = readdir(dir)) != NULL) {
      if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
        continue;

      if (entry->d_type == DT_DIR) {
        // sum up the folder's size
        std::string foldername = filesystem_.BuildPath(path_.c_str(),
                                                       entry->d_name);
        // needn't delete folder_ptr
        FolderInterface *folder_ptr =
          filesystem_.GetFolder(foldername.c_str());

        // add the current folder
        folders_ptr->AddItem(folder_ptr);
        
        FoldersInterface *subfolders_ptr = folder_ptr->GetSubFolders();
        
        // add all the sub-folders of current folder
        folders_ptr->AddItems(subfolders_ptr);

        // destroy the folders points (its elements needn't to be detroyed)
        subfolders_ptr->Destroy();
      }
    }

    closedir(dir);

    return folders_ptr;
  }

  virtual FilesInterface *GetFiles() {
    if (name_ == "" || base_ == "" || path_ == "")
      return NULL;

    if (!filesystem_.FolderExists(path_.c_str()))
      return NULL;

    // creates the Files instance
    Files* files_ptr = new Files();

    DIR *dir = NULL;
    struct dirent *entry = NULL;

    dir = opendir(path_.c_str());
    if (!dir)
      return false;

    while ((entry = readdir(dir)) != NULL) {
      if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
        continue;

      if (entry->d_type == DT_DIR) {
        // sum up the files' size in the subfolders
        std::string foldername = filesystem_.BuildPath(path_.c_str(),
                                                               entry->d_name);
        FolderInterface *folder = filesystem_.GetFolder(foldername.c_str());
        FilesInterface *subfiles = folder->GetFiles();

        // add all the files in subfolders
        files_ptr->AddItems(subfiles);

        subfiles->Destroy();
        folder->Destroy();
      } else {
        // sum up the file's size
        std::string filename = filesystem_.BuildPath(path_.c_str(),
                                                       entry->d_name);
        // needn't delete file_ptr
        FileInterface *file_ptr = filesystem_.GetFile(filename.c_str());

        // add the current file
        files_ptr->AddItem(file_ptr);
      }
    }

    closedir(dir);

    return files_ptr;
  }

  virtual TextStreamInterface *CreateTextFile(const char *filename,
                                              bool overwrite, bool unicode) {
    if (!filename ||!strlen(filename))
      return NULL;
    if (name_ == "" || base_ == "" || path_ == "")
      return NULL;

    std::string str_path(filename);
    ReplaceAll(&str_path, '\\', '/');

    if (str_path[0] == '/') {
      // indicates filename is already the absolute path
      return filesystem_.CreateTextFile(str_path.c_str(), overwrite, unicode);
    } else {
      // if not, generate the absolute path
      std::string fullpath =
        filesystem_.BuildPath(path_.c_str(), str_path.c_str());
      return filesystem_.CreateTextFile(fullpath.c_str(), overwrite, unicode);
    }
  }

 private:
  std::string path_;
  std::string base_;
  std::string name_;

  FileSystem filesystem_;
};



class TextStream : public TextStreamInterface {
 public:
  explicit TextStream(const char *filename) {
    fp_ = fopen(filename, "r+b");
    column_ = line_ = 1;
  }

  TextStream(StandardStreamType type) {
    switch (type) {
      case STD_STREAM_IN: fp_ = stdin; break;
      case STD_STREAM_OUT: fp_ = stdout; break;
      case STD_STREAM_ERR: fp_ = stderr; break;
    }
  }

  ~TextStream() {
    if (fp_) {
      fclose(fp_);
      fp_ = NULL;
    }
  }

  virtual void Destroy() { delete this; }

 public:
  virtual int GetLine() {
    if (!fp_)
      return 0;
    return line_;
  }

  virtual int GetColumn() {
    if (!fp_)
      return 0;
    return column_;
  }

  virtual bool IsAtEndOfStream() {
    if (!fp_)
      return true;
    return feof(fp_);
  }

  virtual bool IsAtEndOfLine() {
    if (!fp_)
      return true;
    if (IsAtEndOfStream())
      return true;

    bool result = '\n' == fgetc(fp_);
    if (!fseek(fp_, -1, SEEK_CUR))
      return result;
    return false;
  }
  
  virtual std::string Read(int characters) {
    if (characters <= 0)
      return "";

    if (!fp_)
      return "";

    char buffer[characters + 1];
    std::string result("");

    while (result.size() < (size_t) characters) {
      // since fgets reads at most size - 1 characters,
      // so characters + 1 is used here.
      if (!fgets(buffer, characters + 1, fp_)) {
        if (feof(fp_))
          // if end of stream
          return result;

        // otherwise, error occurs when reading
        return "";
      }
      result = result + std::string(buffer);
    }

    
    
    // update member variable line_ and column_
    UpdateLineAndColumn(result.c_str());

    return result;
  }

  virtual std::string ReadLine() {
    if (!fp_)
      return "";

    char ch = 0;
    std::string result = "";
    while ((ch = fgetc(fp_)) != EOF) {
      result.append(1, ch);
      if ('\n' == ch)
        break;
    }
    
    // update member variable line_ and column_
    line_ ++;
    column_ = 1;

    return result;
  }

  virtual std::string ReadAll() {
    if (!fp_)
      return "";

    char ch = 0;
    std::string result = "";
    while ((ch = fgetc(fp_)) != EOF) {
      result.append(1, ch);
      
      // update member variable line_ and column_ 
      if (ch == '\n')
        line_ ++, column_ = 1;
      else
        column_ ++;
    }

    return result;
  }

  virtual void Write(const char *text) {
    if (!fp_ || !text || !strlen(text))
      return;

    fputs(text, fp_);

    // update member variable line_ and column_
    UpdateLineAndColumn(text);
  }

  virtual void WriteLine(const char *text) {
    if (!fp_ || !text || !strlen(text))
      return;

    Write(text);
    Write("\n");
    
    // update member variable line_ and column_
    line_ ++, column_ = 1;
  }

  virtual void WriteBlankLines(int lines) {
    if (lines <= 0)
      return;

    for (int i = 0; i < lines; ++i) {
      Write("\n");
    }
    
    // update member variable line_ and column_
    if (lines > 0)
      line_ += lines, column_ = 1;
  }

  virtual void Skip(int characters) {
    Read(characters);
  }

  virtual void SkipLine() {
    ReadLine();
  }

  virtual void Close() {
    if (fp_) {
      fclose(fp_);
      fp_ = NULL;
    }
  }

 private:
  void UpdateLineAndColumn(const char *str) {
    if (!str || !strlen(str))
      return;

    size_t length = strlen(str);

    // update member variable line_ and column_
    bool col_flag = false;
    for (size_t i = 1; i <= length; ++i) {
      if (str[length - i] == '\n') {
        line_ ++;
        if (col_flag)
          continue;
        column_ = i;
        col_flag = true;
      } else {
        if (!col_flag)
          column_ ++;
      }
    }
  }

 private:
  FILE *fp_;
  int column_, line_;
};

// Implementation of FileSystem
FileSystem::FileSystem() {
}

FileSystem::~FileSystem() {
}

DrivesInterface *FileSystem::GetDrives() {
  return NULL;
}

std::string FileSystem::BuildPath(const char *path, const char *name) {
  // both path and name are required according to MSDN
  if (!path || !name || !strlen(path) || !strlen(name))
    return "";

  std::string str_path(path);
  std::string str_name(name);
  ReplaceAll(&str_path, '\\', '/');
  ReplaceAll(&str_name, '\\', '/');

  if (str_path[str_path.size() - 1] == '/')
    return str_path + str_name;
  return str_path + "/" + str_name;
}

std::string FileSystem::GetDriveName(const char *path) {
  return "";
}

std::string FileSystem::GetParentFolderName(const char *path) {
  if (!path || !strlen(path))
    return "";
  
  std::string str_path(path);
  ReplaceAll(&str_path, '\\', '/');

  if (str_path == "/")
    return "";

  int length = str_path.find_last_of('/');
  // if no '/' exist, just return empty string
  if (length == -1)
    return "";

  if (!length)
    return "/";

  return str_path.substr(0, length);
}

std::string FileSystem::GetFileName(const char *path) {
  if (!path || !strlen(path))
    return "";

  std::string str_path(path);
  ReplaceAll(&str_path, '\\', '/');

  int start_index = str_path.find_last_of('/');
  // works correctly even if no '/' exists in input path
  return str_path.substr(start_index + 1, str_path.size() - start_index - 1);
}

std::string FileSystem::GetBaseName(const char *path) {
  if (!path)
    return "";

  std::string str_path(path);
  ReplaceAll(&str_path, '\\', '/');
  int start_index = str_path.find_last_of('/');
  int end_index = str_path.find_last_of('.');
  if (end_index == -1)
    end_index = str_path.size();

  if (start_index >= end_index)
    return "";

  return str_path.substr(start_index + 1, end_index - start_index - 1);
}

std::string FileSystem::GetExtensionName(const char *path) {
  if (!path || !strlen(path))
    return "";

  std::string str_path(path);
  ReplaceAll(&str_path, '\\', '/');
  int start_index = str_path.find_last_of('/');
  int end_index = str_path.find_last_of('.');

  if (start_index >= end_index)
    return "";

  return str_path.substr(end_index + 1, str_path.size() - end_index - 1);
}

std::string FileSystem::GetAbsolutePathName(const char *path) {
  if (!path || !strlen(path))
    return "";
  
  std::string str_path(path);
  ReplaceAll(&str_path, '\\', '/');
  if (str_path[0] == '/')
    return str_path;

  char current_dir[PATH_MAX + 1];
  getcwd(current_dir, PATH_MAX); // get current working directory

  return BuildPath(current_dir, str_path.c_str());
}

// creates the character for filename
static char GetFileChar() {
  struct timeval tv;
  if (gettimeofday(&tv, NULL))
    srand(1);
  else
    srand(tv.tv_sec * tv.tv_usec); // overflow can be ignored.
      
  while (1) {
    char ch = (char) (random() % 123);
    if (ch == '_' ||
        ch == '.' ||
        ch == '-' ||
        ('A' <= ch && ch <= 'Z') ||
        ('a' <= ch && ch <= 'z'))
      return ch;
  }
}

std::string FileSystem::GetTempName() {
  // Typically, however, file names only use alphanumeric characters(mostly
  // lower case), underscores, hyphens and periods. Other characters, such as
  // dollar signs, percentage signs and brackets, have special meanings to the
  // shell and can be distracting to work with. File names should never begin
  // with a hyphen.
  char filename[9] = {0};
  char character = 0;
  while((character = GetFileChar()) != '-')
    filename[0] = character;
  for (int i = 1; i < 8; ++i)
    filename[i] = GetFileChar();
  return std::string(filename) + ".tmp";
}

bool FileSystem::DriveExists(const char *drive_spec) {
  return false;
}

bool FileSystem::FileExists(const char *file_spec) {
  if (!file_spec || !strlen(file_spec))
    return false;

  std::string str_path(file_spec);
  ReplaceAll(&str_path, '\\', '/');

  if (access(str_path.c_str(), F_OK))
    return false;

  struct stat statbuf;
  if (stat(str_path.c_str(), &statbuf))
    return false;
  
  if (statbuf.st_mode & S_IFDIR)
    // it is a directory
    return false;
  
  return true;
}

bool FileSystem::FolderExists(const char *folder_spec) {
  if (!folder_spec || !strlen(folder_spec))
    return false;

  std::string str_path(folder_spec);
  ReplaceAll(&str_path, '\\', '/');

  if (access(str_path.c_str(), F_OK))
    return false;

  struct stat statbuf;
  if (stat(str_path.c_str(), &statbuf))
    return false;
  
  if (!(statbuf.st_mode & S_IFDIR))
    // it is not a directory
    return false;

  return true;
}

DriveInterface *FileSystem::GetDrive(const char *drive_spec) {
  return NULL;
}

// note: if file not exists, return NULL
FileInterface *FileSystem::GetFile(const char *file_path) {
  if (!FileExists(file_path))
    return NULL;
  return new File(file_path);
}

FolderInterface *FileSystem::GetFolder(const char *folder_path) {
  if (!FolderExists(folder_path))
    return NULL;
  return new Folder(folder_path);
}

FolderInterface *FileSystem::GetSpecialFolder(SpecialFolder special_folder) {
  // FIXME
  return NULL;
}

bool FileSystem::DeleteFile(const char *file_spec, bool force) {
  if (!FileExists(file_spec))
    return false;

  std::string str_path(file_spec);
  ReplaceAll(&str_path, '\\', '/');

  return !std::remove(str_path.c_str());
}

bool FileSystem::DeleteFolder(const char *folder_spec, bool force) {
  if (!FolderExists(folder_spec))
    return false;

  std::string str_path(folder_spec);
  ReplaceAll(&str_path, '\\', '/');

  return !std::remove(str_path.c_str());
}

bool FileSystem::MoveFile(const char *source, const char *dest) {
  if (!source || !dest || !strlen(source) || !strlen(dest))
    return false;

  std::string source_path(source);
  ReplaceAll(&source_path, '\\', '/');

  if (!FileExists(source_path.c_str()))
    return false;

  std::string dest_input(dest);
  ReplaceAll(&dest_input, '\\', '/');

  std::string dest_path = BuildPath(dest_input.c_str(),
                            GetFileName(source_path.c_str()).c_str());

  if (dest_path == source_path)
    return true;

  return !std::rename(source_path.c_str(), dest_path.c_str());
}

bool FileSystem::MoveFolder(const char *source, const char *dest) {
  if (!source || !dest || !strlen(source) || !strlen(dest))
    return false;
  
  std::string source_path(source);
  ReplaceAll(&source_path, '\\', '/');

  if (!FolderExists(source_path.c_str()))
    return false;

  std::string dest_input(dest);
  ReplaceAll(&dest_input, '\\', '/');

  std::string dest_path = BuildPath(dest_input.c_str(),
                            GetFileName(source_path.c_str()).c_str());

  if (dest_path == source_path)
    return true;

  return !std::rename(source_path.c_str(), dest_path.c_str());
}

bool FileSystem::CopyFile(const char *source, const char *dest,
                               bool overwrite) {
  if (!source || !dest || !strlen(source) || !strlen(dest))
    return false;

  std::string source_path(source);
  ReplaceAll(&source_path, '\\', '/');

  if (!FileExists(source_path.c_str()))
    return false;

  std::string dest_input(dest);
  ReplaceAll(&dest_input, '\\', '/');

  std::string dest_path = BuildPath(dest_input.c_str(),
                            GetFileName(source_path.c_str()).c_str());

  if (source_path == dest_path)
    return false;

  // if dest-file exists and overwrite is false, just return
  if (FileExists(dest_path.c_str()) && !overwrite) {
    return false;
  }

  if (FolderExists(dest_path.c_str()))
    return false;

  std::string exe_command = "cp " + source_path + " " + dest_path; 

  std::system(exe_command.c_str());

  return true;
}

bool FileSystem::CopyFolder(const char *source, const char *dest,
                                 bool overwrite) {
  if (!source || !dest || !strlen(source) || !strlen(dest))
    return false;

  std::string source_path(source);
  ReplaceAll(&source_path, '\\', '/');

  if (!FolderExists(source_path.c_str()))
    return false;
    
  std::string dest_input(dest);
  ReplaceAll(&dest_input, '\\', '/');

  std::string dest_path = BuildPath(dest_input.c_str(),
                            GetFileName(source_path.c_str()).c_str());

  if (FileExists(dest_path.c_str()))
    return false;

  if (FolderExists(dest_path.c_str())) {
    if (!overwrite) 
      // if dest-folder exists and overwrite is false, just return
      return false;

    // if overwrite is true, delete the existing folder
    std::system(("rm -R " + dest_path).c_str());
  }

  std::string exe_command = "cp -R " + source_path + " " + dest_path;

  std::system(exe_command.c_str());

  return true;
}

FolderInterface *FileSystem::CreateFolder(const char *path) {
  if (!path || !strlen(path))
    return NULL;

  std::string str_path(path);
  ReplaceAll(&str_path, '\\', '/');

  if (FileExists(str_path.c_str()))
    return NULL;

  return new Folder(str_path.c_str());
}

TextStreamInterface *FileSystem::CreateTextFile(const char *filename,
                                                     bool overwrite,
                                                     bool unicode) {
  if (!filename || !strlen(filename))
    return NULL;

  std::string str_path(filename);
  ReplaceAll(&str_path, '\\', '/');

  if (FolderExists(str_path.c_str()))
    return NULL;
  
  if (!FileExists(str_path.c_str())) {
    FILE *fp = fopen(str_path.c_str(), "wb");
    if (!fp)
      return NULL;
    fclose(fp);
  }

  return new TextStream(str_path.c_str());
}

TextStreamInterface *FileSystem::OpenTextFile(const char *filename,
                                                   IOMode mode,
                                                   bool create,
                                                   Tristate format) {
  if (!filename || !strlen(filename))
    return NULL;
  
  std::string str_path(filename);
  ReplaceAll(&str_path, '\\', '/');

  if (FolderExists(str_path.c_str()))
    return NULL;
  
  if (!FileExists(str_path.c_str())) {
    if (!create)
      return NULL;

    FILE *fp = fopen(str_path.c_str(), "wb");
    if (!fp)
      return NULL;
    fclose(fp);
  }
 
  return new TextStream(str_path.c_str());
}

TextStreamInterface *
FileSystem::GetStandardStream(StandardStreamType type, bool unicode) {
  if (type != STD_STREAM_IN &&
      type != STD_STREAM_OUT &&
      type != STD_STREAM_ERR)
    return NULL;

  return new TextStream(type);
}

std::string FileSystem::GetFileVersion(const char *filename) {
  return "";
}

FolderInterface *File::GetParentFolder() {
  if (name_ == "" || base_ == "" || path_ == "")
    return NULL;

  int length = base_.find_last_of('/');
  if (!length || length == -1)
    return NULL;
  return new Folder(base_.substr(0, length).c_str());
}

} // namespace linux_system
} // namespace framework
} // namespace ggadget
