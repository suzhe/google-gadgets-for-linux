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

#ifndef GGADGET_QT_XML_HTTP_REQUEST_H__
#define GGADGET_QT_XML_HTTP_REQUEST_H__
namespace ggadget {
namespace qt {
class XMLHttpRequest;
class HttpHandler : public QObject {
  Q_OBJECT
 public:
  HttpHandler(XMLHttpRequest *request, QHttp *http)
      : request_(request),
        http_(http) {
    connect(http, SIGNAL(responseHeaderReceived(const QHttpResponseHeader&)),
            this, SLOT(OnResponseHeaderReceived(const QHttpResponseHeader&)));
    connect(http, SIGNAL(done(bool)),
            this, SLOT(OnDone(bool)));
    connect(http, SIGNAL(sslErrors(const QList<QSslError>&)),
            this, SLOT(OnSslError(const QList<QSslError>&)));
  }
 private slots:
  void OnResponseHeaderReceived(const QHttpResponseHeader& header);
  void OnDone(bool error);
  void OnSslError(const QList<QSslError>& errors) {
    http_->ignoreSslErrors();
  }
 private:
  XMLHttpRequest *request_;
  QHttp *http_;
};

}
}

#endif
