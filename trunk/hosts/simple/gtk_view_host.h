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

#ifndef HOSTS_SIMPLE_GTK_VIEW_HOST_H__
#define HOSTS_SIMPLE_GTK_VIEW_HOST_H__

#include <set>

#include <gtk/gtk.h>
#include <ggadget/ggadget.h>

class GadgetViewWidget;

/**
 * An implementation of @c ViewHostInterface for the simple gadget host.
 * In this implementation, there is one instance of @c GtkViewHost per view,
 * and one instance of GraphicsInterface per @c GtkViewHost.
 */
class GtkViewHost : public ggadget::ViewHostInterface {
 public:
  GtkViewHost(ggadget::GadgetHostInterface *gadget_host,
              ggadget::GadgetHostInterface::ViewType type,
              ggadget::ScriptableInterface *prototype,
              bool composited);
  virtual ~GtkViewHost();

#if 0
  // TODO: This host should encapsulate all widget creating/switching jobs
  // inside of it, so the interface of this method should be revised.
  /**
   * Switches the GadgetViewWidget associated with this host.
   * When this is done, the original GadgetViewWidget will no longer have a
   * valid GtkCairoHost object. The new GadgetViewWidget is responsible for
   * freeing this host.
   */
  void SwitchWidget(GadgetViewWidget *new_gvw);
#endif

  virtual ggadget::GadgetHostInterface *GetGadgetHost() const {
    return gadget_host_;
  }
  virtual ggadget::ViewInterface *GetView() { return view_; }
  virtual const ggadget::ViewInterface *GetView() const { return view_; }
  virtual ggadget::ScriptContextInterface *GetScriptContext() const {
    return script_context_;
  }
  virtual ggadget::XMLHttpRequestInterface *NewXMLHttpRequest();
  virtual const ggadget::GraphicsInterface *GetGraphics() const { return gfx_; }

  virtual void QueueDraw();
  virtual bool GrabKeyboardFocus();

  virtual void SetResizeable();
  virtual void SetCaption(const char *caption);
  virtual void SetShowCaptionAlways(bool always);
  virtual void SetCursor(ggadget::ElementInterface::CursorType type);
  virtual void RunDialog();

  GadgetViewWidget *GetWidget() { ASSERT(gvw_); return gvw_; }

 private:
  ggadget::GadgetHostInterface *gadget_host_;
  ggadget::ViewInterface *view_;
  ggadget::ScriptContextInterface *script_context_;
  GadgetViewWidget *gvw_;
  ggadget::GraphicsInterface *gfx_;
  ggadget::Connection *onoptionchanged_connection_;

  DISALLOW_EVIL_CONSTRUCTORS(GtkViewHost);
};

#endif // HOSTS_SIMPLE_GTK_VIEW_HOST_H__
