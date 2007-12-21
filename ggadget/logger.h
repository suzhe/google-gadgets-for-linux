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

#ifndef GGADGET_LOGGER_H__
#define GGADGET_LOGGER_H__

#include <ggadget/common.h>

namespace ggadget {

#undef LOG
#undef ASSERT_M
#undef VERIFY
#undef VERIFY_M
#undef DLOG

struct LogHelper {
  LogHelper(const char *file, int line);
  void operator()(const char *format, ...) PRINTF_ATTRIBUTE(2, 3);
  const char *file_;
  int line_;
};

/**
 * Print log with printf format parameters.
 * It works in both debug and release versions.
 */
#define LOG ::ggadget::LogHelper(__FILE__, __LINE__)

#ifdef NDEBUG
#define ASSERT_M(x, y)
#define VERIFY(x)
#define VERIFY_M(x, y)
#define DLOG  true ? (void) 0 : LOG
#else // NDEBUG

/**
 * Assert an expression with a message in printf format.
 * It only works in debug versions.
 * Sample usage: <code>ASSERT_M(a==b, ("%d==%d failed\n", a, b));</code>
 */
#define ASSERT_M(x, y) do { if (!(x)) { DLOG y; ASSERT(x); } } while (0)

/**
 * Verify an expression and print a message if the expression is not true.
 * It only works in debug versions.
 */
#define VERIFY(x) do { if (!(x)) DLOG("VERIFY FAILED: %s", #x); } while (0)

/**
 * Verify an expression with a message in printf format.
 * It only works in debug versions.
 * Sample usage: <code>VERIFY_M(a==b, ("%d==%d failed\n", a, b));</code>
 */
#define VERIFY_M(x, y) do { if (!(x)) { DLOG y; VERIFY(x); } } while (0)

/**
 * Print debug log with printf format parameters.
 * It only works in debug versions.
 */
#define DLOG LOG

#endif // else NDEBUG

} // namespace ggadget

#endif  // GGADGET_LOGGER_H__
