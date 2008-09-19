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

#include "view_decorator_base.h"

#include <string>
#include <algorithm>
#include "logger.h"
#include "common.h"
#include "elements.h"
#include "gadget_consts.h"
#include "gadget.h"
#include "main_loop_interface.h"
#include "options_interface.h"
#include "signals.h"
#include "slot.h"
#include "view.h"
#include "view_element.h"
#include "copy_element.h"

namespace ggadget {

class ViewDecoratorBase::Impl {
 public:
  class SignalPostCallback : public WatchCallbackInterface {
   public:
    SignalPostCallback(const Signal0<void> *signal) : signal_(signal) {}
    virtual bool Call(MainLoopInterface *main_loop, int watch_id) {
      (*signal_)();
      return false;
    }
    virtual void OnRemove(MainLoopInterface *main_loop, int watch_id) {
      delete this;
    }
    const Signal0<void> *signal_;
  };

 public:
  Impl(ViewDecoratorBase *owner, const char *option_prefix,
       bool allow_x_margin, bool allow_y_margin)
    : allow_x_margin_(allow_x_margin),
      allow_y_margin_(allow_y_margin),
      child_frozen_(false),
      child_visible_(true),
      child_resizable_(ViewInterface::RESIZABLE_TRUE),
      option_prefix_(option_prefix),
      owner_(owner),
      view_element_(new ViewElement(NULL, owner, NULL, false)),
      snapshot_(new CopyElement(NULL, owner, NULL)) {
    view_element_->SetVisible(true);
    snapshot_->SetVisible(false);
    owner->GetChildren()->InsertElement(view_element_, NULL);
    owner->GetChildren()->InsertElement(snapshot_, NULL);
  }

  void GetClientSize(double *width, double *height) {
    if (view_element_->IsVisible()) {
      *width = view_element_->GetPixelWidth();
      *height = view_element_->GetPixelHeight();
    } else if (snapshot_->IsVisible()) {
      *width = snapshot_->GetPixelWidth();
      *height = snapshot_->GetPixelHeight();
    } else {
      owner_->GetClientExtents(width, height);
    }
  }

  bool OnClientSizing(double *width, double *height) {
    if (*width <= 0 || *height <= 0)
      return false;

    if (view_element_->IsVisible()) {
      return view_element_->OnSizing(width, height);
    } else if (snapshot_->IsVisible()) {
      // Keep snapshot's aspect ratio.
      double src_width = snapshot_->GetSrcWidth();
      double src_height = snapshot_->GetSrcHeight();
      if (src_width > 0 && src_height > 0) {
        double aspect_ratio = src_width / src_height;
        if (*width / *height < aspect_ratio) {
          *height = *width / aspect_ratio;
        } else {
          *width = *height * aspect_ratio;
        }
      }
      return true;
    } else {
      return owner_->OnClientSizing(width, height);
    }
  }

  void SetClientSize(double width, double height) {
    if (view_element_->IsVisible()) {
      if (view_element_->OnSizing(&width, &height))
        view_element_->SetSize(width, height);
    } else if (snapshot_->IsVisible()) {
      // Keep snapshot's aspect ratio.
      double src_width = snapshot_->GetSrcWidth();
      double src_height = snapshot_->GetSrcHeight();
      if (src_width > 0 && src_height > 0) {
        double aspect_ratio = src_width / src_height;
        if (width / height < aspect_ratio) {
          height = width / aspect_ratio;
        } else {
          width = height * aspect_ratio;
        }
      }
      snapshot_->SetPixelWidth(width);
      snapshot_->SetPixelHeight(height);
    }
  }

