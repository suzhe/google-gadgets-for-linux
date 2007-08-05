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

#include "view.h"
#include "view_impl.h"
//#include "gadget.h"
//#include "filemanager.h"

namespace ggadget {

namespace internal {

ViewImpl::ViewImpl() {
  RegisterSignal(kOnCancelEvent, &oncancle_event_);
  RegisterSignal(kOnClickEvent, &onclick_event_);
  RegisterSignal(kOnCloseEvent, &onclose_event_);
  RegisterSignal(kOnDblClickEvent, &ondblclick_event_);
  RegisterSignal(kOnDockEvent, &ondock_event_);
  RegisterSignal(kOnKeyDownEvent, &onkeydown_event_);
  RegisterSignal(kOnKeyPressEvent, &onkeypress_event_);
  RegisterSignal(kOnKeyReleaseEvent, &onkeyrelease_event_);
  RegisterSignal(kOnMinimizeEvent, &onminimize_event_);
  RegisterSignal(kOnMouseDownEvent, &onmousedown_event_);
  RegisterSignal(kOnMouseOutEvent, &onmouseout_event_);
  RegisterSignal(kOnMouseOverEvent, &onmouseover_event_);
  RegisterSignal(kOnMouseUpEvent, &onmouseup_event_);
  RegisterSignal(kOnOkEvent, &onok_event_);
  RegisterSignal(kOnOpenEvent, &onopen_event_);
  RegisterSignal(kOnOptionChangedEvent, &onoptionchanged_event_);
  RegisterSignal(kOnPopInEvent, &onpopin_event_);
  RegisterSignal(kOnPopOutEvent, &onpopout_event_);
  RegisterSignal(kOnRestoreEvent, &onrestore_event_);
  RegisterSignal(kOnSizeEvent, &onsize_event_);
  RegisterSignal(kOnSizingEvent, &onsizing_event_);
  RegisterSignal(kOnUndockEvent, &onundock_event_);
}

ViewImpl::~ViewImpl() {
}

} // namespace internal

} // namespace ggadget
