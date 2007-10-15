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
#include "xml_http_request.h"
#include "gadget_host_interface.h"
#include "scriptable_helper.h"
#include "signal.h"

namespace ggadget {

class XMLHttpRequest::Impl {
 public:
  Impl(GadgetHostInterface *host)
      : host_(host),
        async_(false),
        curl_(NULL),
        curlm_(NULL),
        socket_(0),
        socket_read_watch_(0),
        socket_write_watch_(0),
        state_(UNSENT),
        send_flag_(false),
        headers_(NULL) {
    // TODO: Register Scriptables.
  }

  ~Impl() {
    Abort();
  }

  Connection *ConnectOnReadyStateChange(Slot0<void> *handler) {
    return onreadystatechange_signal_.Connect(handler);
  }

  XMLHttpRequestInterface::State GetReadyState() {
    return state_;
  }

  void ChangeState(State new_state) {
    state_ = new_state;
    onreadystatechange_signal_();
  }

  bool Open(const char *method, const char *url, bool async,
            const char *user, const char *password) {
    Abort();
    curl_ = curl_easy_init();
    if (!curl_) {
      DLOG("XMLHttpRequest: curl_easy_init failed");
      // TODO: Send errors.
      return false;
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
      return false;
    }
    curl_easy_setopt(curl_, CURLOPT_URL, url);

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
    ChangeState(OPEN);
    return true;
  }

  bool SetRequestHeader(const char *header, const char *value) {
    if (state_ != OPEN || send_flag_) {
      LOG("XMLHttpRequest: SetRequestHeader: Invalid state: %d", state_);
      return false;
    }
    // TODO: Filter headers according to the specification.
    std::string whole_header(header);
    whole_header += ": ";
    if (value)
      whole_header += value;
    // TODO: Check what does curl do when set a header for multiple times.
    headers_ = curl_slist_append(headers_, whole_header.c_str());
    return true;
  }

  bool Send(const char *data) {
    if (state_ != OPEN || send_flag_) {
      LOG("XMLHttpRequest: Send: Invalid state: %d", state_);
      return false;
    }
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
        return false;
      }

      // As described in the spec, here don't change the state, but send
      // an event for historical reasons.
      ChangeState(OPEN);
      int still_running = 1;
      do {
        code = curl_multi_socket_all(curlm_, &still_running);
      } while (code == CURLM_CALL_MULTI_PERFORM);

      if (code != CURLM_OK) {
        DLOG("XMLHttpRequest: Send: curl_multi_perform failed: %s",
             curl_multi_strerror(code));
        return false;
      }

