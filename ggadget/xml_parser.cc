/*
  Copyright 2008 Google Inc.

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

#include "logger.h"
#include "xml_parser_interface.h"

namespace ggadget {

static XMLParserInterface *g_xml_parser = NULL;

bool SetXMLParser(XMLParserInterface *xml_parser) {
  ASSERT(!g_xml_parser && xml_parser);
  if (!g_xml_parser && xml_parser) {
    g_xml_parser = xml_parser;
    return true;
  }
  return false;
}

XMLParserInterface *GetXMLParser() {
  EXPECT_M(g_xml_parser, ("The global xml parser has not been set yet."));
  return g_xml_parser;
}

} // namespace ggadget
