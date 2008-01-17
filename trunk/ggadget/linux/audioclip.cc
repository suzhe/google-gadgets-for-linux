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

#include "audioclip.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef GST_AUDIOCLIP
#include <pthread.h>
#include <gst/gst.h>
#endif

#include <string>

#include <ggadget/logger.h>

namespace ggadget {
namespace framework {
namespace linux_os {

#ifdef GST_AUDIOCLIP

/**
 * Implementation based on gstreamer.
 */
class Audioclip::Impl {
 public:

  Impl(const char *src)
      : init_failed_(false),
        balance_(kMaxBalance + 1),
        volume_(kMaxVolume + 1),
        duration_(-1),
        local_error_(SOUND_ERROR_NO_ERROR) {
    if (!InitGstreamer()) {
      init_failed_ = true;
      return;
    }
    if (src)
      SetSrc(src);
  }

  ~Impl() {
    DestroyGstreamer();
  }

  bool InitIsFailed() {
    return init_failed_;
  }

  int GetBalance() {
    if (init_failed_)
      return (kMaxBalance + kMinBalance) / 2;
    if (!panorama_) {
      DLOG("Balance is not supported.");
      return (kMaxBalance + kMinBalance) / 2;
    }
    if (balance_ != kMaxBalance + 1)
      return balance_;
    float balance;
    g_object_get(G_OBJECT(panorama_), "panorama", &balance, NULL);
    ASSERT(balance >= -1 && balance <= 1);
    balance_ = GstBalanceToLocalBalance(balance);
    return balance_;
  }

  void SetBalance(int balance) {
    if (init_failed_)
      return;
    if (!panorama_) {
      DLOG("Balance is not supported.");
      return;
    }
    if (balance < kMinBalance || balance > kMaxBalance) {
      LOG("Invalid balance value, range: [%d, %d].", kMinBalance, kMaxBalance);
      return;
    }
    if (balance == balance_)
      return;
    g_object_set(G_OBJECT(panorama_),
                 "panorama", LocalBalanceToGstBalance(balance), NULL);
    balance_ = balance;
  }

  int GetCurrentPosition() {
    if (init_failed_)
      return -1;
    if (GetState() == SOUND_STATE_STOPPED)
      return 0;

    gint64 position;
    GstFormat format = GST_FORMAT_TIME;
    if (gst_element_query_position(playbin_, &format, &position))
      return static_cast<int>(position / GST_SECOND);

    // This hardly occurs as the above query always succeeds under non-stopped
    // state.
    return -1;
  }

  void SetCurrentPosition(int position) {
    if (init_failed_)
      return;
    // Seek will only be done under PAUSED or PLAYING state.
    GetState();
    if (local_state_ != SOUND_STATE_PAUSED &&
        local_state_ != SOUND_STATE_PLAYING)
      return;
    if (position < 0)
      position = 0;
    if (position > duration_)
      position = duration_;
    gst_element_seek(playbin_, 1.0, GST_FORMAT_TIME,
                     static_cast<GstSeekFlags>(GST_SEEK_FLAG_FLUSH |
                                               GST_SEEK_FLAG_KEY_UNIT),
                     GST_SEEK_TYPE_SET, (gint64)position * GST_SECOND,
                     GST_SEEK_TYPE_END, 0);
  }

  int GetDuration() {
    if (init_failed_)
      return -1;
    if (duration_ != -1)
      return duration_;

    gint64 duration;
    GstFormat format = GST_FORMAT_TIME;
    if (gst_element_query_duration(playbin_, &format, &duration)) {
      ASSERT(format == GST_FORMAT_TIME);
      duration_ = static_cast<int>(duration / GST_SECOND);
      return duration_;
    }
    return -1;
  }

  ErrorCode GetError() const {
    if (init_failed_)
      return SOUND_ERROR_NO_ERROR;
    return local_error_;
  }

  std::string GetSrc() const {
    if (init_failed_)
      return "";
    return src_;
  }

  void SetSrc(const char *src) {
    if (init_failed_)
      return;
    if (src == NULL)
      return;

    // Playbin won't produce ERROR whether it's a bad uri or the file's format
    // is not supported. We must check here.
    src_ = std::string(src);
    g_object_set(G_OBJECT(playbin_), "uri", src, NULL);

    // Reset vars.
    duration_ = -1;
    local_state_ = SOUND_STATE_STOPPED;
    local_error_ = SOUND_ERROR_NO_ERROR;
  }

