/*
  Copyright 2008 Google Inc.

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

#include "gtk_flash_element.h"

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>

#include <ggadget/view.h>
#include <ggadget/element_factory.h>
#include <ggadget/canvas_interface.h>
#include <ggadget/gtk/cairo_canvas.h>
#include <ggadget/npapi/npapi_plugin.h>
#include <ggadget/npapi/npapi_container.h>

#include "gtk_windowed_flash_element.h"

#define Initialize gtk_flash_element_LTX_Initialize
#define Finalize gtk_flash_element_LTX_Finalize
#define RegisterElementExtension gtk_flash_element_LTX_RegisterElementExtension

extern "C" {
  bool Initialize() {
    LOGI("Initialize gtk_flash_element extension.");
    return true;
  }

  void Finalize() {
    LOGI("Finalize gtk_flash_element extension.");
  }

  bool RegisterElementExtension(ggadget::ElementFactory *factory) {
    if (factory) {
      LOGI("Register gtk_flash_element extension, using name \"flash\".");
      factory->RegisterElementClass(
          "flash", &ggadget::gtk::GtkFlashElement::CreateInstance);
    }
    return true;
  }
}

namespace ggadget {
namespace gtk {

using namespace ggadget::npapi;

// FIXME: currently, the flash plugin only enables flash-javascript
// interaction for IE, but not Firefox, opera.
// Related discussion: http://groups.google.com/group/comp.lang.javascript/
// browse_frm/thread/aa0fff388526cbf4/41b4c06914d7274a
static const bool kPluginSupportScriptableAPI = false;

class GtkFlashElement::Impl {
 public:
  Impl(GtkFlashElement *owner, View *view)
      : owner_(owner), view_(view), native_widget_(NULL),
        plugin_(NULL), scriptable_plugin_(NULL),
        windowless_(true), pixmap_(NULL),
        flash_element_(NULL),
        initialized_(false), focused_(false) {
    // Use windowless/transparent mode by default.
    char *attrn[1] = { strdup("wmode") };
    char *attrv[1] = { strdup("transparent") };
    plugin_ = GetGlobalNPContainer()->CreatePlugin(FLASH_MIME_TYPE, owner,
                                                   true, GTK2, 1, attrn, attrv);
    free(attrn[0]);
    free(attrv[0]);

    if (plugin_) {
      windowless_ =
          plugin_->GetWindowType() == WindowTypeWindowless ? true : false;
      if (!windowless_) {
        LOGW("Plugin doesn't support windowless mode.");
        // The plugin doesn't support windowless, turn to window mode flash
        // element. The child element will create a new plugin instance.
        flash_element_ = new GtkWindowedFlashElement(owner_, view,
                                                     owner_->GetName().c_str());
        if (flash_element_)
          LOGW("Use window mode instead.");
        else
          LOGE("Failed to initialize plugin when try to use window mode.");
        GetGlobalNPContainer()->DestroyPlugin(plugin_);
        plugin_ = NULL;
        return;
      }
      // Get the root scriptable object of the plugin.
      scriptable_plugin_ = plugin_->GetScriptablePlugin();
      initialized_ = true;
      window_.ws_info_ = &ws_info_;
    }
  }

  ~Impl() {
    if (plugin_)
      GetGlobalNPContainer()->DestroyPlugin(plugin_);
    if (pixmap_)
      g_object_unref(pixmap_);
    if (flash_element_)
      delete flash_element_;
  }

  const char *GetSrc() {
    ASSERT(windowless_);
    return src_.c_str();
  }

  void SetSrc(const char *src) {
    ASSERT(windowless_);
    if (!src || src_.compare(src) == 0)
      return;
    src_ = src;
    // FIXME: We cannot use any script control for flash playing. We only
    // provide one basic operation, i.e. play another flash, and what we can
    // do is to create a new flash plugin instance, destroy the old one.
    // Although this is a little ugly, it works.
    if (!kPluginSupportScriptableAPI) {
      // Use windowless/transparent mode by default.
      char *attrn[1] = { strdup("wmode") };
      char *attrv[1] = { strdup("transparent") };
      NPPlugin *plugin =
          GetGlobalNPContainer()->CreatePlugin(FLASH_MIME_TYPE, owner_,
                                               true, GTK2, 1, attrn, attrv);
      free(attrn[0]);
      free(attrv[0]);

      if (plugin) {
        ASSERT(plugin->GetWindowType() == WindowTypeWindowless);
        initialized_ = true;
      }
      if (plugin_)
        GetGlobalNPContainer()->DestroyPlugin(plugin_);
      plugin_ = plugin;
    }
    if (!initialized_) {
      LOGE("The flash player plugin is not initialized.");
      return;
    }
    if (UpdateWindow()) {
      plugin_->SetURL(src);
    }
  }

  void UpdateCoordinates() {
    window_.x_ = 0;
    window_.y_ = 0;
    window_.width_ = static_cast<uint32_t>(ceil(owner_->GetPixelWidth()));
    window_.height_ = static_cast<uint32_t>(ceil(owner_->GetPixelHeight()));
    window_.cliprect_.left_ = window_.x_;
    window_.cliprect_.top_ = window_.y_;
    window_.cliprect_.right_ = window_.x_ + window_.width_;
    window_.cliprect_.bottom_ = window_.y_ + window_.height_;
  }

  bool UpdateWindow() {
#ifdef GDK_WINDOWING_X11
    native_widget_ = GTK_WIDGET(owner_->GetView()->GetNativeWidget());
    GdkWindow *gdk_window = gtk_widget_get_toplevel(native_widget_)->window;
    ws_info_.display_ = GDK_DISPLAY_XDISPLAY(gdk_drawable_get_display(gdk_window));
    ws_info_.visual_ = GDK_VISUAL_XVISUAL(gdk_drawable_get_visual(gdk_window));
    ws_info_.colormap_ = GDK_COLORMAP_XCOLORMAP(gdk_drawable_get_colormap(gdk_window));
    ws_info_.depth_ = gdk_drawable_get_depth(gdk_window);
    UpdateCoordinates();
    return DoUpdateWindow();
#else
    return false;
#endif
  }

  void ResizeDrawable() {
#ifdef GDK_WINDOWING_X11
    if (pixmap_) {
      g_object_unref(pixmap_);
    }
    UpdateCoordinates();
    // Pixmap always has the same top-left origin and size with the element
    // itself. Otherwise, area outside of the pixmap but inside the elment
    // cannot be cleared during view resizing.
    uint32_t pixmap_width = static_cast<uint32_t>(ceil(owner_->GetPixelWidth()));
    uint32_t pixmap_height = static_cast<uint32_t>(ceil(owner_->GetPixelHeight()));
    pixmap_ = gdk_pixmap_new(NULL, pixmap_width, pixmap_height, ws_info_.depth_);
    drawable_ = GDK_PIXMAP_XID(pixmap_);
    // Set the background ourselves. But if opaque mode is used and the plugin
    // occupys the whole area of the element, then we don't need to set as the
    // plugin will do that for us.
    if (plugin_->IsTransparent() ||
        window_.width_ < pixmap_width || window_.height_ < pixmap_height) {
      XGCValues value = { GXset, };
      GC gc = XCreateGC(ws_info_.display_, drawable_, GCFunction, &value);
      XFillRectangle(ws_info_.display_, drawable_, gc,
                     0, 0, pixmap_width, pixmap_height);
    }
    DoUpdateWindow();
#endif
  }

  bool DoUpdateWindow() {
    window_.window_ = NULL;
    window_.type_ = WindowTypeWindowless;
    return plugin_->SetWindow(&window_);
  }

  void Layout() {
    if (!plugin_)
      return;
    if (native_widget_ != GTK_WIDGET(owner_->GetView()->GetNativeWidget())) {
      UpdateWindow();
    } else if (owner_->IsSizeChanged()) {
      ResizeDrawable();
    }
  }

  void DoDraw(CanvasInterface *canvas) {
    if (windowless_ && plugin_) {
      XEvent expose_event;
      expose_event.type = GraphicsExpose;
      expose_event.xgraphicsexpose.display = ws_info_.display_;
      expose_event.xgraphicsexpose.drawable = drawable_;
      plugin_->HandleEvent(expose_event);
      if (canvas) {
        cairo_t *cr = down_cast<CairoCanvas*>(canvas)->GetContext();
        gdk_cairo_set_source_pixmap(cr, pixmap_, 0, 0);
        cairo_paint_with_alpha(cr, owner_->GetOpacity());
      }
    }
  }

  GtkFlashElement *owner_;
  View *view_;
  GtkWidget *native_widget_;
  NPPlugin *plugin_;
  ScriptableInterface *scriptable_plugin_;
  std::string src_;
  bool windowless_;
  Window window_;
  WindowInfoStruct ws_info_;
  GdkPixmap *pixmap_;
  Drawable drawable_;
  BasicElement *flash_element_;
  bool auto_start_;
  bool initialized_;
  bool focused_;
};

GtkFlashElement::GtkFlashElement(View *view, const char *name)
    : BasicElement(view, "flash", name, false),
      impl_(new Impl(this, view)) {
}

GtkFlashElement::~GtkFlashElement() {
  delete impl_;
}

BasicElement *GtkFlashElement::CreateInstance(View *view, const char *name) {
  return new GtkFlashElement(view, name);
}

void GtkFlashElement::DoRegister() {
  BasicElement::DoRegister();
  if (impl_->windowless_) {
    RegisterProperty("src",
                     NewSlot(impl_, &Impl::GetSrc),
                     NewSlot(impl_, &Impl::SetSrc));
    // FIXME: Only when flash plugin supports script API do we
    // export them to javascript.
    if (kPluginSupportScriptableAPI) {
      if (impl_->scriptable_plugin_)
        RegisterConstant("movie", impl_->scriptable_plugin_);
    }
  } else if (impl_->flash_element_) {
    // Trigger property registration.
    impl_->flash_element_->GetProperty("");
  } else {
    LOG("Flash player plugin is not initialized.");
  }
}

void GtkFlashElement::Layout() {
  BasicElement::Layout();
  if (impl_->windowless_) {
    impl_->Layout();
  } else if (impl_->flash_element_) {
    impl_->flash_element_->Layout();
  }
}

void GtkFlashElement::DoDraw(CanvasInterface *canvas) {
  impl_->DoDraw(canvas);
}

EventResult GtkFlashElement::HandleMouseEvent(const MouseEvent &event) {
  if (!impl_->plugin_)
    return EVENT_RESULT_UNHANDLED;
  XEvent x_event;
  Event::Type type = event.GetType();
  x_event.xany.display = impl_->ws_info_.display_;
  if (type == Event::EVENT_MOUSE_OVER || type == Event::EVENT_MOUSE_OUT) {
    x_event.xcrossing.type =
        type == Event::EVENT_MOUSE_OVER ? EnterNotify : LeaveNotify;
    x_event.xcrossing.x = static_cast<int>(event.GetX());
    x_event.xcrossing.y = static_cast<int>(event.GetY());
    x_event.xcrossing.mode = NotifyNormal;
    x_event.xcrossing.detail = NotifyVirtual;
    x_event.xcrossing.focus = impl_->focused_;
  } else if (type == Event::EVENT_MOUSE_MOVE) {
    x_event.xmotion.type = MotionNotify;
    x_event.xmotion.x = static_cast<int>(event.GetX());
    x_event.xmotion.y = static_cast<int>(event.GetY());
    x_event.xmotion.is_hint = NotifyNormal;
  } else {
    GdkEventButton *button =
        reinterpret_cast<GdkEventButton*>(event.GetOriginalEvent());
    if (!button) {
      return EVENT_RESULT_UNHANDLED;
    }
    // These two types events are added by gdk. For a double-click, the order
    // of gdk events will be: GDK_BUTTON_PRESS, GDK_BUTTON_RELEASE,
    // GDK_BUTTON_PRESS, GDK_2BUTTON_PRESS, GDK_BUTTON_RELEASE. But, we should
    // pass the raw X button events to the plugin, i.e. press->release->
    // press->release. So simply discards those button types added by gdk, and
    // let the plugin to deal with double-clicks or triple-clicks itself,
    // returns HANDLED here.
    if (button->type == GDK_2BUTTON_PRESS ||
        button->type == GDK_3BUTTON_PRESS) {
      return EVENT_RESULT_HANDLED;
    }
    // Translate GdkEvent to the original XEvent.
    x_event.xbutton.type =
        button->type == GDK_BUTTON_PRESS ? ButtonPress : ButtonRelease;
    x_event.xbutton.display = impl_->ws_info_.display_;
    x_event.xbutton.time = button->time;
    x_event.xbutton.state = static_cast<unsigned int>(button->state);
    x_event.xbutton.button = button->button;
    // Use the coordinates in MouseEvent structure, but not those in GdkEvent.
    x_event.xbutton.x = static_cast<int>(event.GetX());
    x_event.xbutton.y = static_cast<int>(event.GetY());
  }
  return impl_->plugin_->HandleEvent(x_event);
}

EventResult GtkFlashElement::HandleKeyEvent(const KeyboardEvent &event) {
  if (!impl_->plugin_)
    return EVENT_RESULT_UNHANDLED;
  GdkEventKey *key = reinterpret_cast<GdkEventKey *>(event.GetOriginalEvent());
  if (!key) {
    return EVENT_RESULT_UNHANDLED;
  }
  // Translate GdkEvent to the original XEvent.
  XEvent x_event;
  x_event.xkey.type = key->type == GDK_KEY_PRESS ? KeyPress : KeyRelease;
  x_event.xkey.display = impl_->ws_info_.display_;
  x_event.xkey.time = key->time;
  x_event.xkey.state = static_cast<unsigned int>(key->state);
  x_event.xkey.keycode = key->hardware_keycode;
  return impl_->plugin_->HandleEvent(x_event);
}

EventResult GtkFlashElement::HandleOtherEvent(const Event &event) {
  if (!impl_->plugin_)
    return EVENT_RESULT_UNHANDLED;
  Event::Type type = event.GetType();
  XEvent x_event;
  x_event.xany.display = impl_->ws_info_.display_;
  if (type == Event::EVENT_FOCUS_IN || type == Event::EVENT_FOCUS_OUT) {
    x_event.xfocus.type = type == Event::EVENT_FOCUS_IN ? FocusIn : FocusOut;
    x_event.xfocus.mode = NotifyNormal;
    x_event.xfocus.detail = NotifyDetailNone;
    impl_->focused_ = type == Event::EVENT_FOCUS_IN ? true : false;
  } else {
    return EVENT_RESULT_UNHANDLED;
  }
  return impl_->plugin_->HandleEvent(x_event);
}

} // namespace gtk
} // namespace ggadget
