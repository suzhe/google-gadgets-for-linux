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

#include <algorithm>
#include <vector>
#include <string>

#ifdef _DEBUG
#include <QtCore/QTextStream>
#endif
#include <QtCore/QUrl>
#include <QtCore/QBuffer>
#include <QtNetwork/QHttp>
#include <QtNetwork/QHttpHeader>

#define COOKIE_SUPPORT 0     // TODO: Cookie support is not ready

#if COOKIE_SUPPORT
#include <QtNetwork/QNetworkCookie>
#endif

#include <ggadget/backoff.h>
#include <ggadget/gadget_consts.h>
#include <ggadget/main_loop_interface.h>
#include <ggadget/logger.h>
#include <ggadget/options_interface.h>
#include <ggadget/scriptable_binary_data.h>
#include <ggadget/script_context_interface.h>
#include <ggadget/scriptable_helper.h>
#include <ggadget/signals.h>
#include <ggadget/string_utils.h>
#include <ggadget/xml_http_request_interface.h>
#include <ggadget/xml_dom_interface.h>
#include <ggadget/xml_parser_interface.h>
#include "qt_xml_http_request_internal.h"

namespace ggadget {
namespace qt {

// The name of the options to store backoff data.
static const char kBackoffOptions[] = "backoff";
// The name of the options item to store backoff data.
static const char kBackoffDataOption[] = "backoff";

static const Variant kOpenDefaultArgs[] = {
  Variant(), Variant(),
  Variant(true),
  Variant(static_cast<const char *>(NULL)),
  Variant(static_cast<const char *>(NULL))
};

static const Variant kSendDefaultArgs[] = { Variant("") };

static Backoff::ResultType GetBackoffType(unsigned short status) {
  // status == 0: network error, don't do exponential backoff.
  return status == 0 ? Backoff::CONSTANT_BACKOFF :
         (status >= 200 && status < 400) || status == 404 ? Backoff::SUCCESS :
         Backoff::EXPONENTIAL_BACKOFF;
}

// field-name     = token
// token          = 1*<any CHAR except CTLs or separators>
// separators     = "(" | ")" | "<" | ">" | "@"
//                | "," | ";" | ":" | "\" | <">
//                | "/" | "[" | "]" | "?" | "="
//                | "{" | "}" | SP | HT
static bool IsValidHTTPToken(const char *s) {
  if (s == NULL) return false;
  while (*s) {
    if (*s > 32 && *s < 127 &&
        (isalnum(*s) || strchr("!#$%&'*+ -.^_`~", *s) != NULL)) {
      //valid case
    } else {
      return false;
    }
    ++s;
  }
  return true;
}

// field-value    = *( field-content | LWS )
// field-content  = <the OCTETs making up the field-value
//                  and consisting of either *TEXT or combinations
//                  of token, separators, and quoted-string>
// TEXT           = <any OCTET except CTLs, but including LWS>
static bool IsValidHTTPHeaderValue(const char *s) {
  if (s == NULL) return true;
  while (*s) {
    if ( (*s > 0 && *s<=31
#if 0 // Disallow \r \n \t in header values.
         && *s != '\r' && *s != '\n' && *s != '\t'
#endif
         ) ||
         *s == 127)
      return false;
    ++s;
  }
  return true;
}

class Session {
 public:
#if COOKIE_SUPPORT
  void RestoreCookie(QHttpRequestHeader *header) {
    QString str;
    for (int i = 0; i < cookies_.size(); i++) {
      str += cookies_[i].toRawForm(QNetworkCookie::NameAndValueOnly);
      if (i < cookies_.size() - 1) str += "; ";
    }
    if (!str.isEmpty()) header->setValue("Cookie", str);
    DLOG("Cookie:%s", str.toStdString().c_str());
  }
  void SaveCookie(const QHttpResponseHeader& header) {
    QStringList list = header.allValues("Set-Cookie");
    if (list.size() != 0) DLOG("Get Cookie Line: %d", list.size());
    for (int i = 0; i < list.size(); i++) {
      QList<QNetworkCookie> cookies = QNetworkCookie::parseCookies(list[i].toAscii());
      cookies_ += cookies;
    }
  }
  QList<QNetworkCookie> cookies_;
#else
  void RestoreCookie(QHttpRequestHeader *header) {}
  void SaveCookie(const QHttpResponseHeader& header) {}
#endif
};

class XMLHttpRequest : public ScriptableHelper<XMLHttpRequestInterface> {
 public:
  DEFINE_CLASS_ID(0xa34d00e04d0acfbb, XMLHttpRequestInterface);

