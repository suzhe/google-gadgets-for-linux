/*
  Copyright 2009 Google Inc.

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
#include <string>
#include <gtk/gtk.h>
#include <ggadget/element_factory.h>
#include <ggadget/gadget.h>
#include <ggadget/logger.h>
#include <ggadget/main_loop_interface.h>
#include <ggadget/script_context_interface.h>
#include <ggadget/scriptable_array.h>
#include <ggadget/string_utils.h>
#include <ggadget/view.h>
#include <webkit/webkit.h>

#include "browser_element.h"

#ifdef GGL_GTK_WEBKIT_SUPPORT_JSC
#include <ggadget/script_runtime_manager.h>
#include "../webkit_script_runtime/js_script_runtime.h"
#include "../webkit_script_runtime/js_script_context.h"
#endif

#define Initialize gtkwebkit_browser_element_LTX_Initialize
#define Finalize gtkwebkit_browser_element_LTX_Finalize
#define RegisterElementExtension \
    gtkwebkit_browser_element_LTX_RegisterElementExtension

extern "C" {
  bool Initialize() {
    LOGI("Initialize gtkwebkit_browser_element extension.");
    return true;
  }

  void Finalize() {
    LOGI("Finalize gtkwebkit_browser_element extension.");
  }

  bool RegisterElementExtension(ggadget::ElementFactory *factory) {
    LOGI("Register gtkwebkit_browser_element extension, "
         "using name \"_browser\".");
    if (factory) {
      factory->RegisterElementClass(
          "_browser", &ggadget::gtkwebkit::BrowserElement::CreateInstance);
    }
    return true;
  }
}

using namespace ggadget::webkit;

namespace ggadget {
namespace gtkwebkit {

class BrowserElement::Impl {
 public:
  Impl(BrowserElement *owner)
    : content_type_("text/html"),
      owner_(owner),
      web_view_(NULL),
#ifdef GGL_GTK_WEBKIT_SUPPORT_JSC
      browser_context_(NULL),
#endif
      minimized_connection_(owner_->GetView()->ConnectOnMinimizeEvent(
          NewSlot(this, &Impl::OnViewMinimized))),
      restored_connection_(owner_->GetView()->ConnectOnRestoreEvent(
          NewSlot(this, &Impl::OnViewRestored))),
      popout_connection_(owner_->GetView()->ConnectOnPopOutEvent(
          NewSlot(this, &Impl::OnViewPoppedOut))),
      popin_connection_(owner_->GetView()->ConnectOnPopInEvent(
          NewSlot(this, &Impl::OnViewPoppedIn))),
      dock_connection_(owner_->GetView()->ConnectOnDockEvent(
          NewSlot(this, &Impl::OnViewDockUndock))),
      undock_connection_(owner_->GetView()->ConnectOnUndockEvent(
          NewSlot(this, &Impl::OnViewDockUndock))),
      popped_out_(false),
      minimized_(false),
      x_(0),
      y_(0),
      width_(0),
      height_(0) {
  }

  ~Impl() {
    minimized_connection_->Disconnect();
    restored_connection_->Disconnect();
    popout_connection_->Disconnect();
    popin_connection_->Disconnect();
    dock_connection_->Disconnect();
    undock_connection_->Disconnect();

#ifdef GGL_GTK_WEBKIT_SUPPORT_JSC
    delete browser_context_;
    browser_context_ = NULL;
#endif

    if (GTK_IS_WIDGET(web_view_)) {
      gtk_widget_destroy(web_view_);
      web_view_ = NULL;
    }
  }

  void GetWidgetExtents(gint *x, gint *y, gint *width, gint *height) {
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

    *x = static_cast<gint>(round(widget_x0));
    *y = static_cast<gint>(round(widget_y0));
    *width = static_cast<gint>(ceil(widget_x1 - widget_x0));
    *height = static_cast<gint>(ceil(widget_y1 - widget_y0));
  }

  void EnsureBrowser() {
    if (!web_view_) {
      GtkWidget *container = GTK_WIDGET(owner_->GetView()->GetNativeWidget());
      if (!GTK_IS_FIXED(container)) {
        LOG("BrowserElement needs a GTK_FIXED parent. Actual type: %s",
            G_OBJECT_TYPE_NAME(container));
        return;
      }

      web_view_ = GTK_WIDGET(webkit_web_view_new());
      ASSERT(web_view_);
      g_signal_connect(G_OBJECT(web_view_), "destroy",
                       G_CALLBACK(WebViewDestroyedCallback), this);
      g_signal_connect(G_OBJECT(web_view_), "console-message",
                       G_CALLBACK(WebViewConsoleMessageCallback), this);
      g_signal_connect(G_OBJECT(web_view_), "navigation-requested",
                       G_CALLBACK(WebViewNavigationRequestedCallback), this);

      GetWidgetExtents(&x_, &y_, &width_, &height_);

      gtk_fixed_put(GTK_FIXED(container), web_view_, x_, y_);
      gtk_widget_set_size_request(GTK_WIDGET(web_view_), width_, height_);
      gtk_widget_show(web_view_);

#ifdef GGL_GTK_WEBKIT_SUPPORT_JSC
      JSScriptRuntime *runtime = down_cast<JSScriptRuntime *>(
          ScriptRuntimeManager::get()->GetScriptRuntime("webkitjs"));

      if (runtime) {
        WebKitWebFrame *main_frame =
            webkit_web_view_get_main_frame(WEBKIT_WEB_VIEW(web_view_));
        ASSERT(main_frame);
        JSGlobalContextRef js_context =
            webkit_web_frame_get_global_context(main_frame);
        ASSERT(js_context);

        browser_context_ = runtime->WrapExistingContext(js_context);
        browser_context_->AssignFromNative(NULL, "", "external",
                                           Variant(external_object_.Get()));
      } else {
        LOGE("webkit-script-runtime is not loaded.");
      }
#endif

      if (content_.length()) {
        webkit_web_view_load_html_string(WEBKIT_WEB_VIEW(web_view_),
                                         content_.c_str(), "");
      }
    }
  }

  void Layout() {
    EnsureBrowser();
    GtkWidget *container = GTK_WIDGET(owner_->GetView()->GetNativeWidget());
    if (GTK_IS_FIXED(container) && WEBKIT_IS_WEB_VIEW(web_view_)) {
      bool force_layout = false;
      // check if the contain has changed.
      if (gtk_widget_get_parent(web_view_) != container) {
        gtk_widget_reparent(GTK_WIDGET(web_view_), container);
        force_layout = true;
      }

      gint x, y, width, height;
      GetWidgetExtents(&x, &y, &width, &height);

      if (x != x_ || y != y_ || force_layout) {
        x_ = x;
        y_ = y;
        gtk_fixed_move(GTK_FIXED(container), GTK_WIDGET(web_view_), x, y);
      }
      if (width != width_ || height != height_ || force_layout) {
        width_ = width;
        height_ = height;
        gtk_widget_set_size_request(GTK_WIDGET(web_view_), width, height);
      }
      if (owner_->IsReallyVisible() && (!minimized_ || popped_out_))
        gtk_widget_show(web_view_);
      else
        gtk_widget_hide(web_view_);
    }
  }

  void SetContent(const std::string &content) {
    content_ = content;
    if (GTK_IS_WIDGET(web_view_)) {
      webkit_web_view_load_html_string(WEBKIT_WEB_VIEW(web_view_),
                                       content.c_str(), "");
    }
  }

  void SetExternalObject(ScriptableInterface *object) {
    external_object_.Reset(object);
#ifdef GGL_GTK_WEBKIT_SUPPORT_JSC
    if (browser_context_)
      browser_context_->AssignFromNative(NULL, "", "external", Variant(object));
#endif
  }

  void OnViewMinimized() {
    // The browser widget must be hidden when the view is minimized.
    if (GTK_IS_WIDGET(web_view_) && !popped_out_) {
      gtk_widget_hide(web_view_);
    }
    minimized_ = true;
  }

  void OnViewRestored() {
    if (GTK_IS_WIDGET(web_view_) && owner_->IsReallyVisible() && !popped_out_) {
      gtk_widget_show(web_view_);
    }
    minimized_ = false;
  }

  void OnViewPoppedOut() {
    popped_out_ = true;
    Layout();
  }

  void OnViewPoppedIn() {
    popped_out_ = false;
    Layout();
  }

  void OnViewDockUndock() {
    // The toplevel window might be changed, so it's necessary to reparent the
    // browser widget.
    Layout();
  }

  static void WebViewDestroyedCallback(GtkWidget *widget, Impl *impl) {
    DLOG("WebViewDestroyedCallback(Impl=%p, web_view=%p)", impl, widget);

    impl->web_view_ = NULL;
#ifdef GGL_GTK_WEBKIT_SUPPORT_JSC
    delete impl->browser_context_;
    impl->browser_context_ = NULL;
#endif
  }

  static gboolean WebViewConsoleMessageCallback(WebKitWebView *web_view,
                                                gchar *message,
                                                gint line,
                                                gchar *source_id,
                                                Impl *impl) {
    ScopedLogContext(impl->owner_->GetView()->GetGadget());
    LOGI("BrowserElement (%s:%d): %s", source_id, line, message);
    return TRUE;
  }

  static WebKitNavigationResponse WebViewNavigationRequestedCallback(
      WebKitWebView *web_view, WebKitWebFrame *web_frame,
      WebKitNetworkRequest *request, Impl *impl) {
    DLOG("WebViewNavigationRequestedCallback(Impl=%p, web_view=%p, uri=%p)",
         impl, web_view, webkit_network_request_get_uri(request));

    // TODO
    return WEBKIT_NAVIGATION_RESPONSE_ACCEPT;
  }

  std::string content_type_;
  std::string content_;
  BrowserElement *owner_;
  GtkWidget *web_view_;

#ifdef GGL_GTK_WEBKIT_SUPPORT_JSC
  JSScriptContext *browser_context_;
#endif

  Connection *minimized_connection_;
  Connection *restored_connection_;
  Connection *popout_connection_;
  Connection *popin_connection_;
  Connection *dock_connection_;
  Connection *undock_connection_;

  ScriptableHolder<ScriptableInterface> external_object_;

  bool popped_out_;
  bool minimized_;
  gint x_;
  gint y_;
  gint width_;
  gint height_;
};

BrowserElement::BrowserElement(View *view, const char *name)
  : BasicElement(view, "browser", name, true),
    impl_(new Impl(this)) {
}

BrowserElement::~BrowserElement() {
  delete impl_;
  impl_ = NULL;
}

std::string BrowserElement::GetContentType() const {
  return impl_->content_type_;
}

void BrowserElement::SetContentType(const char *content_type) {
  impl_->content_type_ =
      content_type && *content_type ? content_type : "text/html";
}

void BrowserElement::SetContent(const std::string &content) {
  impl_->SetContent(content);
}

void BrowserElement::SetExternalObject(ScriptableInterface *object) {
  impl_->SetExternalObject(object);
}

void BrowserElement::Layout() {
  BasicElement::Layout();
  impl_->Layout();
}

void BrowserElement::DoDraw(CanvasInterface *canvas) {
}

BasicElement *BrowserElement::CreateInstance(View *view, const char *name) {
  return new BrowserElement(view, name);
}

void BrowserElement::DoClassRegister() {
  BasicElement::DoClassRegister();
  RegisterProperty("contentType",
                   NewSlot(&BrowserElement::GetContentType),
                   NewSlot(&BrowserElement::SetContentType));
  RegisterProperty("innerText", NULL,
                   NewSlot(&BrowserElement::SetContent));
  RegisterProperty("external", NULL,
                   NewSlot(&BrowserElement::SetExternalObject));
}

} // namespace gtkwebkit
} // namespace ggadget
