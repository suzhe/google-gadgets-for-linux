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
#include "scriptable_helper.h"
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
  DEFINE_CLASS_ID(0xc4ee4a622fbc4b7a, ViewInterface)

  /** Creates a new view. */
  View(int width, int height);
  virtual ~View();
  
  virtual bool AttachHost(HostInterface *host);
  
  virtual void OnMouseEvent(MouseEvent *event);
  virtual void OnKeyEvent(KeyboardEvent *event);
  virtual void OnOtherEvent(Event *event);
  virtual void OnTimerEvent(TimerEvent *event);

  virtual void OnElementAdded(ElementInterface *element);
  virtual void OnElementRemoved(ElementInterface *element);
  virtual void FireEvent(Event *event, const EventSignal &event_signal);

  virtual bool SetWidth(int width);
  virtual bool SetHeight(int height);
  virtual bool SetSize(int width, int height);

  virtual int GetWidth() const;
  virtual int GetHeight() const;

  virtual const CanvasInterface *Draw(bool *changed);   

  virtual void SetResizable(ResizableMode resizable);
  virtual ResizableMode GetResizable() const;
  virtual void SetCaption(const char *caption);
  virtual const char *GetCaption() const;
  virtual void SetShowCaptionAlways(bool show_always);
  virtual bool GetShowCaptionAlways() const;

  virtual const Elements *GetChildren() const;
  virtual Elements *GetChildren();
  virtual ElementInterface *AppendElement(const char *tag_name,
                                          const char *name);
  virtual ElementInterface *InsertElement(const char *tag_name,
                                          const ElementInterface *before,
                                          const char *name);
  virtual bool RemoveElement(ElementInterface *child);
  virtual ElementInterface *GetElementByName(const char *name);
  virtual const ElementInterface *GetElementByName(const char *name) const;

  DEFAULT_OWNERSHIP_POLICY
  DELEGATE_SCRIPTABLE_INTERFACE(scriptable_helper_)
  virtual bool IsStrict() const { return true; }

 protected:
  DELEGATE_SCRIPTABLE_REGISTER(scriptable_helper_)

 private: 
  internal::ViewImpl *impl_;
  ScriptableHelper scriptable_helper_;
  DISALLOW_EVIL_CONSTRUCTORS(View);
};

} // namespace ggadget

#endif // GGADGET_VIEW_H__
