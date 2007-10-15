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

#include <curl/curl.h>
#include "common.h"
#include "signal.h"
#include "string_utils.h"
#include "xml_http_request_interface.h"

namespace ggadget {

class GadgetHostInterface;

class XMLHttpRequest : public XMLHttpRequestInterface {
 public:
  DEFINE_CLASS_ID(0x98a6c56c71ae45c7, XMLHttpRequestInterface);

  XMLHttpRequest(GadgetHostInterface *host);
  virtual ~XMLHttpRequest();

  virtual Connection *ConnectOnReadyStateChange(Slot0<void> *handler);
  virtual State GetReadyState();

  virtual bool Open(const char *method, const char *url, bool async,
                    const char *user, const char *password);
  virtual bool SetRequestHeader(const char *header, const char *value);
  virtual bool Send(const char *data);
  virtual bool Send(const DOMDocument *data);
  virtual void Abort();

  virtual const char *GetAllResponseHeaders();
  virtual const char *GetResponseHeader(const char *header);
  virtual const char *GetResponseBody(size_t *size);
  virtual DOMDocument *GetResponseXML();
  virtual unsigned short GetStatus();
  virtual const char *GetStatusText();

  DEFAULT_OWNERSHIP_POLICY
  SCRIPTABLE_INTERFACE_DECL
  virtual bool IsStrict() const { return true; }

 private:
  DISALLOW_EVIL_CONSTRUCTORS(XMLHttpRequest);
  class Impl;
  Impl *impl_;
};

} // namespace ggadget

#endif // GGADGET_XML_HTTP_REQUEST_H__
