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

#include "edit_element.h"
#include "canvas_interface.h"
#include "event.h"
#include "view_interface.h"

namespace ggadget {

class EditElement::Impl {
 public:
  Impl(EditElement *owner)
      : owner_(owner) {
  }
  ~Impl() {
  }

  const char *ScriptGetPasswordChar() {
    // TODO:
    return NULL;
  }

  void ScriptSetPasswordChar(const char *) {
    // TODO:
  }

  EditElement *owner_;
  EventSignal onchange_event_;
};

EditElement::EditElement(ElementInterface *parent,
                         ViewInterface *view,
                         const char *name)
    : DivElement(parent, view, "edit", name, false),
      impl_(new Impl(this)) {
  SetAutoscroll(true);
  SetEnabled(true);

  RegisterProperty("background",
                   NewSlot(implicit_cast<DivElement *>(this),
                           &DivElement::GetBackground),
                   NewSlot(implicit_cast<DivElement *>(this),
                           &DivElement::SetBackground));
  RegisterProperty("bold",
                   NewSlot(this, &EditElement::IsBold),
                   NewSlot(this, &EditElement::SetBold));
  RegisterProperty("color",
                   NewSlot(this, &EditElement::GetColor),
                   NewSlot(this, &EditElement::SetColor));
  RegisterProperty("font",
                   NewSlot(this, &EditElement::GetFont),
                   NewSlot(this, &EditElement::SetFont));
  RegisterProperty("italic",
                   NewSlot(this, &EditElement::IsItalic),
                   NewSlot(this, &EditElement::SetItalic));
  RegisterProperty("multiline",
                   NewSlot(this, &EditElement::IsMultiline),
                   NewSlot(this, &EditElement::SetMultiline));
  RegisterProperty("passwordChar",
                   NewSlot(impl_, &Impl::ScriptGetPasswordChar),
                   NewSlot(impl_, &Impl::ScriptSetPasswordChar));
  RegisterProperty("size",
                   NewSlot(this, &EditElement::GetSize),
                   NewSlot(this, &EditElement::SetSize));
  RegisterProperty("strikeout",
                   NewSlot(this, &EditElement::IsStrikeout),
                   NewSlot(this, &EditElement::SetStrikeout));
  RegisterProperty("underline",
                   NewSlot(this, &EditElement::IsUnderline),
                   NewSlot(this, &EditElement::SetUnderline));
  RegisterProperty("value",
                   NewSlot(this, &EditElement::GetValue),
                   NewSlot(this, &EditElement::SetValue));
  RegisterProperty("wordWrap",
                   NewSlot(this, &EditElement::IsWordWrap),
                   NewSlot(this, &EditElement::SetWordWrap));
  RegisterSignal("onchange", &impl_->onchange_event_);
}

EditElement::~EditElement() {
  delete impl_;
  impl_ = NULL;
}

void EditElement::DoDraw(CanvasInterface *canvas,
                         const CanvasInterface *children_canvas) {
  // TODO: Draw the edit into a canvas and pass the canvas as children_canvas
  // to DivElement::DoDraw().
  DivElement::DoDraw(canvas, children_canvas);
}

bool EditElement::IsBold() const {
  // TODO:
  return false;
}

void EditElement::SetBold(bool bold) {
  // TODO:
}

const char *EditElement::GetColor() const {
  // TODO:
  return "";
}

void EditElement::SetColor(const char *color) {
  // TODO:
}

const char *EditElement::GetFont() const {
  // TODO:
  return "";
}

void EditElement::SetFont(const char *font) {
  // TODO:
}

bool EditElement::IsItalic() const {
  // TODO:
  return false;
}

void EditElement::SetItalic(bool italic) {
  // TODO:
}

bool EditElement::IsMultiline() const {
  // TODO:
  return false;
}

void EditElement::SetMultiline(bool multiline) {
  // TODO:
}

char EditElement::GetPasswordChar() const {
  // TODO:
  return '\0';
}

void EditElement::SetPassordChar(char passwordChar) {
  // TODO:
}

int EditElement::GetSize() const {
  // TODO:
  return 10;
}

void EditElement::SetSize(int size) {
  // TODO:
}

bool EditElement::IsStrikeout() const {
  // TODO:
  return false;
}

void EditElement::SetStrikeout(bool strikeout) {
  // TODO:
}

bool EditElement::IsUnderline() const {
  // TODO:
  return false;
}

void EditElement::SetUnderline(bool underline) {
  // TODO:
}

const char *EditElement::GetValue() const {
  // TODO:
  return "Edit";
}

void EditElement::SetValue(const char *value) {
  // TODO:
}

bool EditElement::IsWordWrap() const {
  // TODO:
  return false;
}

void EditElement::SetWordWrap(bool wrap) {
  // TODO:
}

bool EditElement::OnMouseEvent(MouseEvent *event, bool direct,
                              ElementInterface **fired_element) {
  return DivElement::OnMouseEvent(event, direct, fired_element);
}

bool EditElement::OnKeyEvent(KeyboardEvent *event) {
  return DivElement::OnKeyEvent(event);
}

ElementInterface *EditElement::CreateInstance(ElementInterface *parent,
                                             ViewInterface *view,
                                             const char *name) {
  return new EditElement(parent, view, name);
}

} // namespace ggadget
