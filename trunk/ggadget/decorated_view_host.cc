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

#include <vector>

#include "decorated_view_host.h"

#include "button_element.h"
#include "div_element.h"
#include "event.h"
#include "file_manager_factory.h"
#include "gadget.h"
#include "gadget_consts.h"
#include "image_interface.h"
#include "img_element.h"
#include "elements.h"
#include "graphics_interface.h"
#include "main_loop_interface.h"
#include "messages.h"
#include "scriptable_binary_data.h"
#include "signals.h"
#include "view.h"
#include "view_element.h"

namespace ggadget {

static const double kMainButtonWidth = 19.;
static const double kMainButtonHeight = 17.;
static const double kMainButtonBackgroundHeight = 19.;

// These arrays are indexed by the view type: Main, Options, Details
static const double kInnerViewTopOffset[] = {2., 0., 0.};
static const double kInnerViewBottomOffset[] = {2., 0., 0.};
static const double kInnerViewLeftOffset[] = {2., 0., 0.};
static const double kInnerViewRightOffset[] = {2., 0., 0.};

#define INNERVIEW_WIDTH_OFFSET(t) (kInnerViewLeftOffset[(t)] + kInnerViewRightOffset[(t)])
#define INNERVIEW_HEIGHT_OFFSET(t) (kInnerViewTopOffset[(t)] + kInnerViewBottomOffset[(t)])

/** 
 * The view/viewhosts below are very tricky:
 * The inner view is associated with the inner view host (this class).
 * The outer view encapsulated inside the inner view host is associated with 
 * the outer view host. 
 */
class DecoratedViewHost::Impl {
 public:
  enum BorderType {
    BORDER_TOP = 0,
    BORDER_RIGHT,
    BORDER_BOTTOM,
    BORDER_LEFT,
    BORDER_COUNT
  };

  /** 
   * This view is the decorated outer view that wraps around the actual view
   * of the gadget.
   */
  class DecoratedView : public View {
   public:
    DecoratedView(ViewHostInterface *host,
                  Gadget *gadget,
                  ElementFactory *element_factory,
                  ScriptContextInterface *script_context, 
                  DecoratedViewHost::Impl *owner)
      : View(host, gadget, element_factory, script_context),
        owner_(owner), 
        mouseover_(false),
        collapsed_(false), 
        expanded_(false),
        view_element_(NULL), 
        button_array_div_(NULL),
        background_(NULL) {
      ViewHostInterface::Type t = host->GetType();

      // The order in which the elements are generated and added to view 
      // is important.

      if (owner_->background_) {
        background_ = new DivElement(NULL, this, NULL);
        GetChildren()->InsertElementAtIndex(background_, -1);
        background_->SetBackgroundMode(DivElement::BACKGROUND_MODE_STRETCH_MIDDLE);
        background_->SetBackground(LoadGlobalImageAsVariant(kVDMainBackground));
      }

      view_element_ = new ViewElement(NULL, this, 
                                      owner_->inner_view_host_, 
                                      owner_->inner_view_);      
      GetChildren()->InsertElementAtIndex(view_element_, -1);      
      view_element_->SetPixelX(kInnerViewLeftOffset[t]);
      view_element_->SetPixelY(kInnerViewTopOffset[t]);

      Connection *c = 
        owner_->inner_view_->ConnectOnSizeEvent(NewSlot(this, &DecoratedView::Layout));
      connections_.push_back(c);      

      GenerateBorders();
      GenerateButtonArray();

      // View by default is set to RESIZABLE_ZOOM. Outer view should never be 
      // this value.
      View::SetResizable(RESIZABLE_TRUE);
    }

    virtual ~DecoratedView() {
      DisconnectSlots();
    }

    // Overridden methods.

    virtual bool OnAddContextMenuItems(MenuInterface *menu) {
      bool r = owner_->inner_view_->OnAddContextMenuItems(menu);
      return r;
    }

