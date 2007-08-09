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

#ifndef GGADGET_FONT_INTERFACE_H__
#define GGADGET_FONT_INTERFACE_H__

namespace ggadget {

/**
 * Class representing a font as understood by the graphics interface. 
 * It is created by an associated GraphicsInterface object.
 * Call @c Destroy() to free this object once it is no longer needed.
 */
class FontInterface {
 public:   
  /**
   * Enum used to specify font styles.
   */
  enum Style {
    STYLE_NORMAL,
    STYLE_ITALIC
  };
   
  /**
   * Enum used to specify font weight.
   */
  enum Weight {
    WEIGHT_NORMAL,
    WEIGHT_BOLD
  };
   
  /**
   * @return The style option associated with the font.
   */
  virtual Style style() const = 0;
    
  /**
   * @return The style option associated with the font.
   */
  virtual Weight weight() const = 0;
  
  /**
   * @return The size of the font, in points.
   */
  virtual size_t pt_size() const = 0;
  
  /**
   * Frees the FontInterface object.
   */
  virtual void Destroy() = 0;
  
  /**
   * @return A name unique to this class.
   */
  virtual const char *class_type() const = 0;
};
  
} // namespace ggadget

#endif // GGADGET_FONT_INTERFACE_H__
