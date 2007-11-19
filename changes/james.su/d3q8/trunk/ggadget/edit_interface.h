/*
  Copyright 2007 Google Inc.

  Licensed under the Apache License, Version 2.0 (the "License") = 0;
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

#ifndef GGADGET_EDIT_INTERFACE_H__
#define GGADGET_EDIT_INTERFACE_H__

#include <ggadget/color.h>

namespace ggadget {

class KeyboardEvent;
class MouseEvent;
class CanvasInterface;

/**
 * This is the interface for a edit control.
 */
class EditInterface {

 public:

  /** Draw the edit control */
  virtual CanvasInterface *Draw(bool *modified) = 0;

  /** Handler of the ButtonPress event */
  virtual void OnButtonPress(const MouseEvent &event) = 0;

  /** Handler of the ButtonPress event */
  virtual void OnButtonRelease(const MouseEvent &event) = 0;

  /** Handler of the Mouse Motion event */
  virtual void OnMotionNotify(const MouseEvent &event) = 0;

  /** Handler of the KeyPress event */
  virtual void OnKeyPress(const KeyboardEvent &event) = 0;

  /** Handler of the KeyRelease event */
  virtual void OnKeyRelease(const KeyboardEvent &event) = 0;


  /** Set the width of the edit control */
  virtual void SetWidth(int width) = 0;

  /** Get the width of the edit control */
  virtual int GetWidth() = 0;

  /** Set the height of the edit control */
  virtual void SetHeight(int width) = 0;

  /** Get the height of the edit control */
  virtual int GetHeight() = 0;

  /** Set whether the text is bold */
  virtual void SetBold(bool bold) = 0;

  /** Retrieve whether the text is bold */
  virtual bool IsBold() = 0;

  /** Set whether the text is italic */
  virtual void SetItalic(bool italic) = 0;

  /** Retrieve whether the text is italic */
  virtual bool IsItalic() = 0;

  /** Set whether the text is struck-out */
  virtual void SetStrikeout(bool strikeout) = 0;

  /** Retrieve whether the text is struck-out */
  virtual bool IsStrikeout() = 0;

  /** Set whether the text is underlined */
  virtual void SetUnderline(bool underline) = 0;

  /** Retrieve whether the text is underlined */
  virtual bool IsUnderline() = 0;

  /** Set whether the text will be shown in multiple lines */
  virtual void SetMultiline(bool multiline) = 0;

  /** Retrieve whether the text will be shown in multiple lines */
  virtual bool IsMultiline() = 0;

  /** Set whether the text will be wrapped */
  virtual void SetWordWrap(bool wrap) = 0;

  /** Retrieve whether the text will be wrapped */
  virtual bool IsWordWrap() = 0;

  /** Set the content of the text */
  virtual void SetText(const char *text) = 0;

  /** Retrieve the content the text */
  virtual const char *GetText() = 0;

  /** Set the background color */
  virtual void SetBackgroundColor(const Color &color) = 0;

  /** Retrieve the background color */
  virtual Color GetBackgroundColor() = 0;

  /** Set the text color */
  virtual void SetTextColor(const Color &color) = 0;

  /** Retrieve the text color */
  virtual Color GetTextColor() = 0;

  /** Set the text font */
  virtual void SetFont(const char *font) = 0;

  /** Retrieve the text font */
  virtual const char* GetFont() = 0;

  /** Set the password char */
  virtual void SetPasswordChar(const char *c) = 0;

  /** Retrieve the password char */
  virtual const char* GetPasswordChar() = 0;


};  //class EditInterface

}   //namespace ggadget

#endif  // GGADGET_EDIT_INTERFACE_H__