    virtual EventResult OnMouseEvent(const MouseEvent &event) {
      Event::Type t = event.GetType();
      if (t == Event::EVENT_MOUSE_OVER || t == Event::EVENT_MOUSE_OUT) {
        mouseover_ = (t == Event::EVENT_MOUSE_OVER);
        button_array_div_->SetVisible(mouseover_);
        SetBorderVisibility(owner_->inner_view_->GetResizable());
      }

      return View::OnMouseEvent(event);
    }

    virtual EventResult OnOtherEvent(const Event &event) {
      Event::Type t = event.GetType();
      EventResult r = View::OnOtherEvent(event);
      switch (t) {
       case Event::EVENT_CANCEL:
       case Event::EVENT_CLOSE:
       case Event::EVENT_DOCK:
       case Event::EVENT_MINIMIZE:
       case Event::EVENT_OK:
       case Event::EVENT_OPEN:
       case Event::EVENT_POPIN:
       case Event::EVENT_POPOUT:
       case Event::EVENT_RESTORE:
       case Event::EVENT_UNDOCK:
        r = owner_->inner_view_->OnOtherEvent(event);
        break;     
       case Event::EVENT_SIZING:
        ASSERT(false);
       default:
        // OnOptionChanged is sent directly by gadget to view.
        // OnSize is generated by view directly when it's resized.
        break; // Do nothing.
      }
      return r;
    }

    virtual bool OnSizing(double *width, double *height) {
      bool result = View::OnSizing(width, height);

      if (result) {
        ViewHostInterface::Type t = owner_->outer_view_host_->GetType();
        double zoom = owner_->inner_view_gfx_->GetZoom();

        *width -= INNERVIEW_WIDTH_OFFSET(t);
        *height -= INNERVIEW_HEIGHT_OFFSET(t);
        *width /= zoom;
        *height /= zoom;

        result = owner_->inner_view_->OnSizing(width, height);

        *width *= zoom;
        *height *= zoom;
        *width += INNERVIEW_WIDTH_OFFSET(t);
        *height += INNERVIEW_HEIGHT_OFFSET(t);
      }

      return result;
    }

    virtual void SetResizable(ResizableMode resizable) {
      SetBorderVisibility(resizable);

      if (resizable == RESIZABLE_ZOOM) {
        // All zooming is done inside the outer view using the inner view's
        // private GraphicsInterface. So in order to support zooming on the 
        // inner view, the outer view must be resizable and set the inner view's
        // zoom factor on resize. 
        resizable = RESIZABLE_TRUE;
      }
      View::SetResizable(resizable);
    }

    virtual void SetWidth(double width) {
      ASSERT(owner_->inner_view_);
      ViewHostInterface::Type t = owner_->outer_view_host_->GetType();

      double w = width - INNERVIEW_WIDTH_OFFSET(t);    
      if (owner_->inner_view_->GetResizable() 
          == ViewInterface::RESIZABLE_ZOOM) {
        double h = GetHeight() - INNERVIEW_HEIGHT_OFFSET(t);
        SetZoom(w, h);
      } else {
        // Skip outer view SetWidth. Inner view will invoke this indiretly
        // through the outer view's Layout() method.
        owner_->inner_view_->SetWidth(w);
      }
    }

    virtual void SetHeight(double height) {
      ASSERT(owner_->inner_view_);
      ViewHostInterface::Type t = owner_->outer_view_host_->GetType();

      double h = height - INNERVIEW_HEIGHT_OFFSET(t);
      if (owner_->inner_view_->GetResizable() 
          == ViewInterface::RESIZABLE_ZOOM) {
        double w = GetWidth() - INNERVIEW_WIDTH_OFFSET(t);    
        SetZoom(w, h);
      } else {
        // Skip outer view SetHeight. Inner view will invoke this indiretly
        // through the outer view's Layout() method.
        owner_->inner_view_->SetHeight(h);        
      }
    }

