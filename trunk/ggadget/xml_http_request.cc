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

#include <cstring>
#include <curl/curl.h>

#include "xml_http_request.h"
#include "gadget_host_interface.h"
#include "scriptable_binary_data.h"
#include "script_context_interface.h"
#include "scriptable_helper.h"
#include "signals.h"
#include "string_utils.h"
#include "xml_dom.h"
#include "xml_utils.h"

namespace ggadget {
namespace {

static const Variant kOpenDefaultArgs[] = {
  Variant(), Variant(),
  Variant(true),
  Variant(static_cast<const char *>(NULL)),
  Variant(static_cast<const char *>(NULL))
};

static const Variant kSendDefaultArgs[] = { Variant("") };

class XMLHttpRequest : public ScriptableHelper<XMLHttpRequestInterface> {
 public:
  DEFINE_CLASS_ID(0xda25f528f28a4319, XMLHttpRequestInterface);

  XMLHttpRequest(GadgetHostInterface *host,
                 ScriptContextInterface *script_context)
      : host_(host),
        script_context_(script_context),
        async_(false),
        curl_(NULL),
        curlm_(NULL),
        socket_(0),
        socket_read_watch_(0),
        socket_write_watch_(0),
        state_(UNSENT),
        send_flag_(false),
        headers_(NULL),
        response_dom_(NULL) {
    RegisterSignal("onreadystatechange", &onreadystatechange_signal_);
    RegisterReadonlySimpleProperty("readyState", &state_);
    RegisterMethod("open",
        NewSlotWithDefaultArgs(NewSlot(this, &XMLHttpRequest::ScriptOpen),
                               kOpenDefaultArgs));
    RegisterMethod("setRequestHeader",
                   NewSlot(this, &XMLHttpRequest::ScriptSetRequestHeader));
    RegisterMethod("send",
        NewSlotWithDefaultArgs(NewSlot(this, &XMLHttpRequest::ScriptSend),
                               kSendDefaultArgs));
    RegisterMethod("abort", NewSlot(this, &XMLHttpRequest::Abort));
    RegisterMethod("getAllResponseHeaders",
                   NewSlot(this, &XMLHttpRequest::ScriptGetAllResponseHeaders));
    RegisterMethod("getResponseHeader",
                   NewSlot(this, &XMLHttpRequest::ScriptGetResponseHeader));
    RegisterProperty("responseStream",
                     NewSlot(this, &XMLHttpRequest::ScriptGetResponseBody),
                     NULL);
    RegisterProperty("responseBody",
                     NewSlot(this, &XMLHttpRequest::ScriptGetResponseBody),
                     NULL);
    RegisterProperty("responseText",
                     NewSlot(this, &XMLHttpRequest::ScriptGetResponseText),
                     NULL);
    RegisterProperty("responseXML",
                     NewSlot(this, &XMLHttpRequest::ScriptGetResponseXML),
                     NULL);
    RegisterProperty("status", NewSlot(this, &XMLHttpRequest::ScriptGetStatus),
                     NULL);
    RegisterProperty("statusText",
                     NewSlot(this, &XMLHttpRequest::ScriptGetStatusText), NULL);
  }

  ~XMLHttpRequest() {
    Abort();
  }

  // This object is script owned, because it is created in script with
  // new XMLHttpRequest().
  virtual OwnershipPolicy Attach() {
    return OWNERSHIP_TRANSFERRABLE;
  }
  virtual bool Detach() {
    delete this;
    return true;
  }

  virtual Connection *ConnectOnReadyStateChange(Slot0<void> *handler) {
    return onreadystatechange_signal_.Connect(handler);
  }

  virtual State GetReadyState() {
    return state_;
  }

  void ChangeState(State new_state) {
    DLOG("XMLHttpRequest: ChangeState from %d to %d", state_, new_state);
    state_ = new_state;
    onreadystatechange_signal_();
  }

  // The maximum data size of this class can process.
  static const size_t kMaxDataSize = 8 * 1024 * 1024;

