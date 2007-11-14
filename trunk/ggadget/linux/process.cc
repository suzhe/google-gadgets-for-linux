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

#include "process.h"

namespace ggadget {

namespace framework {

namespace system {

ProcessInfo::ProcessInfo(int pid, const std::string &path) :
    pid_(pid), path_(path) {
}

void ProcessInfo::Destroy() {
  delete this;
}

int ProcessInfo::GetProcessId() const {
  return pid_;
}

const char *ProcessInfo::GetExecutablePath() const {
  return path_.c_str();
}

void Processes::Destroy() {
  delete this;
}

int Processes::GetCount() const {
  return 3;
}

ProcessInfoInterface *Processes::GetItem(int index) {
  switch (index) {
  case 0:
    return new ProcessInfo(15, std::string("/bin/ls"));
  case 1:
    return new ProcessInfo(49, std::string("/bin/vi"));
  case 2:
    return new ProcessInfo(63, std::string("/usr/bin/ggadget"));
  default:
    return NULL;
  }
  return NULL;
}

ProcessesInterface *Process::EnumerateProcesses() {
  return new Processes();
}

ProcessInfoInterface *Process::GetForeground() {
  return new ProcessInfo(49, std::string("/bin/vi"));
}

ProcessInfoInterface *Process::GetInfo(int pid) {
  switch (pid) {
  case 15:
    return new ProcessInfo(15, std::string("/bin/ls"));
  case 49:
    return new ProcessInfo(49, std::string("/bin/vi"));
  case 63:
    return new ProcessInfo(63, std::string("/usr/bin/ggadget"));
  default:
    return NULL;
  }
  return NULL;
}

} // namespace system

} // namespace framework

} // namespace ggadget
