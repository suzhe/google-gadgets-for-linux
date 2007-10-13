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

#ifndef GGADGET_ELEMENT_FACTORY_H__
#define GGADGET_ELEMENT_FACTORY_H__

#include "element_factory_interface.h"

namespace ggadget {

namespace internal {

class ElementFactoryImpl;

} // namespace internal

class ElementFactory : public ElementFactoryInterface {
 public:
  ElementFactory();
  virtual ~ElementFactory();

 public:
  /** @see ElementFactoryInterface::CreateElement() */
  virtual ElementInterface *CreateElement(const char *tag_name,
                                          ElementInterface *parent,
                                          ViewInterface *view,
                                          const char *name);

  /**
   * Used as the @c creator parameter in @c RegisterElementClass().
   */
  typedef ElementInterface *(*ElementCreator)(ElementInterface *parent,
                                              ViewInterface *view,
                                              const char *name);

  /**
   * Registers a new subclass of ElementInterface.
   * @param tag_name the tag name name of the subclass.
   * @param creator the function pointer of the creator, which returns a new
   *     instance of an object of this tag name.
   * @return @c true if registered successfully, or @c false if the specified
   *     tag name already exists.
   */
  bool RegisterElementClass(const char *tag_name,
                            ElementCreator creator);

 private:
  internal::ElementFactoryImpl *impl_;
};

} // namespace ggadget

#endif // GGADGET_ELEMENT_FACTORY_H__