  static bool CheckSize(size_t current, size_t num_blocks, size_t block_size) {
    return current < kMaxDataSize && block_size > 0 &&
           (kMaxDataSize - current) / block_size > num_blocks;
  }

  virtual ExceptionCode Open(const char *method, const char *url, bool async,
                             const char *user, const char *password) {
    Abort();
    if (!method || !url)
      return NULL_POINTER_ERR;

    url_ = url;
    curl_ = curl_easy_init();
    if (!curl_) {
      DLOG("XMLHttpRequest: curl_easy_init failed");
      // TODO: Send errors.
      return OTHER_ERR;
    }

    if (strcasecmp(method, "HEAD") == 0) {
      curl_easy_setopt(curl_, CURLOPT_HTTPGET, 1);
      curl_easy_setopt(curl_, CURLOPT_NOBODY, 1);
    } else if (strcasecmp(method, "GET") == 0) {
      curl_easy_setopt(curl_, CURLOPT_HTTPGET, 1);
    } else if (strcasecmp(method, "POST") == 0) {
      curl_easy_setopt(curl_, CURLOPT_POST, 1);
    } else if (strcasecmp(method, "PUT") == 0) {
      curl_easy_setopt(curl_, CURLOPT_PUT, 1);
      curl_easy_setopt(curl_, CURLOPT_UPLOAD, 1);
    } else {
      LOG("XMLHttpRequest: Unsupported method: %s", method);
      return SYNTAX_ERR;
    }
    curl_easy_setopt(curl_, CURLOPT_URL, url_.c_str());

    if (user || password) {
      std::string user_pwd;
      if (user)
        user_pwd = user;
      user_pwd += ':';
      if (password)
        user_pwd += password;
      curl_easy_setopt(curl_, CURLOPT_USERPWD, user_pwd.c_str());
    }

    async_ = async;
    ChangeState(OPENED);
    return NO_ERR;
  }

  virtual ExceptionCode SetRequestHeader(const char *header,
                                         const char *value) {
    static const char *kForbiddenHeaders[] = {
        "Accept-Charset",
        "Accept-Encoding",
        "Connection",
        "Content-Length",
        "Content-Transfer-Encoding",
        "Date",
        "Expect",
        "Host",
        "Keep-Alive",
        "Referer",
        "TE",
        "Trailer",
        "Transfer-Encoding",
        "Upgrade",
        "Via"
    };

    if (!header)
      return NULL_POINTER_ERR;
    if (state_ != OPENED || send_flag_) {
      LOG("XMLHttpRequest: SetRequestHeader: Invalid state: %d", state_);
      return INVALID_STATE_ERR;
    }

    if (strncasecmp("Proxy-", header, 6) == 0) {
      DLOG("XMLHttpRequest::SetRequestHeader: Forbidden header %s", header);
      return NO_ERR;
    }

    const char **found = std::lower_bound(
        kForbiddenHeaders, kForbiddenHeaders + arraysize(kForbiddenHeaders),
        header, CaseInsensitiveCharPtrComparator());
    if (found != kForbiddenHeaders + arraysize(kForbiddenHeaders) &&
        strcasecmp(*found, header) == 0) {
      DLOG("XMLHttpRequest::SetRequestHeader: Forbidden header %s", header);
      return NO_ERR;
    }

    // TODO: Filter headers according to the specification.
    std::string whole_header(header);
    whole_header += ": ";
    if (value)
      whole_header += value;
    // TODO: Check what does curl do when set a header for multiple times.
    headers_ = curl_slist_append(headers_, whole_header.c_str());
    return NO_ERR;
  }