  State GetState() {
    if (init_failed_)
      return SOUND_STATE_STOPPED;
    return local_state_;
  }

  int GetVolume() {
    if (init_failed_)
      return kMinVolume;
    if (volume_ != kMaxVolume + 1)
      return volume_;
    double volume;
    g_object_get(G_OBJECT(playbin_), "volume", &volume, NULL);
    ASSERT(volume >= 0 && volume <= 4);
    volume_ = GstVolumeToLocalVolume(volume);
    return volume_;
  }

  void SetVolume(int volume) {
    if (init_failed_)
      return;
    if (volume < kMinVolume || volume > kMaxVolume) {
      LOG("Invalid volume value, range: [%d, %d].", kMinVolume, kMaxVolume);
      return;
    }
    if (volume == volume_)
      return;
    g_object_set(G_OBJECT(playbin_),
                 "volume", LocalVolumeToGstVolume(volume), NULL);
    volume_ = volume;
  }

  void Play() {
    if (init_failed_)
      return;
    if (src_.length() == 0) {
      LOG("no audio source");
      return;
    }
    if (gst_element_set_state(playbin_, GST_STATE_PLAYING) ==
        GST_STATE_CHANGE_FAILURE) {
      LOG("failed to play the audio");
    }
  }

  void Pause() {
    if (init_failed_)
      return;
    if (GetState() != SOUND_STATE_PLAYING)
      return;
    if (gst_element_set_state(playbin_, GST_STATE_PAUSED) ==
        GST_STATE_CHANGE_FAILURE) {
      LOG("failed to pause the audio");
    }
  }

  void Stop() {
    if (init_failed_)
      return;
    if (GetState() == SOUND_STATE_STOPPED)
      return;
    // If set "NULL" state here, playbin won't produce "STATE CHANGED" message.
    if (gst_element_set_state(playbin_, GST_STATE_READY) ==
        GST_STATE_CHANGE_FAILURE) {
      LOG("failed to stop the audio");
    }
  }

  Connection *ConnectOnStateChange(OnStateChangeHandler *handler) {
    return on_state_change_signal_.Connect(handler);
  }

  void OnStateChange(GstMessage *msg) {
    ASSERT(msg);
    GstState old_state, new_state;
    gst_message_parse_state_changed(msg, &old_state, &new_state, NULL);
    State new_local_state = GstStateToLocalState(new_state);
    if (local_state_ != new_local_state) {
      DLOG("AudioClip OnStateChange: old=%d new=%d",
           local_state_, new_local_state);
      local_state_ = new_local_state;
      on_state_change_signal_(local_state_);
    }
  }

  void OnError(GstMessage *msg) {
    ASSERT(msg);
    GError *gerror;
    gst_message_parse_error(msg, &gerror, NULL);
    DLOG("AudioClip OnError: domain=%d code=%d message=%s",
         gerror->domain, gerror->code, gerror->message);
    if (gerror->domain == GST_RESOURCE_ERROR &&
        (gerror->code == GST_RESOURCE_ERROR_NOT_FOUND ||
         gerror->code == GST_RESOURCE_ERROR_OPEN_READ ||
         gerror->code == GST_RESOURCE_ERROR_OPEN_READ_WRITE)) {
      local_error_ = SOUND_ERROR_BAD_CLIP_SRC;
    } else if (gerror->domain == GST_STREAM_ERROR &&
               (gerror->code == GST_STREAM_ERROR_NOT_IMPLEMENTED ||
                gerror->code == GST_STREAM_ERROR_TYPE_NOT_FOUND ||
                gerror->code == GST_STREAM_ERROR_WRONG_TYPE ||
                gerror->code == GST_STREAM_ERROR_CODEC_NOT_FOUND ||
                gerror->code == GST_STREAM_ERROR_FORMAT)) {
      local_error_ = SOUND_ERROR_FORMAT_NOT_SUPPORTED;
    } else {
      local_error_ = SOUND_ERROR_UNKNOWN;
    }
    local_state_ = SOUND_STATE_ERROR;
    on_state_change_signal_(local_state_);

    // Playbin doesnot change the state to "NULL" or "READY" when it
    // meets an error, we help make a state-change scene.
    Stop();
  }