  XMLHttpRequest(Session *session, MainLoopInterface *main_loop,
                 XMLParserInterface *xml_parser,
                 const QString &default_user_agent)
      : main_loop_(main_loop),
        xml_parser_(xml_parser),
        default_user_agent_(default_user_agent),
        http_(NULL),
        request_header_(NULL),
        session_(session),
        handler_(NULL),
        send_data_(NULL),
        async_(false),
        state_(UNSENT),
        send_flag_(false),
        status_(0),
        succeeded_(false),
        response_dom_(NULL) {
    VERIFY_M(EnsureBackoffOptions(main_loop->GetCurrentTime()),
             ("Required options module have not been loaded"));
  }

  ~XMLHttpRequest() {
    Abort();
  }

  static bool EnsureBackoffOptions(uint64_t now) {
    if (!backoff_options_) {
      backoff_options_ = CreateOptions(kBackoffOptions);
      if (backoff_options_) {
        std::string data;
        Variant value = backoff_options_->GetValue(kBackoffDataOption);
        if (value.ConvertToString(&data))
          backoff_.SetData(now, data.c_str());
      }
    }
    return backoff_options_ != NULL;
  }

  static void SaveBackoffData(uint64_t now) {
    if (EnsureBackoffOptions(now)) {
      backoff_options_->PutValue(kBackoffDataOption,
                                 Variant(backoff_.GetData(now)));
      backoff_options_->Flush();
    }
  }

  virtual void DoClassRegister() {
    RegisterClassSignal("onreadystatechange",
                       &XMLHttpRequest::onreadystatechange_signal_);
    RegisterProperty("readyState",
                     NewSlot(&XMLHttpRequest::GetReadyState), NULL);
    RegisterMethod("open",
        NewSlotWithDefaultArgs(NewSlot(&XMLHttpRequest::ScriptOpen),
                               kOpenDefaultArgs));
    RegisterMethod("setRequestHeader",
                   NewSlot(&XMLHttpRequest::ScriptSetRequestHeader));
    RegisterMethod("send",
        NewSlotWithDefaultArgs(NewSlot(&XMLHttpRequest::ScriptSend),
                               kSendDefaultArgs));
    RegisterMethod("abort", NewSlot(&XMLHttpRequest::Abort));
    RegisterMethod("getAllResponseHeaders",
                   NewSlot(&XMLHttpRequest::ScriptGetAllResponseHeaders));
    RegisterMethod("getResponseHeader",
                   NewSlot(&XMLHttpRequest::ScriptGetResponseHeader));
    RegisterProperty("responseStream",
                     NewSlot(&XMLHttpRequest::ScriptGetResponseBody),
                     NULL);
    RegisterProperty("responseBody",
                     NewSlot(&XMLHttpRequest::ScriptGetResponseBody),
                     NULL);
    RegisterProperty("responseText",
                     NewSlot(&XMLHttpRequest::ScriptGetResponseText),
                     NULL);
    RegisterProperty("responseXML",
                     NewSlot(&XMLHttpRequest::ScriptGetResponseXML),
                     NULL);
    RegisterProperty("status", NewSlot(&XMLHttpRequest::ScriptGetStatus),
                     NULL);
    RegisterProperty("statusText",
                     NewSlot(&XMLHttpRequest::ScriptGetStatusText), NULL);
  }

  virtual Connection *ConnectOnReadyStateChange(Slot0<void> *handler) {
    return onreadystatechange_signal_.Connect(handler);
  }

  virtual State GetReadyState() {
    return state_;
  }

