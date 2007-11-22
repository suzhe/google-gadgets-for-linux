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

#ifndef GGADGET_TEXT_FRAME_H__
#define GGADGET_TEXT_FRAME_H__

#include <ggadget/canvas_interface.h>
#include <ggadget/variant.h>

namespace ggadget {

class Texture;
class BasicElement;
class ViewInterface;

class TextFrame {
 public:
  TextFrame(BasicElement *owner, ViewInterface *view);
  ~TextFrame();
  
  /** Gets and sets the text of the frame. */
  const char *GetText() const;
  void SetText(const char *text);
  
  void DrawWithTexture(CanvasInterface *canvas, double x, double y, 
                       double width, double height, Texture *texture);
  void Draw(CanvasInterface *canvas, double x, double y, 
            double width, double height);
  
  /** 
   * Returns the width and height required to display the text 
   * without wrapping or trimming.
   */ 
  bool GetSimpleExtents(CanvasInterface *canvas, double *width, double *height);
  
 public: // registered properties  
  /** Gets and sets the text horizontal alignment. */
  CanvasInterface::Alignment GetAlign() const;
  void SetAlign(CanvasInterface::Alignment align);
  
  /** Gets and sets whether the text is bold. */
  bool IsBold() const;
  void SetBold(bool bold);

  /**
   * Gets and sets the text color or image texture of the text. The image is
   * repeated if necessary, not stretched.
   */
  Variant GetColor() const;
  void SetColor(const Variant &color);

  /** Gets and sets the text font. */
  const char *GetFont() const;
  void SetFont(const char *font);
  
  /** Gets and sets whether the text is italicized. */
  bool IsItalic() const;
  void SetItalic(bool italic);

  /** Gets and sets the text size in points. */
  int GetSize() const;
  void SetSize(int size);

  /** Gets and sets whether the text is struke-out. */
  bool IsStrikeout() const;
  void SetStrikeout(bool strikeout);
 
  /** Gets and sets the trimming mode when the text is too large to display. */
  CanvasInterface::Trimming GetTrimming() const;
  void SetTrimming(CanvasInterface::Trimming trimming);

  /** Gets and sets whether the text is underlined. */
  bool IsUnderline() const;
  void SetUnderline(bool underline);

  /** Gets and sets the text vertical alignment. */
  CanvasInterface::VAlignment GetVAlign() const;
  void SetVAlign(CanvasInterface::VAlignment valign);

  /** Gets and sets whether to wrap the text when it's too large to display. */
  bool IsWordWrap() const;
  void SetWordWrap(bool wrap); 
 
 private:
  BasicElement *owner_;
  ViewInterface *view_;
   
  FontInterface *font_;
  Texture *color_texture_;
  CanvasInterface::Alignment align_;
  CanvasInterface::VAlignment valign_;
  CanvasInterface::Trimming trimming_;
  bool bold_, italic_;
  CanvasInterface::TextFlag flag_;
  int size_;
  std::string font_name_, color_, text_;
  
  bool SetUpFont();
  void ClearFont();
};

} // namespace ggadget

#endif // GGADGET_TEXT_FRAME_H__
