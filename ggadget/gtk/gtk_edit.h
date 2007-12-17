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

#ifndef  GGADGET_GTK_GTKEDIT_H__
#define  GGADGET_GTK_GTKEDIT_H__

#include <functional>
#include <string>
#include <stack>
#include <vector>
#include <gtk/gtk.h>
#include <cairo.h>

#include <ggadget/common.h>
#include <ggadget/edit_interface.h>

namespace ggadget {

class Texture;

namespace gtk {

class CairoCanvas;
class CairoGraphics;
class GtkViewHost;
class GtkGadgetHost;

/** GtkEdit is the gtk implementation of EditInterface */
class GtkEdit : public EditInterface {
 protected:
  virtual ~GtkEdit();

 public:
  GtkEdit(GtkViewHost *host, int width, int height);
  virtual void Destroy();
  virtual CanvasInterface *Draw(bool *modified);
  virtual EventResult OnMouseEvent(const MouseEvent &event);
  virtual EventResult OnKeyEvent(const KeyboardEvent &event);
  virtual void FocusIn();
  virtual void FocusOut();
  virtual void SetWidth(int width);
  virtual int GetWidth();
  virtual void SetHeight(int height);
  virtual int GetHeight();
  virtual void GetSizeRequest(int *width, int *height);
  virtual void SetBold(bool bold);
  virtual bool IsBold();
  virtual void SetItalic(bool italic);
  virtual bool IsItalic();
  virtual void SetStrikeout(bool strikeout);
  virtual bool IsStrikeout();
  virtual void SetUnderline(bool underline);
  virtual bool IsUnderline();
  virtual void SetMultiline(bool multiline);
  virtual bool IsMultiline();
  virtual void SetWordWrap(bool wrap);
  virtual bool IsWordWrap();
  virtual void SetReadOnly(bool readonly);
  virtual bool IsReadOnly();
  virtual void SetText(const char *text);
  virtual std::string GetText();
  virtual void SetBackground(Texture *background);
  virtual const Texture *GetBackground();
  virtual void SetTextColor(const Color &color);
  virtual Color GetTextColor();
  virtual void SetFontFamily(const char *font);
  virtual std::string GetFontFamily();
  virtual void SetFontSize(int size);
  virtual int GetFontSize();
  virtual void SetPasswordChar(const char *c);
  virtual std::string GetPasswordChar();
  virtual bool IsScrollBarRequired();
  virtual void GetScrollBarInfo(int *range, int *line_step,
                                int *page_step, int *cur_pos);
  virtual void ScrollTo(int position);
  virtual Connection* ConnectOnQueueDraw(Slot0<void> *callback);
  virtual Connection* ConnectOnTextChanged(Slot0<void> *callback);

 private:
  /**
   * Enum used to specify different motion types.
   */
  enum MovementStep {
    VISUALLY,
    WORDS,
    DISPLAY_LINES,
    DISPLAY_LINE_ENDS,
    PAGES,
    BUFFER
  };

  /** Remove the cached layout. */
  void ResetLayout();
  /**
   * Create pango layout on-demand. If the layout is not changed, return the
   * cached one.
   */
  PangoLayout* EnsureLayout();
  /** Create a new layout containning current edit content */
  PangoLayout* CreateLayout();
  /** Create cairo canvas on-demand. */
  CairoCanvas* EnsureCanvas();
  /** Adjust the scroll information */
  void AdjustScroll();
  /**
   * Send out a request to refresh all informations of the edit control
   * and queue a draw request.
   * If @c relayout is true then the layout will be regenerated.
   * */
  void QueueRefresh(bool relayout);
  /** Callback to do real refresh task */
  bool RefreshCallback(int timer_id);
  /** Send a request to redraw the edit control */
  void QueueDraw();
  /** Reset the input method context */
  void ResetImContext();
  /** Reset preedit text */
  void ResetPreedit();
  /** Create a new im context according to current visibility setting */
  void InitImContext();
  /** Set the visibility of the edit control */
  void SetVisibility(bool visible);

  /** Check if the cursor should be blinking */
  bool IsCursorBlinking();
  /** Send out a request to blink the cursor if necessary */
  void QueueCursorBlink();
  /** Timer callback to blink the cursor */
  bool CursorBlinkCallback(int timer_id);
  void ShowCursor();
  void HideCursor();

  /** Draw the Cursor to the canvas */
  void DrawCursor(CairoCanvas *canvas);
  /** Draw the text to the canvas */
  void DrawText(CairoCanvas *canvas);

  /** Move cursor */
  void MoveCursor(MovementStep step, int count, bool extend_selection);
  /** Move cursor visually, meaning left or right */
  int MoveVisually(int current_pos, int count);
  /** Move cursor in words */
  int MoveWords(int current_pos, int count);
  /** Move cursor in display lines */
  int MoveDisplayLines(int current_pos, int count);
  /** Move cursor in pages */
  int MovePages(int current_pos, int count);
  /** Move cursor to the beginning or end of a display line */
  int MoveLineEnds(int current_pos, int count);

  /** Set the current cursor offset, in number of characters. */
  void SetCursor(int cursor);
  /** Get the most reasonable character offset according to the pixel
   * coordinate in the layout */
  int XYToOffset(int x, int y);
  /** Get the offset range that is currently selected,in number of characters.*/
  bool GetSelectionBounds(int *start, int *end);
  /** Set the offest range that should be selected, in number of characters. */
  void SetSelectionBounds(int selection_bound, int cursor);

