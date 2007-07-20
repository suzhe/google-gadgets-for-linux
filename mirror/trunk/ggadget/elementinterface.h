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

#ifndef GOOGLE_GADGETS_LIB_ELEMENT_INTERFACE_H__
#define GOOGLE_GADGETS_LIB_ELEMENT_INTERFACE_H__

#include "scriptableinterface.h"

/**
 * Interface for representing an Element in the Gadget API.
 */
class ElementInterface : public ScriptableInterface {
 public:
  /** 
   * Initializes an element.
   */
  virtual bool Init() = 0;
};

#endif // GOOGLE_GADGETS_LIB_ELEMENT_INTERFACE_H__
