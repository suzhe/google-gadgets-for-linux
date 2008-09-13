/*
  Copyright 2008 Google Inc.

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

#include "video_element_base.h"

#include <ggadget/canvas_interface.h>
#include <ggadget/basic_element.h>
#include <ggadget/elements.h>

namespace ggadget {

const char kOnStateChangeEvent[] = "onstatechange";
const char kOnPositionChangeEvent[]  = "onpositionchange";
const char kOnMediaChangeEvent[]  = "onmediachange";

class VideoElementBase::Impl {
 public:
  Impl()
      : autoplay_(true), image_data_(NULL),
        image_x_(0), image_y_(0), image_w_(0), image_h_(0), image_stride_(0) {
  }
  ~Impl() { }

  bool autoplay_;

  // Datas for image.
  const char *image_data_;
  int image_x_;
  int image_y_;
  int image_w_;
  int image_h_;
  int image_stride_;

  // Signal events
  EventSignal on_state_change_event_;
  EventSignal on_position_change_event_;
  EventSignal on_media_change_event_;
};

VideoElementBase::VideoElementBase(BasicElement *parent, View *view,
                                   const char *tag_name, const char *name,
                                   bool children)
    : BasicElement(parent, view, tag_name, name, children),
      impl_(new Impl) {
  // When hosted by an object element, map the size to the parent's size.
  if (parent && parent->IsInstanceOf(ObjectVideoPlayer::CLASS_ID)) {
    SetRelativeX(0);
    SetRelativeY(0);
    SetRelativeWidth(1.0);
    SetRelativeHeight(1.0);
  }
}

VideoElementBase::~VideoElementBase() {
  delete impl_;
}

bool VideoElementBase::IsAvailable(const std::string &name) {
  State state = GetState();
  if (name.compare("play") == 0) {
    if (state == gddVideoStateReady || state == gddVideoStatePaused ||
        state == gddVideoStateStopped)
      return true;
  } else if (name.compare("pause") == 0) {
    if (state == gddVideoStatePlaying)
      return true;
  } else if (name.compare("stop") == 0) {
    if (state == gddVideoStatePlaying || state == gddVideoStatePaused ||
        state == gddVideoStateEnded)
      return true;
  } else if (name.compare("seek") == 0 || name.compare("currentPosition")) {
    if (state == gddVideoStatePlaying || state == gddVideoStatePaused)
      return Seekable();
  }

  // For "volume", "balance", and "mute", let the real video element to
  // decide whether these controls can be supported.
  return false;
}

bool VideoElementBase::GetAutoPlay() {
  return impl_->autoplay_;
}

void VideoElementBase::SetAutoPlay(bool autoplay) {
  impl_->autoplay_ = autoplay;
}

Connection *VideoElementBase::ConnectOnStateChangeEvent(Slot0<void> *handler) {
  return impl_->on_state_change_event_.Connect(handler);
}

Connection *VideoElementBase::ConnectOnPositionChangeEvent(Slot0<void> *handler) {
  return impl_->on_position_change_event_.Connect(handler);
}

Connection *VideoElementBase::ConnectOnMediaChangeEvent(Slot0<void> *handler) {
  return impl_->on_media_change_event_.Connect(handler);
}

void VideoElementBase::DoRegister() {
  BasicElement::DoRegister();

  RegisterProperty("autoPlay",
                   NewSlot(this, &VideoElementBase::GetAutoPlay),
                   NewSlot(this, &VideoElementBase::SetAutoPlay));
  RegisterProperty("currentTime",
                   NewSlot(this, &VideoElementBase::GetCurrentPosition),
                   NewSlot(this, &VideoElementBase::SetCurrentPosition));
  RegisterProperty("duration",
                   NewSlot(this, &VideoElementBase::GetDuration),
                   NULL);
  RegisterProperty("error",
                   NewSlot(this, &VideoElementBase::GetErrorCode),
                   NULL);
  RegisterProperty("state",
                   NewSlot(this, &VideoElementBase::GetState),
                   NULL);
  RegisterProperty("seekable",
                   NewSlot(this, &VideoElementBase::Seekable),
                   NULL);
  RegisterProperty("src",
                   NewSlot(this, &VideoElementBase::GetSrc),
                   NewSlot(this, &VideoElementBase::SetSrc));
  RegisterProperty("volume",
                   NewSlot(this, &VideoElementBase::GetVolume),
                   NewSlot(this, &VideoElementBase::SetVolume));
  RegisterProperty("balance",
                   NewSlot(this, &VideoElementBase::GetBalance),
                   NewSlot(this, &VideoElementBase::SetBalance));
  RegisterProperty("mute",
                   NewSlot(this, &VideoElementBase::GetMute),
                   NewSlot(this, &VideoElementBase::SetMute));

  RegisterMethod("isAvailable", NewSlot(this, &VideoElementBase::IsAvailable));
  RegisterMethod("play", NewSlot(this, &VideoElementBase::Play));
  RegisterMethod("pause", NewSlot(this, &VideoElementBase::Pause));
  RegisterMethod("stop", NewSlot(this, &VideoElementBase::Stop));

  RegisterSignal(kOnStateChangeEvent, &impl_->on_state_change_event_);
  RegisterSignal(kOnPositionChangeEvent, &impl_->on_position_change_event_);
  RegisterSignal(kOnMediaChangeEvent, &impl_->on_media_change_event_);
}

void VideoElementBase::DoDraw(CanvasInterface *canvas) {
  if (canvas && impl_->image_data_) {
    canvas->DrawRawImage(impl_->image_x_,
                         impl_->image_y_,
                         impl_->image_data_,
                         CanvasInterface::RAWIMAGE_FORMAT_RGB24,
                         impl_->image_w_, impl_->image_h_,
                         impl_->image_stride_);
  }
  if (IsSizeChanged()) {
    SetGeometry(GetPixelWidth(), GetPixelHeight());
  }
}

bool VideoElementBase::PutImage(const void *data,
                                int x, int y, int w, int h, int stride) {
  impl_->image_data_ = reinterpret_cast<const char*>(data);
  impl_->image_x_ = x;
  impl_->image_y_ = y;
  impl_->image_w_ = w;
  impl_->image_h_ = h;
  impl_->image_stride_ = stride;
  QueueDraw();
  return true;
}

void VideoElementBase::ClearImage() {
  impl_->image_data_ = NULL;
  QueueDraw();
}

void VideoElementBase::FireOnStateChangeEvent() {
  impl_->on_state_change_event_();
}

void VideoElementBase::FireOnPositionChangeEvent() {
  impl_->on_position_change_event_();
}

void VideoElementBase::FireOnMediaChangeEvent() {
  impl_->on_media_change_event_();
}

} // namespace ggadget
