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

#include "display_window.h"
#include "button_element.h"
#include "checkbox_element.h"
#include "combobox_element.h"
#include "edit_element.h"
#include "element_interface.h"
#include "elements_interface.h"
#include "item_element.h"
#include "label_element.h"
#include "listbox_element.h"
#include "scriptable_array.h"
#include "text_frame.h"
#include "view.h"

namespace ggadget {

class DisplayWindow::Impl {
 public:
  enum ButtonId {
    ID_OK = 1,
    ID_CANCEL = 2
  };

  enum ControlClass {
    CLASS_LABEL = 0,
    CLASS_EDIT = 1,
    CLASS_LIST = 2,
    CLASS_BUTTON = 3
  };

  enum ControlType {
    TYPE_NONE = 0,
    TYPE_LIST_OPEN = 0,
    TYPE_LIST_DROP = 1,
    TYPE_BUTTON_PUSH = 2,
    TYPE_BUTTON_CHECK = 3,
    TYPE_EDIT_PASSWORD = 10,
  };

  class Control : public ScriptableHelper<ScriptableInterface> {
   public:
    DEFINE_CLASS_ID(0x811cc6d8013643f4, ScriptableInterface);

    Control(DisplayWindow *window, BasicElement *element)
        : window_(window), element_(element) {
      // Incompatibility: we don't allow chaning id of a control.
      RegisterProperty("id", NewSlot(element_, &BasicElement::GetName), NULL);
      RegisterProperty("enabled",
                       NewSlot(element_, &BasicElement::IsEnabled),
                       NewSlot(element_, &BasicElement::SetEnabled));
      RegisterProperty("text",
                       NewSlot(this, &Control::GetText),
                       NewSlot(this, &Control::SetText));
      RegisterProperty("value",
                       NewSlot(this, &Control::GetValue),
                       NewSlot(this, &Control::SetValue));
      RegisterProperty("x", NULL, // No getter.
                       NewSlot(element_, &BasicElement::SetPixelX));
      RegisterProperty("y", NULL, // No getter.
                       NewSlot(element_, &BasicElement::SetPixelY));
      RegisterProperty("width", NULL, // No getter.
                       NewSlot(element_, &BasicElement::SetPixelWidth));
      RegisterProperty("height", NULL, // No getter.
                       NewSlot(element_, &BasicElement::SetPixelHeight));
      RegisterSignal("onChanged", &onchanged_signal_);
      RegisterSignal("onClicked", &onclicked_signal_);
    }

    virtual OwnershipPolicy Attach() { return OWNERSHIP_TRANSFERRABLE; }
    virtual bool Detach() { delete this; return true; }

    ScriptableArray *GetListBoxItems(ListBoxElement *listbox) {
      int count = listbox->GetChildren()->GetCount();
      Variant *array = new Variant[count];
      size_t actual_count = 0;
      for (int i = 0; i < count; i++) {
        ElementInterface *item = listbox->GetChildren()->GetItemByIndex(i);
        if (item->IsInstanceOf(ItemElement::CLASS_ID)) {
          array[actual_count] =
              Variant(down_cast<ItemElement *>(item)->GetLabelText());
          actual_count++;
        }
      }
      return ScriptableArray::Create(array, actual_count, false);
    }

    // Gets the full content of the control.
    Variant GetText() {
      if (element_->IsInstanceOf(ButtonElement::CLASS_ID)) {
        ButtonElement *button = down_cast<ButtonElement *>(element_);
        return Variant(button->GetTextFrame()->GetText());
      }
      if (element_->IsInstanceOf(CheckBoxElement::CLASS_ID)) {
        CheckBoxElement *checkbox = down_cast<CheckBoxElement *>(element_);
        return Variant(checkbox->GetTextFrame()->GetText());
      }
      if (element_->IsInstanceOf(LabelElement::CLASS_ID)) {
        LabelElement *label = down_cast<LabelElement *>(element_);
        return Variant(label->GetTextFrame()->GetText());
      }
      if (element_->IsInstanceOf(ListBoxElement::CLASS_ID)) {
        ListBoxElement *listbox = down_cast<ListBoxElement *>(element_);
        return Variant(GetListBoxItems(listbox));
      }
      if (element_->IsInstanceOf(ComboBoxElement::CLASS_ID)) {
        ComboBoxElement *combobox = down_cast<ComboBoxElement *>(element_);
        return Variant(GetListBoxItems(combobox->GetListBox()));
      }
      if (element_->IsInstanceOf(EditElement::CLASS_ID)) {
        EditElement *edit = down_cast<EditElement *>(element_);
        return Variant(edit->GetValue());
      }
      ASSERT(false);
      return Variant();
    }

