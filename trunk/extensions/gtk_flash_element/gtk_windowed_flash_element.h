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

#ifndef EXTENSION_GTK_FLASH_ELEMENT_GTK_WINDOWED_FLASH_ELEMENT_H__
#define EXTENSION_GTK_FLASH_ELEMENT_GTK_WINDOWED_FLASH_ELEMENT_H__

#include <stdlib.h>
#include <ggadget/basic_element.h>

namespace ggadget {
namespace gtk {

class GtkFlashElement;

class GtkWindowedFlashElement : public BasicElement {
 public:
  DEFINE_CLASS_ID(0xed12e94863ac3d86, BasicElement);

  GtkWindowedFlashElement(GtkFlashElement *parent,
                          View *view, const char *name);
  virtual ~GtkWindowedFlashElement();

 protected:
  virtual void DoRegister();
  virtual void Layout();
  virtual void DoDraw(CanvasInterface *canvas);

 private:
  DISALLOW_EVIL_CONSTRUCTORS(GtkWindowedFlashElement);
  class Impl;
  Impl *impl_;
};

} // namespace gtk
} // namespace ggadget

#endif // EXTENSION_GTK_FLASH_ELEMENT_GTK_WINDOWED_FLASH_ELEMENT_H__