  virtual ExceptionCode Send(const char *data, size_t size) {
    if (state_ != OPENED || send_flag_) {
      LOG("XMLHttpRequest: Send: Invalid state: %d", state_);
      return INVALID_STATE_ERR;
    }

    if (!CheckSize(size, 0, 512)) {
      LOG("XMLHttpRequest: Size too big: %zu", size);
      return SYNTAX_ERR;
    }

    send_data_.assign(data, size);

  #ifdef _DEBUG
    curl_easy_setopt(curl_, CURLOPT_VERBOSE, 1);
  #endif
    curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, headers_);
    curl_easy_setopt(curl_, CURLOPT_FRESH_CONNECT, 1);
    curl_easy_setopt(curl_, CURLOPT_FORBID_REUSE, 1);
    curl_easy_setopt(curl_, CURLOPT_NOSIGNAL, 1);

    curl_easy_setopt(curl_, CURLOPT_READFUNCTION, ReadCallback);
    curl_easy_setopt(curl_, CURLOPT_READDATA, this);
    curl_easy_setopt(curl_, CURLOPT_HEADERFUNCTION, WriteHeaderCallback);
    curl_easy_setopt(curl_, CURLOPT_HEADERDATA, this);
    curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, WriteBodyCallback);
    curl_easy_setopt(curl_, CURLOPT_WRITEDATA, this);

    if (async_) {
      send_flag_ = true;
      curlm_ = curl_multi_init();
      curl_multi_setopt(curlm_, CURLMOPT_SOCKETFUNCTION, SocketCallback);
      curl_multi_setopt(curlm_, CURLMOPT_SOCKETDATA, this);
      CURLMcode code = curl_multi_add_handle(curlm_, curl_);
      if (code != CURLM_OK) {
        DLOG("XMLHttpRequest: Send: curl_multi_add_handle failed: %s",
             curl_multi_strerror(code));
        return NETWORK_ERR;
      }

      // As described in the spec, here don't change the state, but send
      // an event for historical reasons.
      ChangeState(OPENED);
      int still_running = 1;
      do {
        code = curl_multi_socket_all(curlm_, &still_running);
      } while (code == CURLM_CALL_MULTI_PERFORM);

      if (code != CURLM_OK) {
        DLOG("XMLHttpRequest: Send: curl_multi_perform failed: %s",
             curl_multi_strerror(code));
        return NETWORK_ERR;
      }

      if (!still_running) {
        DLOG("XMLHttpRequest: Send(async): DONE");
        Done();
      }

      // Prevent this object from being GC'ed during handling the request.
      if (script_context_)
        script_context_->LockObject(this);
    } else {
      // As described in the spec, here don't change the state, but send
      // an event for historical reasons.
      ChangeState(OPENED);
      CURLcode code = curl_easy_perform(curl_);
      if (code != CURLE_OK) {
        DLOG("XMLHttpRequest: Send: curl_easy_perform failed: %s",
             curl_easy_strerror(code));
        return NETWORK_ERR;
      }
      DLOG("XMLHttpRequest: Send(sync): DONE");
      Done();
    }
    return NO_ERR;
  }

  virtual ExceptionCode Send(const DOMDocumentInterface *data) {
    if (!data)
      return Send(static_cast<char *>(NULL), 0);

    const char *xml = data->GetXML();
    return Send(xml, strlen(xml));
  }

  void OnIOReady(int fd) {
    // DLOG("XMLHttpRequest: OnIOReady: %d", fd);
    int still_running = 1;
    CURLMcode code;
    do {
      code = curl_multi_socket(curlm_, socket_, &still_running);
    } while (code == CURLM_CALL_MULTI_PERFORM);

    if (code != CURLM_OK) {
      DLOG("XMLHttpRequest: OnIOReady: curl_multi_socket failed: %s",
           curl_multi_strerror(code));
      return;
    }

    if (!still_running) {
      DLOG("XMLHttpRequest: OnIOReady: DONE");
      Done();
    }
  }

