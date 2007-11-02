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

#include "scriptable_interface.h"

namespace ggadget {

template <typename R> class Slot0;
class Connection;
class DOMDocumentInterface;

/**
 * References:
 *   - http://www.w3.org/TR/XMLHttpRequest/
 *   - http://msdn.microsoft.com/library/default.asp?url=/library/en-us/xmlsdk/html/xmobjxmlhttprequest.asp
 *   - http://developer.mozilla.org/cn/docs/XMLHttpRequest
 */
class XMLHttpRequestInterface : public ScriptableInterface {
 public:
  CLASS_ID_DECL(0x301dceaec56141d6);

  enum ExceptionCode {
    NO_ERR = 0,
    INVALID_STATE_ERR = 11,
    SYNTAX_ERR = 12,
    SECURITY_ERR = 18,
    NETWORK_ERR = 101,
    ABORT_ERR = 102,
    NULL_POINTER_ERR = 200,
    OTHER_ERR = 300,
  };

  enum State {
    UNSENT,
    OPENED,
    HEADERS_RECEIVED,
    LOADING,
    DONE,
  };

  virtual ~XMLHttpRequestInterface() { }

  virtual Connection *ConnectOnReadyStateChange(Slot0<void> *handler) = 0;
  virtual State GetReadyState() = 0;

  virtual ExceptionCode Open(const char *method, const char *url, bool async,
                             const char *user, const char *password) = 0;
  virtual ExceptionCode SetRequestHeader(const char *header,
                                         const char *value) = 0;
  virtual ExceptionCode Send(const char *data, size_t size) = 0;
  virtual ExceptionCode Send(const DOMDocumentInterface *data) = 0;
  virtual void Abort() = 0;

  virtual ExceptionCode GetAllResponseHeaders(const char **result) = 0;
  virtual ExceptionCode GetResponseHeader(const char *header,
                                          const char **result) = 0;
  virtual ExceptionCode GetResponseText(const char **result) = 0;
  virtual ExceptionCode GetResponseBody(const char **result,
                                        size_t *size) = 0;
  virtual ExceptionCode GetResponseXML(DOMDocumentInterface **result) = 0;
  virtual ExceptionCode GetStatus(unsigned short *result) = 0;
  virtual ExceptionCode GetStatusText(const char **result) = 0;
};

CLASS_ID_IMPL(XMLHttpRequestInterface, ScriptableInterface)

} // namespace ggadget

#endif // GGADGETS_XML_HTTP_REQUEST_INTERFACE_H__
