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
#ifndef HOSTS_QT_HOST_INTERNAL_H__
#define HOSTS_QT_HOST_INTERNAL_H__

namespace hosts {
namespace qt {

class QtHostObject : public QObject {
  Q_OBJECT
 public:
  QtHostObject(QtHost *owner, GadgetBrowserHost *ghost)
    : owner_(owner),
      gadget_browser_host_(ghost),
      show_(true) {}

 signals:
  void show(bool);

 public slots:
  void OnAddGadget() {
    GetGadgetManager()->ShowGadgetBrowserDialog(gadget_browser_host_);
  }
  void OnShowAll() {
    emit show(true);
    show_ = true;
  }
  void OnHideAll() {
    emit show(false);
    show_ = false;
  }

  void OnTrayActivated(QSystemTrayIcon::ActivationReason reason) {
    if (reason == QSystemTrayIcon::DoubleClick) {
      if (show_)
        OnHideAll();
      else
        OnShowAll();
    }
  }

 private:
  QtHost *owner_;
  GadgetBrowserHost *gadget_browser_host_;
  bool show_;
};

}
}

#endif // HOSTS_QT_HOST_INTERNAL_H__