  void UpdateClientPosition() {
    if (view_element_->IsVisible() || snapshot_->IsVisible()) {
      double top, left, bottom, right;
      owner_->GetMargins(&top, &left, &bottom, &right);
      double client_width;
      double client_height;
      GetClientSize(&client_width, &client_height);
      double space_width = owner_->GetWidth() - left - right;
      double space_height = owner_->GetHeight() - top - bottom;
      double x = left + (space_width - client_width) / 2.0;
      double y = top + (space_height - client_height) / 2.0;
      if (view_element_->IsVisible()) {
        view_element_->SetPixelX(x);
        view_element_->SetPixelY(y);
      } else {
        snapshot_->SetPixelX(x);
        snapshot_->SetPixelY(y);
      }
    }
  }

  void UpdateClientSize() {
    double width = owner_->GetWidth();
    double height = owner_->GetHeight();
    double top, left, bottom, right;
    double min_width, min_height;
    owner_->GetMargins(&top, &left, &bottom, &right);
    owner_->GetMinimumClientExtents(&min_width, &min_height);
    double client_width = std::max(width - left - right, min_width);
    double client_height = std::max(height - top - bottom, min_height);
    SetClientSize(client_width, client_height);
  }

  void Layout() {
    UpdateClientPosition();
    owner_->DoLayout();
  }

  // Returns true if the view size was changed.
  bool SetViewSize(double req_w, double req_h, double min_w, double min_h) {
    if (!allow_x_margin_)
      req_w = min_w;
    if (!allow_y_margin_)
      req_h = min_h;

    if (req_w != owner_->GetWidth() || req_h != owner_->GetHeight()) {
      // DLOG("DecoratedView::SetViewSize(%lf, %lf)", req_w, req_h);
      owner_->View::SetSize(req_w, req_h);
      return true;
    }
    return false;
  }

  void UpdateSnapshot() {
    if (!child_frozen_) {
      // Clear snapshot if child is not frozen.
      snapshot_->SetFrozen(false);
      snapshot_->SetSrc(Variant());
    } else {
      view_element_->SetVisible(true);
      snapshot_->SetFrozen(false);
      snapshot_->SetSrc(Variant(view_element_));
      snapshot_->SetFrozen(true);
      snapshot_->SetSrc(Variant());
      snapshot_->SetPixelWidth(snapshot_->GetSrcWidth());
      snapshot_->SetPixelHeight(snapshot_->GetSrcHeight());
      view_element_->SetVisible(false);
    }
  }