  static int SocketCallback(CURL *handle, curl_socket_t socket, int type,
                            void *user_p, void *sock_p) {
    DLOG("XMLHttpRequest: SocketCallback: socket: %d, type: %d", socket, type);
    XMLHttpRequest* this_p = static_cast<XMLHttpRequest *>(user_p);
    ASSERT(this_p->curl_ == handle);

    if (this_p->socket_ == 0)
      this_p->socket_ = socket;
    else
      ASSERT(this_p->socket_ = socket);

    if (!this_p->socket_read_watch_ && (type & CURL_POLL_IN)) {
      this_p->socket_read_watch_ = this_p->host_->RegisterReadWatch(
          socket, NewSlot(this_p, &XMLHttpRequest::OnIOReady));
    }
    if (!this_p->socket_write_watch_ && (type & CURL_POLL_OUT)) {
      this_p->socket_write_watch_ = this_p->host_->RegisterWriteWatch(
          socket, NewSlot(this_p, &XMLHttpRequest::OnIOReady));
    }

    if (type & CURL_POLL_REMOVE)
      this_p->RemoveIOWatches();
    return 0;
  }

  static size_t ReadCallback(void *ptr, size_t size,
                             size_t mem_block, void *data) {
    // DLOG("XMLHttpRequest: ReadCallback: %zu*%zu", size, mem_block);
    XMLHttpRequest* this_p = static_cast<XMLHttpRequest *>(data);
    ASSERT(this_p);
    ASSERT(this_p->state_ == OPENED);
    ASSERT(!this_p->async_ || this_p->send_flag_);

    if (!CheckSize(this_p->send_data_.length(), size, mem_block)) {
      this_p->Abort();
      return 0;
    }

    size_t real_size = std::min(this_p->send_data_.length(), size * mem_block);
    strncpy(reinterpret_cast<char *>(ptr), this_p->send_data_.c_str(),
            real_size);
    this_p->send_data_.erase(0, real_size);
    return real_size;
  }

  static size_t WriteHeaderCallback(void *ptr, size_t size,
                                    size_t mem_block, void *data) {
    // DLOG("XMLHttpRequest: WriteHeaderCallback: %zu*%zu", size, mem_block);
    XMLHttpRequest* this_p = static_cast<XMLHttpRequest *>(data);
    ASSERT(this_p);
    ASSERT(this_p->state_ == OPENED);
    ASSERT(!this_p->async_ || this_p->send_flag_);

    if (!CheckSize(this_p->response_headers_.length(), size, mem_block)) {
      this_p->Abort();
      return 0;
    }

    size_t real_size = size * mem_block;
    this_p->response_headers_.append(reinterpret_cast<char *>(ptr), real_size);
    return real_size;
  }

  bool SplitStatusAndHeaders() {
    // RFC 2616 doesn't mentioned if HTTP/1.1 is case-sensitive, so implies
    // it is case-insensitive.
    // We only support HTTP version 1.0 or above.
    if (strncasecmp(response_headers_.c_str(), "HTTP/", 5) == 0) {
      // First split the status line and headers.
      std::string::size_type end_of_status = response_headers_.find("\r\n", 0);
      if (end_of_status == std::string::npos) {
        status_text_ = response_headers_;
        response_headers_.clear();
      } else {
        status_text_ = response_headers_.substr(0, end_of_status);
        response_headers_.erase(0, end_of_status + 2);
      }

      // Then extract the status text from the status line.
      std::string::size_type space_pos = status_text_.find(' ', 0);
      if (space_pos != std::string::npos) {
        space_pos = status_text_.find(' ', space_pos + 1);
        if (space_pos != std::string::npos)
          status_text_.erase(0, space_pos + 1);
      }
      // Otherwise, just leave the whole status line in status.
      return true;
    }
    // Otherwise already splitted.
    return false;
  }

