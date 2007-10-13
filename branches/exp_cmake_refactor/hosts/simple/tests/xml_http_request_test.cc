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

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <time.h>

#include "hosts/simple/gtk_cairo_host.h"
#include "hosts/simple/xml_http_request.h"
#include "unittest/gunit.h"

using ggadget::XMLHttpRequestInterface;
using ggadget::NewSlot;

TEST(XMLHttpRequest, States) {
  GtkCairoHost *host = new GtkCairoHost(NULL, 0);
  XMLHttpRequestInterface *request = host->NewXMLHttpRequest();
  ASSERT_EQ(XMLHttpRequestInterface::UNSENT, request->GetReadyState());
  // Invalid request method.
  ASSERT_FALSE(request->Open("DELETE", "http://localhost", false, NULL, NULL));
  ASSERT_EQ(XMLHttpRequestInterface::UNSENT, request->GetReadyState());
  // Invalid state.
  ASSERT_FALSE(request->Send(static_cast<const char *>(NULL)));
  ASSERT_EQ(XMLHttpRequestInterface::UNSENT, request->GetReadyState());
  // Valid request.
  ASSERT_TRUE(request->Open("GET", "http://localhost", false, NULL, NULL));
  ASSERT_EQ(XMLHttpRequestInterface::OPEN, request->GetReadyState());
  ASSERT_TRUE(request->SetRequestHeader("aaa", "bbb"));
  request->Abort();
  ASSERT_EQ(XMLHttpRequestInterface::UNSENT, request->GetReadyState());
  ASSERT_FALSE(request->SetRequestHeader("ccc", "ddd"));
  delete request;
  delete host;
}

class Callback {
 public:
  Callback(XMLHttpRequestInterface *request)
      : callback_count_(0), request_(request) { }
  void Call() {
    callback_count_++;
    switch (callback_count_) {
      case 1:
        ASSERT_EQ(XMLHttpRequestInterface::OPEN, request_->GetReadyState());
        break;
      case 2:
        ASSERT_EQ(XMLHttpRequestInterface::OPEN, request_->GetReadyState());
        break;
      case 3:
        // ASSERT(XMLHttpRequestInterface::SENT == request_->GetReadyState());
        ASSERT_EQ(XMLHttpRequestInterface::SENT, request_->GetReadyState());
        break;
      case 4:
        ASSERT_EQ(XMLHttpRequestInterface::LOADING,
                  request_->GetReadyState());
        break;
      case 5:
        ASSERT_EQ(XMLHttpRequestInterface::DONE, request_->GetReadyState());
        break;
      default:
        ASSERT_TRUE(false);
        break;
    }
  }
  int callback_count_;
  XMLHttpRequestInterface *request_;
};

TEST(XMLHttpRequest, SyncLocalFile) {
  GtkCairoHost *host = new GtkCairoHost(NULL, 0);
  XMLHttpRequestInterface *request = host->NewXMLHttpRequest();

  Callback callback(request);

  system("echo ABCDEFG >/tmp/xml_http_request_test_data");
  // Valid request.
  request->ConnectOnReadyStateChange(NewSlot(&callback, &Callback::Call));
  ASSERT_EQ(0, callback.callback_count_);
  ASSERT_TRUE(request->Open("GET", "file:///tmp/xml_http_request_test_data",
                            false, NULL, NULL));
  ASSERT_EQ(1, callback.callback_count_);
  ASSERT_EQ(XMLHttpRequestInterface::OPEN, request->GetReadyState());
  ASSERT_TRUE(request->Send(static_cast<const char *>(NULL)));
  ASSERT_EQ(5, callback.callback_count_);
  ASSERT_EQ(XMLHttpRequestInterface::DONE, request->GetReadyState());
  ASSERT_STREQ("", request->GetAllResponseHeaders());
  size_t size;
  ASSERT_STREQ("ABCDEFG\n", request->GetResponseBody(&size));
  ASSERT_EQ(8u, size);
  delete request;
  delete host;
}