  bool ChangeState(State new_state) {
    DLOG("XMLHttpRequest: ChangeState from %d to %d this=%p",
         state_, new_state, this);
    state_ = new_state;
    onreadystatechange_signal_();
    // ChangeState may re-entered during the signal, so the current state_
    // may be different from the input parameter.
    return state_ == new_state;
  }

  // The maximum data size of this class can process.
  static const size_t kMaxDataSize = 8 * 1024 * 1024;

  static bool CheckSize(size_t current, size_t num_blocks, size_t block_size) {
    return current < kMaxDataSize && block_size > 0 &&
           (kMaxDataSize - current) / block_size > num_blocks;
  }

  ExceptionCode OpenInternal(const char *url) {
    QUrl qurl(url);
    if (!qurl.isValid()) return SYNTAX_ERR;

    QHttp::ConnectionMode mode;

    if (qurl.scheme().toLower() == "https")
      mode = QHttp::ConnectionModeHttps;
    else if (qurl.scheme().toLower() == "http")
      mode = QHttp::ConnectionModeHttp;
    else
      return SYNTAX_ERR;

    if (!qurl.userName().isEmpty() || !qurl.password().isEmpty()) {
      // GDWin Compatibility.
      DLOG("Username:password in URL is not allowed: %s", url);
      return SYNTAX_ERR;
    }

    url_ = url;
    host_ = qurl.host().toStdString();
    http_ = new QHttp(qurl.host(), mode);
    http_->setUser(user_, password_);
    handler_ = new HttpHandler(this, http_);

    std::string path = "/";
    size_t sep = url_.find('/', qurl.scheme().length() + strlen("://"));
    if (sep != std::string::npos) path = url_.substr(sep);

    request_header_ = new QHttpRequestHeader(method_, path.c_str());
    request_header_->setValue("Host", host_.c_str());
    if (!default_user_agent_.isEmpty())
      request_header_->setValue("User-Agent", default_user_agent_);
    DLOG("HOST: %s, PATH: %s", host_.c_str(), path.c_str());
    return NO_ERR;
  }

