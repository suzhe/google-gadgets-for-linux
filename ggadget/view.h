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

#ifndef GOOGLE_GADGETS_LIB_VIEW_H__
#define GOOGLE_GADGETS_LIB_VIEW_H__

#include "viewinterface.h"

/**
 * Main View implementation.
 */
class View : public ViewInterface {
 public:
  /** 
   * Initializes a view.
   * @param xml XML document specifying the view to generate.
   */
  virtual bool Init(const std::string &xml);

 private: 
  View(const View &view);
};

#endif // GOOGLE_GADGETS_LIB_VIEW_H__
