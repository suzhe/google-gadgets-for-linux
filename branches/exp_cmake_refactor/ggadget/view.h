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

#include <ggadget/common.h>
#include <ggadget/scriptable_helper.h>
#include <ggadget/view_interface.h>

namespace ggadget {

template <typename R> class Slot0;
class ViewHostInterface;
class ElementFactoryInterface;
class ScriptContextInterface;

/**
 * Main View implementation.
 */
class View : public ScriptableHelper<ViewInterface> {
 public:
  DEFINE_CLASS_ID(0xc4ee4a622fbc4b7a, ViewInterface)

  View(ViewHostInterface *host,
       ScriptableInterface *prototype,
       ElementFactoryInterface *element_factory,
       int debug_mode);
  virtual ~View();

  virtual ScriptContextInterface *GetScriptContext() const;
  virtual FileManagerInterface *GetFileManager() const;
  virtual bool InitFromFile(FileManagerInterface *file_manager,
                            const char *filename);

  virtual bool OnMouseEvent(MouseEvent *event);
  virtual bool OnKeyEvent(KeyboardEvent *event);
  virtual bool OnOtherEvent(Event *event);

  virtual void OnElementAdd(ElementInterface *element);
  virtual void OnElementRemove(ElementInterface *element);
  virtual void FireEvent(ScriptableEvent *event,
                         const EventSignal &event_signal);
  virtual ScriptableEvent *GetEvent();
  virtual const ScriptableEvent *GetEvent() const;
  virtual void SetFocus(ElementInterface *element);

  virtual bool SetWidth(int width);
  virtual bool SetHeight(int height);
  virtual bool SetSize(int width, int height);

  virtual int GetWidth() const;
  virtual int GetHeight() const;

  virtual const CanvasInterface *Draw(bool *changed);
  virtual const GraphicsInterface *GetGraphics() const;
  virtual void QueueDraw();

  virtual void SetResizable(ResizableMode resizable);
  virtual ResizableMode GetResizable() const;
  virtual void SetCaption(const char *caption);
  virtual const char *GetCaption() const;
  virtual void SetShowCaptionAlways(bool show_always);
  virtual bool GetShowCaptionAlways() const;

  virtual ElementFactoryInterface *GetElementFactory() const;
  virtual const Elements *GetChildren() const;
  virtual Elements *GetChildren();
  virtual ElementInterface *GetElementByName(const char *name);
  virtual const ElementInterface *GetElementByName(const char *name) const;

  virtual int BeginAnimation(Slot0<void> *slot,
                             int start_value,
                             int end_value,
                             unsigned int duration);
  virtual void CancelAnimation(int token);
  virtual int SetTimeout(Slot0<void> *slot, unsigned int duration);
  virtual void ClearTimeout(int token);
  virtual int SetInterval(Slot0<void> *slot, unsigned int duration);
  virtual void ClearInterval(int token);
  virtual int GetDebugMode() const;
  virtual void OnOptionChanged(const char *name);

  virtual Image *LoadImage(const char *name, bool is_mask);
  virtual Image *LoadImageFromGlobal(const char *name, bool is_mask);
  virtual Texture *LoadTexture(const char *name);
  
  virtual bool OpenURL(const char *url) const; 
  
 private:
  class Impl;
  Impl *impl_;
  DISALLOW_EVIL_CONSTRUCTORS(View);
};

} // namespace ggadget

#endif // GGADGET_VIEW_H__
