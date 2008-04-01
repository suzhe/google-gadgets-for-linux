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
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/time.h>
#include <time.h>

#include <stdint.h>
#include <map>
#include <QtGui/QApplication>
#include <ggadget/common.h>
#include <ggadget/logger.h>
#include "qt_main_loop.h"

namespace ggadget {
namespace qt {

#include "qt_main_loop.moc"

class QtMainLoop::Impl {
 public:
  Impl(MainLoopInterface *main_loop)
    : main_loop_(main_loop) {
  }

  int AddIOWatch(MainLoopInterface::WatchType type, int fd,
                 WatchCallbackInterface *callback) {
    if (fd < 0 || !callback) return -1;

    QSocketNotifier::Type qtype;
    switch (type) {
      case MainLoopInterface::INVALID_WATCH:
        return -1;
      case MainLoopInterface::IO_READ_WATCH:
        qtype = QSocketNotifier::Read;
        break;
      case MainLoopInterface::IO_WRITE_WATCH:
        qtype = QSocketNotifier::Write;
        break;
      default:
        break;
    }

    QSocketNotifier *notifier = new QSocketNotifier(fd, qtype);
    WatchNode *node = new WatchNode();
    node->type_ = type;
    node->data_ = fd;
    node->callback_ = callback;
    node->object_ = notifier;
    node->main_loop_ = main_loop_;
    QObject::connect(notifier, SIGNAL(activated(int)),
                     node, SLOT(OnIOEvent(int)));
    int ret = AddWatchNode(node);
    return ret;
  }

  int AddTimeoutWatch(int interval, WatchCallbackInterface *callback) {
    if (interval < 0 || !callback) return -1;
    QTimer *timer = new QTimer();
    timer->setInterval(interval);
    WatchNode *node = new WatchNode();
    node->type_ = MainLoopInterface::TIMEOUT_WATCH;
    node->data_ = interval;
    node->callback_ = callback;
    node->object_ = timer;
    node->main_loop_ = main_loop_;
    QObject::connect(timer, SIGNAL(timeout(void)),
                     node, SLOT(OnTimeout(void)));
    timer->start();
    return AddWatchNode(node);
  }

  MainLoopInterface::WatchType GetWatchType(int watch_id) {
    WatchNode *node = GetWatchNode(watch_id);
    return node ? node->type_ : MainLoopInterface::INVALID_WATCH;
  }

  int GetWatchData(int watch_id) {
    WatchNode *node = GetWatchNode(watch_id);
    return node ? node->data_ : -1;
  }

  void RemoveWatch(int watch_id) {
    WatchNode *node = GetWatchNode(watch_id);
    if (node && !node->removing_) {
      node->removing_ = true;
      if (!node->calling_) {
        WatchCallbackInterface *callback = node->callback_;
        //DLOG("QtMainLoop::RemoveWatch: id=%d", watch_id);
        callback->OnRemove(main_loop_, watch_id);
        RemoveWatchNode(watch_id);
        delete node;
      }
    }
  }

  void Run() { }

  bool DoIteration(bool may_block) { return true; }

  void Quit() {
    QApplication::exit();
  }

  bool IsRunning() {
    return true;
  }

  uint64_t GetCurrentTime() const {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return static_cast<uint64_t>(tv.tv_sec) * 1000 + tv.tv_usec / 1000;
  }

 private:
  WatchNode* GetWatchNode(int watch_id) {
    if (watches_.find(watch_id) == watches_.end())
      return NULL;
    else
      return watches_[watch_id];
  }

  void RemoveWatchNode(int watch_id) {
    watches_.erase(watch_id);
  }

  int AddWatchNode(WatchNode *node) {
    int i;
    while (1) {
      i = abs(random());
      if (watches_.find(i) == watches_.end()) break;
    }
    node->watch_id_ = i;
    watches_[i] = node;
    return i;
  }

  std::map<int, WatchNode*> watches_;
  MainLoopInterface *main_loop_;
};

QtMainLoop::QtMainLoop()
  : impl_(new Impl(this)) {
}
QtMainLoop::~QtMainLoop() {
  delete impl_;
}
int QtMainLoop::AddIOReadWatch(int fd, WatchCallbackInterface *callback) {
  return impl_->AddIOWatch(IO_READ_WATCH, fd, callback);
}
int QtMainLoop::AddIOWriteWatch(int fd, WatchCallbackInterface *callback) {
  return impl_->AddIOWatch(IO_WRITE_WATCH, fd, callback);
}
int QtMainLoop::AddTimeoutWatch(int interval,
                                WatchCallbackInterface *callback) {
  return impl_->AddTimeoutWatch(interval, callback);
}
MainLoopInterface::WatchType QtMainLoop::GetWatchType(int watch_id) {
  return impl_->GetWatchType(watch_id);
}
int QtMainLoop::GetWatchData(int watch_id) {
  return impl_->GetWatchData(watch_id);
}
void QtMainLoop::RemoveWatch(int watch_id) {
  impl_->RemoveWatch(watch_id);
}
void QtMainLoop::Run() {
  impl_->Run();
}
bool QtMainLoop::DoIteration(bool may_block) {
  return impl_->DoIteration(may_block);
}
void QtMainLoop::Quit() {
  impl_->Quit();
}
bool QtMainLoop::IsRunning() const {
  return impl_->IsRunning();
}
uint64_t QtMainLoop::GetCurrentTime() const {
  return impl_->GetCurrentTime();
}

} // namespace qt
} // namespace ggadget
