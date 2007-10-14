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

#ifndef GGADGET_FRAMEWORK_SYSTEM_PERFMON_H__
#define GGADGET_FRAMEWORK_SYSTEM_PERFMON_H__

#include "perfmon_interface.h"

namespace ggadget {

namespace framework {

namespace system {

class Perfmon : public PerfmonInterface {
 public:
  virtual int64_t GetCurrentValue(const char *counter_path);
};

} // namespace system

} // namespace framework

} // namespace ggadget

#endif // GGADGET_FRAMEWORK_SYSTEM_PERFMON_H__