TEST(XMLHttpRequest, AsyncLocalFile) {
  GtkCairoHost *host = new GtkCairoHost(NULL, 0);
  XMLHttpRequestInterface *request = host->NewXMLHttpRequest();

  Callback callback(request);
  system("echo GFEDCBA123 >/tmp/xml_http_request_test_data");
  // Valid request.
  request->ConnectOnReadyStateChange(NewSlot(&callback, &Callback::Call));
  ASSERT_EQ(0, callback.callback_count_);
  ASSERT_TRUE(request->Open("GET", "file:///tmp/xml_http_request_test_data",
                            true, NULL, NULL));
  ASSERT_EQ(1, callback.callback_count_);
  ASSERT_EQ(XMLHttpRequestInterface::OPEN, request->GetReadyState());
  ASSERT_TRUE(request->Send(static_cast<const char *>(NULL)));
  ASSERT_EQ(5, callback.callback_count_);
  ASSERT_EQ(XMLHttpRequestInterface::DONE, request->GetReadyState());
  ASSERT_STREQ("", request->GetAllResponseHeaders());
  size_t size;
  ASSERT_STREQ("GFEDCBA123\n", request->GetResponseBody(&size));
  ASSERT_EQ(11u, size);
  delete request;
  delete host;
}

bool server_thread_succeeded = false;
class SocketCloser {
 public:
  SocketCloser(int socket) : socket_(socket) { }
  ~SocketCloser() { close(socket_); }
  int socket_;
};

const char *kResponse0 = "HTTP/1.1 200 OK\r\n";
const char *kResponse1 = "Connection: Close\r\nTestHeader1: Value1\r\n";
const char *kResponse2 = "TestHeader2: Value2a\r\ntestheader2: Value2b\r\n\r\n";
const char *kResponse3 = "Some contents\r\n";
const char *kResponse4 = "More contents\r\n";

int semaphore = 0;

void Wait(int ms) {
  static timespec tm = { 0, ms * 1000000 }; // Sleep for 100ms.
  nanosleep(&tm, NULL);
}

void WaitFor(int value) {
  while (semaphore != value)
    Wait(2);
}

int port = 0;

void ServerThreadInner(bool async) {
  int s = socket(PF_INET, SOCK_STREAM, 0);
  LOG("Server created socket: %d", s);
  ASSERT_GT(s, 0);
  SocketCloser closer1(s);

  sockaddr_in sin;
  memset(&sin, 0, sizeof(sin));
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = htonl(INADDR_ANY);
  socklen_t socklen = sizeof(sin);
  ASSERT_EQ(0, bind(s, reinterpret_cast<sockaddr *>(&sin), socklen));
  ASSERT_EQ(0, listen(s, 5));
  ASSERT_EQ(0, getsockname(s, reinterpret_cast<sockaddr *>(&sin), &socklen));
  port = ntohs(sin.sin_port);
  LOG("Server bound to port: %d", port);

  sockaddr_in cin;
  LOG("Server is waiting for connection");
  int s1 = accept(s, reinterpret_cast<sockaddr *>(&cin), &socklen);
  ASSERT_GT(s1, 0);
  SocketCloser closer2(s1);
  LOG("Server accepted a connection: %d", s1);

  std::string line;
  int lineno = 0;
  char last_b = 0;
  bool test_header_met = false;
  while (true) {
    char b;
    read(s1, &b, 1);
    if (b == '\n' && last_b == '\r') {
      lineno++;
      if (lineno == 0)
        ASSERT_STREQ("GET /test HTTP/1.1", line.c_str());
      if (line == "TestHeader: TestHeaderValue")
        test_header_met = true;
      last_b = 0;
      // End of request.
      if (line.empty())
        break;
      line.clear();
    } else {
      last_b = b;
      if (b != '\r')
        line += b;
    }
  }
  LOG("Server got the whole request");

  ASSERT_TRUE(test_header_met);
  if (async) WaitFor(1); else Wait(100);
  LOG("Server write response0");
  write(s1, kResponse0, strlen(kResponse0));
  LOG("Server write response1");
  write(s1, kResponse1, strlen(kResponse1));
  if (async) WaitFor(2); else Wait(100);
  LOG("Server write response2");
  write(s1, kResponse2, strlen(kResponse2));
  if (async) WaitFor(3); else Wait(100);
  LOG("Server write response3");
  write(s1, kResponse3, strlen(kResponse3));
  if (async) WaitFor(4); else Wait(100);
  LOG("Server write response4");
  write(s1, kResponse4, strlen(kResponse4));
  server_thread_succeeded = true;
}

