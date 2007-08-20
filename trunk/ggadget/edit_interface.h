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

#ifndef  GGADGET_EDIT_INTERFACE_H_
#define  GGADGET_EDIT_INTERFACE_H_

namespace ggadget {

class Canvas;

/**
 * This is the interface for a edit control.
 */
class EditInterface {

 public:

  /** Ask the GtkEdit to draw itself */
  virtual void Expose(Canvas *canvas) = 0;

  /** Handler of the ButtonPress event */
  virtual void ButtonPress(GdkEventButton *event) = 0;

  /** Handler of the ButtonPress event */
  virtual void ButtonRelease(GdkEventButton *event) = 0;

  /** Handler of the Mouse Motion event */
  virtual void MotionNotify(GdkEventMotion *event) = 0;

  /** Handler of the KeyPress event */
  virtual void KeyPress(GdkEventKey *event) = 0;

  /** Handler of the KeyRelease event */
  virtual void KeyRelease(GdkEventKey *event) = 0;

  /** Set whether the text is bold */
  virtual void SetBold(virtual bool bold) = 0;

  /** Set whether the text is italic */
  virtual void SetItalic(virtual bool italic) = 0;

  /** Set whether the text is struck-out */
  virtual void SetStrikeout(virtual bool strikeout) = 0;

  /** Set whether the text is underlined */
  virtual void SetUnderline(virtual bool underline) = 0;

  /** Set whether the text will be shown in multiple lines */
  virtual void SetMultiLine(virtual bool multiline) = 0;

  /** Set whether the text will be wrapped */
  virtual void SetWordWrap(virtual bool wrap) = 0;

  /** Set the content of the text */
  virtual void SetText(const string &text) = 0;

  /** Set the background color */
  virtual void SetBackgroundColor(const string &color) = 0;

  /** Set the text color */
  virtual void SetTextColor(const string &color) = 0;

  /** Set the text font */
  virtual void SetFont(const string &font) = 0;

  /** Set the password char */
  virtual void SetPasswordChar(const char *c) = 0;

  /** Retrieve whether the text is bold */
  virtual bool GetBold() = 0;

  /** Retrieve whether the text is italic */
  virtual bool GetItalic() = 0;

  /** Retrieve whether the text is struck-out */
  virtual bool GetStrikeout() = 0;

  /** Retrieve whether the text is underlined */
  virtual bool GetUnderline() = 0;

  /** Retrieve whether the text will be shown in multiple lines */
  virtual bool GetMultiLine() = 0;

  /** Retrieve whether the text will be wrapped */
  virtual bool GetWrap() = 0;

  /** Retrieve the content the text */
  virtual string GetText() = 0;

  /** Retrieve the background color */
  virtual string GetBackgroundColor() = 0;

  /** Retrieve the text color */
  virtual string GetTextColor() = 0;

  /** Retrieve the current font */
  virtual string GetFont() = 0;

  /** Retrieve the password char */
  virtual char GetPasswordChar() = 0;

};  //class EditInterface

}   //namespace ggadget

#endif  // GGADGET_EDIT_INTERFACE_H_

