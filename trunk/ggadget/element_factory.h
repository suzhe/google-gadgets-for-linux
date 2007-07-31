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
#include "scoped_ptr.h"

namespace ggadget {

namespace internal {

class ElementFactoryImpl;

} // namespace internal

class ElementInterface;

/**
 * Singleton for creating an Element in the Gadget API.
 */
class ElementFactory : public ElementFactoryInterface {
 public:
  // not virtual because no inheritance to this class is allowed.
  ~ElementFactory();

 public:
  /** @see ElementFactoryInterface::CreateElement. */
  virtual ElementInterface *CreateElement(const char *tag_name,
                                          ElementInterface *parent,
                                          const char *name);
  /** @see ElementFactoryInterface::RegisterElementClass. */
  virtual bool RegisterElementClass(
      const char *tag_name, ElementInterface *(*creator)(ElementInterface *,
                                                         const char *));

 public:
  /**
   * Retrieve the only instance of the class.
   */
  static ElementFactory *GetInstance(void);

 private:
  ElementFactory(); // Singleton.

 private:
  internal::ElementFactoryImpl *impl_;
  static scoped_ptr<ElementFactory> instance_;
};


} // namespace ggadget

#endif // GGADGET_ELEMENT_FACTORY_H__