  void OnEnd() {
    // Playbin doesnot change the state to "NULL" or "READY" when it
    // reaches the end of the stream, we help make a state-change scene.
    Stop();
  }

 private:
  static gboolean OnNewMessage(GstBus *bus, GstMessage *msg, gpointer object) {
    switch (GST_MESSAGE_TYPE(msg)) {
      case GST_MESSAGE_ERROR:
        (reinterpret_cast<Impl*>(object))->OnError(msg);
        break;
      case GST_MESSAGE_EOS:
        (reinterpret_cast<Impl*>(object))->OnEnd();
        break;
      case GST_MESSAGE_STATE_CHANGED:
        (reinterpret_cast<Impl*>(object))->OnStateChange(msg);
        break;
      default:
        break;
    }
    return true;
  }

  // Mapping from gstreamer state to local state.
  static State GstStateToLocalState(GstState state) {
    switch (state) {
      case GST_STATE_NULL:
      case GST_STATE_READY:
        return SOUND_STATE_STOPPED;
      case GST_STATE_PAUSED:
        return SOUND_STATE_PAUSED;
      case GST_STATE_PLAYING:
        return SOUND_STATE_PLAYING;
      default:
        return SOUND_STATE_ERROR;
    }
  }

  static int GstBalanceToLocalBalance(float balance) {
    return static_cast<int>(((balance + 1) / 2) * (kMaxBalance - kMinBalance) +
                            kMinBalance);
  }

  static gfloat LocalBalanceToGstBalance(int balance) {
    return (((static_cast<gfloat>(balance) - kMinBalance) /
             (kMaxBalance - kMinBalance)) * 2 - 1);
  }

  static int GstVolumeToLocalVolume(double volume) {
    return static_cast<int>((volume / 4) * (kMaxVolume - kMinVolume) +
                            kMinVolume);
  }

  static gdouble LocalVolumeToGstVolume(int volume) {
    return 4 * ((static_cast<gdouble>(volume) - kMinVolume) /
                (kMaxVolume - kMinVolume));
  }

  bool InitGstreamer() {
    gst_init(NULL, NULL);

    // Create elements.
    playbin_ = gst_element_factory_make("playbin", "play");
    videofilter_ = gst_element_factory_make("fakesink", "videofilter");
    if (!playbin_ || !videofilter_) {
      LOG("failed to create gstreamer elements.\n");
      return false;
    }
    panorama_ = gst_element_factory_make("audiopanorama", "panorama");
    audioctl_ = NULL;
    if (panorama_) {
      alsaoutput_ = gst_element_factory_make("alsasink", "alsaoutput");
      audioctl_ = gst_bin_new("audio-control");
      if (audioctl_ && alsaoutput_) {
        // Initialize audio bin.
        gst_bin_add_many(GST_BIN(audioctl_), panorama_, alsaoutput_, NULL);
        gst_element_link(panorama_, alsaoutput_);
        GstPad *sink_pad = gst_element_get_pad(panorama_, "sink");
        gst_element_add_pad(audioctl_, gst_ghost_pad_new("sink", sink_pad));
        gst_object_unref(GST_OBJECT(sink_pad));
      } else {
        LOG("Balance cannot be supported.");
        gst_object_unref(G_OBJECT(panorama_));
        panorama_ = NULL;
        if (alsaoutput_) {
          gst_object_unref(G_OBJECT(alsaoutput_));
          alsaoutput_ = NULL;
        }
        if (audioctl_) {
          gst_object_unref(G_OBJECT(audioctl_));
          audioctl_ = NULL;
        }
      }
    } else {
      LOG("Balance cannot be supported.");
    }

    // Watch the message bus.
    // The host using this class must use a g_main_loop to capture the
    // message in the default context.
    GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(playbin_));
    gst_bus_add_watch(bus, OnNewMessage, this);
    gst_object_unref(bus);

