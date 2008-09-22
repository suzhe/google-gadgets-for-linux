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

#ifndef GGADGET_HOST_UTILS_H__
#define GGADGET_HOST_UTILS_H__

#include <string>

namespace ggadget {

template <typename R> class Slot0;
class OptionsInterface;

/**
 * Setup the global file manager.
 * @param profile_dir path name of the user profile directory.
 * @return @c true if succeeds.
 */
bool SetupGlobalFileManager(const char *profile_dir);

/**
 * Setup the logger.
 * @param log_level the minimum @c LogLevel.
 * @param long_log whether to output logs using long format.
 */
void SetupLogger(int log_level, bool long_log);

/**
 * Checks if the required extensions are properly loaded.
 * @param[out] message the error message that should be displayed to the user
 *     if any required extension is not property loaded.
 * @return @c true if all the required extensions are property loaded.
 */
bool CheckRequiredExtensions(std::string *error_message);

/**
 * Initialize the default user agent for XMLHttpRequest class.
 * @param app_name the name of the main application.
 */
void InitXHRUserAgent(const char *app_name);

/**
 * Get popup position to show a (w1, h1) rectangle for an existing (x,
 * y, w, h) rectangle on a scene of sw width and sh height.
 */
void GetPopupPosition(int x, int y, int w, int h,
                      int w1, int h1, int sw, int sh,
                      int *x1, int *y1);

} // namespace ggadget

#endif // GGADGET_HOST_UTILS_H__
