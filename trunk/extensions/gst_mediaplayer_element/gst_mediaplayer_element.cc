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

#include "gst_mediaplayer_element.h"

#include <gst/gst.h>
#include <gst/video/gstvideosink.h>
#include <gst/gstinfo.h>
#include <string>

#include <ggadget/basic_element.h>
#include <ggadget/element_factory.h>
#include <ggadget/logger.h>
#include <ggadget/signals.h>
#include <ggadget/slot.h>
#include <ggadget/math_utils.h>
#include <ggadget/scriptable_interface.h>
#include <ggadget/scriptable_framework.h>
#include <ggadget/registerable_interface.h>

#include <extensions/gst_mediaplayer_element/gadget_videosink.h>

#define Initialize gst_mediaplayer_element_LTX_Initialize
#define Finalize gst_mediaplayer_element_LTX_Finalize
#define RegisterElementExtension gst_mediaplayer_element_LTX_RegisterElementExtension

extern "C" {
  bool Initialize() {
    LOG("Initialize gst_mediaplayer_element extension.");
    return true;
  }

  void Finalize() {
    LOG("Finalize gst_mediaplayer_element extension.");
  }

  bool RegisterElementExtension(ggadget::ElementFactory *factory) {
    LOG("Register gst_mediaplayer_element extension.");
    if (factory) {
      // Used when the mediaplayer element is hosted by an object element.
      factory->RegisterElementClass("clsid:6BF52A52-394A-11d3-B153-00C04F79FAA6",
                                    &ggadget::gst::GstMediaPlayerElement::CreateInstance);
      factory->RegisterElementClass("progid:WMPlayer.OCX.7",
                                    &ggadget::gst::GstMediaPlayerElement::CreateInstance);
      // Used when the mediaplayer element acts as a normal element.
      factory->RegisterElementClass("_mediaplayer",
                                    &ggadget::gst::GstMediaPlayerElement::CreateInstance);
    }
    return true;
  }
}

namespace ggadget {
namespace gst {

#ifndef GGL_DEFAULT_GST_AUDIO_SINK
  #define GGL_DEFAULT_GST_AUDIO_SINK "autoaudiosink"
#endif

static const char *kGstAudioSinks[] = {
  GGL_DEFAULT_GST_AUDIO_SINK,
#if GGL_HOST_LINUX
  "alsasink",
  "osssink",
#endif
#if GGL_HOST_MACOSX
  "osxaudiosink",
#endif
#if GGL_HOST_WINDOWS
  "directsoundsink",
#endif
  NULL,
};

static double kMaxGstVolume = 10.0;

static const char *tag_strings[] = {
  GST_TAG_ARTIST,  // TAG_AUTHOR
  GST_TAG_TITLE,   // TAG_TITLE
  GST_TAG_ALBUM,   // TAG_ALBUM
  GST_TAG_DATE,    // TAG_DATE
  GST_TAG_GENRE,   // TAG_GENRE
  GST_TAG_COMMENT, // TAG_COMMENT
  NULL             // Others not supported yet.
};

GstMediaPlayerElement::GstMediaPlayerElement(BasicElement *parent,
                                             View *view,
                                             const char *name)
    : MediaPlayerElementBase(parent, view, "_mediaplayer", name, false),
      playbin_(NULL),
      receive_image_handler_(NULL),
      tag_list_(NULL),
      media_changed_(false), position_changed_(false),
      local_state_(PLAYSTATE_UNDEFINED), local_error_(MEDIA_ERROR_NO_ERROR) {

  // Initialize Gstreamer.
  gst_init(NULL, NULL);

  // Register our video sink.
  if (!GadgetVideoSink::Register())
    return;

  playbin_ = gst_element_factory_make("playbin", "player");
  videosink_ = gst_element_factory_make("gadget_videosink", "videosink");

  // Only do further initialize if playbin is created correctly.
  if (!playbin_) {
    LOG("Failed to create gstreamer playbin element.");
    return;
  }

  if (!videosink_) {
    LOG("Failed to create gadget_videosink element.");
    gst_object_unref(GST_OBJECT(playbin_));
    playbin_ = NULL;
    return;
  } else {
    g_object_get(G_OBJECT(videosink_),
                 "receive-image-handler", &receive_image_handler_, NULL);
    if (!receive_image_handler_) {
      gst_object_unref(GST_OBJECT(playbin_));
      gst_object_unref(GST_OBJECT(videosink_));
      playbin_ = NULL;
      return;
    }
  }

  // Set videosink to receive video output.
  g_object_set(G_OBJECT(playbin_), "video-sink", videosink_, NULL);

  // Create new audio sink with panorama support if possible.
  GstElement *audiosink = NULL;
  for (size_t i = 0; kGstAudioSinks[i]; ++i) {
    audiosink = gst_element_factory_make(kGstAudioSinks[i],
                                         "audiosink");
    if (audiosink) break;
  }

  if (!audiosink) {
    LOG("Failed to find a suitable gstreamer audiosink.");
    if (playbin_) gst_object_unref(GST_OBJECT(playbin_));
    playbin_ = NULL;
    return;
  }

  volume_ = gst_element_factory_make("volume", "mute");
  panorama_ = gst_element_factory_make("audiopanorama", "balance");

  // If volume or panorama is available then construct a new compound audiosink
  // with volume or panorama support.
  if (volume_ || panorama_) {
    GstElement *audiobin = gst_bin_new("audiobin");
    GstPad *sinkpad;
    if (volume_ && panorama_) {
      gst_bin_add_many(GST_BIN(audiobin), volume_, panorama_, audiosink, NULL);
      gst_element_link_many(volume_, panorama_, audiosink, NULL);
      sinkpad = gst_element_get_pad(volume_, "sink");
    } else if (volume_) {
      gst_bin_add_many(GST_BIN(audiobin), volume_, audiosink, NULL);
      gst_element_link(volume_, audiosink);
      sinkpad = gst_element_get_pad(volume_, "sink");
    } else {
      gst_bin_add_many(GST_BIN(audiobin), panorama_, audiosink, NULL);
      gst_element_link(panorama_, audiosink);
      sinkpad = gst_element_get_pad(panorama_, "sink");
    }
    gst_element_add_pad(audiobin, gst_ghost_pad_new("sink", sinkpad));
    gst_object_unref(GST_OBJECT(sinkpad));
    audiosink = audiobin;
  }

  // Set audio-sink to our new audiosink.
  g_object_set(G_OBJECT(playbin_), "audio-sink", audiosink, NULL);

  // Watch the message bus.
  // The host using this class must use a g_main_loop to capture the
  // message in the default context.
  GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(playbin_));
  gst_bus_add_watch(bus, OnNewMessage, this);
  gst_object_unref(bus);

