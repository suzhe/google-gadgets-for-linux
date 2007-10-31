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

#ifndef GGADGET_XML_HTTP_REQUEST_H__
#define GGADGET_XML_HTTP_REQUEST_H__

#include "xml_http_request_interface.h"

namespace ggadget {

class GadgetHostInterface;

XMLHttpRequestInterface *CreateXMLHttpRequest(GadgetHostInterface *host);

} // namespace ggadget

#endif // GGADGET_XML_HTTP_REQUEST_H__
