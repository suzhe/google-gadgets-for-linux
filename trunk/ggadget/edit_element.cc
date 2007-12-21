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
#include "edit_element.h"
#include "canvas_interface.h"
#include "edit_interface.h"
#include "event.h"
#include "graphics_interface.h"
#include "logger.h"
#include "scriptable_event.h"
#include "scrolling_element.h"
#include "slot.h"
#include "string_utils.h"
#include "texture.h"
#include "view.h"

namespace ggadget {

static const int kDefaultEditElementWidth = 60;
static const int kDefaultEditElementHeight = 16;
static const Color kDefaultBackgroundColor(1, 1, 1);
static const char *const kOnChangeEvent = "onchange";

class EditElement::Impl {
 public:
  Impl(EditElement *owner)
      : owner_(owner),
        accum_wheel_delta_(0) {
    edit_ = owner_->GetView()->NewEdit(kDefaultEditElementWidth,
                                       kDefaultEditElementHeight);
    edit_->SetBackground(new Texture(kDefaultBackgroundColor, 1.0));
  }
  ~Impl() {
    edit_->Destroy();
  }

  void FireOnChangeEvent() {
    SimpleEvent event(Event::EVENT_CHANGE);
    ScriptableEvent s_event(&event, owner_, NULL);
    owner_->GetView()->FireEvent(&s_event, onchange_event_);
  }

  JSONString GetIdealBoundingRect() {
    int w, h;
    owner_->GetIdealBoundingRect(&w, &h);
    return JSONString(StringPrintf("{\"width\":%d,\"height\":%d}", w, h));
  }

  void OnScrolled() {
    DLOG("EditElement::OnScrolled(%d)", owner_->GetScrollYPosition());
    edit_->ScrollTo(owner_->GetScrollYPosition());
  }

  EditElement *owner_;
  EditInterface *edit_;
  EventSignal onchange_event_;
  int accum_wheel_delta_;
};

EditElement::EditElement(BasicElement *parent, View *view, const char *name)
    : ScrollingElement(parent, view, "edit", name, false),
      impl_(new Impl(this)) {
  SetEnabled(true);
  SetAutoscroll(true);

  impl_->edit_->ConnectOnQueueDraw(NewSlot(static_cast<BasicElement*>(this),
                                           &BasicElement::QueueDraw));
  impl_->edit_->ConnectOnTextChanged(NewSlot(impl_, &Impl::FireOnChangeEvent));
  ConnectOnScrolledEvent(NewSlot(impl_, &Impl::OnScrolled));

  RegisterProperty("background",
                   NewSlot(this, &EditElement::GetBackground),
                   NewSlot(this, &EditElement::SetBackground));
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
                   NewSlot(this, &EditElement::GetPasswordChar),
                   NewSlot(this, &EditElement::SetPasswordChar));
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
  RegisterProperty("readonly",
                   NewSlot(this, &EditElement::IsReadOnly),
                   NewSlot(this, &EditElement::SetReadOnly));
  RegisterProperty("idealBoundingRect",
                   NewSlot(impl_, &Impl::GetIdealBoundingRect),
                   NULL);

  RegisterSignal(kOnChangeEvent, &impl_->onchange_event_);
}

EditElement::~EditElement() {
  delete impl_;
}

void EditElement::Layout() {
  ScrollingElement::Layout();
  int range, line_step, page_step, cur_pos;

  impl_->edit_->SetWidth(static_cast<int>(ceil(GetClientWidth())));
  impl_->edit_->SetHeight(static_cast<int>(ceil(GetClientHeight())));
  impl_->edit_->GetScrollBarInfo(&range, &line_step, &page_step, &cur_pos);

  // If the scrollbar display state was changed, then call Layout() recursively
  // to redo Layout.
  if (UpdateScrollBar(0, range)) {
    Layout();
  } else {
    SetScrollYPosition(cur_pos);
    SetYLineStep(line_step);
    SetYPageStep(page_step);
  }
}

Variant EditElement::GetBackground() const {
  return Variant(Texture::GetSrc(impl_->edit_->GetBackground()));
}

void EditElement::SetBackground(const Variant &background) {
  impl_->edit_->SetBackground(GetView()->LoadTexture(background));
}

bool EditElement::IsBold() const {
  return impl_->edit_->IsBold();
}

void EditElement::SetBold(bool bold) {
  impl_->edit_->SetBold(bold);
}