    // Redirect the audio output of playbin to the audio bin, and discard
    // the video output.
    if (audioctl_)
      g_object_set(G_OBJECT(playbin_), "audio-sink", audioctl_, NULL);
    g_object_set(G_OBJECT(playbin_), "video-sink", videofilter_, NULL);

    // We are ready to play.
    local_state_ = SOUND_STATE_STOPPED;

    return true;
  }

  void DestroyGstreamer() {
    if (playbin_) {
      gst_element_set_state(playbin_, GST_STATE_NULL);
      gst_object_unref(GST_OBJECT(playbin_));
      playbin_ = NULL;
    }
  }

 private:
  // Indicate whether gstreamer is initialized successfully.
  bool init_failed_;

  // Audio source to play.
  std::string src_;

  // Audio & video stream player.
  GstElement *playbin_;
  // Video filter, the video output of playbin_ goes here.
  GstElement *videofilter_;
  // Audio controller, the audio output of playbin_ goes here.
  GstElement *audioctl_;
  // Control audio balance.
  GstElement *panorama_;
  // Audio output.
  GstElement *alsaoutput_;

  // Current balance, volume, and length of the audio stream (in seconds).
  int balance_;
  int volume_;
  int duration_;

  // Audio state and the latest reported error code.
  State local_state_;
  ErrorCode local_error_;

  // Closure to be called when the state of audio player changes.
  Signal1<void, State> on_state_change_signal_;
};

#else // GST_AUDIOCLIP

/**
 * Dummy implementation.
 */
class Audioclip::Impl {
 public:
  Impl(const char *src) { }
  ~Impl() { }

  bool InitIsFailed() {
    return true;
  }

  int GetBalance() {
    return (kMaxBalance + kMinBalance) / 2;
  }

  void SetBalance(int balance) {
  }

  int GetCurrentPosition() const {
    return -1;
  }

  void SetCurrentPosition(int position) {
  }

  int GetDuration() const {
    return -1;
  }

  ErrorCode GetError() const {
    return SOUND_ERROR_NO_ERROR;
  }

  std::string GetSrc() const {
    return "";
  }

  void SetSrc(const char *src) {
  }

  State GetState() const {
    return SOUND_STATE_STOPPED;
  }

  int GetVolume() const {
    return kMinVolume;
  }

  void SetVolume(int volume) const {
  }

  void Play() {
  }

  void Pause() {
  }

  void Stop() {
  }

  Connection *ConnectOnStateChange(OnStateChangeHandler *handler) {
    return NULL;
  }
};

#endif

Audioclip::Audioclip() : impl_(new Impl(NULL)) {
}

Audioclip::Audioclip(const char *src) : impl_(new Impl(src)) {
}

Audioclip::~Audioclip() {
  delete impl_;
}

void Audioclip::Destroy() {
  delete this;
}

bool Audioclip::InitIsFailed() {
  return impl_->InitIsFailed();
}

int Audioclip::GetBalance() const {
  return impl_->GetBalance();
}

void Audioclip::SetBalance(int balance) {
  impl_->SetBalance(balance);
}

int Audioclip::GetCurrentPosition() const {
  return impl_->GetCurrentPosition();
}

void Audioclip::SetCurrentPosition(int position) {
  impl_->SetCurrentPosition(position);
}

int Audioclip::GetDuration() const {
  return impl_->GetDuration();
}

Audioclip::ErrorCode Audioclip::GetError() const {
  return impl_->GetError();
}

std::string Audioclip::GetSrc() const {
  return impl_->GetSrc();
}

void Audioclip::SetSrc(const char *src) {
  impl_->SetSrc(src);
}

Audioclip::State Audioclip::GetState() const {
  return impl_->GetState();
}

int Audioclip::GetVolume() const {
  return impl_->GetVolume();
}

void Audioclip::SetVolume(int volume) const {
  impl_->SetVolume(volume);
}

void Audioclip::Play() {
  impl_->Play();
}

void Audioclip::Pause() {
  impl_->Pause();
}

void Audioclip::Stop() {
  impl_->Stop();
}

Connection *Audioclip::ConnectOnStateChange(Audioclip::OnStateChangeHandler *handler) {
  return impl_->ConnectOnStateChange(handler);
}

} // namespace linux_os
} // namespace framework
} // namespace ggadget
