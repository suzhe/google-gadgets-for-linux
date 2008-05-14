#ifndef GGADGET_QTWEBKIT_BROWSER_ELEMENT_INTERNAL_H__
#define GGADGET_QTWEBKIT_BROWSER_ELEMENT_INTERNAL_H__

#include <string>
#if QT_VERSION < 0x040400
#include <qwebnetworkinterface.h>
#else
#include <QtNetwork/QNetworkRequest>
#endif
#include <ggadget/gadget.h>

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

class BrowserElement::Impl;

class WebPage : public QWebPage {
  Q_OBJECT
 public:
  WebPage(BrowserElement::Impl *url_handler) : QWebPage(), handler_(url_handler) {
#if QT_VERSION >= 0x040400
    connect(this, SIGNAL(linkHovered(const QString &, const QString &,
                                     const QString &)),
            this, SLOT(OnLinkHovered(const QString &, const QString &,
                                     const QString &)));
#else
    connect(this, SIGNAL(hoveringOverLink(const QString &, const QString &,
                                          const QString &)),
            this, SLOT(OnLinkHovered(const QString &, const QString &,
                                     const QString &)));
#endif
  }
 protected:
  virtual QWebPage *createWindow(
#if QT_VERSION >= 0x040400
     WebWindowType type
#endif
   );

 private slots:
  void OnLinkHovered(const QString &link, const QString & title, const QString & textContent ) {
    url_ = link;
  }

 private:
  QString url_;
  BrowserElement::Impl *handler_;
};


class WebView : public QWebView {
  Q_OBJECT
 public:
  WebView(BrowserElement::Impl *owner) : owner_(owner) {}
  BrowserElement::Impl *owner_;

 public slots:
  void OnParentDestroyed(QObject* obj);
};

class BrowserElement::Impl {
 public:
  Impl(BrowserElement *owner)
    : owner_(owner),
      parent_(NULL),
      child_(new WebView(this)),
      content_type_("text/html") {
    child_->setPage(new WebPage(this));
  }

  ~Impl() {
    // if parent set, child_ will deleted by parent_
    if (!parent_) delete child_;
  }

  void OpenUrl(const QString &url) const {
    std::string u = url.toStdString();
    if (!open_url_signal_.HasActiveConnections() ||
        open_url_signal_(u)) {
      Gadget *gadget = owner_->GetView()->GetGadget();
      if (gadget) {
        // Let the gadget allow this OpenURL gracefully.
        bool old_interaction = gadget->SetInUserInteraction(true);
        gadget->OpenURL(u.c_str());
        gadget->SetInUserInteraction(old_interaction);
      }
    }
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
    if (!parent_) {
      parent_ = static_cast<QGadgetWidget*>(
          owner_->GetView()->GetNativeWidget()
          );
      if (!parent_) return;
      parent_->SetChild(child_);
      QObject::connect(parent_, SIGNAL(destroyed(QObject*)),
                       child_, SLOT(OnParentDestroyed(QObject*)));
      child_->show();
    }
    int x, y, w, h;
    GetWidgetExtents(&x, &y, &w, &h);
    LOG("Layout:%d,%d,%d,%d", x, y, w, h);
    child_->setFixedSize(w, h);
    child_->move(x, y);
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
    child_->setContent(utf8str.c_str());
  }

 public:
  BrowserElement *owner_;
  QGadgetWidget *parent_;
  QWebView *child_;
  std::string content_type_;
  std::string content_;
  Signal1<JSONString, JSONString> get_property_signal_;
  Signal2<void, JSONString, JSONString> set_property_signal_;
  Signal2<JSONString, JSONString, ScriptableArray *> callback_signal_;
  Signal1<bool, const std::string &> open_url_signal_;
};

QWebPage *WebPage::createWindow(
#if QT_VERSION >= 0x040400
     WebWindowType type
#endif
     ) {
  handler_->OpenUrl(url_);
  return NULL;
}

void WebView::OnParentDestroyed(QObject *obj) {
  ASSERT(owner_->parent_ == obj);
  LOG("Parent widget destroyed");
  owner_->parent_ = NULL;
}

}
}
#endif
