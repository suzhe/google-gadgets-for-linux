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

#ifndef GGADGET_TESTS_MOCKED_VIEW_HOST_H__
#define GGADGET_TESTS_MOCKED_VIEW_HOST_H__

#include "ggadget/graphics_interface.h"
#include "ggadget/canvas_interface.h"
#include "ggadget/view.h"
#include "ggadget/view_interface.h"
#include "ggadget/view_host_interface.h"
#include "mocked_gadget_host.h"

class MockedCanvas : public ggadget::CanvasInterface {
 public:
  MockedCanvas(size_t w, size_t h) : w_(w), h_(h) { }
  virtual void Destroy() { delete this; }
  virtual size_t GetWidth() const { return w_; }
  virtual size_t GetHeight() const { return h_; }
  virtual bool PushState() { return true; }
  virtual bool PopState() { return true; }
  virtual bool MultiplyOpacity(double opacity) { return true; }
  virtual void RotateCoordinates(double radians) { }
  virtual void TranslateCoordinates(double dx, double dy) { }
  virtual void ScaleCoordinates(double cx, double cy) { }
  virtual bool ClearCanvas() { return true; }
  virtual bool DrawLine(double x0, double y0, double x1, double y1,
                        double width, const ggadget::Color &c) {
    return true;
  }
  virtual bool DrawFilledRect(double x, double y,
                              double w, double h, const ggadget::Color &c) {
    return true;
  }
  virtual bool DrawCanvas(double x, double y, const CanvasInterface *img) {
    return true;
  }
  virtual bool DrawFilledRectWithCanvas(double x, double y,
                                        double w, double h,
                                        const CanvasInterface *img) {
    return true;
  }
  virtual bool DrawCanvasWithMask(double x, double y,
                                  const CanvasInterface *img,
                                  double mx, double my,
                                  const CanvasInterface *mask) {
    return true;
  }
  virtual bool DrawText(double x, double y, double width, double height,
                        const char *text, const ggadget::FontInterface *f,
                        const ggadget::Color &c, Alignment align,
                        VAlignment valign, Trimming trimming, int text_flags) {
    return true;
  }
  virtual bool DrawTextWithTexture(double x, double y, double width,
                                   double height, const char *text,
                                   const ggadget::FontInterface *f,
                                   const CanvasInterface *texture,
                                   Alignment align, VAlignment valign,
                                   Trimming trimming, int text_flags) {
    return true;
  }
  virtual bool IntersectRectClipRegion(double x, double y,
                                       double w, double h) {
    return true;
  }
  virtual bool GetTextExtents(const char *text, const ggadget::FontInterface *f,
                              int text_flags, double in_width,
                              double *width, double *height) {
    return false;
  }
  virtual bool GetPointValue(double x, double y,
                             ggadget::Color *color, double *opacity) const {
    return false;
  }
 private:
  size_t w_, h_;
};

class MockedGraphics : public ggadget::GraphicsInterface {
  virtual ggadget::CanvasInterface *NewCanvas(size_t w, size_t h) const {
    return new MockedCanvas(w, h);
  }
  virtual ggadget::ImageInterface *NewImage(const std::string &data,
                                            bool is_mask) const {
      return NULL;
  }
  virtual ggadget::FontInterface *NewFont(
      const char *family, size_t pt_size,
      ggadget::FontInterface::Style style,
      ggadget::FontInterface::Weight weight) const {
    return NULL;
  }
};

class MockedViewHost : public ggadget::ViewHostInterface {
 public:
  MockedViewHost(ggadget::ElementFactoryInterface *factory)
      : view_(new ggadget::View(this, NULL, factory, 0)), draw_queued_(false) {
  }
  virtual ~MockedViewHost() {
    delete view_;
  }

  virtual ggadget::GadgetHostInterface *GetGadgetHost() const {
    return const_cast<MockedGadgetHost *>(&gadget_host_);
  }
  virtual ggadget::ViewInterface *GetView() { return view_; }
  virtual const ggadget::ViewInterface *GetView() const { return view_; }
  virtual ggadget::ScriptContextInterface *GetScriptContext() const {
    return NULL;
  }
  virtual ggadget::XMLHttpRequestInterface *NewXMLHttpRequest() { return NULL; }
  virtual const ggadget::GraphicsInterface *GetGraphics() const {
    return const_cast<MockedGraphics *>(&graphics_);
  }
  virtual void QueueDraw() { draw_queued_ = true; }
  virtual bool GrabKeyboardFocus() { return false; }
  virtual void SetResizable(ggadget::ViewInterface::ResizableMode mode) { }
  virtual void SetCaption(const char *caption) { }
  virtual void SetShowCaptionAlways(bool always) { }
  virtual void SetCursor(CursorType type) { }
  virtual void SetTooltip(const char *tooltip) { }
  virtual void RunDialog() { }
  virtual void ShowInDetailsView(
      const char *title, int flags,
      ggadget::Slot1<void, int> *feedback_handler) { }
  virtual void CloseDetailsView() { }
  virtual void Alert(const char *message) { }
  virtual bool Confirm(const char *message) { return false; }
  virtual std::string Prompt(const char *message, const char *default_value) {
    return std::string();
  }
  virtual ggadget::EditInterface* NewEdit(size_t, size_t) { return NULL; }

  bool GetQueuedDraw() {
    bool b = draw_queued_;
    draw_queued_ = false;
    ggadget::CanvasInterface *canvas = new MockedCanvas(100, 100);
    view_->Draw(canvas);
    canvas->Destroy();
    return b;
  }
  ggadget::View *GetViewInternal() { return view_; }

 private:
  MockedGadgetHost gadget_host_;
  MockedGraphics graphics_;
  ggadget::View *view_;
  bool draw_queued_;
};

#endif // GGADGET_TESTS_MOCKED_VIEW_HOST_H__
