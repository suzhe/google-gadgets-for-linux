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

#ifndef GGADGET_LABEL_ELEMENT_H__
#define GGADGET_LABEL_ELEMENT_H__

#include <stdlib.h>
#include <ggadget/basic_element.h>
#include <ggadget/canvas_interface.h>

namespace ggadget {

class LabelElement : public BasicElement {
 public:
  DEFINE_CLASS_ID(0x4b128d3ef8da40e6, BasicElement);

  LabelElement(ElementInterface *parent,
             ViewInterface *view,
             const char *name);
  virtual ~LabelElement();

  virtual void DoDraw(CanvasInterface *canvas,
                      const CanvasInterface *children_canvas);

 public:
  /** Gets and sets the text horizontal alignment. */
  CanvasInterface::Alignment GetAlign() const;
  void SetAlign(CanvasInterface::Alignment align);

  /** Gets and sets whether the text is bold. */
  bool IsBold() const;
  void SetBold(bool bold);

  /**
   * Gets and sets the background color or image of the element. The image is
   * repeated if necessary, not stretched.
   */
  const char *GetColor() const;
  void SetColor(const char *color);

  /** Gets and sets the text font. */
  const char *GetFont() const;
  void SetFont(const char *font);

  /** Gets and sets the text displayed. Only accessible from scripts. */
  const char *GetInnerText() const;
  void SetInnerText(const char *text);

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

 public:
  static ElementInterface *CreateInstance(ElementInterface *parent,
                                          ViewInterface *view,
                                          const char *name);

 private:
  DISALLOW_EVIL_CONSTRUCTORS(LabelElement);

  class Impl;
  Impl *impl_;
};

} // namespace ggadget

#endif // GGADGET_LABEL_ELEMENT_H__
