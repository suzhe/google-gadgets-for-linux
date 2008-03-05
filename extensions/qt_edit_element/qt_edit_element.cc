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

#include <cmath>
#include <QPainter>
#include <QKeyEvent>
#include <QAbstractTextDocumentLayout>
#include <QTextLine>
#include <QTextBlock>
#include <ggadget/canvas_interface.h>
#include <ggadget/event.h>
#include <ggadget/logger.h>
#include <ggadget/scrolling_element.h>
#include <ggadget/slot.h>
#include <ggadget/texture.h>
#include <ggadget/view.h>
#include <ggadget/main_loop_interface.h>
#include <ggadget/element_factory.h>
#include <ggadget/qt/qt_canvas.h>
#include "qt_edit_element.h"

#define Initialize qt_edit_element_LTX_Initialize
#define Finalize qt_edit_element_LTX_Finalize
#define RegisterElementExtension qt_edit_element_LTX_RegisterElementExtension

extern "C" {
  bool Initialize() {
    LOG("Initialize qt_edit_element extension.");
    return true;
  }

  void Finalize() {
    LOG("Finalize qt_edit_element extension.");
  }

  bool RegisterElementExtension(ggadget::ElementFactory *factory) {
    LOG("Register qt_edit_element extension.");
    if (factory) {
      factory->RegisterElementClass(
          "edit", &ggadget::qt::QtEditElement::CreateInstance);
    }
    return true;
  }
}

