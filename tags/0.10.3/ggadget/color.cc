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

#include <cstring>
#include "logger.h"
#include "color.h"

namespace ggadget {

const Color Color::kWhite(1, 1, 1);
const Color Color::kBlack(0, 0, 0);
const Color Color::kMiddleColor(0.5, 0.5, 0.5);

std::string Color::ToString() const {
  return StringPrintf("#%02X%02X%02X",
                      static_cast<int>(round(red * 255)),
                      static_cast<int>(round(green * 255)),
                      static_cast<int>(round(blue * 255)));
}

Color Color::FromChars(unsigned char r, unsigned char g, unsigned char b) {
  return Color(r / 255.0, g / 255.0, b / 255.0);
}

bool Color::FromString(const char *name, Color *color, double *alpha) {
  ASSERT(name && color);

  if (!name || ! *name || !color)
    return false;

  size_t len = strlen(name);
  if (name[0] == '#' && (len == 7 || (len == 9 && alpha))) {
    std::string name_copy(name);
    // Set all invalid chars to '0', which is required for compatibility.
    for (size_t i = 1; i < len; ++i) {
      char c = name_copy[i];
      if ((c < '0' || c > '9') && (c < 'A' || c > 'F') && (c < 'a' || c > 'f'))
        name_copy[i] = '0';
    }

    int r = 0, g = 0, b = 0, a = 255;
    const char *ptr = name_copy.c_str() + 1;
    if ((len == 7 && sscanf(ptr, "%02x%02x%02x", &r, &g, &b) == 3) ||
        (len == 9 && sscanf(ptr, "%02x%02x%02x%02x", &a, &r, &g, &b) == 4)) {
      *color = Color(r / 255.0, g / 255.0, b / 255.0);
      if (alpha)
        *alpha = a / 255.0;
      return true;
    }
  }

  LOG("Color::FromString(%s): Invalid color name.", name);
  return false;
}

} // namespace ggadget