      if (!still_running) {
        DLOG("XMLHttpRequest: Send(async): DONE");
        Done();
      }
    } else {
      // As described in the spec, here don't change the state, but send
      // an event for historical reasons.
      ChangeState(OPEN);
      CURLcode code = curl_easy_perform(curl_);
      if (code != CURLE_OK) {
        DLOG("XMLHttpRequest: Send: curl_easy_perform failed: %s",
             curl_easy_strerror(code));
        return false;
      }
      DLOG("XMLHttpRequest: Send(sync): DONE");
      Done();
    }
    return true;
  }

  bool Send(const DOMDocument *data) {
    // TODO:
    return true;
  }

  void OnIOReady(int fd) {
    DLOG("XMLHttpRequest: OnIOReady: %d", fd);
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
    DLOG("SocketCallback: the socket: %d, type: %d\n", socket, type);
    Impl* this_p = reinterpret_cast<Impl *>(user_p);
    ASSERT(this_p->curl_ == handle);

    if (this_p->socket_ == 0)
      this_p->socket_ = socket;
    else
      ASSERT(this_p->socket_ = socket);

    if (!this_p->socket_read_watch_ && (type & CURL_POLL_IN)) {
      this_p->socket_read_watch_ = this_p->host_->RegisterReadWatch(
          socket, NewSlot(this_p, &Impl::OnIOReady));
    }
    if (!this_p->socket_write_watch_ && (type & CURL_POLL_OUT)) {
      this_p->socket_write_watch_ = this_p->host_->RegisterWriteWatch(
          socket, NewSlot(this_p, &Impl::OnIOReady));
    }

    if (type & CURL_POLL_REMOVE) {
      if (this_p->socket_read_watch_)
        this_p->host_->RemoveIOWatch(this_p->socket_read_watch_);
      if (this_p->socket_write_watch_)
        this_p->host_->RemoveIOWatch(this_p->socket_write_watch_);
      this_p->socket_read_watch_ = 0;
      this_p->socket_write_watch_ = 0;
    }
    return 0;
  }

  static size_t ReadCallback(void *ptr, size_t size,
                             size_t mem_block, void *data) {
    DLOG("XMLHttpRequest: ReadCallback: %d*%d", size, mem_block);
    Impl *this_p = reinterpret_cast<Impl *>(data);
    ASSERT(this_p);
    ASSERT(this_p->state_ == OPEN);
    ASSERT(!this_p->async_ || this_p->send_flag_);

    size_t real_size = std::min(this_p->send_data_.length(), size * mem_block);
    strncpy(reinterpret_cast<char *>(ptr), this_p->send_data_.c_str(),
            real_size);
    this_p->send_data_.erase(0, real_size);
    return real_size;
  }

  static size_t WriteHeaderCallback(void *ptr, size_t size,
                                    size_t mem_block, void *data) {
    DLOG("XMLHttpRequest: WriteHeaderCallback: %d*%d", size, mem_block);
    Impl *this_p = reinterpret_cast<Impl *>(data);
    ASSERT(this_p);
    ASSERT(this_p->state_ == OPEN);
    ASSERT(!this_p->async_ || this_p->send_flag_);

    size_t real_size = size * mem_block;
    this_p->response_headers_.append(reinterpret_cast<char *>(ptr), real_size);
    return real_size;
  }

  static bool SplitStatusAndHeaders(std::string *headers, std::string *status) {
    // RFC 2616 doesn't mentioned if HTTP/1.1 is case-sensitive, so implies
    // it is case-insensitive.
    // We only support HTTP version 1.0 or above.
    if (strncasecmp(headers->c_str(), "HTTP/", 5) == 0) {
      // First split the status line and headers.
      std::string::size_type end_of_status = headers->find("\r\n", 0);
      if (end_of_status == std::string::npos) {
        *status = *headers;
        headers->erase();
      } else {
        *status = headers->substr(0, end_of_status);
        headers->erase(0, end_of_status + 2);
      }

      // Then extract the status text from the status line.
      std::string::size_type space_pos = status->find(' ', 0);
      if (space_pos != std::string::npos) {
        space_pos = status->find(' ', space_pos + 1);
        if (space_pos != std::string::npos)
          status->erase(0, space_pos + 1);
      }
      // Otherwise, just leave the whole status line in status.
      return true;
    }
    // Otherwise already splitted.
    return false;
  }

  static void ParseResponseHeaders(const std::string &headers,
                                   CaseInsensitiveStringMap *map) {
    // http://www.w3.org/Protocols/rfc2616/rfc2616-sec4.html#sec4.2
    // http://www.w3.org/TR/XMLHttpRequest
    std::string::size_type pos = 0;
    while (pos != std::string::npos) {
      std::string::size_type new_pos = headers.find("\r\n", pos);
      std::string line;
      if (new_pos == std::string::npos) {
        line = headers.substr(pos);
        pos = new_pos;
      } else {
        line = headers.substr(pos, new_pos - pos);
        pos = new_pos + 2;
      }

      std::string name, value;
      std::string::size_type separator = line.find(':');
      if (separator != std::string::npos) {
        name = TrimString(line.substr(0, separator));
        value = TrimString(line.substr(separator + 1));
        if (!name.empty()) {
          CaseInsensitiveStringMap::iterator it = map->find(name);
          if (it == map->end()) {
            map->insert(std::make_pair(name, value));
          } else if (!value.empty()) {
            // According to XMLHttpRequest specification, the values of multiple
            // headers with the same name should be concentrated together.
            if (!it->second.empty())
              it->second += ", ";
            it->second += value;
          }
        }
      }
    }
  }

  static size_t WriteBodyCallback(void *ptr, size_t size,
                                  size_t mem_block, void *data) {
    DLOG("XMLHttpRequest: WriteBodyCallback: %d*%d", size, mem_block);
    Impl *this_p = reinterpret_cast<Impl *>(data);
    ASSERT(this_p);
    ASSERT(this_p->state_ == OPEN || this_p->state_ == LOADING);
    ASSERT(!this_p->async_ || this_p->send_flag_);

    if (this_p->state_ == OPEN) {
      SplitStatusAndHeaders(&this_p->response_headers_, &this_p->status_text_);
      ParseResponseHeaders(this_p->response_headers_,
                           &this_p->response_headers_map_);
      this_p->ChangeState(SENT);
      this_p->ChangeState(LOADING);
    }
    size_t real_size = size * mem_block;
    this_p->response_body_.append(reinterpret_cast<char *>(ptr), real_size);
    return real_size;
  }

  void Done() {
    socket_ = 0;
    if (socket_read_watch_)
      host_->RemoveIOWatch(socket_read_watch_);
    if (socket_write_watch_)
      host_->RemoveIOWatch(socket_write_watch_);
    socket_read_watch_ = 0;
    socket_write_watch_ = 0;

    if ((state_ == OPEN && send_flag_) || state_ == SENT || state_ == LOADING)
      ChangeState(DONE);
    send_flag_ = false;
  }

  void Abort() {
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
    send_data_.clear();
    status_text_.clear();

    // Don't dispatch this state change event, according to the specification.
    state_ = UNSENT;
  }

  const char *GetAllResponseHeaders() {
    if (state_ == LOADING || state_ == DONE)
      return response_headers_.c_str();
    return NULL;
  }

  const char *GetResponseHeader(const char *header) {
    if (state_ == LOADING || state_ == DONE) {
      CaseInsensitiveStringMap::iterator it = response_headers_map_.find(
          header);
      return it == response_headers_map_.end() ? NULL : it->second.c_str();
    }
    return NULL;
  }

  const char *GetResponseBody(size_t *size) {
    if (state_ == LOADING || state_ == DONE) {
      *size = response_body_.length();
      return response_body_.c_str();
    }
    *size = 0;
    return NULL;
  }

  DOMDocument *GetResponseXML() {
    if (state_ == DONE) {
      // TODO:
    }
    return NULL;
  }

  unsigned short GetStatus() {
    long curl_status = 0;
    if (curl_)
      curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &curl_status);
    return static_cast<unsigned short>(curl_status);
  }

  const char *GetStatusText() {
    if (state_ == LOADING || state_ == DONE)
      return status_text_.c_str();
    return NULL;
  }

  DELEGATE_SCRIPTABLE_REGISTER(scriptable_helper_);

  ScriptableHelper scriptable_helper_;
  GadgetHostInterface *host_;
  Signal0<void> onreadystatechange_signal_;

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

  CaseInsensitiveStringMap response_headers_map_;
};

