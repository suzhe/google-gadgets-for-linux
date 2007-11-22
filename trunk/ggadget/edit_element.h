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

#ifndef GGADGET_EDIT_ELEMENT_H__
#define GGADGET_EDIT_ELEMENT_H__

#include <ggadget/div_element.h>

namespace ggadget {

class EditElement : public DivElement {
 public:
  DEFINE_CLASS_ID(0xc321ec8aeb4142c4, DivElement);

  EditElement(ElementInterface *parent, ViewInterface *view, const char *name);
  virtual ~EditElement();

 public:
  /** Gets and sets whether the text is bold. */
  bool IsBold() const;
  void SetBold(bool bold);

  /**
   * Gets and sets the text color or image texture of the text. The image is
   * repeated if necessary, not stretched.
   */
  const char *GetColor() const;
  void SetColor(const char *color);

  /** Gets and sets the text font. */
  const char *GetFont() const;
  void SetFont(const char *font);

  /** Gets and sets whether the text is italicized. */
  bool IsItalic() const;
  void SetItalic(bool italic);

  /**
   * Gets and sets whether the edit can display multiple lines of text.
   * If @c false, incoming "\n"s are ignored, as are presses of the Enter key.
   */
  bool IsMultiline() const;
  void SetMultiline(bool multiline);

  /**
   * Gets and sets the character that should be displayed each time the user
   * enters a character. By default, the value is @c '\0', which means that the
   * typed character is displayed as-is.
   */
  char GetPasswordChar() const;
  void SetPassordChar(char passwordChar);

  /** Gets and sets the text size in points. */
  int GetSize() const;
  void SetSize(int size);

  /** Gets and sets whether the text is struke-out. */
  bool IsStrikeout() const;
  void SetStrikeout(bool strikeout);
 
  /** Gets and sets whether the text is underlined. */
  bool IsUnderline() const;
  void SetUnderline(bool underline);

  /** Gets and sets the value of the element. */
  const char *GetValue() const;
  void SetValue(const char *value);

  /** Gets and sets whether to wrap the text when it's too large to display. */
  bool IsWordWrap() const;
  void SetWordWrap(bool wrap);

  virtual bool OnMouseEvent(MouseEvent *event, bool direct,
                            ElementInterface **fired_element);
  virtual bool OnKeyEvent(KeyboardEvent *event);

 public:
  static ElementInterface *CreateInstance(ElementInterface *parent,
                                          ViewInterface *view,
                                          const char *name);

 protected:
  virtual void DoDraw(CanvasInterface *canvas,
                      const CanvasInterface *children_canvas);

 private:
  DISALLOW_EVIL_CONSTRUCTORS(EditElement);

  class Impl;
  Impl *impl_;
};

} // namespace ggadget

#endif // GGADGET_EDIT_ELEMENT_H__
