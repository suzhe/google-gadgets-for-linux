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

#ifndef GGADGET_VIEW_INTERFACE_H__
#define GGADGET_VIEW_INTERFACE_H__

#include "scriptable_interface.h"

/**
 * Interface for representing a View in the Gadget API.
 */
class ViewInterface : public ScriptableInterface {
 public:
  /** 
   * Initializes a view.
   * @param xml XML document specifying the view to generate.
   */
  virtual bool Init() = 0;
};

#endif // GGADGET_VIEW_INTERFACE_H__
