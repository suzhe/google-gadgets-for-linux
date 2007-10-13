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

#ifndef GGADGET_FRAMEWORK_SYSTEM_MEMORY_INTERFACE_H__
#define GGADGET_FRAMEWORK_SYSTEM_MEMORY_INTERFACE_H__

#include <stdint.h>

namespace ggadget {

namespace framework {

namespace system {

/** Interface for retrieving the information of the memory. */
class MemoryInterface {
 protected:
  virtual ~MemoryInterface() {}

 public:
  /** Gets the total number of bytes of virtual memory. */
  virtual int64_t GetTotal() const = 0;
  /** Gets the total number of bytes of virtual memory currently free. */
  virtual int64_t GetFree() const = 0;
  /** Gets the number of bytes of virtual memory currently in use. */
  virtual int64_t GetUsed() const = 0;
  /** Gets the number of bytes of physical memory currently free. */
  virtual int64_t GetFreePhysical() const = 0;
  /** Gets the total number of bytes of physical memory. */
  virtual int64_t GetTotalPhysical() const = 0;
  /** Gets the number of bytes of physical memory currently in use. */
  virtual int64_t GetUsedPhysical() const = 0;
};

} // namespace system

} // namespace framework

} // namespace ggadget

#endif // GGADGET_FRAMEWORK_SYSTEM_MEMORY_INTERFACE_H__
