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
#include "div_element.h"
#include "edit_element.h"
#include "element_interface.h"
#include "elements_interface.h"
#include "item_element.h"
#include "label_element.h"
#include "listbox_element.h"
#include "logger.h"
#include "scriptable_array.h"
#include "string_utils.h"
#include "text_frame.h"
#include "view.h"

namespace ggadget {

static const int kLabelTextSize = 9;
static const int kListItemHeight = 19;
static const double kZoomRatio = 1.1;
static const char *kControlBorderColor = "#808080";
static const char *kBackgroundColor = "#FFFFFF";
static const int kMaxComboBoxHeight = 150;

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
                       NewSlot(this, &Control::SetEnabled));
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
      return ScriptableArray::Create(array, actual_count);
    }

    void SetEnabled(bool enabled) {
      element_->SetEnabled(enabled);
      element_->SetOpacity(enabled ? 1.0 : 0.5);
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

    static const int kMaxListItems = 512;
    void SetListBoxItems(ListBoxElement *listbox, ScriptableInterface *array) {
      listbox->GetChildren()->RemoveAllElements();
      if (array) {
        Variant length_v = GetPropertyByName(array, "length");
        int length;
        if (length_v.ConvertToInt(&length)) {
          if (length > kMaxListItems)
            length = kMaxListItems;
          for (int i = 0; i < length; i++) {
            Variant v = array->GetProperty(i);
            std::string str_value;
            if (v.ConvertToString(&str_value)) {
              listbox->AppendString(str_value.c_str());
            } else {
              LOG("Invalid type of array item(%s) for control %s",
                  v.Print().c_str(), element_->GetName().c_str());
            }
          }
        }
      }
    }

    void SetText(const Variant &text) {
      bool invalid = false;
      if (text.type() == Variant::TYPE_SCRIPTABLE) {
        ScriptableInterface *array =
             VariantValue<ScriptableInterface *>()(text);
        if (array) {
          if (element_->IsInstanceOf(ListBoxElement::CLASS_ID)) {
            SetListBoxItems(down_cast<ListBoxElement *>(element_), array);
          } else if (element_->IsInstanceOf(ComboBoxElement::CLASS_ID)) {
            SetListBoxItems(
                down_cast<ComboBoxElement *>(element_)->GetListBox(), array);
          } else {
            invalid = true;
          }
        }
      } else {
        std::string text_str;
        if (text.ConvertToString(&text_str)) {
          if (element_->IsInstanceOf(ButtonElement::CLASS_ID)) {
            ButtonElement *button = down_cast<ButtonElement *>(element_);
            button->GetTextFrame()->SetText(text_str.c_str());
          } else if (element_->IsInstanceOf(CheckBoxElement::CLASS_ID)) {
            CheckBoxElement *checkbox = down_cast<CheckBoxElement *>(element_);
            checkbox->GetTextFrame()->SetText(text_str.c_str());
          } else if (element_->IsInstanceOf(LabelElement::CLASS_ID)) {
            LabelElement *label = down_cast<LabelElement *>(element_);
            label->GetTextFrame()->SetText(text_str.c_str());
          } else if (element_->IsInstanceOf(EditElement::CLASS_ID)) {
            EditElement *edit = down_cast<EditElement *>(element_);
            edit->SetValue(text_str.c_str());
          } else {
            invalid = true;
          }
        } else {
          invalid = true;
        }
      }

      if (invalid) {
        LOG("Invalid type of text(%s) for control %s",
            text.Print().c_str(), element_->GetName().c_str());
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
      bool invalid = false;
      std::string value_str;
      if (value.ConvertToString(&value_str)) {
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
          invalid = true;
        }
      } else {
        bool value_bool;
        if (value.ConvertToBool(&value_bool)) {
          if (element_->IsInstanceOf(CheckBoxElement::CLASS_ID)) {
            // For check box it is a boolean idicating the check state.
            CheckBoxElement *checkbox = down_cast<CheckBoxElement *>(element_);
            checkbox->SetValue(VariantValue<bool>()(value));
          } else {
            invalid = true;
          }
        } else {
          invalid = true;
        } 
      }
      if (invalid) {
        LOG("Invalid type of value(%s) for control %s",
            value.Print().c_str(), element_->GetName().c_str());
      }
    }

    void SetRect(int x, int y, int width, int height) {
      element_->SetPixelX(x);
      element_->SetPixelY(y);
      element_->SetPixelWidth(width);
      element_->SetPixelHeight(height);
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
      : owner_(owner), view_(view),
        min_x_(9999), min_y_(9999), max_x_(0), max_y_(0) {
  }

  ~Impl() {
    for (ControlsMap::iterator it = controls_.begin();
         it != controls_.end(); ++it) {
      delete it->second;
    }
    controls_.clear();
  }

  DivElement *CreateFrameDiv(ElementsInterface *elements) {
    DivElement *div = down_cast<DivElement *>(elements->AppendElement("div",
                                                                      NULL));
    div->SetBackground(Variant(kControlBorderColor));
    return div;
  }

  Control *AddControl(ControlClass ctrl_class, ControlType ctrl_type,
                      const char *ctrl_id, const Variant &text,
                      int x, int y, int width, int height) {
    Control *control = NULL;
    ElementsInterface *elements = view_->GetChildren();
    DivElement *div = NULL;  // Some elements may need a frame.

    switch (ctrl_class) {
      case CLASS_LABEL: {
        LabelElement *element = down_cast<LabelElement *>(
            elements->AppendElement("label", ctrl_id));
        element->GetTextFrame()->SetWordWrap(true);
        element->GetTextFrame()->SetSize(kLabelTextSize);
        control = new Control(owner_, element);
        break;
      }
      case CLASS_EDIT: {
        // Our border is thinner than Windows version, so thrink the height.
        y += 1;
        height -= 2;
        div = CreateFrameDiv(elements);
        EditElement *element = down_cast<EditElement *>(
            elements->AppendElement("edit", ctrl_id));
        if (ctrl_type == TYPE_EDIT_PASSWORD)
          element->SetPasswordChar("*");
        control = new Control(owner_, element);
        element->ConnectOnChangeEvent(NewSlot(control, &Control::OnChange));
        break;
      }
      case CLASS_LIST:
        switch (ctrl_type) {
          default:
          case TYPE_LIST_OPEN: {
            div = CreateFrameDiv(elements);
            ListBoxElement *element = down_cast<ListBoxElement *>(
                elements->AppendElement("listbox", ctrl_id));
            element->SetItemWidth(Variant("100%"));
            element->SetItemHeight(Variant(kListItemHeight));
            control = new Control(owner_, element);
            element->ConnectOnChangeEvent(NewSlot(control, &Control::OnChange));
            break;
          }
          case TYPE_LIST_DROP: {
            div = CreateFrameDiv(elements);
            ComboBoxElement *element = down_cast<ComboBoxElement *>(
                elements->AppendElement("combobox", ctrl_id));
            element->SetType(ComboBoxElement::COMBO_DROPLIST);
            element->GetListBox()->SetItemWidth(Variant("100%"));
            element->GetListBox()->SetItemHeight(Variant(kListItemHeight));
            element->SetBackground(Variant(kBackgroundColor));
            control = new Control(owner_, element);
            element->ConnectOnChangeEvent(NewSlot(control, &Control::OnChange));
            break;
          }
        }
        break;
      case CLASS_BUTTON:
        switch (ctrl_type) {
          default:
          case TYPE_BUTTON_PUSH: {
            ButtonElement *element = down_cast<ButtonElement *>(
                elements->AppendElement("button", ctrl_id));
            element->GetTextFrame()->SetSize(kLabelTextSize);
            element->UseDefaultImages();
            control = new Control(owner_, element);
            element->ConnectOnClickEvent(NewSlot(control, &Control::OnClicked));
            break;
          }
          case TYPE_BUTTON_CHECK: {
            CheckBoxElement *element = down_cast<CheckBoxElement *>(
                elements->AppendElement("checkbox", ctrl_id));
            element->GetTextFrame()->SetSize(kLabelTextSize);
            element->UseDefaultImages();
            // Default value of gadget checkbox element is false, but here
            // the default value should be false. 
            element->SetValue(false);
            // Note: the event name is "onchange", not "onclick", but the
            // handler is "onclick", because of the difference between
            // checkbox API and the old options API.
            control = new Control(owner_, element);
            element->ConnectOnChangeEvent(NewSlot(control,
                                                  &Control::OnClicked));
            break;
          }
        }
        break;
      default:
        LOG("Unknown control class: %d", ctrl_class);
        break;
    }

    if (control) {
      // The control sizes in the gadgets are too small for GTK.
      x = static_cast<int>(x * kZoomRatio);
      y = static_cast<int>(y * kZoomRatio);
      width = static_cast<int>(width * kZoomRatio);
      height = static_cast<int>(height * kZoomRatio);

      if (div) {
        div->SetPixelX(x);
        div->SetPixelY(y);
        div->SetPixelWidth(width);
        if (ctrl_type == TYPE_LIST_DROP) {
          div->SetPixelHeight(kListItemHeight + 2);
          // Because our combobox can't pop out of the dialog box, we must
          // limit the height of the combobox
          if (height > kMaxComboBoxHeight)
            height = kMaxComboBoxHeight;
        } else {
          div->SetPixelHeight(height);
        }
        control->SetRect(x + 1, y + 1, width - 2, height - 2);
      } else {
        control->SetRect(x, y, width, height);
      }
      control->SetText(text);
      min_x_ = std::min(std::max(0, x), min_x_);
      min_y_ = std::min(std::max(0, y), min_y_);
      max_x_ = std::max(x + width, max_x_);
      max_y_ = std::max(y + height, max_y_);

      controls_.insert(make_pair(std::string(ctrl_id), control));
      return control;
    }

    return NULL;
  }

  Control *GetControl(const char *ctrl_id) {
    ControlsMap::iterator it = controls_.find(ctrl_id);
    return it == controls_.end() ? NULL : it->second;
  }

  void OnOk() {
    onclose_signal_(owner_, ID_OK);
  }

  void OnCancel() {
    onclose_signal_(owner_, ID_CANCEL);
  }

  DisplayWindow *owner_;
  View *view_;
  Signal2<void, DisplayWindow *, ButtonId> onclose_signal_;
  int min_x_, min_y_, max_x_, max_y_;
  typedef std::multimap<std::string, Control *,
                        GadgetStringComparator> ControlsMap;
  ControlsMap controls_;
};

DisplayWindow::DisplayWindow(ViewInterface *view)
    : impl_(new Impl(this, down_cast<View *>(view))) {
  ASSERT(view);
  RegisterMethod("AddControl", NewSlot(impl_, &Impl::AddControl));
  RegisterMethod("GetControl", NewSlot(impl_, &Impl::GetControl));
  RegisterSignal("OnClose", &impl_->onclose_signal_);
  impl_->view_->ConnectOnOkEvent(NewSlot(impl_, &Impl::OnOk));
  impl_->view_->ConnectOnCancelEvent(NewSlot(impl_, &Impl::OnCancel));
}

DisplayWindow::~DisplayWindow() {
  delete impl_;
  impl_ = NULL;
}

void DisplayWindow::AdjustSize() {
  impl_->view_->SetSize(impl_->max_x_ + impl_->min_x_,
                        impl_->max_y_ + impl_->min_y_);
}

} // namespace ggadget