    virtual void SetSize(double width, double height) {
      ASSERT(owner_->inner_view_);
      ViewHostInterface::Type t = owner_->outer_view_host_->GetType();

      double w = width - INNERVIEW_WIDTH_OFFSET(t);
      double h = height - INNERVIEW_HEIGHT_OFFSET(t);
      if (owner_->inner_view_->GetResizable() 
          == ViewInterface::RESIZABLE_ZOOM) {
        SetZoom(w, h);        
      } else {
        // Skip outer view SetSize. Inner view will invoke this indiretly
        // through the outer view's Layout() method.
        owner_->inner_view_->SetSize(w, h);        
      }
    }

    // Non-virtual methods
    void SetZoom(double container_width, double container_height) {
      double width = owner_->inner_view_->GetWidth();
      double height = owner_->inner_view_->GetHeight();
      if (width && height) {
        double xzoom = container_width / width;
        double yzoom = container_height / height;
        double zoom = std::min(xzoom, yzoom);
        if (zoom != owner_->inner_view_gfx_->GetZoom()) {
          DLOG("Set inner view scale to: %lf", zoom);
          owner_->inner_view_gfx_->SetZoom(zoom * 
              owner_->outer_view_host_->GetGraphics()->GetZoom());
          view_element_->SetScale(zoom); // This will queue draw.
          Layout();
        }
      }
    }

    void Layout() { 
      ASSERT(owner_->inner_view_);
      ViewHostInterface::Type t = owner_->outer_view_host_->GetType();

      double iv_w, iv_h;
      view_element_->GetDefaultSize(&iv_w, &iv_h);

      double w = iv_w + INNERVIEW_WIDTH_OFFSET(t);
      double h = iv_h + INNERVIEW_HEIGHT_OFFSET(t);

      DLOG("outer view set size %f %f", w, h);
      View::SetSize(w, h);

      if (background_) {
        background_->SetPixelWidth(w);
        background_->SetPixelHeight(h);
      }      

      LayoutBorders();
      LayoutButtonArray();
    }

    Variant LoadGlobalImageAsVariant(const char *img) {
      std::string data;
      if (GetGlobalFileManager()->ReadFile(img, &data)) {
        ScriptableBinaryData *binary = 
          new ScriptableBinaryData(data.c_str(), data.size());
        if (binary) {
          return Variant(binary);
        }
      } 
      return Variant();
    }