  void ParseResponseHeaders() {
    // http://www.w3.org/Protocols/rfc2616/rfc2616-sec4.html#sec4.2
    // http://www.w3.org/TR/XMLHttpRequest
    std::string::size_type pos = 0;
    while (pos != std::string::npos) {
      std::string::size_type new_pos = response_headers_.find("\r\n", pos);
      std::string line;
      if (new_pos == std::string::npos) {
        line = response_headers_.substr(pos);
        pos = new_pos;
      } else {
        line = response_headers_.substr(pos, new_pos - pos);
        pos = new_pos + 2;
      }

      std::string name, value;
      std::string::size_type pos = line.find(':');
      if (pos != std::string::npos) {
        name = TrimString(line.substr(0, pos));
        value = TrimString(line.substr(pos + 1));
        if (!name.empty()) {
          CaseInsensitiveStringMap::iterator it =
              response_headers_map_.find(name);
          if (it == response_headers_map_.end()) {
            response_headers_map_.insert(std::make_pair(name, value));
          } else if (!value.empty()) {
            // According to XMLHttpRequest specification, the values of multiple
            // headers with the same name should be concentrated together.
            if (!it->second.empty())
              it->second += ", ";
            it->second += value;
          }
        }

        if (strcasecmp(name.c_str(), "Content-Type") == 0) {
          // Extract content type and encoding from the headers.
          pos = value.find(';');
          if (pos != std::string::npos) {
            response_content_type_ = TrimString(value.substr(0, pos));
            pos = value.find("encoding");
            if (pos != std::string::npos) {
              pos += 8;
              while (pos < value.length() &&
                     (isspace(value[pos]) || value[pos] == '=')) {
                pos++;
              }
              std::string::size_type pos1 = pos;
              while (pos1 < value.length() && !isspace(value[pos1]) &&
                     value[pos1] != ';') {
                pos1++;
              }
              response_encoding_ = value.substr(pos, pos1 - pos); 
            }
          } else {
            response_content_type_ = value;
          }
        }
      }
    }
  }

  static size_t WriteBodyCallback(void *ptr, size_t size,
                                  size_t mem_block, void *data) {
    // DLOG("XMLHttpRequest: WriteBodyCallback: %zu*%zu", size, mem_block);
    XMLHttpRequest *this_p = static_cast<XMLHttpRequest *>(data);
    ASSERT(this_p);
    ASSERT(this_p->state_ == OPENED || this_p->state_ == LOADING);
    ASSERT(!this_p->async_ || this_p->send_flag_);

    if (!CheckSize(this_p->response_body_.length(), size, mem_block)) {
      this_p->Abort();
      return 0;
    }

    if (this_p->state_ == OPENED) {
      this_p->SplitStatusAndHeaders();
      this_p->ParseResponseHeaders();
      this_p->ChangeState(HEADERS_RECEIVED);
      this_p->ChangeState(LOADING);
    }
    size_t real_size = size * mem_block;
    this_p->response_body_.append(static_cast<char *>(ptr), real_size);
    return real_size;
  }

  void RemoveIOWatches() {
    if (socket_read_watch_)
      host_->RemoveIOWatch(socket_read_watch_);
    if (socket_write_watch_)
      host_->RemoveIOWatch(socket_write_watch_);
    socket_read_watch_ = 0;
    socket_write_watch_ = 0;
  }

  void Done() {
    socket_ = 0;
    RemoveIOWatches();
    if ((state_ == OPENED && send_flag_) ||
        state_ == HEADERS_RECEIVED || state_ == LOADING)
      ChangeState(DONE);
    send_flag_ = false;

    if (async_ && script_context_)
      script_context_->UnlockObject(this);
  }

  virtual void Abort() {
    Done();

    if (curl_) {
      curl_easy_cleanup(curl_);
      curl_ = NULL;
    }
    if (curlm_) {
      curl_multi_cleanup(curlm_);
      curlm_ = NULL;
    }
    curl_slist_free_all(headers_);
    headers_ = NULL;

    response_headers_.clear();
    response_headers_map_.clear();
    response_body_.clear();
    response_text_.clear();
    send_data_.clear();
    status_text_.clear();
    if (response_dom_) {
      response_dom_->Detach();
      response_dom_ = NULL;
    }

    // Don't dispatch this state change event, according to the specification.
    state_ = UNSENT;
  }

