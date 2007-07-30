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

namespace ggadget {

class ElementFactoryInterface;
class ElementInterface;

namespace internal {

class ElementsImpl {
 public:
  ElementsImpl(ElementFactoryInterface *factory, ElementInterface *owner);
  ~ElementsImpl();
  int GetCount() const;
  ElementInterface *GetItem(int index);
  const ElementInterface *GetItem(int index) const;
  ElementInterface *AppendElement(const char *type);
  ElementInterface *InsertElement(const char *type,
                                  const ElementInterface *before);
  bool RemoveElement(const ElementInterface *element);

  ElementFactoryInterface *factory_;
  ElementInterface *owner_;
  std::vector<ElementInterface *> children_;
};

} // namespace internal

} // namespace ggadget

#endif // GGADGET_ELEMENTS_IMPL_H__
