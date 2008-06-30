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

#include "run_once.h"
#include <cstdlib>
#include <list>
#include <map>
#include <signal.h>
#include <string>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include "main_loop_interface.h"
#include "signals.h"

namespace ggadget {

class RunOnce::Impl : public WatchCallbackInterface {
 public:
  struct Session {
    int watch_id;
    std::string data;
  };

  Impl(const char *path)
      : path_(path),
      is_running_(false),
      watch_id_(-1),
      fd_(-1) {
    int fd = RunAsServer();
    if (fd == -1) {
      fd = RunAsClient();
      if (fd != -1) {
        is_running_ = true;
        fd_ = fd;
        return;
      } else {
        unlink(path_.c_str());
        fd = RunAsServer();
      }
    }

    is_running_ = false;
    fd_ = fd;
    watch_id_ = GetGlobalMainLoop()->AddIOReadWatch(fd, this);
  }

  ~Impl() {
    if (is_running_) {
      close(fd_);
    } else {
      // Removes watch for all running clients.
      for (std::map<int, Session>::iterator ite = connections_.begin();
           ite != connections_.end();
           ++ite) {
        GetGlobalMainLoop()->RemoveWatch(ite->second.watch_id);
      }
      // Removes watch for the listening socket.
      GetGlobalMainLoop()->RemoveWatch(watch_id_);
      unlink(path_.c_str());
    }
  }

  bool IsRunning() const {
    return is_running_;
  }

  static void DoNothing(int) {
  }

  size_t SendMessage(const std::string &data) {
    if (!is_running_)
      return 0;

    if (fd_ == -1)
      fd_ = RunAsClient();

    void (*old_proc)(int);
    old_proc = signal(SIGPIPE, DoNothing);

    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(fd_, &fds);
    struct timeval time;

    size_t written = 0;
    while (written < data.size()) {
      // Waits for at most 1 second.
      time.tv_sec = 1;
      time.tv_usec = 0;
      int result = select(fd_ + 1, NULL, &fds, NULL, &time);
      if (result == -1 || result == 0) {
        goto end;
      }
      int current = write(fd_, &data.c_str()[written], data.size() - written);
      if (current < 1) {
        goto end;
      }
      written += current;
    }

end:
    FD_CLR(fd_, &fds);
    close(fd_);
    fd_ = -1;
    signal(SIGPIPE, old_proc);
    return written;
  }

  Connection *ConnectOnMessage(Slot1<void, const std::string&> *slot) {
    return signal_.Connect(slot);
  }

  virtual bool Call(MainLoopInterface *main_loop, int watch_id) {
    int fd;
    char buf;

    fd = main_loop->GetWatchData(watch_id);

    if (fd_ == fd) {
      socklen_t len;
      fd = accept(fd, NULL, &len);
      main_loop->AddIOReadWatch(fd, this);
      Session data = {
        watch_id,
        std::string()
      };
      connections_[fd] = data;
      return true;
    }

    if (read(fd, &buf, 1) > 0) {
      connections_[fd].data += buf;
    } else {
      std::map<int, Session>::iterator ite = connections_.find(fd);
      if (ite != connections_.end()) {
        signal_(ite->second.data);
        main_loop->RemoveWatch(watch_id);
        connections_.erase(ite);
      }
      return false;
    }

    return true;
  }

  virtual void OnRemove(MainLoopInterface *main_loop, int watch_id) {
    close(main_loop->GetWatchData(watch_id));
  }

  int RunAsServer() {
    int fd;
    struct sockaddr_un uaddr;
    uaddr.sun_family = AF_UNIX;
    strcpy(uaddr.sun_path, path_.c_str());
    fd = socket(PF_UNIX, SOCK_STREAM, 0);
    if (bind(fd, (struct sockaddr*) &uaddr, sizeof(uaddr)) == -1) {
      close(fd);
      return -1;
    }
    listen(fd, 5);
    return fd;
  }

  int RunAsClient() {
    int fd;
    struct sockaddr_un uaddr;
    uaddr.sun_family = AF_UNIX;
    strcpy(uaddr.sun_path, path_.c_str());
    fd = socket(PF_UNIX, SOCK_STREAM, 0);
    if (connect(fd, (struct sockaddr*) &uaddr, sizeof(uaddr)) == -1) {
      close(fd);
      return -1;
    }
    return fd;
  }

  std::string path_;
  bool is_running_;
  int watch_id_;
  int fd_;
  std::map<int, Session> connections_;
  Signal1<void, const std::string &> signal_;
};

RunOnce::RunOnce(const char *path)
  : impl_(new Impl(path)) {
}

RunOnce::~RunOnce() {
  delete impl_;
}

bool RunOnce::IsRunning() const {
  return impl_->IsRunning();
}

size_t RunOnce::SendMessage(const std::string &data) {
  return impl_->SendMessage(data);
}

Connection *RunOnce::ConnectOnMessage(Slot1<void, const std::string&> *slot) {
  return impl_->ConnectOnMessage(slot);
}

} // namespace ggadget