    void GenerateButtonArray() {
      ViewHostInterface::Type t = owner_->outer_view_host_->GetType();

      ButtonElement *button = NULL;
      Connection *c = NULL;
      button_array_div_ = new DivElement(NULL, this, NULL);
      GetChildren()->InsertElementAtIndex(button_array_div_, -1);
      // X, Width is set on Layout instead.
      button_array_div_->SetPixelY(kInnerViewTopOffset[t]);
      button_array_div_->SetPixelHeight(kMainButtonBackgroundHeight);
      button_array_div_->SetVisible(mouseover_);
      button_array_div_->SetBackgroundMode(DivElement::BACKGROUND_MODE_STRETCH_MIDDLE);
      button_array_div_->SetBackground(LoadGlobalImageAsVariant(kVDButtonBackground));

      // Create buttons in left to right order.
      if (t == ViewHostInterface::VIEW_HOST_MAIN) {
        button = new ButtonElement(button_array_div_, this, NULL);
        button_array_.push_back(button);
	button->SetTooltip(GM_("VD_BACK_BUTTON_TOOLTIP"));
        button->SetImage(LoadGlobalImageAsVariant(kVDButtonBackNormal));
        button->SetOverImage(LoadGlobalImageAsVariant(kVDButtonBackOver));
        button->SetDownImage(LoadGlobalImageAsVariant(kVDButtonBackDown));
        c = button->ConnectOnClickEvent(NewSlot(this, &DecoratedView::BackButtonClicked));
        connections_.push_back(c);

        button = new ButtonElement(button_array_div_, this, NULL);
        button_array_.push_back(button);
	button->SetTooltip(GM_("VD_FORWARD_BUTTON_TOOLTIP"));
        button->SetImage(LoadGlobalImageAsVariant(kVDButtonForwardNormal));
        button->SetOverImage(LoadGlobalImageAsVariant(kVDButtonForwardOver));
        button->SetDownImage(LoadGlobalImageAsVariant(kVDButtonForwardDown));
        c = button->ConnectOnClickEvent(NewSlot(this, &DecoratedView::ForwardButtonClicked));
        connections_.push_back(c);

        button = new ButtonElement(button_array_div_, this, NULL);
        button_array_.push_back(button);
	button->SetTooltip(GM_("VD_TOGGLE_EXPANDED_BUTTON_TOOLTIP"));
        SetToggleExpandedButtons(); // Call to set button images.
        c = button->ConnectOnClickEvent(NewSlot(this, &DecoratedView::ToggleExpandedButtonClicked));
        connections_.push_back(c);

        button = new ButtonElement(button_array_div_, this, NULL);
        button_array_.push_back(button);
	button->SetTooltip(GM_("VD_MENU_BUTTON_TOOLTIP"));
        button->SetImage(LoadGlobalImageAsVariant(kVDButtonMenuNormal));
        button->SetOverImage(LoadGlobalImageAsVariant(kVDButtonMenuOver));
        button->SetDownImage(LoadGlobalImageAsVariant(kVDButtonMenuDown));
        c = button->ConnectOnClickEvent(NewSlot(this, &DecoratedView::MenuButtonClicked));
        connections_.push_back(c);

        button = new ButtonElement(button_array_div_, this, NULL);
        button_array_.push_back(button);
	button->SetTooltip(GM_("VD_CLOSE_BUTTON_TOOLTIP"));
        button->SetImage(LoadGlobalImageAsVariant(kVDButtonCloseNormal));
        button->SetOverImage(LoadGlobalImageAsVariant(kVDButtonCloseOver));
        button->SetDownImage(LoadGlobalImageAsVariant(kVDButtonCloseDown));      
        c = button->ConnectOnClickEvent(NewSlot(this, &DecoratedView::CloseButtonClicked));
        connections_.push_back(c);
      }

      Elements *elements = button_array_div_->GetChildren();
      for (std::vector<ButtonElement *>::iterator i = button_array_.begin();
           i != button_array_.end(); ++i) {
        elements->InsertElementAtIndex(*i, -1);
      }              
    }

    void LayoutButtonArray() {
      ASSERT(button_array_div_);
      ViewHostInterface::Type t = owner_->outer_view_host_->GetType();

      if (t == ViewHostInterface::VIEW_HOST_MAIN) {
        // Index 0, 1 are back and forward buttons, respectively.
        int plugin_flags = owner_->inner_view_->GetGadget()->GetPluginFlags();
        button_array_[0]->SetVisible(plugin_flags & Gadget::PLUGIN_FLAG_TOOLBAR_BACK);
        button_array_[1]->SetVisible(plugin_flags & Gadget::PLUGIN_FLAG_TOOLBAR_FORWARD);
      }

      double x = 0.;
      for (std::vector<ButtonElement *>::iterator i = button_array_.begin();
           i != button_array_.end(); ++i) {
        if ((*i)->IsVisible()) {
          (*i)->SetPixelX(x);
          x += kMainButtonWidth;
        }
      }      
      button_array_div_->SetPixelWidth(x);
      button_array_div_->SetPixelX(GetWidth() - x - kInnerViewRightOffset[t]);
    }

