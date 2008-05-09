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

#include <QtWebKit/QWebView>
#include <ggadget/gadget.h>
#include <ggadget/logger.h>
#include <ggadget/main_loop_interface.h>
#include <ggadget/scriptable_array.h>
#include <ggadget/string_utils.h>
#include <ggadget/unicode_utils.h>
#include <ggadget/view.h>
#include <ggadget/element_factory.h>
#include <ggadget/script_context_interface.h>
#include <ggadget/qt/qt_gadget_widget.h>

#include "browser_element.h"

#define Initialize qtwebkit_browser_element_LTX_Initialize
#define Finalize qtwebkit_browser_element_LTX_Finalize
#define RegisterElementExtension \
    qtwebkit_browser_element_LTX_RegisterElementExtension

namespace ggadget {
namespace qt {
// This function is only used to decode the HTML/Text content sent in CONTENT.
// We can't use JSONDecode because the script context is not available now.

static bool DecodeJSONString(const char *json_string, UTF16String *result) {
  if (!json_string || *json_string != '"')
    return false;
  while (*++json_string != '"') {
    if (*json_string == '\0') {
      // Unterminated JSON string.
      return false;
    }
    if (*json_string == '\\') {
      switch (*++json_string) {
        case 'b': result->push_back('\b'); break;
        case 'f': result->push_back('\f'); break;
        case 'n': result->push_back('\n'); break;
        case 'r': result->push_back('\r'); break;
        case 't': result->push_back('\t'); break;
        case 'u': {
          UTF16Char unichar = 0;
          for (int i = 1; i <= 4; i++) {
            char c = json_string[i];
            if (c >= '0' && c <= '9') c -= '0';
            else if (c >= 'A' && c <= 'F') c = c - 'A' + 10;
            else if (c >= 'a' && c <= 'f') c = c - 'a' + 10;
            else return false;
            unichar = (unichar << 4) + c;
          }
          result->push_back(unichar);
          json_string += 4;
          break;
        }
        case '\0': return false;
        default: result->push_back(*json_string); break;
      }
    } else {
      result->push_back(*json_string);
    }
  }
  return true;
}

class BrowserElement::Impl {
 public:
  Impl(BrowserElement *owner)
      : owner_(owner),
        content_type_("text/html") {
    QGadgetWidget *parent = static_cast<QGadgetWidget*>(
        owner_->GetView()->GetNativeWidget()
        );
    parent->SetChild(&child_);
  }

  ~Impl() {
  }
  void GetWidgetExtents(int *x, int *y, int *width, int *height) {
    double widget_x0, widget_y0;
    double widget_x1, widget_y1;
    owner_->SelfCoordToViewCoord(0, 0, &widget_x0, &widget_y0);
    owner_->SelfCoordToViewCoord(owner_->GetPixelWidth(),
                                 owner_->GetPixelHeight(),
                                 &widget_x1, &widget_y1);

    owner_->GetView()->ViewCoordToNativeWidgetCoord(widget_x0, widget_y0,
                                                    &widget_x0, &widget_y0);
    owner_->GetView()->ViewCoordToNativeWidgetCoord(widget_x1, widget_y1,
                                                    &widget_x1, &widget_y1);

    *x = static_cast<int>(round(widget_x0));
    *y = static_cast<int>(round(widget_y0));
    *width = static_cast<int>(ceil(widget_x1 - widget_x0));
    *height = static_cast<int>(ceil(widget_y1 - widget_y0));
  }

  void Layout() {
    int x, y, w, h;
    GetWidgetExtents(&x, &y, &w, &h);
    LOG("Layout:%d,%d,%d,%d", x, y, w, h);
    child_.setFixedSize(w, h);
    child_.move(x, y);
  }

  void SetContent(const JSONString &content) {
    UTF16String utf16str;
    if (!DecodeJSONString(content.value.c_str(), &utf16str)) {
      LOG("Invalid content for browser");
      return;
    }

    std::string utf8str = "";
    ConvertStringUTF16ToUTF8(utf16str.c_str(), utf16str.length(), &utf8str);

    LOG("Content: %s", utf8str.c_str());
    child_.setContent(utf8str.c_str());
  }

  QWebView child_;
  BrowserElement *owner_;
  std::string content_type_;
  std::string content_;
  Signal1<JSONString, JSONString> get_property_signal_;
  Signal2<void, JSONString, JSONString> set_property_signal_;
  Signal2<JSONString, JSONString, ScriptableArray *> callback_signal_;
  Signal1<bool, const std::string &> open_url_signal_;
};

BrowserElement::BrowserElement(BasicElement *parent, View *view,
                               const char *name)
    : BasicElement(parent, view, "browser", name, true),
      impl_(new Impl(this)) {
}

void BrowserElement::DoRegister() {
  BasicElement::DoRegister();
  RegisterProperty("contentType",
                   NewSlot(this, &BrowserElement::GetContentType),
                   NewSlot(this, &BrowserElement::SetContentType));
  RegisterProperty("innerText", NULL,
                   NewSlot(this, &BrowserElement::SetContent));
  RegisterSignal("onGetProperty", &impl_->get_property_signal_);
  RegisterSignal("onSetProperty", &impl_->set_property_signal_);
  RegisterSignal("onCallback", &impl_->callback_signal_);
  RegisterSignal("onOpenURL", &impl_->open_url_signal_);
}

BrowserElement::~BrowserElement() {
  delete impl_;
  impl_ = NULL;
}

std::string BrowserElement::GetContentType() const {
  return impl_->content_type_;
}

void BrowserElement::SetContentType(const char *content_type) {
  impl_->content_type_ = content_type && *content_type ? content_type :
                         "text/html";
}

void BrowserElement::SetContent(const JSONString &content) {
  impl_->SetContent(content);
}

void BrowserElement::Layout() {
  BasicElement::Layout();
  impl_->Layout();
}

void BrowserElement::DoDraw(CanvasInterface *canvas) {
}

BasicElement *BrowserElement::CreateInstance(BasicElement *parent, View *view,
                                             const char *name) {
  return new BrowserElement(parent, view, name);
}

} // namespace gtkmoz
} // namespace ggadget

extern "C" {
  bool Initialize() {
    LOG("Initialize qtwebkit_browser_element extension.");
    return true;
  }

  void Finalize() {
    LOG("Finalize qtwebkit_browser_element extension.");
  }

  bool RegisterElementExtension(ggadget::ElementFactory *factory) {
    LOG("Register qtwebkit_browser_element extension, using name \"_browser\".");
    if (factory) {
      factory->RegisterElementClass(
          "_browser", &ggadget::qt::BrowserElement::CreateInstance);
    }
    return true;
  }
}
