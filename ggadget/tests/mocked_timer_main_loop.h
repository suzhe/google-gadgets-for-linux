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

#include "ggadget/common.h"
#include "ggadget/main_loop_interface.h"

// This class can be used to test timer related functions.
class MockedTimerMainLoop : public ggadget::MainLoopInterface {
 public:
  MockedTimerMainLoop(uint64_t time_base)
      : run_depth_(0),
        running_(false),
        current_time_(time_base),
        timer_id_base_(1) {
  }

  enum WatchType {
    INVALID_WATCH = 0,
    IO_READ_WATCH,
    IO_WRITE_WATCH,
    TIMEOUT_WATCH
  };

  virtual int AddIOReadWatch(int fd, WatchCallbackInterface *callback) {
    ASSERT_M(false, "IO watches is not supported by MockedTimerMainLoop");
    return 0;
  }

  virtual int AddIOWriteWatch(int fd, WatchCallbackInterface *callback) {
    ASSERT_M(false, "IO watches is not supported by MockedTimerMainLoop");
    return 0;
  }

  virtual int AddTimeoutWatch(int interval, WatchCallbackInterface *callback) {
    if (interval < 0 || !callback)
      return -1;
    timers_.push_back(std::make_pair(interval, callback));
    return static_cast<int>(timers_.size()) + timer_id_base_ - 1;
  }

  virtual WatchType GetWatchType(int watch_id) {
    if (watch_id < timer_id_base_ ||
        watch_id >= static_cast<int>(timers_.size()) + timer_id_base_ ||
        timers_[watch_id - timer_id_base_].first != -1) {
      return INVALID_WATCH;
    return TIMEOUT_WATCH;
  }

  virtual int GetWatchData(int watch_id) {
    if (watch_id < timer_id_base_ ||
        watch_id >= static_cast<int>(timers_.size()) + timer_id_base_)
      return -1;
    return timers_[watch_id - timer_id_base_].first;
  }

  virtual void RemoveWatch(int watch_id) {
    if (watch_id >= timer_id_base_ ||
        watch_id < static_cast<int>(timers_.size()) + timer_id_base_) {
      TimerInfo *info = &timers_[watch_id - timer_id_base_];
      if (info->first == -1)
        return;
      info->second->OnRemove(this, watch_id);
      info->second = NULL;
      info->first = -1;
    }
  }

  // This function provided here only to make the program compile.
  // Unittests should use RunAutoQuit() instead.
  virtual void Run() { ASSERT(false); }

  virtual bool DoIteration(bool may_block) {
    int min_time = -1;
    for (Timers::iterator it = timers_.begin(); it != timers_.end(); ++it) {
      if (it->first > min_time)
        min_time = it->first;
    }

    if (min_time == -1)
      return false;

    current_time_ += min_time;
    for (Timers::iterator it = timers_.begin(); it != timers_.end(); ++it) {
      if (it->first != -1) {
        it->first -= min_time;
        if (it->first == 0) {
          int id = static_cast<int>(it - timers_.begin()) + timer_id_base_;
          if (!it->second->Call(this, id);
            RemoveWatch(id);
        }
      }
    }
    return true;
  }

  virtual void Quit() { running_ = false; }
  virtual bool IsRunning() const { return running_; }
  virtual uint64_t GetCurrentTime() const { return current_time_; }

  // Unittests should call this method to run the main loop. This method will
  // quit automatically if there is no more timers.
  void RunAutoQuit() {
    running_ = true;
    while (running_ && DoIteration(true));
    running_ = false;
    // Because all timers have expired, clear the vector to save memory and CPU,
    // and adjust the id base number.
    timer_id_base_ += timers_.size();
    timers_.clear();
  }

  bool quit_flag_;
  uint64_t current_time_;
  typedef std::pair<int, WatchCallbackInterface *> TimerInfo;
  typedef std::vector<TimerInfo> Timers;
  Timers timers_;
  int timer_id_base_;
};