    void GenerateBorders() {
      ASSERT(owner_->inner_view_);
      ViewHostInterface::Type t = owner_->outer_view_host_->GetType();      
      ImgElement *img = NULL;      
      Elements *elements = GetChildren();
      for (int i = 0; i < BORDER_COUNT; ++i) {
        img = new ImgElement(NULL, this, NULL);
        border_array_[i] = img;
        elements->InsertElementAtIndex(img, -1);
      }     

      Variant border_h = LoadGlobalImageAsVariant(kVDBorderH);
      Variant border_v = LoadGlobalImageAsVariant(kVDBorderV);
      border_array_[BORDER_TOP]->SetSrc(border_h);
      border_array_[BORDER_BOTTOM]->SetSrc(border_h);
      border_array_[BORDER_RIGHT]->SetSrc(border_v);
      border_array_[BORDER_LEFT]->SetSrc(border_v);

      border_array_[BORDER_TOP]->SetPixelHeight(kInnerViewTopOffset[t]);
      border_array_[BORDER_BOTTOM]->SetPixelHeight(kInnerViewBottomOffset[t]);
      border_array_[BORDER_RIGHT]->SetPixelWidth(kInnerViewRightOffset[t]);
      border_array_[BORDER_LEFT]->SetPixelWidth(kInnerViewLeftOffset[t]);

      border_array_[BORDER_BOTTOM]->SetFlip(ImgElement::FLIP_HORIZONTAL);
      border_array_[BORDER_RIGHT]->SetFlip(ImgElement::FLIP_VERTICAL);

      border_array_[BORDER_TOP]->SetPixelX(kInnerViewLeftOffset[t]);
      border_array_[BORDER_BOTTOM]->SetPixelX(kInnerViewLeftOffset[t]);
      border_array_[BORDER_RIGHT]->SetPixelY(kInnerViewTopOffset[t]);
      border_array_[BORDER_LEFT]->SetPixelY(kInnerViewTopOffset[t]);

      border_array_[BORDER_TOP]->SetHitTest(HT_TOP);
      border_array_[BORDER_RIGHT]->SetHitTest(HT_RIGHT);
      border_array_[BORDER_BOTTOM]->SetHitTest(HT_BOTTOM);
      border_array_[BORDER_LEFT]->SetHitTest(HT_LEFT);

      SetBorderVisibility(owner_->inner_view_->GetResizable());
    }

    void SetBorderVisibility(ResizableMode resizable) {
      bool show_border = (resizable != RESIZABLE_FALSE) && mouseover_;
      for (int i = 0; i < BORDER_COUNT; ++i) {
        border_array_[i]->SetVisible(show_border);
      }  
    }

    void LayoutBorders() {
      ViewHostInterface::Type t = owner_->outer_view_host_->GetType();
      double w = GetWidth();
      double h = GetHeight();

      border_array_[BORDER_RIGHT]->SetPixelX(w - kInnerViewRightOffset[t]);
      border_array_[BORDER_BOTTOM]->SetPixelY(h - kInnerViewBottomOffset[t]);

      border_array_[BORDER_TOP]->SetPixelWidth(w - INNERVIEW_WIDTH_OFFSET(t));
      border_array_[BORDER_BOTTOM]->SetPixelWidth(w - INNERVIEW_WIDTH_OFFSET(t));
      border_array_[BORDER_RIGHT]->SetPixelHeight(h - INNERVIEW_HEIGHT_OFFSET(t));
      border_array_[BORDER_LEFT]->SetPixelHeight(h - INNERVIEW_HEIGHT_OFFSET(t));      
    }

    class RemoveGadgetCallback :  public WatchCallbackInterface {
     public:
      RemoveGadgetCallback(Gadget *gadget) : gadget_(gadget) {}

      virtual bool Call(MainLoopInterface *main_loop, int watch_id) {
        gadget_->RemoveMe(true);
        return false;
      }

      virtual void OnRemove(MainLoopInterface *main_loop, int watch_id) {
        delete this;
      }      

      Gadget *gadget_;
    };

    void CloseButtonClicked() {
      // Cannot remove the gadget inside the event handler.
      RemoveGadgetCallback *watch = 
        new RemoveGadgetCallback(owner_->inner_view_->GetGadget());

      GetGlobalMainLoop()->AddTimeoutWatch(0, watch);
    }

