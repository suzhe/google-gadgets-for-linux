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

#ifndef GGADGET_TESTS_MOCKED_GADGET_HOST_H__
#define GGADGET_TESTS_MOCKED_GADGET_HOST_H__

#include "ggadget/file_manager_interface.h"
#include "ggadget/gadget_host_interface.h"
#include "ggadget/string_utils.h"
#include "ggadget/xml_parser.h"

class MockedFileManager : public ggadget::FileManagerInterface {
 public:
  virtual bool Init(const char *base_path) { return false; }
  virtual bool GetFileContents(const char *file,
                               std::string *data,
                               std::string *path) { return false; }
  virtual bool GetXMLFileContents(const char *file,
                                  std::string *data,
                                  std::string *path) { return false; }
  virtual bool ExtractFile(const char *file, std::string *into_file) {
    return false;
  }
  virtual const ggadget::GadgetStringMap *GetStringTable() const {
    return &strings_;
  }
  virtual bool FileExists(const char *file) { return false; }
 private:
  ggadget::GadgetStringMap strings_;
};


class MockedGadgetHost : public ggadget::GadgetHostInterface {
 public:
  MockedGadgetHost() : xml_parser_(ggadget::CreateXMLParser()) { }
  virtual ~MockedGadgetHost() { delete xml_parser_; }
  virtual ggadget::ScriptRuntimeInterface *GetScriptRuntime(
      ScriptRuntimeType type) { return NULL; }
  virtual ggadget::ElementFactory *GetElementFactory() {
    return NULL;
  }
  virtual ggadget::FileManagerInterface *GetFileManager() {
    return &file_manager_;
  }
  virtual ggadget::FileManagerInterface *GetGlobalFileManager() {
    return &file_manager_;
  }
  virtual ggadget::OptionsInterface *GetOptions() { return NULL; }
  virtual ggadget::GadgetInterface *GetGadget() { return NULL; }
  virtual ggadget::ViewHostInterface *NewViewHost(
      ViewType type, ggadget::ViewInterface *view) { return NULL; }
  virtual ggadget::XMLParserInterface *GetXMLParser() { return xml_parser_; }
  virtual void SetPluginFlags(int plugin_flags) { }
  virtual void RemoveMe(bool save_data) { }
  virtual void DebugOutput(DebugLevel level, const char *message) const { }
  virtual uint64_t GetCurrentTime() const { return 0; }
  virtual bool OpenURL(const char *url) const { return true; }

  virtual bool LoadFont(const char *filename) { return true; }
  virtual bool UnloadFont(const char *filename) { return true; }
  virtual bool BrowseForFiles(const char *filter, bool multiple,
                              std::vector<std::string> *result) {
    return false;
  }
  virtual void GetCursorPos(int *x, int *y) const { }
  virtual void GetScreenSize(int *width, int *height) const { }
  virtual std::string GetFileIcon(const char *filename) const { return ""; }
 private:
  MockedFileManager file_manager_;
  ggadget::XMLParserInterface *xml_parser_;
};

#endif // GGADGET_TESTS_MOCKED_GADGET_HOST_H__
