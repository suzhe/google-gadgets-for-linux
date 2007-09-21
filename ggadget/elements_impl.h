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

#ifndef GGADGET_ELEMENTS_IMPL_H__
#define GGADGET_ELEMENTS_IMPL_H__

#include <vector>
#include "scriptable_helper.h"
#include "variant.h"

namespace ggadget {

class CanvasInterface;
class ElementFactoryInterface;
class ElementInterface;
class ViewInterface;

namespace internal {

class ElementsImpl {
 public:
  ElementsImpl(ElementFactoryInterface *factory,
               ElementInterface *owner,
               ViewInterface *view);
  ~ElementsImpl();
  int GetCount();
  ElementInterface *AppendElement(const char *tag_name, const char *name);
  ElementInterface *InsertElement(const char *tag_name,
                                  const ElementInterface *before,
                                  const char *name);
  bool RemoveElement(ElementInterface *element);
  void RemoveAllElements();

  ElementInterface *GetItem(const Variant &index_or_name);
  ElementInterface *GetItemByIndex(int index);
  ElementInterface *GetItemByName(const char *name);
  int GetIndexByName(const char *name);

  void OnParentWidthChange(double width);
  void OnParentHeightChange(double height);
  
  const CanvasInterface *Draw(bool *changed);
  
  DELEGATE_SCRIPTABLE_REGISTER(scriptable_helper_)

  ScriptableHelper scriptable_helper_;
  ElementFactoryInterface *factory_;
  ElementInterface *owner_;
  ViewInterface *view_;
  std::vector<ElementInterface *> children_;
  double width_;
  double height_;
  CanvasInterface *canvas_;
  bool count_changed_;
};

} // namespace internal

} // namespace ggadget

#endif // GGADGET_ELEMENTS_IMPL_H__