    void MenuButtonClicked() {
      owner_->outer_view_host_->ShowContextMenu(MouseEvent::BUTTON_LEFT);
    }  

    void ForwardButtonClicked() {
      owner_->inner_view_->GetGadget()->OnCommand(Gadget::CMD_TOOLBAR_FORWARD);
    }

    void BackButtonClicked() {
      owner_->inner_view_->GetGadget()->OnCommand(Gadget::CMD_TOOLBAR_BACK);
    }

    void SetToggleExpandedButtons() {
      // Index 2 is the "toggle expanded" button.
      ButtonElement *button = button_array_[2];
      if (expanded_) {
        button->SetImage(LoadGlobalImageAsVariant(kVDButtonUnexpandNormal));
        button->SetOverImage(LoadGlobalImageAsVariant(kVDButtonUnexpandOver));
        button->SetDownImage(LoadGlobalImageAsVariant(kVDButtonUnexpandDown));
      } else {
        button->SetImage(LoadGlobalImageAsVariant(kVDButtonExpandNormal));
        button->SetOverImage(LoadGlobalImageAsVariant(kVDButtonExpandOver));
        button->SetDownImage(LoadGlobalImageAsVariant(kVDButtonExpandDown));
      }
    }

    void ToggleExpandedButtonClicked() {
      // TODO

      expanded_ = !expanded_;
      SetToggleExpandedButtons();
    }

    void DisconnectSlots() {
      // Clear all connections in View on destruction.
      for (std::vector<Connection *>::iterator iter = connections_.begin();
           connections_.end() != iter; iter++) {
        (*iter)->Disconnect();
      }
      connections_.clear();
    }

    DecoratedViewHost::Impl* owner_;
    bool mouseover_;
    // This is confusing here: collapsed_ refers to the "collapse" menu item.
    // expanded_ refers to the "toggle expanded" toolbar button.
    bool collapsed_;
    bool expanded_; 
    std::vector<Connection *> connections_;

    // Once added to outer_view_ (this view), these are owned by outer_view_. 
    // Do not delete.
    ViewElement *view_element_;
    DivElement *button_array_div_;
    DivElement *background_;
    std::vector<ButtonElement *> button_array_;
    ImgElement *border_array_[BORDER_COUNT];
  };

  Impl(DecoratedViewHost *inner_view_host, ViewHostInterface *outer_view_host,
       bool background) 
       : background_(background), inner_view_host_(inner_view_host), 
       inner_view_(NULL), 
       outer_view_host_(outer_view_host), 
       outer_view_(NULL), 
       inner_view_gfx_(NULL) {
  }

  ~Impl() {    
    delete outer_view_;
    outer_view_ = NULL;    

    if (outer_view_host_) {
      outer_view_host_->Destroy();
      outer_view_host_ = NULL;
    }

    delete inner_view_gfx_;
    inner_view_gfx_ = NULL;
  }

  void SetView(ViewInterface *view) {
    inner_view_ = down_cast<View *>(view);
    if (outer_view_) {    
      outer_view_->DisconnectSlots();
    } else if (inner_view_) {    
      outer_view_ = new DecoratedView(outer_view_host_, 
                                      inner_view_->GetGadget(), 
                                      inner_view_->GetElementFactory(), 
                                      inner_view_->GetScriptContext(),
                                      this);
      ASSERT(outer_view_);
      outer_view_host_->SetView(outer_view_);
    }

    // View element will queue the redraw.
    outer_view_->view_element_->SetChildView(inner_view_host_, inner_view_);
    if (inner_view_) {     
      delete inner_view_gfx_;
      inner_view_gfx_ = outer_view_host_->GetGraphics()->Clone();

      outer_view_->Layout();
    }
  }

  bool background_;

  // Not owned by DecoratedViewHost.
  DecoratedViewHost *inner_view_host_; // This is the owner.
  View *inner_view_;  

