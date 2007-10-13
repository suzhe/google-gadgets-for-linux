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

#ifndef GGADGETS_XML_HTTP_REQUEST_INTERFACE_H__
#define GGADGETS_XML_HTTP_REQUEST_INTERFACE_H__

#include "element_interface.h"
#include "scriptable_helper.h"

namespace ggadget {

template <typename R> class Slot0;
class Connection;
class DOMDocument;

/**
 * References:
 *   - http://www.w3.org/TR/XMLHttpRequest/
 *   - http://msdn.microsoft.com/library/default.asp?url=/library/en-us/xmlsdk/html/xmobjxmlhttprequest.asp
 *   - http://developer.mozilla.org/cn/docs/XMLHttpRequest
 */
class XMLHttpRequestInterface {
 public:

  enum State {
    UNSENT,
    OPEN,
    SENT,
    LOADING,
    DONE,
  };

  virtual ~XMLHttpRequestInterface() { }

  virtual Connection *ConnectOnReadyStateChange(Slot0<void> *handler) = 0;
  virtual State GetReadyState() = 0;

  virtual bool Open(const char *method, const char *url, bool async,
                    const char *user, const char *password) = 0;
  virtual bool SetRequestHeader(const char *header, const char *value) = 0;
  virtual bool Send(const char *data) = 0;
  virtual bool Send(const DOMDocument *data) = 0;
  virtual void Abort() = 0;

  virtual const char *GetAllResponseHeaders() = 0;
  virtual const char *GetResponseHeader(const char *header) = 0;
  virtual const char *GetResponseBody(size_t *size) = 0;
  virtual DOMDocument *GetResponseXML() = 0;
  virtual unsigned short GetStatus() = 0;
  virtual const char *GetStatusText() = 0;
};

} // namespace ggadget

#endif // GGADGETS_XML_HTTP_REQUEST_INTERFACE_H__