  /** Insert text at current caret position */
  void EnterText(const char *str);
  /** Delete text in a specified range, in number of characters. */
  void DeleteText(int start, int end);

  /** Select the current word under cursor */
  void SelectWord();
  /** Select the current display line under cursor */
  void SelectLine();
  /** Select all text */
  void SelectAll();
  /** Delete the text that is currently selected */
  void DeleteSelection();

  /** Cut the current selected text to the clipboard */
  void CutClipboard();
  /** Copy the current selected text to the clipboard */
  void CopyClipboard();
  /** Paste the text in the clipboard to current offset */
  void PasteClipboard();
  /** Delete a character before the offset of the cursor */
  void BackSpace();
  /** Delete a character at the offset of the cursor */
  void Delete();
  /** Switch between the overwrite mode and the insert mode*/
  void ToggleOverwrite();

  /** Gets the color of selection background. */
  Color GetSelectionBackgroundColor();
  /** Gets the color of selection text. */
  Color GetSelectionTextColor();

  /**
   * Gets the cursor location for gtk im context. relative to the widget
   * coordinate
   */
  void GetCursorLocationForIMContext(GdkRectangle *cur);

  /** Callback function for IM "commit" signal */
  static void CommitCallback(GtkIMContext *context,
                             const char *str, void *gg);
  /** Callback function for IM "retrieve-surrounding" signal */
  static gboolean RetrieveSurroundingCallback(GtkIMContext *context,
                                              void *gg);
  /** Callback function for IM "delete-surrounding" signal */
  static gboolean DeleteSurroundingCallback(GtkIMContext *context, int offset,
                                            int n_chars, void *gg);
  /** Callback function for IM "preedit-start" signal */
  static void PreeditStartCallback(GtkIMContext *context, void *gg);
  /** Callback function for IM "preedit-changed" signal */
  static void PreeditChangedCallback(GtkIMContext *context, void *gg);
  /** Callback function for IM "preedit-end" signal */
  static void PreeditEndCallback(GtkIMContext *context, void *gg);
  /**
   * Callback for gtk_clipboard_request_text function.
   * This function performs real paste.
   */
  static void PasteCallback(GtkClipboard *clipboard,
                            const gchar *str, void *gg);

  /** View host of the view which contains the edit element. */
  GtkViewHost *view_host_;
  /** The CairoCanvas which hold cairo_t inside */
  CairoCanvas *canvas_;
  /** Gtk InputMethod Context */
  GtkIMContext *im_context_;

  /** The cached Pango Layout */
  PangoLayout *cached_layout_;

  /** The text content of the edit control */
  std::string text_;
  /** The preedit text of the edit control */
  std::string preedit_;
  /** Attribute list of the preedit text */
  PangoAttrList *preedit_attrs_;
  /**
   *  The character that should be displayed in invisible mode.
   *  If this is empty, then the edit control is visible
   */
  std::string password_char_;

  /** Canvas width */
  int width_;
  /** Canvas height */
  int height_;

  /** The current cursor position in number of characters */
  int cursor_;
  /** The preedit cursor position within the preedit string */
  int preedit_cursor_;
  /**
   * The current selection bound in number of characters,
   * range between cursor_ and selection_bound_ are selected.
   */
  int selection_bound_;
  /** Length of current text in number of chars */
  int text_length_;

  /** X offset of current scroll, in pixels */
  int scroll_offset_x_;
  /** Y offset of current scroll, in pixels */
  int scroll_offset_y_;
  /** Timer id of refresh callback */
    int refresh_timer_;
  /** Timer id of cursor blink callback */
  int cursor_blink_timer_;
  /**
   * Indicates the status of cursor blinking,
   * 0 means hide cursor
   * otherwise means show cursor.
   * The maximum value would be 2, and decrased by one in each cursor blink
   * callback, then there would be 2/3 visible time and 1/3 invisible time.
   */
  int cursor_blink_status_;

  /** Whether the text is visible, decided by password_char_ */
  bool visible_;
  /** Whether the edit control is focused */
  bool focused_;
  /** Whether the input method should be reset */
  bool need_im_reset_;
  /** Whether the keyboard in overwrite mode */
  bool overwrite_;
  /** Whether the button click should select words */
  bool select_words_;
  /** Whether the button click should select lines */
  bool select_lines_;
  /** Whether the left button is pressed */
  bool button_;
  /** Whether the text should be bold */
  bool bold_;
  /** Whether the text should be underlined */
  bool underline_;
  /** Whether the text should be struck-out */
  bool strikeout_;
  /** Whether the text should be italic */
  bool italic_;
  /** Whether the text could be shown in multilines */
  bool multiline_;
  /** Whether the text should be wrapped */
  bool wrap_;
  /** whether the cursor should be displayed */
  bool cursor_visible_;
  /** whether the edit control is readonly */
  bool readonly_;
  /** Indicates if the edit control has been modified since last draw */
  bool modified_;

  /** The font family of the text */
  std::string font_family_;
  /** The font size of the text */
  int font_size_;
  /** The background texture of the edit control */
  Texture *background_;
  /** The text color of the edit control */
  Color text_color_;

  Signal0<void> queue_draw_signal_;
  Signal0<void> text_changed_signal_;

  DISALLOW_EVIL_CONSTRUCTORS(GtkEdit);
};  // class GtkEdit

} // namespace gtk
} // namespace ggadget

#endif   // GGADGET_GTK_GTKEDIT_H__
