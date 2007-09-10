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

#ifndef GGADGET_VIEW_H__
#define GGADGET_VIEW_H__

#include "common.h"
#include "view_interface.h"

namespace ggadget {

namespace internal {
  class ViewImpl;
}

/**
 * Main View implementation.
 */
class View : public ViewInterface {
 public:
  /** Creates a new view. */
  View(int width, int height);
  
  virtual bool AttachHost(HostInterface *host);
  
  virtual void OnMouseDown(const MouseEvent *event);
  virtual void OnMouseUp(const MouseEvent *event);
  virtual void OnMouseClick(const MouseEvent *event);
  virtual void OnMouseDblClick(const MouseEvent *event);
  virtual void OnMouseMove(const MouseEvent *event);
  virtual void OnMouseOut(const MouseEvent *event);
  virtual void OnMouseOver(const MouseEvent *event);
  virtual void OnMouseWheel(const MouseEvent *event);
  
  virtual void OnKeyDown(const KeyboardEvent *event);
  virtual void OnKeyUp(const KeyboardEvent *event);  
  virtual void OnKeyPress(const KeyboardEvent *event);
  
  virtual void OnFocusOut(const Event *event);
  virtual void OnFocusIn(const Event *event);

  virtual bool SetWidth(int width);
  virtual bool SetHeight(int height);
  virtual bool SetSize(int width, int height);

  virtual int GetWidth() const;
  virtual int GetHeight() const;
   
  virtual const CanvasInterface *Draw(bool *changed);   
  
  virtual ~View();
  
 private: 
   internal::ViewImpl *impl_;

   DISALLOW_EVIL_CONSTRUCTORS(View);
};

} // namespace ggadget

#endif // GGADGET_VIEW_H__