namespace ggadget {
namespace qt {

static const int kDefaultEditElementWidth = 60;
static const int kDefaultEditElementHeight = 16;
static const Color kDefaultBackgroundColor(1, 1, 1);
static const int kDefaultFontSize = 10;

QtEditElement::QtEditElement(BasicElement *parent, View *view, const char *name)
    : EditElementBase(parent, view, name) {
  ConnectOnScrolledEvent(NewSlot(this, &QtEditElement::OnScrolled));
  cursor_ = new QTextCursor(&doc_);
  multiline_ = overwrite_ = readonly_ = false;
}

QtEditElement::~QtEditElement() {
}

void QtEditElement::Layout() {
  //int range, line_step, page_step, cur_pos;
  ScrollingElement::Layout();
  /*SetWidth(static_cast<int>(ceil(GetClientWidth())));
  SetHeight(static_cast<int>(ceil(GetClientHeight())));
  GetScrollBarInfo(&range, &line_step, &page_step, &cur_pos);*/
}

void QtEditElement::MarkRedraw() {
  ScrollingElement::MarkRedraw();
}

Variant QtEditElement::GetBackground() const {
  return Variant(Texture::GetSrc(background_));
}

void QtEditElement::SetBackground(const Variant &background) {
  background_ = GetView()->LoadTexture(background);
}

bool QtEditElement::IsBold() const {
  return false;
}

void QtEditElement::SetBold(bool bold) {
}

std::string QtEditElement::GetColor() const {
  return text_color_.ToString();
}

void QtEditElement::SetColor(const char *color) {
}

std::string QtEditElement::GetFont() const {
  return doc_.defaultFont().defaultFamily().toStdString();
}

void QtEditElement::SetFont(const char *font) {
}

bool QtEditElement::IsItalic() const {
  return false;
}

void QtEditElement::SetItalic(bool italic) {
}

bool QtEditElement::IsMultiline() const {
  return false;
}

void QtEditElement::SetMultiline(bool multiline) {
}

std::string QtEditElement::GetPasswordChar() const {
  return "hello, world";
}

void QtEditElement::SetPasswordChar(const char *passwordChar) {
}

int QtEditElement::GetSize() const {
  return kDefaultFontSize;
}

void QtEditElement::SetSize(int size) {
}

bool QtEditElement::IsStrikeout() const {
  return false;
}

void QtEditElement::SetStrikeout(bool strikeout) {
}

bool QtEditElement::IsUnderline() const {
  return false;
}

void QtEditElement::SetUnderline(bool underline) {
}

std::string QtEditElement::GetValue() const {
  return doc_.toPlainText().toStdString();
}

void QtEditElement::SetValue(const char *value) {
  doc_.setPlainText(value);
}

bool QtEditElement::IsWordWrap() const {
  return false;
}

void QtEditElement::SetWordWrap(bool wrap) {
}

bool QtEditElement::IsReadOnly() const {
  return false;
}

void QtEditElement::SetReadOnly(bool readonly) {
}

void QtEditElement::GetIdealBoundingRect(int *width, int *height) {
  const QSize s = doc_.pageSize().toSize();

  if (width) *width = s.width();
  if (height) *height = s.height();
}

static QRectF GetRectForPosition(QTextDocument *doc, int position) {
  const QTextBlock block = doc->findBlock(position);
  if (!block.isValid()) return QRectF();
  const QAbstractTextDocumentLayout *docLayout = doc->documentLayout();
  const QTextLayout *layout = block.layout();
  const QPointF layoutPos = docLayout->blockBoundingRect(block).topLeft();
  int relativePos = position - block.position();
/*  if (preeditCursor != 0) {
    int preeditPos = layout->preeditAreaPosition();
    if (relativePos == preeditPos)
      relativePos += preeditCursor;
    else if (relativePos > preeditPos)
      relativePos += layout->preeditAreaText().length();
  }*/
  QTextLine line = layout->lineForTextPosition(relativePos);

  int cursorWidth;
  {
    bool ok = false;
    cursorWidth = docLayout->property("cursorWidth").toInt(&ok);
    if (!ok)
      cursorWidth = 1;
  }

  QRectF r;

  if (line.isValid())
    r = QRectF(layoutPos.x() + line.cursorToX(relativePos) - 5 - cursorWidth, layoutPos.y() + line.y(),
        2 * cursorWidth + 10, line.ascent() + line.descent()+1.);
  else
    r = QRectF(layoutPos.x() - 5 - cursorWidth, layoutPos.y(), 2 * cursorWidth + 10, 10); // #### correct height

  return r;
}

static const int kInnerBorderX = 2;
static const int kInnerBorderY = 1;
void QtEditElement::DoDraw(CanvasInterface *canvas) {
/*  canvas->IntersectRectClipRegion(kInnerBorderX - 1,
                                  kInnerBorderY - 1,
                                  width_*/
  QtCanvas *c = down_cast<QtCanvas*>(canvas);
  c->DrawTextDocument(doc_);
  QRectF r = GetRectForPosition(&doc_, cursor_->position());
  double x = (r.left() + r.right())/2;
  c->DrawLine(x, r.top(), x, r.bottom(), 1, Color::kBlack);
  DrawScrollbar(canvas);
}

EventResult QtEditElement::HandleMouseEvent(const MouseEvent &event) {
  if (ScrollingElement::HandleMouseEvent(event) == EVENT_RESULT_HANDLED)
    return EVENT_RESULT_HANDLED;
  return EVENT_RESULT_HANDLED;
}

EventResult QtEditElement::HandleKeyEvent(const KeyboardEvent &event) {
  QKeyEvent *qevent = static_cast<QKeyEvent*>(event.GetOriginalEvent());
  Event::Type type = event.GetType();
  int mod = event.GetModifier();
  bool shift = (mod & Event::MOD_SHIFT);
  bool ctrl = (mod & Event::MOD_CONTROL);
  int keyval = qevent->key();

  if (type == Event::EVENT_KEY_DOWN) {
    if (keyval == Qt::Key_Left) {
      if (!ctrl) MoveCursor(QTextCursor::Left, 1, shift);
      else MoveCursor(QTextCursor::WordLeft, 1, shift);
    } else if (keyval == Qt::Key_Right) {
      if (!ctrl) MoveCursor(QTextCursor::Right, 1, shift);
      else MoveCursor(QTextCursor::WordRight, 1, shift);
    } else if (keyval == Qt::Key_Up) {
      MoveCursor(QTextCursor::Up, 1, shift);
    } else if (keyval == Qt::Key_Down) {
      MoveCursor(QTextCursor::Down, 1, shift);
    } else if (keyval == Qt::Key_Home) {
      if (!ctrl) MoveCursor(QTextCursor::StartOfLine, 1, shift);
      else MoveCursor(QTextCursor::Start, 1, shift);
    } else if (keyval == Qt::Key_End) {
      if (!ctrl) MoveCursor(QTextCursor::EndOfLine, 1, shift);
      else MoveCursor(QTextCursor::End, 1, shift);
    } else if (keyval == Qt::Key_PageUp) {
      if (!ctrl) MoveCursor(QTextCursor::Up, page_line_, shift);
    } else if (keyval == Qt::Key_PageDown) {
      if (!ctrl) MoveCursor(QTextCursor::Down, page_line_, shift);
    } else if ((keyval == 'x' && ctrl && !shift) ||
               (keyval == Qt::Key_Delete && shift && !ctrl)) {
      CutClipboard();
    } else if ((keyval == 'c' && ctrl && !shift) ||
               (keyval == Qt::Key_Insert && ctrl && !shift)) {
      CopyClipboard();
    } else if ((keyval == 'v' && ctrl && !shift) ||
               (keyval == Qt::Key_Insert && shift && !ctrl)) {
      PasteClipboard();
    } else if (keyval == Qt::Key_Backspace) {
      cursor_->deletePreviousChar();
    } else if (keyval == Qt::Key_Delete && !shift) {
      cursor_->deleteChar();
    } else if (keyval == Qt::Key_Insert && !shift && !ctrl) {
      overwrite_ = !overwrite_;
    } else if (keyval == Qt::Key_Return) {
      // If multiline_ is unset, just ignore new_line.
      if (multiline_) EnterText("\n");
    } else if (keyval == Qt::Key_Tab) {
      // The Tab key will likely be consumed by input method.
      EnterText("\t");
    } else if (!qevent->text().isEmpty()){
      EnterText(qevent->text());
    }
  }
  QueueDraw();
  return EVENT_RESULT_HANDLED;
} 

void QtEditElement::EnterText(QString str) {
  if (readonly_) return;

  if (cursor_->hasSelection() || overwrite_) {
    cursor_->deleteChar();
  }
  
  cursor_->insertText(str);

  FireOnChangeEvent();
}

EventResult QtEditElement::HandleOtherEvent(const Event &event) {
  if (event.GetType() == Event::EVENT_FOCUS_IN) {
    return EVENT_RESULT_HANDLED;
  } else if(event.GetType() == Event::EVENT_FOCUS_OUT) {
    return EVENT_RESULT_HANDLED;
  }
  return EVENT_RESULT_UNHANDLED;
}

void QtEditElement::GetDefaultSize(double *width, double *height) const {
  ASSERT(width && height);

  *width = kDefaultEditElementWidth;
  *height = kDefaultEditElementHeight;
}

void QtEditElement::OnScrolled() {
  DLOG("QtEditElement::OnScrolled(%d)", GetScrollYPosition());
}

BasicElement *QtEditElement::CreateInstance(BasicElement *parent,
                                             View *view,
                                             const char *name) {
  return new QtEditElement(parent, view, name);
}

void QtEditElement::MoveCursor(QTextCursor::MoveOperation op, int count,
                               bool extend_selection) {
  QTextCursor::MoveMode mode =
      extend_selection ? QTextCursor::MoveAnchor:QTextCursor::KeepAnchor;
  cursor_->movePosition(op, mode, count);
}

} // namespace qt
} // namespace ggadget