  // We are ready to play.
  local_state_ = PLAYSTATE_STOPPED;

  // Initialize the geometry set.
  SetGeometry(static_cast<int>(GetPixelWidth()),
              static_cast<int>(GetPixelHeight()));
}

GstMediaPlayerElement::~GstMediaPlayerElement() {
  if (playbin_) {
    gst_element_set_state(playbin_, GST_STATE_NULL);
    gst_object_unref(GST_OBJECT(playbin_));
    playbin_ = NULL;
    videosink_ = NULL;
    panorama_ = NULL;
  }
  if (tag_list_) {
    gst_tag_list_free(tag_list_);
    tag_list_ = NULL;
  }
  gst_deinit();
}

BasicElement *GstMediaPlayerElement::CreateInstance(BasicElement *parent,
                                                    View *view,
                                                    const char *name) {
  return new GstMediaPlayerElement(parent, view, name);
}

bool GstMediaPlayerElement::IsAvailable(const std::string& name) {
  if (MediaPlayerElementBase::IsAvailable(name))
    return true;

  if (name.compare("currentPosition") == 0) {
    GstQuery *query;
    gboolean res;
    gboolean seekable = FALSE;

    query = gst_query_new_seeking(GST_FORMAT_TIME);
    res = gst_element_query(playbin_, query);
    if (res) {
      gst_query_parse_seeking(query, NULL, &seekable, NULL, NULL);
    }
    gst_query_unref(query);
    if (seekable == TRUE)
      return true;
  } else if (name.compare("Volume") == 0) {
    if (playbin_)
      return true;
  } else if (name.compare("Balance") == 0) {
    if (playbin_ && panorama_)
      return true;
  } else if (name.compare("Mute") == 0) {
    if (playbin_ && volume_)
      return true;
  }

  return false;
}

void GstMediaPlayerElement::Play() {
  std::string new_src_ = GetCurrentMediaUri();

  if (src_.compare(new_src_) != 0) {
    src_ = new_src_;
    media_changed_ = true;
    g_object_set(G_OBJECT(playbin_), "uri", src_.c_str(), NULL);

    // Empty the tag cache when loading a new media.
    if (tag_list_) {
      gst_tag_list_free(tag_list_);
      tag_list_ = NULL;
    }
  }

  if (playbin_ && src_.length()) {
    if (gst_element_set_state(playbin_, GST_STATE_PLAYING) ==
        GST_STATE_CHANGE_FAILURE) {
      LOG("Failed to play the media.");
    }
  } else {
    if (!playbin_)
      DLOG("Playbin was not initialized correctly.");
    else
      LOG("No media source.");
  }
}

void GstMediaPlayerElement::Pause() {
  if (playbin_ && local_state_ == PLAYSTATE_PLAYING) {
    if (gst_element_set_state(playbin_, GST_STATE_PAUSED) ==
        GST_STATE_CHANGE_FAILURE) {
      LOG("Failed to pause the media.");
    }
  }
}

void GstMediaPlayerElement::Stop() {
  if (playbin_ && local_state_ != PLAYSTATE_STOPPED) {
    if (gst_element_set_state(playbin_, GST_STATE_NULL) ==
        GST_STATE_CHANGE_FAILURE) {
      LOG("Failed to stop the media.");
    } else if (local_state_ != PLAYSTATE_ERROR) {
      // If an error has ever happened, the state of gstreamer is "PAUSED",
      // so we set it to "NULL" state above. But we don't clear the ERROR
      // sign, let it be there until gstreamer itself changes its state.

      // Playbin won't post "STATE CHANGED" message when being set to
      // "NULL" state. We make a state-change scene manually.
      local_state_ = PLAYSTATE_STOPPED;
      FireOnPlayStateChangeEvent(local_state_);
    }

    // Clear the last image frame.
    ClearImage();
  }
}

int GstMediaPlayerElement::GetCurrentPosition() {
  if (playbin_ && (local_state_ == PLAYSTATE_PLAYING ||
                   local_state_ == PLAYSTATE_PAUSED)) {
    gint64 position;
    GstFormat format = GST_FORMAT_TIME;

    if (gst_element_query_position(playbin_, &format, &position)) {
      return static_cast<int>(position / GST_SECOND);
    }
  }
  return 0;
}

void GstMediaPlayerElement::SetCurrentPosition(int position) {
  // Seek will only be successful under PAUSED or PLAYING state.
  // It's ok to check local state.
  if (local_state_ != PLAYSTATE_PLAYING &&
      local_state_ != PLAYSTATE_PAUSED)
    return;

  if (gst_element_seek(playbin_, 1.0, GST_FORMAT_TIME,
                       static_cast<GstSeekFlags>(GST_SEEK_FLAG_FLUSH |
                                                 GST_SEEK_FLAG_KEY_UNIT),
                       GST_SEEK_TYPE_SET, (gint64)position * GST_SECOND,
                       GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE) == TRUE) {
    gst_element_get_state(playbin_, NULL, NULL, 100 * GST_MSECOND);
    position_changed_ = true;
  }
}

int GstMediaPlayerElement::GetDuration() {
  if (playbin_ && local_state_ != PLAYSTATE_ERROR) {
    gint64 duration;
    GstFormat format = GST_FORMAT_TIME;
    if (gst_element_query_duration(playbin_, &format, &duration) &&
        format == GST_FORMAT_TIME)
    return static_cast<int>(duration / GST_SECOND);
  }
  return 0;
}

std::string GstMediaPlayerElement::GetTagInfo(TagType tag) {
  gchar *info;
  const char *tag_string = tag_strings[tag];
  if (tag_list_ && tag_string &&
      gst_tag_list_get_string(tag_list_, tag_string, &info)) {
    std::string s(info);
    delete info;
    return s;
  } else {
    return "";
  }
}

void GstMediaPlayerElement::SetGeometry(int width, int height) {
  if (playbin_ && videosink_) {
    g_object_set(G_OBJECT(videosink_),
                 "geometry-width", width, "geometry-height", height, NULL);
  } else {
    if (!playbin_)
      DLOG("Playbin was not initialized correctly.");
    else
      DLOG("videosink was not initialized correctly.");
  }
}

int GstMediaPlayerElement::GetVolume() {
  if (playbin_) {
    double volume;
    g_object_get(G_OBJECT(playbin_), "volume", &volume, NULL);
    int gg_volume = static_cast<int>((volume / kMaxGstVolume) *
                                      (kMaxVolume - kMinVolume) +
                                      kMinVolume);
    return Clamp(gg_volume, kMinVolume, kMaxVolume);
  }
  DLOG("Playbin was not initialized correctly.");
  return kMinVolume;
}

void GstMediaPlayerElement::SetVolume(int volume) {
  if (playbin_) {
    if (volume < kMinVolume || volume > kMaxVolume) {
      LOG("Invalid volume value, range: [%d, %d].",
          kMinVolume, kMaxVolume);
      volume = Clamp(volume, kMinVolume, kMaxVolume);
    }

    gdouble gst_volume =
      (gdouble(volume - kMinVolume) / (kMaxVolume - kMinVolume)) *
      kMaxGstVolume;

    g_object_set(G_OBJECT(playbin_), "volume", gst_volume, NULL);
  } else {
    DLOG("Playbin was not initialized correctly.");
  }
}

int GstMediaPlayerElement::GetBalance() {
  if (playbin_ && panorama_) {
    gfloat balance;
    g_object_get(G_OBJECT(panorama_), "panorama", &balance, NULL);
    int gg_balance = static_cast<int>(((balance + 1) / 2) *
                                      (kMaxBalance - kMinBalance) +
                                      kMinBalance);
    return Clamp(gg_balance, kMinBalance, kMaxBalance);
  }

  if (!playbin_)
    DLOG("Playbin was not initialized correctly.");
  else
    DLOG("Balance is not supported.");

  return (kMaxBalance + kMinBalance) / 2;
}

void GstMediaPlayerElement::SetBalance(int balance) {
  if (playbin_ && panorama_) {
    if (balance < kMinBalance || balance > kMaxBalance) {
      LOG("Invalid balance value, range: [%d, %d].",
          kMinBalance, kMaxBalance);
      balance = Clamp(balance, kMinBalance, kMaxBalance);
    }
    gfloat gst_balance =
      (gfloat(balance - kMinBalance) / (kMaxBalance - kMinBalance)) * 2 - 1;
    g_object_set(G_OBJECT(panorama_), "panorama", gst_balance, NULL);
  } else {
    if (!playbin_)
      DLOG("Playbin was not initialized correctly.");
    else
      DLOG("Balance is not supported.");
  }
}

bool GstMediaPlayerElement::GetMute() {
  if (playbin_ && volume_) {
    gboolean mute;
    g_object_get(G_OBJECT(volume_), "mute", &mute, NULL);
    return static_cast<bool>(mute);
  } else {
    if (!playbin_)
      DLOG("Playbin was not initialized correctly.");
    else
      DLOG("Mute is not supported.");
    return false;
  }
}

void GstMediaPlayerElement::SetMute(bool mute) {
  if (playbin_ && volume_) {
    g_object_set(G_OBJECT(volume_), "mute", static_cast<gboolean>(mute), NULL);
  } else {
    if (!playbin_)
      DLOG("Playbin was not initialized correctly.");
    else
      DLOG("Mute is not supported.");
  }
}

GstMediaPlayerElement::PlayState GstMediaPlayerElement::GetPlayState() {
  return local_state_;
}

GstMediaPlayerElement::ErrorCode GstMediaPlayerElement::GetErrorCode() {
  return local_error_;
}

GstMediaPlayerElement::PlayState
GstMediaPlayerElement::GstStateToLocalState(GstState state) {
  switch (state) {
    case GST_STATE_NULL:
    case GST_STATE_READY:
      return PLAYSTATE_STOPPED;
    case GST_STATE_PAUSED:
      return PLAYSTATE_PAUSED;
    case GST_STATE_PLAYING:
      return PLAYSTATE_PLAYING;
    default:
      return PLAYSTATE_ERROR;
  }
}

gboolean GstMediaPlayerElement::OnNewMessage(GstBus *bus,
                                             GstMessage *msg,
                                             gpointer data) {
  ASSERT(msg && data);
  GstMediaPlayerElement *object = static_cast<GstMediaPlayerElement*>(data);

  switch (GST_MESSAGE_TYPE(msg)) {
    case GST_MESSAGE_ERROR:
      object->OnError(msg);
      break;
    case GST_MESSAGE_EOS:
      object->OnMediaEnded();
      break;
    case GST_MESSAGE_STATE_CHANGED:
      object->OnStateChange(msg);
      break;
    case GST_MESSAGE_ELEMENT:
      object->OnElementMessage(msg);
      break;
    case GST_MESSAGE_TAG:
      object->OnTagInfo(msg);
      break;
    default:
      break;
  }

  return true;
}

void GstMediaPlayerElement::OnError(GstMessage *msg) {
  ASSERT(msg);
  GError *gerror;
  gchar *debug;

  gst_message_parse_error(msg, &gerror, &debug);
  DLOG("GstMediaPlayerElement OnError: domain=%d code=%d message=%s debug=%s",
       gerror->domain, gerror->code, gerror->message, debug);

  if (gerror->domain == GST_RESOURCE_ERROR &&
      (gerror->code == GST_RESOURCE_ERROR_NOT_FOUND ||
       gerror->code == GST_RESOURCE_ERROR_OPEN_READ ||
       gerror->code == GST_RESOURCE_ERROR_OPEN_READ_WRITE)) {
    local_error_ = MEDIA_ERROR_BAD_SRC;
  } else if (gerror->domain == GST_STREAM_ERROR &&
             (gerror->code == GST_STREAM_ERROR_NOT_IMPLEMENTED ||
              gerror->code == GST_STREAM_ERROR_TYPE_NOT_FOUND ||
              gerror->code == GST_STREAM_ERROR_WRONG_TYPE ||
              gerror->code == GST_STREAM_ERROR_CODEC_NOT_FOUND ||
              gerror->code == GST_STREAM_ERROR_FORMAT)) {
    local_error_ = MEDIA_ERROR_FORMAT_NOT_SUPPORTED;
  } else {
    local_error_ = MEDIA_ERROR_UNKNOWN;
  }

  local_state_ = PLAYSTATE_ERROR;
  FireOnPlayStateChangeEvent(local_state_);

  g_error_free(gerror);
  g_free(debug);
}

void GstMediaPlayerElement::OnMediaEnded() {
  local_state_ = PLAYSTATE_MEDIAENDED;
  FireOnPlayStateChangeEvent(local_state_);
}

void GstMediaPlayerElement::OnStateChange(GstMessage *msg) {
  ASSERT(msg);
  GstState old_state, new_state;
  PlayState state;

  gst_message_parse_state_changed(msg, &old_state, &new_state, NULL);
  state = GstStateToLocalState(new_state);

  if (state == PLAYSTATE_PLAYING) {
    // If any change-event is waiting, we invoke it here as the state of
    // the media stream actually changed.
    if (media_changed_) {
      FireOnMediaChangeEvent();
      media_changed_ = false;
    }
    if (position_changed_) {
      FireOnPositionChangeEvent();
      position_changed_ = false;
    }
  } else if (state == PLAYSTATE_ERROR) {
    media_changed_ = false;
    position_changed_ = false;
  }

  if (local_state_ != state) {
    local_state_ = state;
    FireOnPlayStateChangeEvent(state);
  }
}

void GstMediaPlayerElement::OnElementMessage(GstMessage *msg) {
  ASSERT(msg);
  if (GST_MESSAGE_SRC(msg) == reinterpret_cast<GstObject*>(videosink_)) {
    const GstStructure *structure = gst_message_get_structure(msg);
    const GValue* gvalue = gst_structure_get_value(structure,
                                                   GADGET_VIDEOSINK_MESSAGE);
    GadgetVideoSink::MessageType type =
        static_cast<GadgetVideoSink::MessageType>(g_value_get_int(gvalue));
    if (type == GadgetVideoSink::NEW_IMAGE) {
      ASSERT(receive_image_handler_);
      GadgetVideoSink::Image *image = (*receive_image_handler_)(videosink_);
      if (image) {
        PutImage(reinterpret_cast<const void*>(image->data),
                 image->x, image->y, image->w, image->h, image->stride);
      }
    }
  }
}

void GstMediaPlayerElement::OnTagInfo(GstMessage *msg) {
  ASSERT(msg);
  GstTagList *new_tag_list;

  gst_message_parse_tag(msg, &new_tag_list);
  if (new_tag_list) {
    tag_list_ = gst_tag_list_merge(tag_list_, new_tag_list,
                                   GST_TAG_MERGE_PREPEND);
  }
}

} // namespace gst
} // namespace ggadget
