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

#include "perfmon.h"

#include <cmath>
#include <ggadget/main_loop_interface.h>
#include <ggadget/string_utils.h>

namespace ggadget {
namespace framework {
namespace linux_system {


// Internal structure for holding real-time CPU statistics.
// All fields in this structure are measured in units of USER_HZ.
struct CpuStat {
  CpuStat()
      : user(0),
        nice(0),
        system(0),
        idle(0),
        iowait(0),
        hardirq(0),
        softirq(0),
        steal(0),
        uptime(0),
        worktime(0) {}
  int64_t user;
  int64_t nice;
  int64_t system;
  int64_t idle;
  int64_t iowait;
  int64_t hardirq;
  int64_t softirq;
  int64_t steal;
  int64_t uptime;
  int64_t worktime;
};


// the ZERO for comparing between double numbers
static const double kThreshold = 1e-9;
// the max length of a line
static const int kMaxLength = 1024;
// the time interval for time out watch (milliseconds)
static const int kUpdateInterval = 2000;


// the filename for CPU state in proc system
static const char *kProcStatFile = "/proc/stat";
// the cpu time operation
static const char *kPerfmonCpuTime = "\\Processor(_Total)\\% Processor Time";
// the cpu state head name
static const char *kCpuHeader = "cpu";


// the current cpu usage status
static CpuStat current_cpu_status;
// the last cpu usage status
static CpuStat last_cpu_status;


// Gets the current cpu usage
static double GetCurrentCpuUsage() {
  FILE* fp = fopen(kProcStatFile, "rt");
  if (!fp)
    return 0.0;
  
  char line[kMaxLength];

  if (!fgets(line, kMaxLength, fp)) {
    fclose(fp);
    return 0.0;  
  }

  fclose(fp);

  size_t head_length = strlen(kCpuHeader);

  if (!line || strlen(line) <= head_length)
    return 0.0;
  
  if (!GadgetStrNCmp(line, kCpuHeader, head_length)) {
    sscanf(line + head_length + 1,
           "%jd %jd %jd %jd %jd %jd %jd",
           &current_cpu_status.user,
           &current_cpu_status.nice,
           &current_cpu_status.system,
           &current_cpu_status.idle,
           &current_cpu_status.iowait,
           &current_cpu_status.hardirq,
           &current_cpu_status.softirq);
    
    // calculates the cpu total time
    current_cpu_status.uptime =
      current_cpu_status.user + current_cpu_status.nice +
      current_cpu_status.system + current_cpu_status.idle +
      current_cpu_status.iowait + current_cpu_status.hardirq +
      current_cpu_status.softirq;

    // calculates the cpu working time
    current_cpu_status.worktime =
          current_cpu_status.user + current_cpu_status.nice +
          current_cpu_status.system + current_cpu_status.hardirq +
          current_cpu_status.softirq;

    // calculates percentage of cpu usage
    int64_t current_work_time = current_cpu_status.worktime
                                - last_cpu_status.worktime;
    
    int64_t current_total_time = current_cpu_status.uptime
                                 - last_cpu_status.uptime;

    // update the last cpu status
    last_cpu_status = current_cpu_status;

    return current_total_time
            ? (double) current_work_time / (double) current_total_time
            : 0.0;
  }

  return 0.0;
}


/**
 * A special WatchCallbackInterface implementation that calls a specified slot
 * when the CPU usage varies.
 *
 * Then if you want to add a slot into main loop, you can just call:
 * main_loop->AddTimeoutWatch(interval, new WatchCallbackSlot(
 *              new ProcessorUsageCallBackSlot(counter_path, slot)));
 *
 * You can refer to slot's type PerfmonInterface::CallbackSlot for its details.
 */
class ProcessorUsageCallBackSlot : public WatchCallbackInterface {
 public:

  ProcessorUsageCallBackSlot(const char *counter_path,
                             PerfmonInterface::CallbackSlot *slot)
    : counter_path_(counter_path),
      slot_(slot),
      last_cpu_usage_(0.0),
      current_cpu_usage_(0.0) {
  }

  virtual bool Call(MainLoopInterface *main_loop, int watch_id) {
    if (CpuUsageDiffer()) {
      // if cpu usage varies, call the give function slot
      (*slot_)(counter_path_.c_str(), Variant(current_cpu_usage_ * 100.0));
    }
    return true;
  }

  virtual void OnRemove(MainLoopInterface *main_loop, int watch_id) {
    delete slot_;
    delete this;
  }

  // Checks whether the cpu usage varies
  bool CpuUsageDiffer() {

    current_cpu_usage_ = GetCurrentCpuUsage();

    if (fabs(current_cpu_usage_ - last_cpu_usage_) <= kThreshold) {
      return false;
    }

    // update the last cpu usage
    last_cpu_usage_ = current_cpu_usage_;

    return true;
  }

 private:
  std::string counter_path_;
  PerfmonInterface::CallbackSlot *slot_;
  double last_cpu_usage_;
  double current_cpu_usage_;
};


Variant Perfmon::GetCurrentValue(const char *counter_path) {
  if (counter_path && !GadgetStrCmp(counter_path, kPerfmonCpuTime)) {
    return Variant(GetCurrentCpuUsage() * 100.0);
  }

  return Variant(0.0);
}


int Perfmon::AddCounter(const char *counter_path, CallbackSlot *slot) {
  if (!slot)
    return -1;

  if (counter_path && !GadgetStrCmp(counter_path, kPerfmonCpuTime)) {
    // operations for CPU usage
    MainLoopInterface *main_loop = GetGlobalMainLoop();

    if (main_loop)
      return main_loop->AddTimeoutWatch(kUpdateInterval,
                          new ProcessorUsageCallBackSlot(counter_path, slot));
  }

  delete slot;
  return -1;
}


void Perfmon::RemoveCounter(int id) {
  // remove the watch from global main loop
  MainLoopInterface *main_loop = GetGlobalMainLoop();
  if (main_loop)
    main_loop->RemoveWatch(id);
}


} // namespace linux_system
} // namespace framework
} // namespace ggadget
