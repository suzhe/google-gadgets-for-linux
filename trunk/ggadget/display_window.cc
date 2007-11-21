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
// TODO: #include "edit_element.h"
#include "element_interface.h"
#include "label_element.h"
// TODO: #include "listbox_element.h"
#include "text_frame.h"
#include "view_interface.h"

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

    Control(DisplayWindow *window, ElementInterface *element)
        : window_(window), element_(element) {
      // Incompatibility: we don't allow chaning id of a control.
      RegisterProperty("id",
                       NewSlot(element_, &ElementInterface::GetName), NULL);
      RegisterProperty("enabled",
                       NewSlot(element_, &ElementInterface::IsEnabled),
                       NewSlot(element_, &ElementInterface::SetEnabled));
      RegisterProperty("text",
                       NewSlot(this, &Control::GetText),
                       NewSlot(this, &Control::SetText));
      RegisterProperty("value",
                       NewSlot(this, &Control::GetValue),
                       NewSlot(this, &Control::SetValue));
      RegisterProperty("x", NULL, // No getter.
                       NewSlot(element_, &ElementInterface::SetPixelX));
      RegisterProperty("y", NULL, // No getter.
                       NewSlot(element_, &ElementInterface::SetPixelY));
      RegisterProperty("width", NULL, // No getter.
                       NewSlot(element_, &ElementInterface::SetPixelWidth));
      RegisterProperty("height", NULL, // No getter.
                       NewSlot(element_, &ElementInterface::SetPixelHeight));
      RegisterSignal("onChanged", &onchanged_signal_);
      RegisterSignal("onClicked", &onclicked_signal_);
    }

    virtual OwnershipPolicy Attach() { return OWNERSHIP_TRANSFERRABLE; }
    virtual bool Detach() { delete this; return true; }

    // The full content of the control.
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
      // TODO: if (element_->IsInstanceOf(ListBoxElement::CLASS_ID)) {
        // For list control it is an array of strings.
      // TODO: if (element_->IsInstanceOf(EditElement::CLASS_ID)) {
      //   EditElement *edit = down_cast<EditElement *>(element_);
      //   return Variant(....);
      ASSERT(false);
      return Variant();
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
          }
          // TODO: } else if (element_->IsInstanceOf(EditElement::CLASS_ID)) {
          else {
            LOG("Invalid type of text for control %s", element_->GetName());
          }
          break;
        }
        case Variant::TYPE_SCRIPTABLE: {
          // For list control it is an array of strings.
          // TODO:
          // ScriptableInterface *scriptable =
          //     VariantValue<ScriptableInterfae *>()(text);
          // if (scriptable->IsInstanceOf(ScriptableArray::CLASS_ID) &&
          //     element_->InstanceOf(ListBoxElement::CLASS_ID)) {
          // } else {
          //   LOG("Invalid type of text for control %s", element_->GetName());
          // }
        }
        default:
          LOG("Invalid type of text for control %s", element_->GetName());
          break;
      }
    }

    // The current value of the control.
    Variant GetValue() {
      if (element_->IsInstanceOf(CheckBoxElement::CLASS_ID)) {
        // For check box it is a boolean idicating the check state.
        CheckBoxElement *checkbox = down_cast<CheckBoxElement *>(element_);
        return Variant(checkbox->GetValue());
      }
      // TODO: if (element_->IsInstanceOf(ListBoxElement::CLASS_ID)) {
      // For list control it is the currently selected string. 
      //
      // For others it is the displayed text.
      return GetText();
    }

    void SetValue(const Variant &value) {
      switch (value.type()) {
        case Variant::TYPE_BOOL:
          if (element_->IsInstanceOf(CheckBoxElement::CLASS_ID)) {
            // For check box it is a boolean idicating the check state.
            CheckBoxElement *checkbox = down_cast<CheckBoxElement *>(element_);
            checkbox->SetValue(VariantValue<bool>()(value));
          } else {
            LOG("Invalid type of value for control %s", element_->GetName());
          }
          break;
        case Variant::TYPE_STRING:
          if (element_->IsInstanceOf(ButtonElement::CLASS_ID) ||
              element_->IsInstanceOf(LabelElement::CLASS_ID) // TODO: ||
              // TODO: element_->IsInstanceOf(EditElement::CLASS_ID))
              ) {
            SetText(value);
          // TODO: } else if (element_->IsInstanceOf(ListBoxElement::CLASS_ID))
          } else {
            LOG("Invalid type of value for control %s", element_->GetName());
          }
          break;
        default:
          LOG("Invalid type of value for control %s", element_->GetName());
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
    ElementInterface *element_;
    Signal2<void, DisplayWindow *, Control *> onchanged_signal_;
    Signal2<void, DisplayWindow *, Control *> onclicked_signal_;
  };

  Impl(DisplayWindow *owner, ViewInterface *view)
      : owner_(owner), view_(view) {
  }

  Control *AddControl(ControlClass ctrl_class, ControlType ctrl_type,
                      const char *ctrl_id, const Variant &text,
                      int x, int y, int width, int height) {
    ElementInterface *element = NULL;
    Control *control = NULL;
    switch (ctrl_class) {
      case CLASS_LABEL:
        element = LabelElement::CreateInstance(NULL, view_, ctrl_id);
        control = new Control(owner_, element);
        break;
      case CLASS_EDIT:
        // TODO:
        // element = EditElement::CreateInstance(NULL, view_, ctrl_id);
        // control = new Control(this, element);
        // if (ctrl_type == TYPE_EDIT_PASSWORD)
        //   down_cast<EditElement *>(element)->SetPasswordChar('*');
        // element->ConnectEvent("onchange",
        //                       NewSlot(control, &Control::OnClick));
        break;
      case CLASS_LIST:
        // TODO:
        // element = ListBoxElement::CreateInstance(NULL, view_, ctrl_id);
        // control = new Control(this, element);
        // element->ConnectEvent("onchange",
        //                       NewSlot(control, &Control::OnClick));
        break;
      case CLASS_BUTTON:
        switch (ctrl_type) {
          case TYPE_BUTTON_PUSH:
            element = ButtonElement::CreateInstance(NULL, view_, ctrl_id);
            control = new Control(owner_, element);
            element->ConnectEvent("onclick",
                                  NewSlot(control, &Control::OnClicked));
            break;
          case TYPE_BUTTON_CHECK:
            element = CheckBoxElement::CreateCheckBoxInstance(NULL, view_,
                                                              ctrl_id);
            control = new Control(owner_, element);
            // Note: the event name is "onchange", not "onclick", but the
            // handler is "onclick", because of the difference between
            // checkbox API and the old options API.
            element->ConnectEvent("onchange",
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
    return element ? new Control(owner_, element) : NULL;
  }

  DisplayWindow *owner_;
  ViewInterface *view_;
  Signal1<void, ButtonId> onclose_signal_;
};

DisplayWindow::DisplayWindow(ViewInterface *view)
    : impl_(new Impl(this, view)) {
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
