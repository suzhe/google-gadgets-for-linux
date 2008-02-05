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
#include <dirent.h>

#include "process.h"
#include "machine.h"

namespace ggadget {
namespace framework {
namespace linux_system {

static const char* kDirName = "/proc";

static bool ReadCmdPath(int pid, std::string *cmdline);

// ---------------------------PrcoessInfo Class-------------------------------//

ProcessInfo::ProcessInfo(int pid, const std::string &path) :
  pid_(pid), path_(path) {
}

void ProcessInfo::Destroy() {
  delete this;
}

int ProcessInfo::GetProcessId() const {
  return pid_;
}

std::string ProcessInfo::GetExecutablePath() const {
  return path_;
}

// ---------------------------Processes Class---------------------------------//

void Processes::Destroy() {
  delete this;
}

Processes::Processes() {
  InitProcesses();
}

int Processes::GetCount() const {
  return procs_.size();
}

ProcessInfoInterface *Processes::GetItem(int index) {
  if (index < 0|| index >= GetCount())
    return NULL;
  return new ProcessInfo(procs_[index].first, procs_[index].second);
}

void Processes::InitProcesses() {
  DIR *dir = NULL;
  struct dirent *entry = NULL;

  dir = opendir(kDirName);
  if (!dir) // if can't open the directory, just return
    return;

  while ((entry = readdir(dir)) != NULL) {
    char *name = entry->d_name;
    char *end;
    int pid = strtol(name, &end, 10);

    // if it is not a process folder, so skip it
    if (!pid || *end)
      continue;

    std::string cmdline;
    if (ReadCmdPath(pid, &cmdline) && cmdline != "") {
      procs_.push_back(IntStringPair(pid, cmdline));
    }
  }
}

// -----------------------------Process Class---------------------------------//

ProcessesInterface *Process::EnumerateProcesses() {
  return new Processes;
}

ProcessInfoInterface *Process::GetForeground() {
  // TODO: implement this
  return NULL;
}

ProcessInfoInterface *Process::GetInfo(int pid) {
  std::string cmdline;
  if (ReadCmdPath(pid, &cmdline)) {
    return new ProcessInfo(pid, cmdline);
  }
  return NULL;
}

// ---------------------------Utility Functions-------------------------------//

// Reads the command path with pid from proc system.
// EMPTY string will be returned if any error.
static bool ReadCmdPath(int pid, std::string *cmdline) {
  if (pid <= 0 || !cmdline)
    return false;

  char filename[PATH_MAX + 2] = {0};
  snprintf(filename, sizeof(filename) - 1, "%s/%d/exe", kDirName, pid);

  char command[PATH_MAX + 2] = {0};
  readlink(filename, command, sizeof(command) - 1);

  for (int i = 0; command[i]; i++) {
    if (command[i] == ' ' || command[i] == '\n') {
      command[i] = 0;
      break;
    }
  }
  *cmdline = std::string(command);

  return true;
}

} // namespace linux_system
} // namespace framework
} // namespace ggadget