  virtual ExceptionCode GetAllResponseHeaders(const char **result) {
    ASSERT(result);
    if (state_ == LOADING || state_ == DONE) {
      *result = response_headers_.c_str();
      return NO_ERR;
    }

    *result = NULL;
    return INVALID_STATE_ERR;
  }

  virtual ExceptionCode GetResponseHeader(const char *header,
                                          const char **result) {
    ASSERT(result);
    if (!header)
      return NULL_POINTER_ERR;

    *result = NULL;
    if (state_ == LOADING || state_ == DONE) {
      CaseInsensitiveStringMap::iterator it = response_headers_map_.find(
          header);
      if (it != response_headers_map_.end())
        *result = it->second.c_str();
      return NO_ERR;
    }
    return INVALID_STATE_ERR;
  }

  void DecodeResponseText() {
    std::string encoding(response_encoding_);
    const char *content_type = response_content_type_.c_str();
    std::string::size_type content_type_len = response_content_type_.length();

    if (content_type_len == 0 ||
        strcasecmp(content_type, "text/xml") == 0 ||
        strcasecmp(content_type, "application/xml") == 0 ||
        (content_type_len > 4 &&
         strcasecmp(content_type + content_type_len - 4, "+xml") == 0)) {
      // The content type is XML.
      response_dom_ = CreateDOMDocument();
      response_dom_->Attach();
      if (!ParseXMLIntoDOM(response_body_.c_str(), url_.c_str(), response_dom_,
                           &encoding)) {
        response_dom_->Detach();
        response_dom_ = NULL;
      }
    } else if (strcasecmp(content_type, "text/html") == 0) {
      // The content type is HTML.
      // The spec 20071026 doesn't have this feature 
      response_dom_ = CreateDOMDocument();
      response_dom_->Attach();
      if (!ParseHTMLIntoDOM(response_body_.c_str(), url_.c_str(), response_dom_,
                            &encoding)) {
        response_dom_->Detach();
        response_dom_ = NULL;
      }
    }

    // NOTE: Here the priority of encodings does not conform to XMLHttpRequest
    // specification (and XML, HTTP, HTML specifications), because for now
    // libxml2 doesn't allow specifying external encoding.
    if (encoding.empty())
      encoding = response_encoding_;
    ConvertStringToUTF8(response_body_, &encoding, &response_text_);
  }

  virtual ExceptionCode GetResponseText(const char **result) {
    ASSERT(result);

    if (state_ == LOADING) {
      // Though the spec allows getting responseText while loading, we can't
      // afford this because we rely on XML/HTML parser to get the encoding. 
      *result = "";
      return NO_ERR;
    } else if (state_ == DONE) {
      if (response_text_.empty() && !response_body_.empty())
        DecodeResponseText();

      *result = response_text_.c_str();
      return NO_ERR;
    }

    *result = NULL;
    return INVALID_STATE_ERR;
  }

  virtual ExceptionCode GetResponseBody(const char **result, size_t *size) {
    ASSERT(result);
    ASSERT(size);

    if (state_ == LOADING || state_ == DONE) {
      *size = response_body_.length();
      *result = response_body_.c_str();
      return NO_ERR;
    }

    *size = 0;
    *result = NULL;
    return INVALID_STATE_ERR;
  }

  virtual ExceptionCode GetResponseXML(DOMDocumentInterface **result) {
    ASSERT(result);

    if (state_ == DONE) {
      if (!response_dom_ && !response_body_.empty())
        DecodeResponseText();

      *result = response_dom_;
      return NO_ERR;
    }

    result = NULL;
    return INVALID_STATE_ERR;
  }

  virtual ExceptionCode GetStatus(unsigned short *result) {
    ASSERT(result);

    if (state_ == LOADING || state_ == DONE) {
      long curl_status = 0;
      if (curl_)
        curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &curl_status);
      *result = static_cast<unsigned short>(curl_status);
      return NO_ERR;
    }