    void SetListBoxItems(ListBoxElement *listbox, ScriptableArray *array) {
      listbox->GetChildren()->RemoveAllElements();
      for (size_t i = 0; i < array->GetCount(); i++) {
        Variant v = array->GetItem(i);
        if (v.type() == Variant::TYPE_STRING) {
          listbox->AppendString(VariantValue<const char *>()(v));
        } else {
          LOG("Invalid type of array item(%s) for control %s",
              v.ToString().c_str(), element_->GetName().c_str());
        }
      }
    }

    void SetText(const Variant &text) {
      switch (text.type()) {
        case Variant::TYPE_STRING: {
          const char *text_str = VariantValue<const char *>()(text);
          if (element_->IsInstanceOf(ButtonElement::CLASS_ID)) {
            ButtonElement *button = down_cast<ButtonElement *>(element_);
            button->GetTextFrame()->SetText(text_str);
          } else if (element_->IsInstanceOf(CheckBoxElement::CLASS_ID)) {
            CheckBoxElement *checkbox = down_cast<CheckBoxElement *>(element_);
            checkbox->GetTextFrame()->SetText(text_str);
          } else if (element_->IsInstanceOf(LabelElement::CLASS_ID)) {
            LabelElement *label = down_cast<LabelElement *>(element_);
            label->GetTextFrame()->SetText(text_str);
          } else if (element_->IsInstanceOf(EditElement::CLASS_ID)) {
            EditElement *edit = down_cast<EditElement *>(element_);
            edit->SetValue(text_str);
          } else {
            LOG("Invalid type of text(%s) for control %s",
                text.ToString().c_str(), element_->GetName().c_str());
          }
          break;
        }
        case Variant::TYPE_SCRIPTABLE: {
          ScriptableInterface *scriptable =
               VariantValue<ScriptableInterface *>()(text);
          if (scriptable &&
              scriptable->IsInstanceOf(ScriptableArray::CLASS_ID)) {
            ScriptableArray *array = down_cast<ScriptableArray *>(scriptable);
            if (element_->IsInstanceOf(ListBoxElement::CLASS_ID)) {
              SetListBoxItems(down_cast<ListBoxElement *>(element_), array);
            } else if (element_->IsInstanceOf(ComboBoxElement::CLASS_ID)) {
              SetListBoxItems(
                  down_cast<ComboBoxElement *>(element_)->GetListBox(), array);
            } else {
              LOG("Invalid type of text(%s) for control %s",
                  text.ToString().c_str(), element_->GetName().c_str());
            }
          }
        }
        default:
          LOG("Invalid type of text(%s) for control %s",
              text.ToString().c_str(), element_->GetName().c_str());
          break;
      }
    }

    std::string GetListBoxValue(ListBoxElement *listbox) {
      ItemElement *item = listbox->GetSelectedItem();
      return item ? item->GetLabelText() : std::string();
    }

    // The current value of the control.
    Variant GetValue() {
      if (element_->IsInstanceOf(CheckBoxElement::CLASS_ID)) {
        // For check box it is a boolean idicating the check state.
        CheckBoxElement *checkbox = down_cast<CheckBoxElement *>(element_);
        return Variant(checkbox->GetValue());
      }
      if (element_->IsInstanceOf(ListBoxElement::CLASS_ID)) {
        ListBoxElement *listbox = down_cast<ListBoxElement *>(element_);
        return Variant(GetListBoxValue(listbox));
      }
      if (element_->IsInstanceOf(ComboBoxElement::CLASS_ID)) {
        ComboBoxElement *combobox = down_cast<ComboBoxElement *>(element_);
        return Variant(GetListBoxValue(combobox->GetListBox()));
      }
      // For others it is the displayed text.
      return GetText();
    }

    void SetListBoxValue(ListBoxElement *listbox, const char *value) {
      ItemElement *item = listbox->FindItemByString(value);
      if (item)
        listbox->SetSelectedItem(item);
    }

    void SetValue(const Variant &value) {
      switch (value.type()) {
        case Variant::TYPE_BOOL:
          if (element_->IsInstanceOf(CheckBoxElement::CLASS_ID)) {
            // For check box it is a boolean idicating the check state.
            CheckBoxElement *checkbox = down_cast<CheckBoxElement *>(element_);
            checkbox->SetValue(VariantValue<bool>()(value));
          } else {
            LOG("Invalid type of value(%s) for control %s",
                value.ToString().c_str(), element_->GetName().c_str());
          }
          break;
        case Variant::TYPE_STRING:
          if (element_->IsInstanceOf(ButtonElement::CLASS_ID) ||
              element_->IsInstanceOf(LabelElement::CLASS_ID) ||
              element_->IsInstanceOf(EditElement::CLASS_ID)) {
            SetText(value);
          } else if (element_->IsInstanceOf(ListBoxElement::CLASS_ID)) {
            ListBoxElement *listbox = down_cast<ListBoxElement *>(element_);
            SetListBoxValue(listbox, VariantValue<const char *>()(value));
          } else if (element_->IsInstanceOf(ComboBoxElement::CLASS_ID)) {
            ComboBoxElement *combobox = down_cast<ComboBoxElement *>(element_);
            SetListBoxValue(combobox->GetListBox(),
                            VariantValue<const char *>()(value));
          } else {
            LOG("Invalid type of value(%s) for control %s",
                value.ToString().c_str(), element_->GetName().c_str());
          }
          break;
        default:
          LOG("Invalid type of value(%s) for control %s",
              value.ToString().c_str(), element_->GetName().c_str());
          break;
      }
    }

