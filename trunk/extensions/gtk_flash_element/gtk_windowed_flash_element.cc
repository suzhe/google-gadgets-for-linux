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

#include "gtk_windowed_flash_element.h"

#include <unistd.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <cmath>
#include <ggadget/gadget.h>
#include <ggadget/logger.h>
#include <ggadget/main_loop_interface.h>
#include <ggadget/view.h>
#include <ggadget/element_factory.h>
#include <ggadget/npapi/npapi_container.h>
#include <ggadget/npapi/npapi_plugin.h>

#include "gtk_flash_element.h"

namespace ggadget {
namespace gtk {

using namespace ggadget::npapi;

// FIXME: currently, the flash plugin only enables flash-javascript
// interaction for IE, but not Firefox, opera.
// Related discussion: http://groups.google.com/group/comp.lang.javascript/
// browse_frm/thread/aa0fff388526cbf4/41b4c06914d7274a
static const bool kPluginSupportScriptableAPI = false;

class GtkWindowedFlashElement::Impl {
 public:
  Impl(GtkWindowedFlashElement *owner)
      : owner_(owner),
        container_(NULL),
        socket_(NULL), socket_realized_(false),
        x_(0), y_(0), width_(0), height_(0),
        minimized_(false), popped_out_(false),
        scriptable_plugin_(NULL),
        initialized_(false) {
    owner_->GetView()->ConnectOnMinimizeEvent(
        NewSlot(this, &Impl::OnViewMinimized));
    owner_->GetView()->ConnectOnRestoreEvent(
        NewSlot(this, &Impl::OnViewRestored));
    owner_->GetView()->ConnectOnPopOutEvent(
        NewSlot(this, &Impl::OnViewPoppedOut));
    owner_->GetView()->ConnectOnPopInEvent(
        NewSlot(this, &Impl::OnViewPoppedIn));
    plugin_ = GetGlobalNPContainer()->CreatePlugin(FLASH_MIME_TYPE, owner_,
                                                   true, GTK2, 0, NULL, NULL);
    if (plugin_) {
      initialized_ = true;
      // Get the root scriptable object of the plugin.
      scriptable_plugin_ = plugin_->GetScriptablePlugin();
      // Create socket window. Although we don't know the coordinates of the
      // element yet, we can update them once we get it. We create it here
      // because the socket needs some time to be realized (asynchronously).
      // If we do this after the element is contructed (i.e. all the xml
      // attributes has been set, and likely the whole view has been setup),
      // gadget may fail to play flash if they want to play on view-open,
      // because the plugin window and socket window must be initialized before
      // it can play anything.
      CreateSocket();
      // Initialize the window info structure, such as display, visual, etc..
      InitWindowInfoStruct();
      window_.ws_info_ = &ws_info_;
    }
  }

  ~Impl() {
    if (plugin_)
      GetGlobalNPContainer()->DestroyPlugin(plugin_);
    if (socket_ && GTK_IS_WIDGET(socket_))
      gtk_widget_destroy(socket_);
  }

  static void OnSocketRealize(GtkWidget *widget, gpointer user_data) {
    Impl *impl = static_cast<Impl *>(user_data);
    impl->socket_realized_ = true;
  }

  static gboolean OnPlugRemoved(GtkSocket *socket, gpointer user_data) {
    // The default handler will destroy the socket together with the plug
    // widget. Since we want to reuse the socket, returns true to stop
    // the default handler and other handlers from being invoked.
    return true;
  }

