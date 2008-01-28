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

#include "edit_element_base.h"
#include "event.h"
#include "logger.h"
#include "scriptable_event.h"
#include "scrolling_element.h"
#include "slot.h"
#include "string_utils.h"
#include "view.h"

namespace ggadget {

class EditElementBase::Impl {
 public:
  Impl(EditElementBase *owner) : owner_(owner) { }

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

  EditElementBase *owner_;
  EventSignal onchange_event_;
};

EditElementBase::EditElementBase(BasicElement *parent, View *view, const char *name)
    : ScrollingElement(parent, view, "edit", name, false),
      impl_(new Impl(this)) {
  SetEnabled(true);
  SetAutoscroll(true);
}

void EditElementBase::DoRegister() {
  ScrollingElement::DoRegister();
  RegisterProperty("background",
                   NewSlot(this, &EditElementBase::GetBackground),
                   NewSlot(this, &EditElementBase::SetBackground));
  RegisterProperty("bold",
                   NewSlot(this, &EditElementBase::IsBold),
                   NewSlot(this, &EditElementBase::SetBold));
  RegisterProperty("color",
                   NewSlot(this, &EditElementBase::GetColor),
                   NewSlot(this, &EditElementBase::SetColor));
  RegisterProperty("font",
                   NewSlot(this, &EditElementBase::GetFont),
                   NewSlot(this, &EditElementBase::SetFont));
  RegisterProperty("italic",
                   NewSlot(this, &EditElementBase::IsItalic),
                   NewSlot(this, &EditElementBase::SetItalic));
  RegisterProperty("multiline",
                   NewSlot(this, &EditElementBase::IsMultiline),
                   NewSlot(this, &EditElementBase::SetMultiline));
  RegisterProperty("passwordChar",
                   NewSlot(this, &EditElementBase::GetPasswordChar),
                   NewSlot(this, &EditElementBase::SetPasswordChar));
  RegisterProperty("size",
                   NewSlot(this, &EditElementBase::GetSize),
                   NewSlot(this, &EditElementBase::SetSize));
  RegisterProperty("strikeout",
                   NewSlot(this, &EditElementBase::IsStrikeout),
                   NewSlot(this, &EditElementBase::SetStrikeout));
  RegisterProperty("underline",
                   NewSlot(this, &EditElementBase::IsUnderline),
                   NewSlot(this, &EditElementBase::SetUnderline));
  RegisterProperty("value",
                   NewSlot(this, &EditElementBase::GetValue),
                   NewSlot(this, &EditElementBase::SetValue));
  RegisterProperty("wordWrap",
                   NewSlot(this, &EditElementBase::IsWordWrap),
                   NewSlot(this, &EditElementBase::SetWordWrap));
  RegisterProperty("readonly",
                   NewSlot(this, &EditElementBase::IsReadOnly),
                   NewSlot(this, &EditElementBase::SetReadOnly));
  RegisterProperty("idealBoundingRect",
                   NewSlot(impl_, &Impl::GetIdealBoundingRect),
                   NULL);

  RegisterSignal(kOnChangeEvent, &impl_->onchange_event_);
}

EditElementBase::~EditElementBase() {
  delete impl_;
}

Connection *EditElementBase::ConnectOnChangeEvent(Slot0<void> *slot) {
  return impl_->onchange_event_.Connect(slot);
}

void EditElementBase::FireOnChangeEvent() const {
  impl_->FireOnChangeEvent();
}

} // namespace ggadget
