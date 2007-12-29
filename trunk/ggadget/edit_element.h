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

#include <string>
#include <ggadget/basic_element.h>
#include <ggadget/scrolling_element.h>

namespace ggadget {

class EditElement : public ScrollingElement {
 public:
  DEFINE_CLASS_ID(0xc321ec8aeb4142c4, BasicElement);

  EditElement(BasicElement *parent, View *view, const char *name);
  virtual ~EditElement();

  virtual void Layout();
  virtual void MarkRedraw();

 public:
  /** Gets and sets the background color or texture of the text */
  Variant GetBackground() const;
  void SetBackground(const Variant &background);

  /** Gets and sets whether the text is bold. */
  bool IsBold() const;
  void SetBold(bool bold);

  /** Gets and sets the text color of the text. */
  std::string GetColor() const;
  void SetColor(const char *color);

  /** Gets and sets the text font. */
  std::string GetFont() const;
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
   * enters a character. By default, the value is @c "\0", which means that the
   * typed character is displayed as-is.
   */
  std::string GetPasswordChar() const;
  void SetPasswordChar(const char *passwordChar);

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
  std::string GetValue() const;
  void SetValue(const char *value);

  /** Gets and sets whether to wrap the text when it's too large to display. */
  bool IsWordWrap() const;
  void SetWordWrap(bool wrap);

  /** Gets and sets whether the edit element is readonly */
  bool IsReadOnly() const;
  void SetReadOnly(bool readonly);

  /** Gets the ideal bounding rect for the edit element which is large enough
   * for displaying the content without scrolling. */
  void GetIdealBoundingRect(int *width, int *height);

  Connection *ConnectOnChangeEvent(Slot0<void> *slot);

 public:
  static BasicElement *CreateInstance(BasicElement *parent, View *view,
                                      const char *name);

 protected:
  virtual void DoDraw(CanvasInterface *canvas,
                      const CanvasInterface *children_canvas);
  virtual EventResult HandleMouseEvent(const MouseEvent &event);
  virtual EventResult HandleKeyEvent(const KeyboardEvent &event);
  virtual EventResult HandleOtherEvent(const Event &event);
  virtual void GetDefaultSize(double *width, double *height) const;

 private:
  DISALLOW_EVIL_CONSTRUCTORS(EditElement);

  class Impl;
  Impl *impl_;
};

} // namespace ggadget

#endif // GGADGET_EDIT_ELEMENT_H__
