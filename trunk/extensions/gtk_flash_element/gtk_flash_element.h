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

#ifndef EXTENSION_GTK_FLASH_ELEMENT_GTK_FLASH_ELEMENT_H___
#define EXTENSION_GTK_FLASH_ELEMENT_GTK_FLASH_ELEMENT_H__

#include <ggadget/basic_element.h>
#include <ggadget/event.h>

namespace ggadget {
namespace gtk {

#define FLASH_MIME_TYPE "application/x-shockwave-flash"

class GtkFlashElement : public BasicElement {
 public:
  DEFINE_CLASS_ID(0xdc04f82152b34c05, BasicElement);

  GtkFlashElement(BasicElement *parent, View *view, const char *name);
  virtual ~GtkFlashElement();

 public:
  static BasicElement *CreateInstance(BasicElement *parent, View *view,
                                      const char *name);

 protected:
  virtual void DoRegister();
  virtual void Layout();
  virtual void DoDraw(CanvasInterface *canvas);
  virtual EventResult HandleMouseEvent(const MouseEvent &event);
  virtual EventResult HandleKeyEvent(const KeyboardEvent &event);
  virtual EventResult HandleOtherEvent(const Event &event);

 private:
  DISALLOW_EVIL_CONSTRUCTORS(GtkFlashElement);

  class Impl;
  Impl *impl_;
};

} // namespace gtk
} // namespace ggadget

#endif // EXTENSION_GTK_FLASH_ELEMENT_GTK_FLASH_ELEMENT_H__