    *result = 0;
    return INVALID_STATE_ERR;
  }

  virtual ExceptionCode GetStatusText(const char **result) {
    ASSERT(result);

    if (state_ == LOADING || state_ == DONE) {
      *result = status_text_.c_str();
      return NO_ERR;
    }

    *result = NULL;
    return INVALID_STATE_ERR;
  }

  class XMLHttpRequestException : public ScriptableHelper<ScriptableInterface> {
   public:
    DEFINE_CLASS_ID(0x277d75af73674d06, ScriptableInterface);

    XMLHttpRequestException(ExceptionCode code) : code_(code) {
      RegisterSimpleProperty("code", &code_);
    }
    // This is a script owned object.
    virtual OwnershipPolicy Attach() { return OWNERSHIP_TRANSFERRABLE; }
    virtual bool Detach() { delete this; return true; }

   private:
    ExceptionCode code_;
  };

  // Used in the methods for script to throw an script exception on errors.
  bool CheckException(ExceptionCode code) {
    if (code != NO_ERR) {
      DLOG("XMLHttpRequest: Set pending exception: %d", code);
      SetPendingException(new XMLHttpRequestException(code));
      return false;
    }
    return true;
  }

  void ScriptOpen(const char *method, const char *url, bool async,
                  const char *user, const char *password) {
    CheckException(Open(method, url, async, user, password));
  }

  void ScriptSetRequestHeader(const char *header, const char *value) {
    CheckException(SetRequestHeader(header, value));
  }

  void ScriptSend(const Variant &v_data) {
    if (v_data.type() == Variant::TYPE_STRING) {
      std::string data = VariantValue<std::string>()(v_data);
      CheckException(Send(data.c_str(), data.length()));
    } else if (v_data.type() == Variant::TYPE_SCRIPTABLE) {
      ScriptableInterface *scriptable =
          VariantValue<ScriptableInterface *>()(v_data);
      if (!scriptable ||
          scriptable->IsInstanceOf(DOMDocumentInterface::CLASS_ID)) {
        CheckException(Send(down_cast<DOMDocumentInterface *>(scriptable)));
      } else {
        CheckException(SYNTAX_ERR);
      }
    } else {
      CheckException(SYNTAX_ERR);
    }
  }

  const char *ScriptGetAllResponseHeaders() {
    const char *result = NULL;
    CheckException(GetAllResponseHeaders(&result));
    return result;
  }

  const char *ScriptGetResponseHeader(const char *header) {
    const char *result = NULL;
    CheckException(GetResponseHeader(header, &result));
    return result;
  }

  ScriptableBinaryData *ScriptGetResponseBody() {
    const char *result = NULL;
    size_t size = 0;
    if (CheckException(GetResponseBody(&result, &size)))
      return new ScriptableBinaryData(result, size);
    return NULL;
  }

  const char *ScriptGetResponseText() {
    const char *result = NULL;
    CheckException(GetResponseText(&result));
    return result;
  }

  DOMDocumentInterface *ScriptGetResponseXML() {
    DOMDocumentInterface *result = NULL;
    CheckException(GetResponseXML(&result));
    return result;
  }

  unsigned short ScriptGetStatus() {
    unsigned short result = 0;
    CheckException(GetStatus(&result));
    return result;
  }

  const char *ScriptGetStatusText() {
    const char *result = NULL;
    CheckException(GetStatusText(&result));
    return result;
  }

  GadgetHostInterface *host_;
  ScriptContextInterface *script_context_;
  Signal0<void> onreadystatechange_signal_;

  std::string url_;
  bool async_;

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
  std::string response_content_type_;
  std::string response_encoding_;
  std::string status_text_;
  std::string response_body_;
  std::string response_text_;
  DOMDocumentInterface *response_dom_;
  CaseInsensitiveStringMap response_headers_map_;
};

} // anonymous namespace

XMLHttpRequestInterface *CreateXMLHttpRequest(
    GadgetHostInterface *host, ScriptContextInterface *script_context) {
  return new XMLHttpRequest(host, script_context);
}

} // namespace ggadget
