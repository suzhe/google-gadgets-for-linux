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

#ifndef GGADGET_ELEMENT_FACTORY_IMPL_H__
#define GGADGET_ELEMENT_FACTORY_IMPL_H__

#include <map>
#include <string>

namespace ggadget {

class ElementInterface;

namespace internal {

/**
 * Interface for creating an Element in the Gadget API.
 */
class ElementFactoryImpl {
 public:
  bool Init();
  ElementInterface *CreateElement(const char *type,
                                  ElementInterface *parent);
  bool RegisterElementClass(
      const char *type,
      ElementInterface *(*creator)(ElementInterface *));

  std::map<std::string, ElementInterface *(*)(ElementInterface *)> creators_;
};

} // namespace internal

} // namespace ggadget

#endif // GGADGET_ELEMENT_FACTORY_IMPL_H__
