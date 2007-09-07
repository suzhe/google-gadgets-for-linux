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

#ifndef GGADGETS_TEST_MOCKED_ELEMENT_H__
#define GGADGETS_TEST_MOCKED_ELEMENT_H__

#include <string>
#include "ggadget/view_interface.h"

class MockedView : public ggadget::ViewInterface {
 public:
  virtual ~MockedView() {}
 public:
   double GetPixelWidth() const { return 400.0; }
   double GetPixelHeight() const { return 300.0; }
};

#endif // GGADGETS_TEST_MOCKED_ELEMENT_H__
