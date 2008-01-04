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
#include <ggadget/event.h>
#include <ggadget/slot.h>
#include <ggadget/signals.h>

namespace ggadget {

class CanvasInterface;
class Texture;

/**
 * This is the interface for a edit control.
 */
class EditInterface {
 protected:
  virtual ~EditInterface() { }

 public:
  /** Destroy the EditInterface instance */
  virtual void Destroy() = 0;

  /** Draw the edit control */
  virtual void Draw(CanvasInterface *canvas) = 0;

  /** Handler of mouse event */
  virtual EventResult OnMouseEvent(const MouseEvent &event) = 0;

  /** Handler of Key event */
  virtual EventResult OnKeyEvent(const KeyboardEvent &event) = 0;

  /** Set input focus to the EditInterface instance */
  virtual void FocusIn() = 0;

  /** Remove input focus from the EditInterface instance */
  virtual void FocusOut() = 0;

  /** Set width of the edit control canvas in pixel */
  virtual void SetWidth(int width) = 0;

  /** Get width of the edit control canvas */
  virtual int GetWidth() = 0;

  /** Set height of the edit control canvas in pixel */
  virtual void SetHeight(int height) = 0;

  /** Get height of the edit control canvas */
  virtual int GetHeight() = 0;

  /** Get the actual size that is ideal for drawing the edit control,
   * may larger than actual canvas size */
  virtual void GetSizeRequest(int *width, int *height) = 0;

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

  /** Set whether the edit control is readonly or not */
  virtual void SetReadOnly(bool readonly) = 0;

  /** Check if the edit control is readonly */
  virtual bool IsReadOnly() = 0;

  /** Set the content of the text */
  virtual void SetText(const char *text) = 0;

  /** Retrieve the content the text */
  virtual std::string GetText() = 0;

  /** Set the background texture. Ownership of @background object will be
   * assumed by the Edit object. So the caller shall not delete it. */
  virtual void SetBackground(Texture *background) = 0;

  /** Retrieve the background texture. The returned Texture object is owned by
   * the Edit object, caller shall not change or delete it. */
  virtual const Texture *GetBackground() = 0;

  /** Set the text color */
  virtual void SetTextColor(const Color &color) = 0;

  /** Retrieve the text color */
  virtual Color GetTextColor() = 0;

  /** Set font family */
  virtual void SetFontFamily(const char *font) = 0;

  /** Retrieve font family */
  virtual std::string GetFontFamily() = 0;

  /** Set font size in pixel */
  virtual void SetFontSize(int size) = 0;

  /** Retrieve font size in pixel */
  virtual int GetFontSize() = 0;

  /** Set the password char, @c shall be pointed to a valid UTF-8 char
   * if @c == NULL then disable the password char feature */
  virtual void SetPasswordChar(const char *c) = 0;

  /** Retrieve the password char */
  virtual std::string GetPasswordChar() = 0;

  /** Check if a scroll bar is required. A scroll bar is required if the
   * current canvas size is not enough to display the content.
   * Only vertical scroll bar is supported. */
  virtual bool IsScrollBarRequired() = 0;

  /** Get the information required for scroll bar.
   * The scroll range is from 0 to @c range. If @c range == 0, then means no
   * scroll is needed. */
  virtual void GetScrollBarInfo(int *range, int *line_step,
                                int *page_step, int *cur_pos) = 0;

  /** Request the edit control to scroll to a specified position.
   * The position shall inside the scroll bar range [0, range],
   * returned by GetScrollBarInfo() function. */
  virtual void ScrollTo(int position) = 0;

  /**
   * Sets a redraw mark so that the edit control will be redrawed during the
   * next draw.
   */
  virtual void MarkRedraw() = 0;

  /** Register a callback to fire the queue draw request.
   * The specified callback slot must be created by NewSlot() function.
   * the returned <code>Connection*</code> can be used to remove the callback,
   * but itself is owned by the edit control and can't be deleted. */
  virtual Connection* ConnectOnQueueDraw(Slot0<void> *callback) = 0;

  /** Register a callback to fire the text changed event.
   * Whenever the text content is changed, this callback will be called.
   * The specified callback slot must be created by NewSlot() function.
   * the returned <code>Connection*</code> can be used to remove the callback,
   * but itself is owned by the edit control and can't be deleted. */
  virtual Connection* ConnectOnTextChanged(Slot0<void> *callback) = 0;
};  //class EditInterface

}   //namespace ggadget

#endif  // GGADGET_EDIT_INTERFACE_H__