XMLHttpRequest::XMLHttpRequest(GadgetHostInterface *host)
    : impl_(new Impl(host)) {
}

XMLHttpRequest::~XMLHttpRequest() {
  delete impl_;
  impl_ = NULL;
}

Connection *XMLHttpRequest::ConnectOnReadyStateChange(Slot0<void> *handler) {
  return impl_->ConnectOnReadyStateChange(handler);
}

XMLHttpRequestInterface::State XMLHttpRequest::GetReadyState() {
  return impl_->GetReadyState();
}

bool XMLHttpRequest::Open(const char *method, const char *url, bool async,
                          const char *user, const char *password) {
  return impl_->Open(method, url, async, user, password);
}

bool XMLHttpRequest::SetRequestHeader(const char *header, const char *value) {
  return impl_->SetRequestHeader(header, value);
}

bool XMLHttpRequest::Send(const char *data) {
  return impl_->Send(data);
}

bool XMLHttpRequest::Send(const DOMDocument *data) {
  return impl_->Send(data);
}

void XMLHttpRequest::Abort() {
  impl_->Abort();
}

const char *XMLHttpRequest::GetAllResponseHeaders() {
  return impl_->GetAllResponseHeaders();
}

const char *XMLHttpRequest::GetResponseHeader(const char *header) {
  return impl_->GetResponseHeader(header);
}

const char *XMLHttpRequest::GetResponseBody(size_t *size) {
  return impl_->GetResponseBody(size);
}

DOMDocument *XMLHttpRequest::GetResponseXML() {
  return impl_->GetResponseXML();
}

unsigned short XMLHttpRequest::GetStatus() {
  return impl_->GetStatus();
}

const char *XMLHttpRequest::GetStatusText() {
  return impl_->GetStatusText();
}

DELEGATE_SCRIPTABLE_INTERFACE_IMPL(XMLHttpRequest, impl_->scriptable_helper_)

} // namespace ggadget
