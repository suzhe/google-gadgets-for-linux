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

#ifndef GGADGET_EDIT_ELEMENT_BASE_H__
#define GGADGET_EDIT_ELEMENT_BASE_H__

#include <string>
#include <ggadget/basic_element.h>
#include <ggadget/scrolling_element.h>

namespace ggadget {

/**
 * Base class for EditElement. Real EditElement implementation shall inherit
 * this class and implement its pure virtual functions.
 *
 * All scriptable properties will be registered in EditElementBase class,
 * derived class does not need to care about them.
 */
class EditElementBase : public ScrollingElement {
 public:
  DEFINE_CLASS_ID(0x6C5D2E793806428F, ScrollingElement);

  EditElementBase(BasicElement *parent, View *view, const char *name);
  virtual ~EditElementBase();

 protected:
  virtual void DoRegister();

 public:
  /**
   * Connects a specified slot to OnChange event signal.
   * The signal will be fired when FireOnChangeEvent() method is called by
   * derived class.
   */
  Connection *ConnectOnChangeEvent(Slot0<void> *slot);

 public:
  /** Gets and sets the background color or texture of the text */
  virtual Variant GetBackground() const = 0;
  virtual void SetBackground(const Variant &background) = 0;

  /** Gets and sets whether the text is bold. */
  virtual bool IsBold() const = 0;
  virtual void SetBold(bool bold) = 0;

  /** Gets and sets the text color of the text. */
  virtual std::string GetColor() const = 0;
  virtual void SetColor(const char *color) = 0;

  /** Gets and sets the text font. */
  virtual std::string GetFont() const = 0;
  virtual void SetFont(const char *font) = 0;

  /** Gets and sets whether the text is italicized. */
  virtual bool IsItalic() const = 0;
  virtual void SetItalic(bool italic) = 0;

  /**
   * Gets and sets whether the edit can display multiple lines of text.
   * If @c false, incoming "\n"s are ignored, as are presses of the Enter key.
   */
  virtual bool IsMultiline() const = 0;
  virtual void SetMultiline(bool multiline) = 0;

  /**
   * Gets and sets the character that should be displayed each time the user
   * enters a character. By default, the value is @c "\0", which means that the
   * typed character is displayed as-is.
   */
  virtual std::string GetPasswordChar() const = 0;
  virtual void SetPasswordChar(const char *passwordChar) = 0;

  /** Gets and sets the text size in points. */
  virtual double GetSize() const = 0;
  virtual void SetSize(double size) = 0;

  /** Gets and sets whether the text is struke-out. */
  virtual bool IsStrikeout() const = 0;
  virtual void SetStrikeout(bool strikeout) = 0;

  /** Gets and sets whether the text is underlined. */
  virtual bool IsUnderline() const = 0;
  virtual void SetUnderline(bool underline) = 0;

  /** Gets and sets the value of the element. */
  virtual std::string GetValue() const = 0;
  virtual void SetValue(const char *value) = 0;

  /** Gets and sets whether to wrap the text when it's too large to display. */
  virtual bool IsWordWrap() const = 0;
  virtual void SetWordWrap(bool wrap) = 0;

  /** Gets and sets whether the edit element is readonly */
  virtual bool IsReadOnly() const = 0;
  virtual void SetReadOnly(bool readonly) = 0;

  /** Gets the ideal bounding rect for the edit element which is large enough
   * for displaying the content without scrolling. */
  virtual void GetIdealBoundingRect(int *width, int *height) = 0;

 public:
  /**
   * Derived class shall call this method if the value is changed.
   */
  void FireOnChangeEvent() const;

 private:
  DISALLOW_EVIL_CONSTRUCTORS(EditElementBase);

  class Impl;
  Impl *impl_;
};

} // namespace ggadget

#endif // GGADGET_EDIT_ELEMENT_BASE_H__
