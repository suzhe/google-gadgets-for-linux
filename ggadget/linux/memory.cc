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

#include "memory.h"

namespace ggadget {
namespace framework {

int64_t Memory::GetTotal() const {
  return 12345678;
}

int64_t Memory::GetFree() const {
  return 2345678;
}

int64_t Memory::GetUsed() const {
  return 98765434;
}

int64_t Memory::GetFreePhysical() const {
  return 123456;
}

int64_t Memory::GetTotalPhysical() const {
  return 1223456;
}

int64_t Memory::GetUsedPhysical() const {
  return 7623456;
}

} // namespace framework
} // namespace ggadget