void *ServerThread(void *arg) {
  ServerThreadInner(*static_cast<bool *>(arg));
  return arg;
}

TEST(XMLHttpRequest, SyncNetworkFile) {
  GtkCairoHost *host = new GtkCairoHost(NULL, 0);
  XMLHttpRequestInterface *request = host->NewXMLHttpRequest();

  pthread_t thread;
  bool async = false;
  ASSERT_EQ(0, pthread_create(&thread, NULL, ServerThread, &async));
  while (port == 0) Wait(100);
  Wait(50);

  Callback callback(request);
  ASSERT_EQ(0, callback.callback_count_);
  request->ConnectOnReadyStateChange(NewSlot(&callback, &Callback::Call));
  char url[256];
  snprintf(url, sizeof(url), "http://localhost:%d/test", port);
  LOG("URL=%s", url);
  ASSERT_TRUE(request->Open("GET", url, false, NULL, NULL));
  ASSERT_EQ(1, callback.callback_count_);
  ASSERT_TRUE(request->SetRequestHeader("TestHeader", "TestHeaderValue"));
  ASSERT_EQ(XMLHttpRequestInterface::OPEN, request->GetReadyState());
  ASSERT_TRUE(request->Send(static_cast<const char *>(NULL)));
  ASSERT_EQ(XMLHttpRequestInterface::DONE, request->GetReadyState());
  ASSERT_EQ(5, callback.callback_count_);

  size_t size;
  ASSERT_STREQ((std::string(kResponse1) + kResponse2).c_str(),
               request->GetAllResponseHeaders());
  ASSERT_STREQ((std::string(kResponse1) + kResponse2).c_str(),
               request->GetAllResponseHeaders());
  ASSERT_STREQ((std::string(kResponse3) + kResponse4).c_str(),
               request->GetResponseBody(&size));
  ASSERT_EQ(strlen(kResponse3) + strlen(kResponse4), size);
  ASSERT_EQ(200, request->GetStatus());
  ASSERT_STREQ("OK", request->GetStatusText());
  ASSERT_TRUE(NULL == request->GetResponseHeader("NoSuchHeader"));
  ASSERT_STREQ("Value1", request->GetResponseHeader("TestHeader1"));
  ASSERT_STREQ("Value1", request->GetResponseHeader("testheader1"));
  ASSERT_STREQ("Value2a, Value2b", request->GetResponseHeader("testheader2"));
  ASSERT_STREQ("Value2a, Value2b", request->GetResponseHeader("TestHeader2"));

  pthread_join(thread, NULL);
  ASSERT_TRUE(server_thread_succeeded);
  delete request;
  delete host;
}

