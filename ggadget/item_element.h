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

#ifndef GGADGET_ITEM_ELEMENT_H__
#define GGADGET_ITEM_ELEMENT_H__

#include <ggadget/div_element.h>

namespace ggadget {

class ItemElement : public DivElement {
 public:
  DEFINE_CLASS_ID(0x93a09b61fb8a4fda, DivElement);

  ItemElement(ElementInterface *parent,
              ViewInterface *view,
              const char *tag_name,
              const char *name);
  virtual ~ItemElement();

 public:
  /** Gets and sets whether this item is currently selected. */
  bool IsSelected() const;
  void SetSelected(bool selected);

 public:
  static ElementInterface *CreateInstance(ElementInterface *parent,
                                          ViewInterface *view,
                                          const char *name);

  /** For backward compatibility of listitem. */
  static ElementInterface *CreateListItemInstance(ElementInterface *parent,
                                                  ViewInterface *view,
                                                  const char *name);

 protected:
  virtual void DoDraw(CanvasInterface *canvas,
                      const CanvasInterface *children_canvas);

 private:
  DISALLOW_EVIL_CONSTRUCTORS(ItemElement);

  class Impl;
  Impl *impl_;
};

} // namespace ggadget

#endif // GGADGET_ITEM_ELEMENT_H__
