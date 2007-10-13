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

#ifndef XML_HTTP_REQUEST_H__
#define XML_HTTP_REQUEST_H__

#include <curl/curl.h>
#include "ggadget/common.h"
#include "ggadget/signal.h"
#include "ggadget/string_utils.h"
#include "ggadget/xml_http_request_interface.h"

class GtkCairoHost;

namespace ggadget {
class Connection;
class DOMDocument;
}

class XMLHttpRequest : public ggadget::XMLHttpRequestInterface {
 public:
  XMLHttpRequest(GtkCairoHost *host);
  virtual ~XMLHttpRequest();

  virtual ggadget::Connection *ConnectOnReadyStateChange(
      ggadget::Slot0<void> *handler);
  virtual State GetReadyState();

  virtual bool Open(const char *method, const char *url, bool async,
                    const char *user, const char *password);
  virtual bool SetRequestHeader(const char *header, const char *value);
  virtual bool Send(const char *data);
  virtual bool Send(const ggadget::DOMDocument *data);
  virtual void Abort();

  virtual const char *GetAllResponseHeaders();
  virtual const char *GetResponseHeader(const char *header);
  virtual const char *GetResponseBody(size_t *size);
  virtual ggadget::DOMDocument *GetResponseXML();
  virtual unsigned short GetStatus();
  virtual const char *GetStatusText();

 private:
  DISALLOW_EVIL_CONSTRUCTORS(XMLHttpRequest);

  void OnIOReady(int fd);
  void ChangeState(State new_state);
  void Done();

  static int SocketCallback(CURL *handle, curl_socket_t socket, int type,
                            void *user_p, void *sock_p);
  static size_t ReadCallback(void *ptr, size_t size,
                             size_t mem_block, void *data);
  static size_t WriteHeaderCallback(void *ptr, size_t size,
                                    size_t mem_block, void *data);
  static size_t WriteBodyCallback(void *ptr, size_t size,
                                  size_t mem_block, void *data);

  GtkCairoHost *host_;
  ggadget::Signal0<void> onreadystatechange_signal_;

  bool async_;
  std::string user_;
  std::string password_;

  CURL *curl_;
  CURLM *curlm_;
  curl_socket_t socket_;
  int socket_read_watch_;
  int socket_write_watch_;

  State state_;
  // Required by the specification.
  // It will be true after send() is called in async mode. 
  bool send_flag_;

  curl_slist *headers_;
  std::string send_data_;
  std::string response_headers_;
  std::string status_text_;
  std::string response_body_;

  ggadget::CaseInsensitiveStringMap response_headers_map_;
};

#endif // XML_HTTP_REQUEST_H__