  bool allow_x_margin_;
  bool allow_y_margin_;
  bool child_frozen_;
  bool child_visible_;
  ViewInterface::ResizableMode child_resizable_;
  const char *option_prefix_;
  ViewDecoratorBase *owner_;
  ViewElement *view_element_;
  CopyElement *snapshot_;
  Signal0<void> on_close_signal_;;
};

ViewDecoratorBase::ViewDecoratorBase(ViewHostInterface *host,
                                     const char *option_prefix,
                                     bool allow_x_margin,
                                     bool allow_y_margin)
  : View(host, NULL, NULL, NULL),
    impl_(new Impl(this, option_prefix, allow_x_margin, allow_y_margin)) {
  // Always resizable.
  View::SetResizable(RESIZABLE_TRUE);
  View::EnableCanvasCache(false);
}

ViewDecoratorBase::~ViewDecoratorBase() {
  delete impl_;
  impl_ = NULL;
}

void ViewDecoratorBase::SetChildView(View *child_view) {
  View *old_ = GetChildView();
  if (old_ != child_view) {
    impl_->view_element_->SetChildView(child_view);
    if (child_view)
      impl_->child_resizable_ = child_view->GetResizable();
    OnChildViewChanged();
    UpdateViewSize();
  }
}

View *ViewDecoratorBase::GetChildView() const {
  return impl_->view_element_->GetChildView();
}

void ViewDecoratorBase::SetAllowXMargin(bool allow) {
  if (impl_->allow_x_margin_ != allow) {
    impl_->allow_x_margin_ = allow;
    UpdateViewSize();
  }
}

void ViewDecoratorBase::SetAllowYMargin(bool allow) {
  if (impl_->allow_y_margin_ != allow) {
    impl_->allow_y_margin_ = allow;
    UpdateViewSize();
  }
}

void ViewDecoratorBase::UpdateViewSize() {
  // DLOG("DecoratedView::UpdateViewSize()");
  double top, left, bottom, right;
  GetMargins(&top, &left, &bottom, &right);
  double width = GetWidth();
  double height = GetHeight();
  double client_width = width - left - right;
  double client_height = height - top - bottom;
  impl_->GetClientSize(&client_width, &client_height);

  client_width += (left + right);
  client_height += (top + bottom);

  impl_->SetViewSize(width, height, client_width, client_height);
  // Always do layout event if the view size is not changed, because
  // child view's size has been changed.
  impl_->Layout();
}

bool ViewDecoratorBase::LoadChildViewSize() {
  Gadget *gadget = GetGadget();
  if (gadget && impl_->option_prefix_ && *impl_->option_prefix_) {
    std::string option_prefix(impl_->option_prefix_);
    OptionsInterface *opt = gadget->GetOptions();
    Variant vw =
        opt->GetInternalValue((option_prefix + "_width").c_str());
    Variant vh =
        opt->GetInternalValue((option_prefix + "_height").c_str());
    Variant vs =
        opt->GetInternalValue((option_prefix + "_scale").c_str());

    if (vs.type() == Variant::TYPE_DOUBLE) {
      impl_->view_element_->SetScale(VariantValue<double>()(vs));
    } else {
      impl_->view_element_->SetScale(1.0);
    }
    // view size is only applicable to resizable view.
    if (GetChildViewResizable() == ViewInterface::RESIZABLE_TRUE) {
      double width, height;
      if (vw.type() == Variant::TYPE_DOUBLE &&
          vh.type() == Variant::TYPE_DOUBLE) {
        width = VariantValue<double>()(vw);
        height = VariantValue<double>()(vh);
      } else {
        // Restore to default size if there is no size information saved.
        GetChildView()->GetDefaultSize(&width, &height);
      }
      if (impl_->view_element_->OnSizing(&width, &height))
        impl_->view_element_->SetSize(width, height);
    }
    DLOG("LoadChildViewSize(%d): w:%.0lf h:%.0lf s: %.2lf",
         gadget->GetInstanceID(),
         impl_->view_element_->GetPixelWidth(),
         impl_->view_element_->GetPixelHeight(),
         impl_->view_element_->GetScale());
    impl_->UpdateClientSize();
    return true;
  }
  return false;
}

bool ViewDecoratorBase::SaveChildViewSize() const {
  Gadget *gadget = GetGadget();
  if (gadget && impl_->option_prefix_ && *impl_->option_prefix_) {
    std::string option_prefix(impl_->option_prefix_);
    OptionsInterface *opt = gadget->GetOptions();
    opt->PutInternalValue((option_prefix + "_width").c_str(),
                          Variant(impl_->view_element_->GetPixelWidth()));
    opt->PutInternalValue((option_prefix + "_height").c_str(),
                          Variant(impl_->view_element_->GetPixelHeight()));
    opt->PutInternalValue((option_prefix + "_scale").c_str(),
                          Variant(impl_->view_element_->GetScale()));
    DLOG("SaveChildViewSize(%d): w:%.0lf h:%.0lf s: %.2lf",
         gadget->GetInstanceID(),
         impl_->view_element_->GetPixelWidth(),
         impl_->view_element_->GetPixelHeight(),
         impl_->view_element_->GetScale());
    return true;
  }
  return false;
}

void ViewDecoratorBase::SetChildViewVisible(bool visible) {
  if (impl_->child_visible_ != visible) {
    impl_->child_visible_ = visible;
    impl_->view_element_->SetVisible(visible && !impl_->child_frozen_);
    impl_->snapshot_->SetVisible(visible && impl_->child_frozen_);
    UpdateViewSize();
    impl_->UpdateClientSize();
  }
}

bool ViewDecoratorBase::IsChildViewVisible() const {
  return impl_->child_visible_;
}

void ViewDecoratorBase::SetChildViewFrozen(bool frozen) {
  if (impl_->child_frozen_ != frozen) {
    impl_->child_frozen_ = frozen;
    impl_->UpdateSnapshot();
    impl_->view_element_->SetVisible(impl_->child_visible_ && !frozen);
    impl_->snapshot_->SetVisible(impl_->child_visible_ && frozen);
    UpdateViewSize();
  }
}

bool ViewDecoratorBase::IsChildViewFrozen() const {
  return impl_->child_frozen_;
}

void ViewDecoratorBase::SetChildViewScale(double scale) {
  impl_->view_element_->SetScale(scale);
}

double ViewDecoratorBase::GetChildViewScale() const {
  return impl_->view_element_->GetScale();
}

void ViewDecoratorBase::SetChildViewOpacity(double opacity) {
  impl_->view_element_->SetOpacity(opacity);
  impl_->snapshot_->SetOpacity(opacity);
}

double ViewDecoratorBase::GetChildViewOpacity() const {
  return impl_->view_element_->GetOpacity();
}

void ViewDecoratorBase::SetChildViewCursor(ViewInterface::CursorType type) {
  impl_->view_element_->SetCursor(type);
}

void ViewDecoratorBase::SetChildViewTooltip(const char *tooltip) {
  impl_->view_element_->SetTooltip(tooltip);
  // Make sure the tooltip is updated immediately.
  SetTooltip(tooltip);
}

void ViewDecoratorBase::GetChildViewSize(double *width, double *height) const {
  if (width)
    *width = impl_->view_element_->GetPixelWidth();
  if (height)
    *height = impl_->view_element_->GetPixelHeight();
}

void ViewDecoratorBase::QueueDrawChildView() {
  impl_->view_element_->QueueDraw();
}

void ViewDecoratorBase::ChildViewCoordToViewCoord(double child_x,
                                                  double child_y,
                                                  double *view_x,
                                                  double *view_y) const {
  impl_->view_element_->ChildViewCoordToViewCoord(child_x, child_y,
                                                  view_x, view_y);
}

void ViewDecoratorBase::ViewCoordToChildViewCoord(double view_x,
                                                  double view_y,
                                                  double *child_x,
                                                  double *child_y) const {
  impl_->view_element_->ViewCoordToChildViewCoord(view_x, view_y,
                                                  child_x, child_y);
}

Connection *ViewDecoratorBase::ConnectOnClose(Slot0<void> *slot) {
  return impl_->on_close_signal_.Connect(slot);
}

Gadget *ViewDecoratorBase::GetGadget() const {
  View *child = GetChildView();
  return child ? child->GetGadget() : NULL;
}

bool ViewDecoratorBase::OnAddContextMenuItems(MenuInterface *menu) {
  View *child = GetChildView();
  return child ? child->OnAddContextMenuItems(menu) : false;
}

EventResult ViewDecoratorBase::OnOtherEvent(const Event &event) {
  View::OnOtherEvent(event);
  // Set focus to child view by default.
  if (event.GetType() == Event::EVENT_FOCUS_IN)
    SetFocus(impl_->view_element_);
  View *child = GetChildView();
  return child ? child->OnOtherEvent(event) : EVENT_RESULT_UNHANDLED;
}

bool ViewDecoratorBase::OnSizing(double *width, double *height) {
  ASSERT(width && height);
  if (*width <= 0 || *height <= 0)
    return false;

  double orig_width = *width;
  double orig_height = *height;
  double top, left, bottom, right;
  double min_width, min_height;
  double client_width, client_height;
  GetMargins(&top, &left, &bottom, &right);
  GetMinimumClientExtents(&min_width, &min_height);

  client_width = std::max(*width - left - right, min_width);
  client_height = std::max(*height - top - bottom, min_height);
  bool result = impl_->OnClientSizing(&client_width, &client_height);

  if (!result)
    impl_->GetClientSize(&client_width, &client_height);

  client_width += (left + right);
  client_height += (top + bottom);

  if (!impl_->allow_x_margin_)
    *width = client_width;
  if (!impl_->allow_y_margin_)
    *height = client_height;

  return result || (*width == orig_width && *height == orig_height);
}

void ViewDecoratorBase::SetResizable(ResizableMode resizable) {
  if (impl_->child_resizable_ != resizable) {
    // Reset the zoom factor to 1 if the child view is changed to
    // resizable.
    if (impl_->child_resizable_ != ViewInterface::RESIZABLE_TRUE &&
        resizable == ViewInterface::RESIZABLE_TRUE) {
      impl_->view_element_->SetScale(1.0);
    }
    impl_->child_resizable_ = resizable;
    UpdateViewSize();
  }
}

std::string ViewDecoratorBase::GetCaption() const {
  View *child = GetChildView();
  return child ? child->GetCaption() : View::GetCaption();
}

void ViewDecoratorBase::SetSize(double width, double height) {
  if (GetWidth() == width && GetHeight() == height)
    return;

  double top, left, bottom, right;
  double min_width, min_height;
  GetMargins(&top, &left, &bottom, &right);
  GetMinimumClientExtents(&min_width, &min_height);
  double client_width = std::max(width - left - right, min_width);
  double client_height = std::max(height - top - bottom, min_height);

  impl_->SetClientSize(client_width, client_height);
  impl_->GetClientSize(&client_width, &client_height);

  client_width = std::max(min_width, client_width) + (left + right);
  client_height = std::max(min_height, client_height) + (top + bottom);

  // Call SetViewSize directly here to make sure that
  // allow_x_margin_ and allow_y_margin_ can take effect.
  if (impl_->SetViewSize(width, height, client_width, client_height))
    impl_->Layout();
}

bool ViewDecoratorBase::ShowDecoratedView(bool modal, int flags,
                                          Slot1<bool, int> *feedback_handler) {
  // Derived class shall override this method to do more things.
  return ShowView(modal, flags, feedback_handler);
}

void ViewDecoratorBase::CloseDecoratedView() {
  // Derived class shall override this method to do more things.
  CloseView();
}

void ViewDecoratorBase::PostCloseSignal() {
  GetGlobalMainLoop()->AddTimeoutWatch(
      0, new Impl::SignalPostCallback(&impl_->on_close_signal_));
}

bool ViewDecoratorBase::InsertDecoratorElement(BasicElement *element,
                                               bool background) {
  ASSERT(element);
  return GetChildren()->InsertElement(element,
                                      background ? impl_->view_element_ : NULL);
}

ViewInterface::ResizableMode ViewDecoratorBase::GetChildViewResizable() const {
  return impl_->child_resizable_;
}

void ViewDecoratorBase::OnChildViewChanged() {
  // Nothing to do.
  // To be implemented by derived classes, which will be called when child
  // view is changed.
}

void ViewDecoratorBase::DoLayout() {
  // Nothing to do.
  // To be implemented by derived classes, when the window size is changed.
}

void ViewDecoratorBase::GetMargins(double *top, double *left,
                                   double *bottom, double *right) const {
  *top = 0;
  *left = 0;
  *bottom = 0;
  *right = 0;
}

void ViewDecoratorBase::GetMinimumClientExtents(double *width,
                                                double *height) const {
  *width = 0;
  *height = 0;
}

void ViewDecoratorBase::GetClientExtents(double *width, double *height) const {
  // Derived classes shall override this method to return current client
  // size.
  *width = 0;
  *height = 0;
}

bool ViewDecoratorBase::OnClientSizing(double *width, double *height) {
  // To be implemented by derived classes to report suitable client size when
  // child view is not visible.
  return true;
}

} // namespace ggadget