std::string EditElement::GetColor() const {
  Texture texture(impl_->edit_->GetTextColor(), 1.0);
  return texture.GetSrc();
}

void EditElement::SetColor(const char *color) {
  Texture texture(GetView()->GetGraphics(), GetView()->GetFileManager(), color);
  impl_->edit_->SetTextColor(texture.GetColor());
}

std::string EditElement::GetFont() const {
  return impl_->edit_->GetFontFamily();
}

void EditElement::SetFont(const char *font) {
  impl_->edit_->SetFontFamily(font);
}

bool EditElement::IsItalic() const {
  return impl_->edit_->IsItalic();
}

void EditElement::SetItalic(bool italic) {
  impl_->edit_->SetItalic(italic);
}

bool EditElement::IsMultiline() const {
  return impl_->edit_->IsMultiline();
}

void EditElement::SetMultiline(bool multiline) {
  impl_->edit_->SetMultiline(multiline);
}

std::string EditElement::GetPasswordChar() const {
  return impl_->edit_->GetPasswordChar();
}

void EditElement::SetPasswordChar(const char *passwordChar) {
  impl_->edit_->SetPasswordChar(passwordChar);
}

int EditElement::GetSize() const {
  return impl_->edit_->GetFontSize();
}

void EditElement::SetSize(int size) {
  impl_->edit_->SetFontSize(size);
}

bool EditElement::IsStrikeout() const {
  return impl_->edit_->IsStrikeout();
}

void EditElement::SetStrikeout(bool strikeout) {
  impl_->edit_->SetStrikeout(strikeout);
}

bool EditElement::IsUnderline() const {
  return impl_->edit_->IsUnderline();
}

void EditElement::SetUnderline(bool underline) {
  impl_->edit_->SetUnderline(underline);
}

std::string EditElement::GetValue() const {
  return impl_->edit_->GetText();
}

void EditElement::SetValue(const char *value) {
  impl_->edit_->SetText(value);
}

bool EditElement::IsWordWrap() const {
  return impl_->edit_->IsWordWrap();
}

void EditElement::SetWordWrap(bool wrap) {
  impl_->edit_->SetWordWrap(wrap);
}

bool EditElement::IsReadOnly() const {
  return impl_->edit_->IsReadOnly();
}

void EditElement::SetReadOnly(bool readonly) {
  impl_->edit_->SetReadOnly(readonly);
}

void EditElement::GetIdealBoundingRect(int *width, int *height) {
  int w, h;
  impl_->edit_->GetSizeRequest(&w, &h);
  if (width) *width = w;
  if (height) *height = h;
}

Connection *EditElement::ConnectOnChangeEvent(Slot0<void> *slot) {
  return impl_->onchange_event_.Connect(slot);
}

void EditElement::DoDraw(CanvasInterface *canvas,
                         const CanvasInterface *children_canvas) {
  bool modified;
  CanvasInterface *content = impl_->edit_->Draw(&modified);
  canvas->DrawCanvas(0, 0, content);
  DrawScrollbar(canvas);
}

EventResult EditElement::HandleMouseEvent(const MouseEvent &event) {
  if (ScrollingElement::HandleMouseEvent(event) == EVENT_RESULT_HANDLED)
    return EVENT_RESULT_HANDLED;
  return impl_->edit_->OnMouseEvent(event);
}

EventResult EditElement::HandleKeyEvent(const KeyboardEvent &event) {
  return impl_->edit_->OnKeyEvent(event);
}

EventResult EditElement::HandleOtherEvent(const Event &event) {
  if (event.GetType() == Event::EVENT_FOCUS_IN) {
    impl_->edit_->FocusIn();
    return EVENT_RESULT_HANDLED;
  } else if(event.GetType() == Event::EVENT_FOCUS_OUT) {
    impl_->edit_->FocusOut();
    return EVENT_RESULT_HANDLED;
  }
  return EVENT_RESULT_UNHANDLED;
}

void EditElement::GetDefaultSize(double *width, double *height) const {
  ASSERT(width && height);

  *width = kDefaultEditElementWidth;
  *height = kDefaultEditElementHeight;
}

BasicElement *EditElement::CreateInstance(BasicElement *parent,
                                          View *view,
                                          const char *name) {
  return new EditElement(parent, view, name);
}

} // namespace ggadget