  void CreateSocket() {
    if (socket_)
      return;
    socket_ = gtk_socket_new();
    g_signal_connect_after(socket_, "realize",
                           G_CALLBACK(OnSocketRealize), this);
    g_signal_connect(socket_, "plug-removed", G_CALLBACK(OnPlugRemoved), NULL);
    container_ = GTK_WIDGET(owner_->GetView()->GetNativeWidget());
    if (!GTK_IS_FIXED(container_)) {
      LOG("GtkWindowedFlashElement needs a GTK_FIXED parent. Actual type: %s",
          G_OBJECT_TYPE_NAME(container_));
      gtk_widget_destroy(socket_);
      socket_ = NULL;
      return;
    }
    gtk_fixed_put(GTK_FIXED(container_), socket_, x_, y_);
    gtk_widget_set_size_request(socket_, width_, height_);
    gtk_widget_realize(socket_);
    gtk_widget_show(socket_);
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

  const char *GetSrc() {
    return src_.c_str();
  }

  void SetSrc(const char *src) {
    if (!src || src_.compare(src) == 0)
      return;
    src_ = src;
    // FIXME: We cannot use any script control for flash playing. We only
    // provide one basic operation, i.e. play another flash, and what we can
    // do is to create a new flash plugin instance, destroy the old one.
    // Although this is a little ugly, it works.
    if (!kPluginSupportScriptableAPI) {
      NPPlugin *plugin =
          GetGlobalNPContainer()->CreatePlugin(FLASH_MIME_TYPE, owner_,
                                               true, GTK2, 0, NULL, NULL);
      if (plugin) {
        ASSERT(plugin->GetWindowType() == WindowTypeWindowed);
      }
      if (plugin_)
        GetGlobalNPContainer()->DestroyPlugin(plugin_);
      plugin_ = plugin;
    }
    if (!socket_realized_) {
      LOG("Socket window is not realized yet.");
      return;
    }
    // Our coordinates are relative, do layout before we can get pixel values.
    owner_->Layout();
    if (UpdateWindow(false))
      plugin_->SetURL(src);
    else
      LOGE("Failed to initialize plugin's window");
  }

  void Layout() {
    GtkWidget *container = GTK_WIDGET(owner_->GetView()->GetNativeWidget());
    if (GTK_IS_FIXED(container) && GTK_IS_SOCKET(socket_)) {
      bool force_layout = false;
      // Check if the contain has changed, for example the gadget is
      // docked/undocked.
      if (gtk_widget_get_parent(socket_) != container) {
        gtk_widget_reparent(socket_, container);
        force_layout = true;
      }
      UpdateWindow(force_layout);
      if (owner_->IsReallyVisible() && (!minimized_ || popped_out_))
        gtk_widget_show(socket_);
      else
        gtk_widget_hide(socket_);
    }
  }

  bool UpdateWindow(bool force_layout) {
    gint x, y, width, height;
    GetWidgetExtents(&x, &y, &width, &height);
    if (x != x_ || y != y_ || force_layout) {
      x_ = x;
      y_ = y;
      GtkWidget *container = GTK_WIDGET(owner_->GetView()->GetNativeWidget());
      gtk_fixed_move(GTK_FIXED(container), socket_, x, y);
    }
    if (width != width_ || height != height_ || force_layout) {
      width_ = width;
      height_ = height;
      gtk_widget_set_size_request(socket_, width_, height_);

      window_.window_ =
          reinterpret_cast<void *>(gtk_socket_get_id(GTK_SOCKET(socket_)));
      window_.x_ = 0;
      window_.y_ = 0;
      window_.width_ = width_;
      window_.height_ = height_;
      window_.cliprect_.left_ = 0;
      window_.cliprect_.top_ = 0;
      window_.cliprect_.right_ = width_;
      window_.cliprect_.bottom_ = height_;
      window_.type_ = WindowTypeWindowed;
      return plugin_->SetWindow(&window_);
    }
    return true;
  }

  void InitWindowInfoStruct() {
    // A GtkSocket has its own window;
    GdkWindow *gdk_window = socket_->window;
    ws_info_.display_ = GDK_DISPLAY_XDISPLAY(gdk_drawable_get_display(gdk_window));
    ws_info_.visual_ = GDK_VISUAL_XVISUAL(gdk_drawable_get_visual(gdk_window));
    ws_info_.colormap_ = GDK_COLORMAP_XCOLORMAP(gdk_drawable_get_colormap(gdk_window));
    ws_info_.depth_ = gdk_drawable_get_depth(gdk_window);
  }

  void OnViewMinimized() {
    // The widget must be hidden when the view is minimized.
    if (GTK_IS_SOCKET(socket_) && !popped_out_) {
      gtk_widget_hide(socket_);
    }
    minimized_ = true;
  }

  void OnViewRestored() {
    if (GTK_IS_SOCKET(socket_) && owner_->IsReallyVisible() && !popped_out_) {
      gtk_widget_show(socket_);
    }
    minimized_ = false;
  }

  void OnViewPoppedOut() {
    popped_out_ = true;
    // Layout will be automatically called after the view host
    // actually switched.
  }

  void OnViewPoppedIn() {
    popped_out_ = false;
  }

  GtkWindowedFlashElement *owner_;
  GtkWidget *container_;
  GtkWidget *socket_;
  bool socket_realized_;
  std::string src_;
  gint x_, y_, width_, height_;
  bool minimized_;
  bool popped_out_;

  NPPlugin *plugin_;
  ScriptableInterface *scriptable_plugin_;
  Window window_;
  WindowInfoStruct ws_info_;

  bool initialized_;
};

GtkWindowedFlashElement::GtkWindowedFlashElement(GtkFlashElement *parent,
                                                 View *view, const char *name)
    : BasicElement(view, "flash", name, false),
      impl_(new Impl(this)) {
  SetParentElement(parent);
  SetRelativeX(0);
  SetRelativeY(0);
  SetRelativeWidth(1.0);
  SetRelativeHeight(1.0);
}

GtkWindowedFlashElement::~GtkWindowedFlashElement() {
  delete impl_;
  impl_ = NULL;
}

void GtkWindowedFlashElement::DoRegister() {
  BasicElement::DoRegister();
  BasicElement *parent = GetParentElement();
  parent->RegisterProperty("src",
                           NewSlot(impl_, &Impl::GetSrc),
                           NewSlot(impl_, &Impl::SetSrc));
  // FIXME: Only when flash plugin supports script API do we
  // export them to javascript.
  if (kPluginSupportScriptableAPI) {
    if (impl_->scriptable_plugin_)
      parent->RegisterConstant("movie", impl_->scriptable_plugin_);
  }
}

void GtkWindowedFlashElement::Layout() {
  BasicElement::Layout();
  impl_->Layout();
}

void GtkWindowedFlashElement::DoDraw(CanvasInterface *canvas) {
}

} // namespace gtk
} // namespace ggadget
