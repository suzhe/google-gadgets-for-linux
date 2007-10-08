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

#ifndef GGADGET_FRAMEWORK_SYSTEM_PROCESS_INTERFACE_H__
#define GGADGET_FRAMEWORK_SYSTEM_PROCESS_INTERFACE_H__

namespace ggadget {

namespace framework {

namespace system {

/** Interface for retrieving the process information. */
class ProcessInfoInterface {
 protected:
  virtual ~ProcessInfoInterface() {}

 public:
  virtual void Destroy() = 0;

 public:
  /** Gets the process Id. */
  virtual int GetProcessId() const = 0;
  /** Gets the path of the running process. */
  virtual const char *GetExecutablePath() const = 0;
};

/** Interface for enumerating the processes. */
class ProcessesInterface {
 protected:
  virtual ~ProcessesInterface() {}

 public:
  virtual void Destroy() = 0;

 public:
  /** Get the number of processes. */
  virtual int GetCount() const = 0;
  /**
   * Get the process information according to the index.
   * @return the information of the process.
   *     The user must call @c Destroy() after using this object.
   */
  virtual ProcessInfoInterface *GetItem(int index) = 0;
};

/** Interface for retrieving the information of the processes. */
class ProcessInterface {
 protected:
  virtual ~ProcessInterface() {}

 public:
  /**
   * An enumeration of process IDs for all processes on the system.
   * @return the enumeration of the currently running processes.
   *     The user must call @c Destroy() after using this object.
   */
  virtual ProcessesInterface *EnumerateProcesses() = 0;
  /*
   * Gets the information of the foreground process.
   * @return the information of the foreground process.
   *     The user must call @c Destroy() after using this object.
   */
  virtual ProcessInfoInterface *GetForeground() = 0;
  /**
   * Gets the information of the specified process according to the process ID.
   * @return the information of the specified process.
   *     The user must call @c Destroy() after using this object.
   */
  virtual ProcessInfoInterface *GetInfo(int pid) = 0;
};

} // namespace system

} // namespace framework

} // namespace ggadget

#endif // GGADGET_FRAMEWORK_SYSTEM_PROCESS_INTERFACE_H__