    void OnChange() {
      onchanged_signal_(window_, this);
    }

    void OnClicked() {
      onclicked_signal_(window_, this);
    }

    DisplayWindow *window_;
    BasicElement *element_;
    Signal2<void, DisplayWindow *, Control *> onchanged_signal_;
    Signal2<void, DisplayWindow *, Control *> onclicked_signal_;
  };

  Impl(DisplayWindow *owner, View *view)
      : owner_(owner), view_(view) {
  }

  Control *AddControl(ControlClass ctrl_class, ControlType ctrl_type,
                      const char *ctrl_id, const Variant &text,
                      int x, int y, int width, int height) {
    BasicElement *element = NULL;
    Control *control = NULL;
    switch (ctrl_class) {
      case CLASS_LABEL:
        element = LabelElement::CreateInstance(NULL, view_, ctrl_id);
        control = new Control(owner_, element);
        break;
      case CLASS_EDIT:
        element = EditElement::CreateInstance(NULL, view_, ctrl_id);
        control = new Control(owner_, element);
        if (ctrl_type == TYPE_EDIT_PASSWORD)
          down_cast<EditElement *>(element)->SetPasswordChar("*");
        down_cast<EditElement *>(element)->ConnectOnChangeEvent(
            NewSlot(control, &Control::OnChange));
        break;
      case CLASS_LIST:
        switch (ctrl_type) {
          case TYPE_LIST_OPEN:
            element = ListBoxElement::CreateInstance(NULL, view_, ctrl_id);
            control = new Control(owner_, element);
            down_cast<ListBoxElement *>(element)->ConnectOnChangeEvent(
                NewSlot(control, &Control::OnChange));
            break;
          case TYPE_LIST_DROP:
            element = ComboBoxElement::CreateInstance(NULL, view_, ctrl_id);
            control = new Control(owner_, element);
            down_cast<ComboBoxElement *>(element)->ConnectOnChangeEvent(
                NewSlot(control, &Control::OnChange));
            break;
          default:
            LOG("Unknown button control type: %d", ctrl_type);
            break;
        }
        break;
      case CLASS_BUTTON:
        switch (ctrl_type) {
          case TYPE_BUTTON_PUSH:
            element = ButtonElement::CreateInstance(NULL, view_, ctrl_id);
            control = new Control(owner_, element);
            element->ConnectOnClickEvent(NewSlot(control, &Control::OnClicked));
            break;
          case TYPE_BUTTON_CHECK:
            element = CheckBoxElement::CreateCheckBoxInstance(NULL, view_,
                                                              ctrl_id);
            control = new Control(owner_, element);
            // Note: the event name is "onchange", not "onclick", but the
            // handler is "onclick", because of the difference between
            // checkbox API and the old options API.
            down_cast<CheckBoxElement *>(element)->ConnectOnChangeEvent(
                NewSlot(control, &Control::OnClicked));
            break;
          default:
            LOG("Unknown button control type: %d", ctrl_type);
            break;
        }
        break;
      default:
        LOG("Unknown control class: %d", ctrl_class);
        break;
    }

    if (element) {
      ASSERT(control);
      element->SetPixelX(x);
      element->SetPixelY(y);
      element->SetPixelWidth(width);
      element->SetPixelHeight(height);
      control->SetText(text);
      return control;
    }
    return NULL;
  }

  Control *GetControl(const char *ctrl_id) {
    ElementInterface *element = view_->GetElementByName(ctrl_id);
    return element ? new Control(owner_, down_cast<BasicElement *>(element)) :
           NULL;
  }

  DisplayWindow *owner_;
  View *view_;
  Signal1<void, ButtonId> onclose_signal_;
};

DisplayWindow::DisplayWindow(ViewInterface *view)
    : impl_(new Impl(this, down_cast<View *>(view))) {
  ASSERT(view);
  RegisterMethod("AddControl", NewSlot(impl_, &Impl::AddControl));
  RegisterMethod("GetControl", NewSlot(impl_, &Impl::GetControl));
  RegisterSignal("OnClose", &impl_->onclose_signal_);
}

DisplayWindow::~DisplayWindow() {
  delete impl_;
  impl_ = NULL;
}

} // namespace ggadget
