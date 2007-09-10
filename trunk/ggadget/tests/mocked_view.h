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
#include "ggadget/view_interface.h"

namespace ggadget {
  class MouseEvent;
  class KeyboardEvent;
  class Event;
  class HostInterface;
  class CanvasInterface;
};

class MockedView : public ggadget::ViewInterface {
 public:
  virtual ~MockedView() {}
 public:
   virtual int GetWidth() const { return 400; }
   virtual int GetHeight() const { return 300; }
   
   virtual bool AttachHost(ggadget::HostInterface *host) { return true; };
     
   virtual void OnMouseDown(const ggadget::MouseEvent *event) {};
   virtual void OnMouseUp(const ggadget::MouseEvent *event) {};
   virtual void OnMouseClick(const ggadget::MouseEvent *event) {};
   virtual void OnMouseDblClick(const ggadget::MouseEvent *event) {};
   virtual void OnMouseMove(const ggadget::MouseEvent *event) {};
   virtual void OnMouseOut(const ggadget::MouseEvent *event) {};
   virtual void OnMouseOver(const ggadget::MouseEvent *event) {};
   virtual void OnMouseWheel(const ggadget::MouseEvent *event) {};
   
   virtual void OnKeyDown(const ggadget::KeyboardEvent *event) {};
   virtual void OnKeyUp(const ggadget::KeyboardEvent *event) {};  
   virtual void OnKeyPress(const ggadget::KeyboardEvent *event) {};
   
   virtual void OnFocusOut(const ggadget::Event *event) {};
   virtual void OnFocusIn(const ggadget::Event *event) {};

   virtual bool SetWidth(int width) { return true; };
   virtual bool SetHeight(int height) { return true; };
   virtual bool SetSize(int width, int height) { return true; };   
   
   virtual const ggadget::CanvasInterface *Draw(bool *changed) { return NULL; };
};

#endif // GGADGETS_TEST_MOCKED_ELEMENT_H__
