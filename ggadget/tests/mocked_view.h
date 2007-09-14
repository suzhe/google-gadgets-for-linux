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

#ifndef GGADGETS_TEST_MOCKED_ELEMENT_H__
#define GGADGETS_TEST_MOCKED_ELEMENT_H__

#include <string>
#include "ggadget/signal.h"
#include "ggadget/scriptable_helper.h"
#include "ggadget/view_interface.h"

namespace ggadget {
  class MouseEvent;
  class KeyboardEvent;
  class Event;
  class HostInterface;
  class CanvasInterface;
  class ElementInterface;
};

class MockedView : public ggadget::ViewInterface {
 public:
  virtual ~MockedView() {}
 public:
  virtual int GetWidth() const { return 400; }
  virtual int GetHeight() const { return 300; }

  virtual bool AttachHost(ggadget::HostInterface *host) { return true; };

  virtual void OnClick(ggadget::MouseEvent *event) {};
  virtual void OnDblClick(ggadget::MouseEvent *event) {};
  virtual void OnMouseDown(ggadget::MouseEvent *event) {};
  virtual void OnMouseUp(ggadget::MouseEvent *event) {};
  virtual void OnMouseMove(ggadget::MouseEvent *event) {};
  virtual void OnMouseOut(ggadget::MouseEvent *event) {};
  virtual void OnMouseOver(ggadget::MouseEvent *event) {};
  virtual void OnMouseWheel(ggadget::MouseEvent *event) {};

  virtual void OnKeyDown(ggadget::KeyboardEvent *event) {};
  virtual void OnKeyRelease(ggadget::KeyboardEvent *event) {};  
  virtual void OnKeyPress(ggadget::KeyboardEvent *event) {};

  virtual void OnFocusOut(ggadget::Event *event) {};
  virtual void OnFocusIn(ggadget::Event *event) {};

  virtual bool SetWidth(int width) { return true; };
  virtual bool SetHeight(int height) { return true; };
  virtual bool SetSize(int width, int height) { return true; };   

  virtual void OnElementAdded(ggadget::ElementInterface *element) {};
  virtual void OnElementRemoved(ggadget::ElementInterface *element) {};
  virtual void FireEvent(ggadget::Event *event,
                         const ggadget::EventSignal &event_signal) {};

  virtual const ggadget::CanvasInterface *Draw(bool *changed) { return NULL; };

  virtual void SetResizable(ResizableMode resizable) {};
  virtual ResizableMode GetResizable() const { return RESIZABLE_TRUE; };

  virtual void SetCaption(const char *caption) {};
  virtual const char *GetCaption() const { return ""; };
  virtual void SetShowCaptionAlways(bool show_always) {};
  virtual bool GetShowCaptionAlways() const { return false; };
  virtual ggadget::ElementInterface *AppendElement(
      const char *tag_name, const char *name) { return NULL; };
  virtual ggadget::ElementInterface *InsertElement(
      const char *tag_name,
      const ggadget::ElementInterface *before,
      const char *name) { return NULL; };
  virtual bool RemoveElement(
      ggadget::ElementInterface *child) { return false; };

  DEFINE_CLASS_ID(0x8840c50905e84f15, ViewInterface)
  DEFAULT_OWNERSHIP_POLICY
  DELEGATE_SCRIPTABLE_INTERFACE(scriptable_helper_)
  virtual bool IsStrict() const { return true; }

 private:
  ggadget::ScriptableHelper scriptable_helper_;
};

#endif // GGADGETS_TEST_MOCKED_ELEMENT_H__
