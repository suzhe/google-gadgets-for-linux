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

#include "gadget.h"
#include "gadget_impl.h"

#include "view.h"

namespace ggadget {

using internal::GadgetImpl;

Gadget::Gadget() : impl_(new GadgetImpl()) {
}

Gadget::~Gadget() { 
  delete impl_; 
  impl_ = NULL; 
};

ViewInterface* Gadget::GetMainView() {
  return impl_->main_;  
}
 
ViewInterface* Gadget::GetOptionsView() {
  return impl_->options_;  
}

ViewInterface* Gadget::GetDetailedView() {
  return impl_->detailed_;  
}

bool Gadget::InitFromPath(const char *base_path) {
  return impl_->InitFromPath(base_path);
}

bool GadgetImpl::InitFromPath(const char *base_path) {
  // TODO
  // for now, just create a view
  main_ = new View(200, 200);
  detailed_ = options_ = NULL;
  
  return true;
}

GadgetImpl::~GadgetImpl() {
  delete main_;
  main_ = NULL;
  
  delete detailed_;
  detailed_ = NULL;
  
  delete options_;
  options_ = NULL;
}

} // namespace ggadget
