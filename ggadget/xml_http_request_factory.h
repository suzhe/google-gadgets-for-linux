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

#ifndef GGADGET_XML_HTTP_REQUEST_FACTORY_H__
#define GGADGET_XML_HTTP_REQUEST_FACTORY_H__

#include <ggadget/xml_http_request_interface.h>

namespace ggadget {

class XMLParserInterface;

/**
 * Factory class for creating XMLHttpRequest instances.
 */
class XMLHttpRequestFactory {
 public:

  /**
   * Creates an instance of XMLHttpRequestInterface by using a loaded
   * XMLHttpRequest extension.
   *
   * A XMLHttpRequest extension must be loaded into the global Extension
   * Manager in advanced. If there is no XMLHttpRequest extension loaded, NULL
   * will be returned.
   *
   * @param parser A XML parser instance to be used by the newly created
   * XMLHttpRequestInterface instance.
   */
  XMLHttpRequestInterface *CreateXMLHttpRequest(XMLParserInterface *parser);

 public:

  /** Get the singleton of XMLHttpRequestFactory. */
  static XMLHttpRequestFactory *get();

 private:
  class Impl;
  Impl *impl_;

  /**
   * Private constructor to prevent creating XMLHttpRequestFactory object
   * directly.
   */
  XMLHttpRequestFactory();

  /**
   * Private destructor to prevent deleting XMLHttpRequestFactory object
   * directly.
   */
  ~XMLHttpRequestFactory();

  DISALLOW_EVIL_CONSTRUCTORS(XMLHttpRequestFactory);
};

} // namespace ggadget

#endif // GGADGET_XML_HTTP_REQUEST_FACTORY_H__
