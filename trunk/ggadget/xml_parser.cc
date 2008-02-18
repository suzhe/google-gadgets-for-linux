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

#include "module.h"
#include "extension_manager.h"
#include "xml_parser_interface.h"

namespace ggadget {
namespace {

static const char kXMLParserExtensionSymbolName[] = "GetXMLParser";
static XMLParserInterface *g_xml_parser = NULL;

class XMLParserExtensionRegister : public ExtensionRegisterInterface {
 public:
  typedef XMLParserInterface *(*GetXMLParserFunc)();

  virtual bool RegisterExtension(const Module *extension) {
    ASSERT(extension);
    GetXMLParserFunc func =
        reinterpret_cast<GetXMLParserFunc>(
            extension->GetSymbol(kXMLParserExtensionSymbolName));

    if (func) {
      g_xml_parser = func();
      return true;
    }
    return false;
  }
};

} // anonymous namespace

XMLParserInterface *GetXMLParser() {
  if (!g_xml_parser) {
    const ExtensionManager *manager =
        ExtensionManager::GetGlobalExtensionManager();
    ASSERT(manager);
    if (manager) {
      XMLParserExtensionRegister reg;
      manager->RegisterLoadedExtensions(&reg);
    }
  }
  ASSERT(g_xml_parser);
  return g_xml_parser;
}

} // namespace ggadget