  virtual ExceptionCode Open(const char *method, const char *url, bool async,
                             const char *user, const char *password) {
    DLOG("Open %s with %s", url, method);
    Abort();

    if (strcasecmp(method, "HEAD") != 0 && strcasecmp(method, "GET") != 0
        && strcasecmp(method, "POST") != 0) {
      LOG("XMLHttpRequest: Unsupported method: %s", method);
      return SYNTAX_ERR;
    }
    method_ = method;
    async_ = async;
    user_ = user;
    password_ = password;
    ExceptionCode code = OpenInternal(url);
    if (code != NO_ERR) return code;
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

    if (!IsValidHTTPToken(header)) {
      LOG("XMLHttpRequest::SetRequestHeader: Invalid header %s", header);
      return SYNTAX_ERR;
    }

    if (!IsValidHTTPHeaderValue(value)) {
      LOG("XMLHttpRequest::SetRequestHeader: Invalid value: %s", value);
      return SYNTAX_ERR;
    }

    if (strncasecmp("Proxy-", header, 6) == 0 ||
        strncasecmp("Sec-", header, 4) == 0) {
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
    request_header_->setValue(header, value);
    return NO_ERR;
  }

  virtual ExceptionCode Send(const char *data, size_t size) {
    if (state_ != OPENED || send_flag_) {
      LOG("XMLHttpRequest: Send: Invalid state: %d", state_);
      return INVALID_STATE_ERR;
    }

    if (!CheckSize(size, 0, 512)) {
      LOG("XMLHttpRequest: Send: Size too big: %zu", size);
      return SYNTAX_ERR;
    }

    // As described in the spec, here don't change the state, but send
    // an event for historical reasons.
    if (!ChangeState(OPENED))
      return INVALID_STATE_ERR;

    send_flag_ = true;
    if (async_) {
      // Add an internal reference when this request is working to prevent
      // this object from being GC'ed.
      Ref();
      // Do backoff checking to avoid DDOS attack to the server.
      if (!backoff_.IsOkToRequest(main_loop_->GetCurrentTime(),
                                  host_.c_str())) {
        Abort();
        // Don't raise exception here because async callers might not expect
        // this kind of exception.
        return NO_ERR;
      }
      if (session_)
        session_->RestoreCookie(request_header_);

      if (data && size > 0) {
        send_data_ = new QByteArray(data, static_cast<int>(size));
        http_->request(*request_header_, *send_data_);
      } else {
        http_->request(*request_header_);
      }
    } else {
      // Do backoff checking to avoid DDOS attack to the server.
      if (!backoff_.IsOkToRequest(main_loop_->GetCurrentTime(),
                                  host_.c_str())) {
        Abort();
        return NETWORK_ERR;
      }
      ASSERT(0);
      return NETWORK_ERR;
    }
    return NO_ERR;
  }

  virtual ExceptionCode Send(const DOMDocumentInterface *data) {
    if (!data)
      return Send(static_cast<char *>(NULL), 0);

    std::string xml = data->GetXML();
    return Send(xml.c_str(), xml.size());
  }

  void Done(bool aborting, bool succeeded) {
    bool save_send_flag = send_flag_;
    bool save_async = async_;
    // Set send_flag_ to false early, to prevent problems when Done() is
    // re-entered.
    send_flag_ = false;
    succeeded_ = succeeded_;
    bool no_unexpected_state_change = true;
    if ((state_ == OPENED && save_send_flag) ||
        state_ == HEADERS_RECEIVED || state_ == LOADING) {
      uint64_t now = main_loop_->GetCurrentTime();
      if (!aborting &&
          backoff_.ReportRequestResult(now, host_.c_str(),
                                       GetBackoffType(status_))) {
        SaveBackoffData(now);
      }
      // The caller may call Open() again in the OnReadyStateChange callback,
      // which may cause Done() re-entered.
      no_unexpected_state_change = ChangeState(DONE);
    }

    if (aborting && no_unexpected_state_change) {
      // Don't dispatch this state change event, according to the spec.
      state_ = UNSENT;
    }

    if (save_send_flag && save_async) {
      // Remove the internal reference that was added when the request was
      // started.
      Unref();
    }
  }

  void FreeResource() {
    if (handler_) {
      delete handler_;
      handler_ = NULL;
    }
    if (request_header_) {
      delete request_header_;
      request_header_ = NULL;
    }
    if (http_) {
      delete http_;
      http_ = NULL;
    }
    response_headers_.clear();
    response_headers_map_.clear();
    response_body_.clear();
    response_text_.clear();
    status_ = 0;
    status_text_.clear();
    if (response_dom_) {
      response_dom_->Unref();
      response_dom_ = NULL;
    }

    if (send_data_) {
      delete send_data_;
      send_data_ = NULL;
    }
  }

  virtual void Abort() {
    FreeResource();
    Done(true, false);
  }

  virtual ExceptionCode GetAllResponseHeaders(const char **result) {
    ASSERT(result);
    if (state_ == LOADING || state_ == DONE) {
      *result = response_headers_.c_str();
      return NO_ERR;
    }

    *result = NULL;
    LOG("XMLHttpRequest: GetAllResponseHeaders: Invalid state: %d", state_);
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
    LOG("XMLHttpRequest: GetRequestHeader: Invalid state: %d", state_);
    return INVALID_STATE_ERR;
  }

  void DecodeResponseText() {
    std::string encoding;
    response_dom_ = xml_parser_->CreateDOMDocument();
    response_dom_->Ref();
    if (!xml_parser_->ParseContentIntoDOM(response_body_, NULL, url_.c_str(),
                                          response_content_type_.c_str(),
                                          response_encoding_.c_str(),
                                          kEncodingFallback,
                                          response_dom_,
                                          &encoding, &response_text_) ||
        !response_dom_->GetDocumentElement()) {
      response_dom_->Unref();
      response_dom_ = NULL;
    }
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
    LOG("XMLHttpRequest: GetResponseText: Invalid state: %d", state_);
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
    LOG("XMLHttpRequest: GetResponseBody: Invalid state: %d", state_);
    return INVALID_STATE_ERR;
  }

  virtual ExceptionCode GetResponseBody(std::string *result) {
    ASSERT(result);

    if (state_ == LOADING || state_ == DONE) {
      *result = response_body_;
      return NO_ERR;
    }

    result->clear();
    LOG("XMLHttpRequest: GetResponseBody: Invalid state: %d", state_);
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
    LOG("XMLHttpRequest: GetResponseXML: Invalid state: %d", state_);
    return INVALID_STATE_ERR;
  }

  virtual ExceptionCode GetStatus(unsigned short *result) {
    ASSERT(result);

    if (state_ == LOADING || state_ == DONE) {
      *result = status_;
      return NO_ERR;
    }

    *result = 0;
    LOG("XMLHttpRequest: GetStatus: Invalid state: %d", state_);
    return INVALID_STATE_ERR;
  }

  virtual ExceptionCode GetStatusText(const char **result) {
    ASSERT(result);

    if (state_ == LOADING || state_ == DONE) {
      *result = status_text_.c_str();
      return NO_ERR;
    }

    *result = NULL;
    LOG("XMLHttpRequest: GetStatusText: Invalid state: %d", state_);
    return INVALID_STATE_ERR;
  }

  virtual bool IsSuccessful() {
    return succeeded_;
  }

  class XMLHttpRequestException : public ScriptableHelperDefault {
   public:
    DEFINE_CLASS_ID(0x277d75af73674d06, ScriptableInterface);

    XMLHttpRequestException(ExceptionCode code) : code_(code) {
      RegisterSimpleProperty("code", &code_);
      RegisterMethod("toString",
                     NewSlot(this, &XMLHttpRequestException::ToString));
    }

    std::string ToString() const {
      const char *name;
      switch (code_) {
        case INVALID_STATE_ERR: name = "Invalid State"; break;
        case SYNTAX_ERR: name = "Syntax Error"; break;
        case SECURITY_ERR: name = "Security Error"; break;
        case NETWORK_ERR: name = "Network Error"; break;
        case ABORT_ERR: name = "Aborted"; break;
        case NULL_POINTER_ERR: name = "Null Pointer"; break;
        default: name = "Other Error"; break;
      }
      return StringPrintf("XMLHttpRequestException: %d %s", code_, name);
    }

   private:
    ExceptionCode code_;
  };

  // Used in the methods for script to throw an script exception on errors.
  bool CheckException(ExceptionCode code) {
    if (code != NO_ERR) {
      DLOG("XMLHttpRequest: Set pending exception: %d this=%p", code, this);
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
    std::string data;
    if (v_data.ConvertToString(&data)) {
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

  // We can't return std::string here, because the response body may be binary
  // and can't be converted from UTF-8 to UTF-16 by the script adapter.
  ScriptableBinaryData *ScriptGetResponseBody() {
    const char *result = NULL;
    size_t size = 0;
    if (CheckException(GetResponseBody(&result, &size)))
      return result ? new ScriptableBinaryData(result, size) : NULL;
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
            pos = value.find("charset");
            if (pos != std::string::npos) {
              pos += 7;
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

  void OnResponseHeaderReceived(const QHttpResponseHeader &header) {
    status_ = static_cast<unsigned short>(header.statusCode());
    if (status_ == 301) {
      redirected_url_ = header.value("Location").toUtf8().data();
    } else {
      response_header_ = header;
      response_headers_ = header.toString().toStdString();
      response_content_type_ = header.contentType().toStdString();
      SplitStatusAndHeaders();
      ParseResponseHeaders();

#if _DEBUG
      QTextStream out(stdout);
      out << "Receive Header:"
          << header.contentType() << "\n"
          << header.statusCode() << "\n"
          << header.toString()  << "\n";
#endif

      if (session_)
        session_->SaveCookie(header);

      if (ChangeState(HEADERS_RECEIVED))
        ChangeState(LOADING);
    }
  }

  void OnRequestFinished(int id, bool error) {
    if (status_ == 301) {
      FreeResource();
      send_flag_ = false;
      if (OpenInternal(redirected_url_.c_str()) != NO_ERR) {
        // TODO: Why do the state changes?
        // ChangeState(HEADERS_RECEIVED);
        // ChangeState(LOADING);
        Done(false, false);
      } else {
        Send(NULL, 0);
      }
    } else {
      if (error)
        LOG("Error %s", http_->errorString().toStdString().c_str());
      QByteArray array = http_->readAll();
      response_body_.clear();
      response_body_.append(array.data(), array.length());

      DLOG("responseFinished: %d, %zu, %d",
           id,
           response_body_.length(),
           array.length());
      Done(true, !error);
    }
  }

  MainLoopInterface *main_loop_;
  XMLParserInterface *xml_parser_;
  QString default_user_agent_;
  QHttp *http_;
  QHttpRequestHeader *request_header_;
  QHttpResponseHeader response_header_;
  Session *session_;
  HttpHandler *handler_;
  QByteArray *send_data_;
  Signal0<void> onreadystatechange_signal_;

  std::string url_, host_;
  bool async_;

  State state_;
  // Required by the specification.
  // It will be true after send() is called in async mode.
  bool send_flag_;

  std::string redirected_url_;
  std::string response_headers_;
  std::string response_content_type_;
  std::string response_encoding_;
  unsigned short status_;
  std::string status_text_;
  bool succeeded_;
  std::string response_body_;
  std::string response_text_;
  QString user_;
  QString password_;
  QString method_;
  DOMDocumentInterface *response_dom_;
  CaseInsensitiveStringMap response_headers_map_;
  static Backoff backoff_;
  static OptionsInterface *backoff_options_;
};

void HttpHandler::OnResponseHeaderReceived(const QHttpResponseHeader& header) {
  request_->OnResponseHeaderReceived(header);
}

void HttpHandler::OnDone(bool error) {
  request_->OnRequestFinished(0, error);
}

Backoff XMLHttpRequest::backoff_;
OptionsInterface *XMLHttpRequest::backoff_options_ = NULL;

class XMLHttpRequestFactory : public XMLHttpRequestFactoryInterface {
 public:
  XMLHttpRequestFactory() : next_session_id_(1) {
  }

  virtual int CreateSession() {
    int result = next_session_id_++;
    sessions_[result] = new Session();
    return result;
  }

  virtual void DestroySession(int session_id) {
    Sessions::iterator it = sessions_.find(session_id);
    if (it != sessions_.end()) {
      Session *session = it->second;
      delete session;
      sessions_.erase(it);
    } else {
      DLOG("XMLHttpRequestFactory::DestroySession Invalid session: %d",
           session_id);
    }
  }

  virtual XMLHttpRequestInterface *CreateXMLHttpRequest(
      int session_id, XMLParserInterface *parser) {
    if (session_id == 0)
      return new XMLHttpRequest(NULL, GetGlobalMainLoop(), parser,
                                default_user_agent_);

    Sessions::iterator it = sessions_.find(session_id);
    if (it != sessions_.end())
      return new XMLHttpRequest(it->second, GetGlobalMainLoop(), parser,
                                default_user_agent_);

    DLOG("XMLHttpRequestFactory::CreateXMLHttpRequest: "
         "Invalid session: %d", session_id);
    return NULL;
  }

  virtual void SetDefaultUserAgent(const char *user_agent) {
    if (user_agent)
      default_user_agent_ = user_agent;
  }

 private:
  typedef std::map<int, Session*> Sessions;
  Sessions sessions_;
  int next_session_id_;
  QString default_user_agent_;
};
} // namespace qt
} // namespace ggadget

#define Initialize qt_xml_http_request_LTX_Initialize
#define Finalize qt_xml_http_request_LTX_Finalize
#define CreateXMLHttpRequest qt_xml_http_request_LTX_CreateXMLHttpRequest

static ggadget::qt::XMLHttpRequestFactory gFactory;

extern "C" {
  bool Initialize() {
    LOGI("Initialize qt_xml_http_request extension.");
    return ggadget::SetXMLHttpRequestFactory(&gFactory);
  }

  void Finalize() {
    LOGI("Finalize qt_xml_http_request extension.");
  }
}
#include "qt_xml_http_request_internal.moc"