  // Owned by DecoratedViewHost.
  ViewHostInterface *outer_view_host_;
  DecoratedView *outer_view_;
  GraphicsInterface *inner_view_gfx_;
};


DecoratedViewHost::DecoratedViewHost(ViewHostInterface *outer_view_host, 
                                     bool background)
  : impl_(new Impl(this, outer_view_host, background)) {
}

DecoratedViewHost::~DecoratedViewHost() {
  delete impl_;
  impl_ = NULL;
}

ViewHostInterface::Type DecoratedViewHost::GetType() const {
  return impl_->outer_view_host_->GetType();
}

void DecoratedViewHost::Destroy() {
  delete this;
}

void DecoratedViewHost::SetView(ViewInterface *view) {
  impl_->SetView(view);
}

ViewInterface *DecoratedViewHost::GetView() const {
  return impl_->inner_view_;
}

const GraphicsInterface *DecoratedViewHost::GetGraphics() const {
  return impl_->inner_view_gfx_;
}

void *DecoratedViewHost::GetNativeWidget() const {
  return impl_->outer_view_host_->GetNativeWidget();
}

void DecoratedViewHost::ViewCoordToNativeWidgetCoord(
    double x, double y, double *widget_x, double *widget_y) const {
  impl_->outer_view_host_->ViewCoordToNativeWidgetCoord(x, y, 
                                                        widget_x, widget_y);
  if (impl_->inner_view_) {
    ViewHostInterface::Type t = impl_->outer_view_host_->GetType();
    *widget_x += kInnerViewLeftOffset[t];
    *widget_y += kInnerViewTopOffset[t];  
  }
}

void DecoratedViewHost::QueueDraw() {
  impl_->outer_view_->view_element_->QueueDraw();
  impl_->outer_view_host_->QueueDraw();
}

void DecoratedViewHost::QueueResize() {
  impl_->outer_view_->Layout();
  impl_->outer_view_host_->QueueResize();
}

void DecoratedViewHost::SetResizable(ViewInterface::ResizableMode mode) {
  // Delegate outer view's SetResizable.
  // View::SetResizable will call outer_view_host->SetResizable
  impl_->outer_view_->SetResizable(mode);  
}

void DecoratedViewHost::SetCaption(const char *caption) {
  impl_->outer_view_host_->SetCaption(caption);

  // TODO
}

void DecoratedViewHost::SetShowCaptionAlways(bool always) {
  impl_->outer_view_host_->SetShowCaptionAlways(always);
}

void DecoratedViewHost::SetCursor(int type) {
  impl_->outer_view_host_->SetCursor(type);
}

void DecoratedViewHost::SetTooltip(const char *tooltip) {
  impl_->outer_view_host_->SetTooltip(tooltip);
}

bool DecoratedViewHost::ShowView(bool modal, int flags,
                              Slot1<void, int> *feedback_handler) {
  return impl_->outer_view_host_->ShowView(modal, flags, feedback_handler);
}

void DecoratedViewHost::CloseView() {
  impl_->outer_view_host_->CloseView();
}

bool DecoratedViewHost::ShowContextMenu(int button) {
  return impl_->outer_view_host_->ShowContextMenu(button);
}

void DecoratedViewHost::Alert(const char *message) {
  impl_->outer_view_host_->Alert(message);
}

bool DecoratedViewHost::Confirm(const char *message) {
  return impl_->outer_view_host_->Confirm(message);
}

std::string DecoratedViewHost::Prompt(const char *message,
                                   const char *default_value) {
  return impl_->outer_view_host_->Prompt(message, default_value);
}

ViewInterface::DebugMode DecoratedViewHost::GetDebugMode() const {
  return impl_->outer_view_host_->GetDebugMode();
}

void DecoratedViewHost::BeginResizeDrag(int button, ViewInterface::HitTest hittest) {
  impl_->outer_view_host_->BeginResizeDrag(button, hittest);
}

void DecoratedViewHost::BeginMoveDrag(int button) {
  impl_->outer_view_host_->BeginMoveDrag(button);
}

} // namespace ggadget
