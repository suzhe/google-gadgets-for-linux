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

#ifndef GGADGET_TESTS_MOCKED_ELEMENT_H__
#define GGADGET_TESTS_MOCKED_ELEMENT_H__

#include "ggadget/basic_element.h"
#include "ggadget/view.h"
#include "ggadget/elements.h"

class Muffin : public ggadget::BasicElement {
 public:
  Muffin(ggadget::BasicElement *parent, ggadget::View *view, const char *name)
      : ggadget::BasicElement(parent, view, "muffin", name, true) {
  }

  virtual ~Muffin() {
  }

  virtual void DoDraw(ggadget::CanvasInterface *canvas) { }

 public:
  DEFINE_CLASS_ID(0x6c0dee0e5bbe11dc, ggadget::BasicElement)

 public:
  static ggadget::BasicElement *CreateInstance(ggadget::BasicElement *parent,
                                               ggadget::View *view,
                                               const char *name) {
    return new Muffin(parent, view, name);
  }
};

class Pie : public ggadget::BasicElement {
 public:
  Pie(ggadget::BasicElement *parent, ggadget::View *view, const char *name)
      : ggadget::BasicElement(parent, view, "pie", name, true) {
  }

  virtual ~Pie() {
  }

  virtual void DoDraw(ggadget::CanvasInterface *canvas) { }

 public:
  DEFINE_CLASS_ID(0x829defac5bbe11dc, ggadget::BasicElement)

 public:
  static ggadget::BasicElement *CreateInstance(ggadget::BasicElement *parent,
                                               ggadget::View *view,
                                               const char *name) {
    return new Pie(parent, view, name);
  }
};

#endif // GGADGETS_TESTS_MOCKED_ELEMENT_H__