TEST(XMLHttpRequest, AsyncNetworkFile) {
  GtkCairoHost *host = new GtkCairoHost(NULL, 0);
  XMLHttpRequestInterface *request = host->NewXMLHttpRequest();

  pthread_t thread;
  bool async = true;
  ASSERT_EQ(0, pthread_create(&thread, NULL, ServerThread, &async));
  while (port == 0) Wait(100);
  Wait(50);

  Callback callback(request);
  ASSERT_EQ(0, callback.callback_count_);
  request->ConnectOnReadyStateChange(NewSlot(&callback, &Callback::Call));
  char url[256];
  snprintf(url, sizeof(url), "http://localhost:%d/test", port);
  LOG("URL=%s", url);
  ASSERT_TRUE(request->Open("GET", url, true, NULL, NULL));
  ASSERT_EQ(1, callback.callback_count_);
  ASSERT_TRUE(request->SetRequestHeader("TestHeader", "TestHeaderValue"));
  ASSERT_EQ(XMLHttpRequestInterface::OPEN, request->GetReadyState());
  ASSERT_TRUE(request->Send(static_cast<const char *>(NULL)));
  ASSERT_EQ(XMLHttpRequestInterface::OPEN, request->GetReadyState());
  ASSERT_EQ(2, callback.callback_count_);

  size_t size;
  semaphore = 1;
  for (int i = 0; i < 10; i++) { Wait(10); gtk_main_iteration_do(false); }
  ASSERT_EQ(XMLHttpRequestInterface::OPEN, request->GetReadyState());
  ASSERT_EQ(2, callback.callback_count_);
  // GetAllResponseHeaders and GetResponseBody return NULL in OPEN state. 
  ASSERT_TRUE(NULL == request->GetAllResponseHeaders());
  ASSERT_TRUE(NULL == request->GetResponseBody(&size));
  ASSERT_TRUE(NULL == request->GetStatusText());
  ASSERT_EQ(0u, size);

  semaphore = 2;
  for (int i = 0; i < 10; i++) { Wait(10); gtk_main_iteration_do(false); }
  ASSERT_EQ(XMLHttpRequestInterface::OPEN, request->GetReadyState());
  // GetAllResponseHeaders and GetResponseBody return NULL in OPEN state. 
  ASSERT_TRUE(NULL == request->GetAllResponseHeaders());
  ASSERT_TRUE(NULL == request->GetResponseBody(&size));
  ASSERT_TRUE(NULL == request->GetStatusText());
  ASSERT_EQ(0u, size);
  
  semaphore = 3;
  for (int i = 0; i < 10; i++) { Wait(10); gtk_main_iteration_do(false); }
  ASSERT_EQ(XMLHttpRequestInterface::LOADING, request->GetReadyState());
  ASSERT_EQ(4, callback.callback_count_);
  ASSERT_STREQ((std::string(kResponse1) + kResponse2).c_str(),
               request->GetAllResponseHeaders());
  ASSERT_STREQ(kResponse3, request->GetResponseBody(&size));
  ASSERT_EQ(strlen(kResponse3), size);
  ASSERT_EQ(200, request->GetStatus());
  ASSERT_STREQ("OK", request->GetStatusText());
  ASSERT_TRUE(NULL == request->GetResponseHeader("NoSuchHeader"));
  ASSERT_STREQ("Value1", request->GetResponseHeader("TestHeader1"));
  ASSERT_STREQ("Value1", request->GetResponseHeader("testheader1"));
  ASSERT_STREQ("Value2a, Value2b", request->GetResponseHeader("testheader2"));
  ASSERT_STREQ("Value2a, Value2b", request->GetResponseHeader("TestHeader2"));

  semaphore = 4;
  for (int i = 0; i < 10; i++) { Wait(10); gtk_main_iteration_do(false); }
  ASSERT_EQ(XMLHttpRequestInterface::DONE, request->GetReadyState());
  ASSERT_EQ(5, callback.callback_count_);
  ASSERT_STREQ((std::string(kResponse1) + kResponse2).c_str(),
               request->GetAllResponseHeaders());
  ASSERT_STREQ((std::string(kResponse3) + kResponse4).c_str(),
               request->GetResponseBody(&size));
  ASSERT_EQ(strlen(kResponse3) + strlen(kResponse4), size);
  ASSERT_EQ(200, request->GetStatus());
  ASSERT_STREQ("OK", request->GetStatusText());

  pthread_join(thread, NULL);
  ASSERT_TRUE(server_thread_succeeded);
  delete request;
  delete host;
}

int main(int argc, char **argv) {
  gtk_init(&argc, &argv);
  testing::ParseGUnitFlags(&argc, argv);
  return RUN_ALL_TESTS();
}
