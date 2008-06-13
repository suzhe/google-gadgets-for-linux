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

#include "mediaplayer_element_base.h"

#include <vector>

#include <ggadget/file_manager_interface.h>
#include <ggadget/canvas_interface.h>
#include <ggadget/elements.h>
#include <ggadget/basic_element.h>
#include <ggadget/object_element.h>
#include <ggadget/element_factory.h>
#include <ggadget/slot.h>

namespace ggadget {

const char kOnPlayStateChangeEvent[] = "PlayStateChange";
const char kOnPositionChangeEvent[]  = "PositionChange";
const char kOnMediaChangeEvent[]  = "MediaChange";
const char kOnPlaylistChangeEvent[]  = "PlaylistChange";
const char kOnPlayerDockedStateChangeEvent[]  = "PlayerDockedStateChange";

class MediaPlayerElementBase::Impl {
 public:
  class Media : public ScriptableHelperDefault {
   public:
    DEFINE_CLASS_ID(0x72d10c43fea34b38, ScriptableInterface);

    Media(const std::string& uri) : uri_(uri), duration_(0) {
      name_ = uri.substr(uri.find_last_of('/') + 1);
      size_t n = name_.find_last_of('.');
      if (n != std::string::npos)
        name_ = name_.substr(0, n);

      RegisterProperty("name",
                       NewSlot(this, &Media::GetName),
                       NewSlot(this, &Media::SetName));
      RegisterProperty("sourceURL", NewSlot(this, &Media::GetUri), NULL);
      RegisterProperty("duration", NewSlot(this, &Media::GetDuration), NULL);

      RegisterMethod("getItemInfo", NewSlot(this, &Media::GetItemInfo));
      RegisterMethod("setItemInfo", NewSlot(this, &Media::SetItemInfo));
      RegisterMethod("isReadOnlyItem", NewSlot(this, &Media::IsReadOnlyItem));
    }

    ~Media() { }

    // Gets and sets of scriptable properties.
    const std::string& GetName() { return name_; }
    void SetName(const std::string& name) { name_ = name; }
    const std::string& GetUri() { return uri_; }
    int GetDuration() { return duration_; }

    std::string GetItemInfo(const std::string& attr) {
      if (attr.compare("Author") == 0)
        return author_;
      else if (attr.compare("Title") == 0)
        return title_;
      else if (attr.compare("WM/AlbumTitle") == 0)
        return album_;
      else
        return "";
    }

    void SetItemInfo(const std::string& attr, const std::string& value) {
      // Currently, users are not allowed to modify the tag info.
    }

    bool IsReadOnlyItem(const std::string& attr) {
      return true;
    }

    std::string uri_, name_;
    std::string author_, title_, album_;
    int duration_;
  };

  class Playlist : public ScriptableHelperDefault {
   public:
    DEFINE_CLASS_ID(0x209b1644318849d7, ScriptableInterface);

    Playlist(const std::string& name) : name_(name), previous_(-2), next_(-1) {
      RegisterProperty("count", NewSlot(this, &Playlist::GetCount), NULL);
      RegisterProperty("name",
                       NewSlot(this, &Playlist::GetName),
                       NewSlot(this, &Playlist::SetName));
      RegisterMethod("appendItem", NewSlot(this, &Playlist::AppendItem));
    }

    ~Playlist() {
      if (items_.size()) {
        for (size_t i = 0; i < items_.size(); i++)
          // Don't use delete, the media may be also being used by others.
          items_[i]->Unref();
      }
    }

    bool HasPreviousMedia() {
      return previous_ >= 0;
    }

    bool HasNextMedia() {
      return next_ >= 0 && static_cast<size_t>(next_) < items_.size();
    }

    Media *GetPreviousMedia() {
      if (HasPreviousMedia()) {
        previous_--;
        next_--;
        return items_[previous_ + 1];
      } else {
        return NULL;
      }
    }

    Media *GetNextMedia() {
      if (HasNextMedia()) {
        next_++;
        previous_++;
        return items_[next_ - 1];
      } else {
        return NULL;
      }
    }

    size_t GetCount() { return items_.size(); }
    const std::string& GetName() { return name_; }
    void SetName(const std::string &name) { name_ = name; }

    void AppendItem(Media *media) {
      media->Ref();
      items_.push_back(media);
      if (next_ == -1)
        next_ = 0;
    }

    std::string name_;
    std::vector<Media*> items_;
    int previous_, next_;
  };


  Impl(BasicElement *parent, MediaPlayerElementBase *owner, View *view)
      : owner_(owner), view_(view),
        auto_start_(true),
        position_changed_(false),
        media_changed_(false),
        current_media_(NULL),
        current_playlist_(NULL),
        image_data_(NULL),
        image_x_(0), image_y_(0), image_w_(0), image_h_(0),
        image_stride_(0) {
    controls_.RegisterMethod(
        "isAvailable", NewSlot(owner_, &MediaPlayerElementBase::IsAvailable));
    controls_.RegisterMethod(
        "play", NewSlot(owner_, &MediaPlayerElementBase::Play));
    controls_.RegisterMethod(
        "pause", NewSlot(owner_, &MediaPlayerElementBase::Pause));
    controls_.RegisterMethod(
        "stop", NewSlot(owner_, &MediaPlayerElementBase::Stop));
    controls_.RegisterMethod(
        "previous",
        NewSlot(this, &MediaPlayerElementBase::Impl::PlayPreviousMedia));
    controls_.RegisterMethod(
        "next",
        NewSlot(this, &MediaPlayerElementBase::Impl::PlayNextMedia));
    controls_.RegisterProperty(
        "currentPosition",
        NewSlot(owner_, &MediaPlayerElementBase::GetCurrentPosition),
        NewSlot(owner_, &MediaPlayerElementBase::SetCurrentPosition));

    settings_.RegisterMethod(
        "isAvailable",
        NewSlot(owner_, &MediaPlayerElementBase::IsAvailable));
    settings_.RegisterProperty(
        "autoStart",
        NewSlot(this, &MediaPlayerElementBase::Impl::GetAutoStart),
        NewSlot(this, &MediaPlayerElementBase::Impl::SetAutoStart));
    settings_.RegisterProperty(
        "volume",
        NewSlot(owner_, &MediaPlayerElementBase::GetVolume),
        NewSlot(owner_, &MediaPlayerElementBase::SetVolume));
    settings_.RegisterProperty(
        "balance",
        NewSlot(owner_, &MediaPlayerElementBase::GetBalance),
        NewSlot(owner_, &MediaPlayerElementBase::SetBalance));
    settings_.RegisterProperty(
        "mute",
        NewSlot(owner_, &MediaPlayerElementBase::GetMute),
        NewSlot(owner_, &MediaPlayerElementBase::SetMute));
  }

  ~Impl() {
    if (current_media_)
      current_media_->Unref();
    if (current_playlist_)
      current_playlist_->Unref();
  }

  Media *NewMedia(const char *uri) {
    if (!uri) return NULL;

    std::string real_uri;
    if (strstr(uri, "://")) {
      real_uri = uri;
    } else if (*uri == '/') {
      real_uri = std::string("file://") + uri;
    } else {
      // src may be a relative file name under the base path of the gadget.
      std::string extracted_file;
      if (!view_->GetFileManager()->ExtractFile(uri, &extracted_file))
        return NULL;
      real_uri = "file://" + extracted_file;
    }
    return new Media(real_uri);
  }

  Media *GetCurrentMedia() {
    return current_media_;
  }

  bool SetCurrentMedia(Media *media) {
    if (!media || current_media_ == media)
      return false;
    if (current_media_)
      CloseCurrentMedia();
    current_media_ = media;
    current_media_->Ref();
    media_changed_ = true;
    if (auto_start_)
      owner_->Play();
    return true;
  }

  Playlist *NewPlaylist(const char *name, const char *url) {
    if (name && url) {
      Playlist *new_playlist = new Playlist(name);
      if (strcmp(url, "") != 0) {
        Media *media = NewMedia(url);
        new_playlist->AppendItem(media);
      }
      return new_playlist;
    }
    return NULL;
  }

  Playlist *GetCurrentPlaylist() {
    return current_playlist_;
  }

  bool SetCurrentPlaylist(Playlist *playlist) {
    if (!playlist || current_playlist_ == playlist)
      return false;
    if (current_playlist_)
      CloseCurrentPlaylist();
    current_playlist_ = playlist;
    current_playlist_->Ref();
    on_playlist_change_event_();
    return SetCurrentMedia(current_playlist_->GetNextMedia());
  }

  void CloseCurrentMedia() {
    if (current_media_) {
      owner_->Stop();
      current_media_->Unref();
      current_media_ = NULL;
    }
  }

  void CloseCurrentPlaylist() {
    if (current_playlist_) {
      CloseCurrentMedia();
      current_playlist_->Unref();
      current_playlist_ = NULL;
    }
  }

  void Close() {
    CloseCurrentPlaylist();
  }

  bool PlayPreviousMedia() {
    if (current_playlist_) {
      return SetCurrentMedia(current_playlist_->GetPreviousMedia());
    }
    return false;
  }

  bool PlayNextMedia() {
    if (current_playlist_) {
      return SetCurrentMedia(current_playlist_->GetNextMedia());
    }
    return false;
  }

  bool GetAutoStart() {
    return auto_start_;
  }

  void SetAutoStart(bool auto_start) {
    auto_start_ = auto_start;
  }

  // Not supported currently.
  std::string GetWmpServiceType() {
    return "local";
  }

  void SetWmpServiceType(const std::string& type) {
  }

  std::string GetWmpSkin() {
    return "";
  }

  void SetWmpSkin(const std::string& skin) {
  }

  bool GetEnableContextMenu() {
    return false;
  }

  void SetEnableContextMenu(bool enable) {
  }

  bool GetEnableErrorDialogs() {
    return false;
  }

  void SetEnableErrorDialogs(bool enable) {
  }

  std::string GetUIMode() {
    return "full";
  }

  void SetUIMode(const std::string& uimode) {
  }

  MediaPlayerElementBase *owner_;
  View *view_;

  NativeOwnedScriptable controls_;
  NativeOwnedScriptable settings_;

  // Platform-independent settings.
  bool auto_start_;

  bool position_changed_;
  bool media_changed_;
  Media *current_media_;
  Playlist *current_playlist_;

  // Informations needed to show an image.
  const char *image_data_;
  int image_x_;
  int image_y_;
  int image_w_;
  int image_h_;
  int image_stride_;

  // Signal events
  EventSignal on_play_state_change_event_;
  EventSignal on_position_change_event_;
  EventSignal on_media_change_event_;
  EventSignal on_playlist_change_event_;
  EventSignal on_player_docked_state_change_event_;
};

MediaPlayerElementBase::MediaPlayerElementBase(BasicElement *parent, View *view,
                                               const char *tag_name,
                                               const char *name,
                                               bool children)
    : BasicElement(parent, view, tag_name, name, children),
      impl_(new Impl(parent, this, view)) {
  // If the parent is object-element, we must call DoRegister here so that
  // object-element can know which properties we have before it can create us.
  // Also, we should set our default relative size, otherwise the object
  // element doesn't know our size.
  if (parent && parent->IsInstanceOf(ObjectElement::CLASS_ID)) {
    DoRegister();

    SetRelativeX(0);
    SetRelativeY(0);
    SetRelativeWidth(1.0);
    SetRelativeHeight(1.0);
  }
}

MediaPlayerElementBase::~MediaPlayerElementBase() {
  delete impl_;
}

void MediaPlayerElementBase::DoRegister() {
  RegisterConstant("controls", &impl_->controls_);
  RegisterConstant("settings", &impl_->settings_);
  RegisterProperty("currentMedia",
                   NewSlot(impl_, &Impl::GetCurrentMedia),
                   NewSlot(impl_, &Impl::SetCurrentMedia));
  RegisterProperty("currentPlaylist",
                   NewSlot(impl_, &Impl::GetCurrentPlaylist),
                   NewSlot(impl_, &Impl::SetCurrentPlaylist));
  RegisterProperty("playState",
                   NewSlot(this, &MediaPlayerElementBase::GetPlayState),
                   NULL);

  RegisterMethod("close", NewSlot(impl_, &Impl::Close));
  RegisterMethod("newMedia", NewSlot(impl_, &Impl::NewMedia));
  RegisterMethod("newPlaylist", NewSlot(impl_, &Impl::NewPlaylist));

  BasicElement *parent = GetParentElement();
  if (!parent || !parent->IsInstanceOf(ObjectElement::CLASS_ID)) {
    // If the parent is not an object-element, it means this is an independent
    // element rather than an object hosted by an object-element.  All the
    // basic-element's properties should be registered in this case.
    BasicElement::DoRegister();

    // Register the following signals and properties to ourselves.
    parent = this;
  }
  parent->RegisterSignal(kOnPlayStateChangeEvent,
                         &impl_->on_play_state_change_event_);
  parent->RegisterSignal(kOnPositionChangeEvent,
                         &impl_->on_position_change_event_);
  parent->RegisterSignal(kOnMediaChangeEvent,
                         &impl_->on_media_change_event_);

  parent->RegisterSignal(kOnPlaylistChangeEvent,
                         &impl_->on_playlist_change_event_);
  parent->RegisterSignal(kOnPlayerDockedStateChangeEvent,
                         &impl_->on_player_docked_state_change_event_);
  parent->RegisterProperty("wmpServiceType",
                           NewSlot(impl_, &Impl::GetWmpServiceType),
                           NewSlot(impl_, &Impl::SetWmpServiceType));
  parent->RegisterProperty("wmpSkin",
                           NewSlot(impl_, &Impl::GetWmpSkin),
                           NewSlot(impl_, &Impl::SetWmpSkin));
  RegisterProperty("enableContextMenu",
                   NewSlot(impl_, &Impl::GetEnableContextMenu),
                   NewSlot(impl_, &Impl::SetEnableContextMenu));
  RegisterProperty("enableErrorDialogs",
                   NewSlot(impl_, &Impl::GetEnableErrorDialogs),
                   NewSlot(impl_, &Impl::SetEnableErrorDialogs));
  RegisterProperty("uiMode",
                   NewSlot(impl_, &Impl::GetUIMode),
                   NewSlot(impl_, &Impl::SetUIMode));
}

void MediaPlayerElementBase::DoDraw(CanvasInterface *canvas) {
  if (canvas && impl_->image_data_) {
    canvas->DrawRawImage(GetPixelX() + impl_->image_x_,
                         GetPixelY() + impl_->image_y_,
                         impl_->image_data_,
                         CanvasInterface::RAWIMAGE_FORMAT_RGB24,
                         impl_->image_w_, impl_->image_h_,
                         impl_->image_stride_);
  }

  if (IsSizeChanged()) {
    SetGeometry(static_cast<int>(GetPixelWidth()),
                static_cast<int>(GetPixelHeight()));
  }
}

bool MediaPlayerElementBase::IsAvailable(const std::string& name) {
  if (name.compare("currentItem") == 0) {
    if (impl_->current_media_)
      return true;
  } else if (name.compare("next") == 0) {
    if (impl_->current_playlist_ &&
        impl_->current_playlist_->HasNextMedia())
      return true;
  } else if (name.compare("previous") == 0) {
    if (impl_->current_playlist_ &&
        impl_->current_playlist_->HasPreviousMedia())
      return true;
  } else if (name.compare("pause") == 0) {
    PlayState state = GetPlayState();
    if (impl_->current_media_ != NULL &&
        (state == PLAYSTATE_PLAYING ||
         state == PLAYSTATE_SCANFWD ||
         state == PLAYSTATE_SCANREV))
      return true;
  } else if (name.compare("play") == 0) {
    PlayState state = GetPlayState();
    if (impl_->current_media_ != NULL &&
        (state == PLAYSTATE_STOPPED ||
         state == PLAYSTATE_PAUSED ||
         state == PLAYSTATE_SCANFWD ||
         state == PLAYSTATE_SCANREV ||
         state == PLAYSTATE_MEDIAENDED ||
         state == PLAYSTATE_READY))
      return true;
  } else if (name.compare("stop") == 0) {
    PlayState state = GetPlayState();
    if (impl_->current_media_ != NULL &&
        state != PLAYSTATE_UNDEFINED &&
        state != PLAYSTATE_STOPPED &&
        state != PLAYSTATE_ERROR)
      return true;
  } else if (name.compare("AutoStart") == 0) {
    return true;
  }

  return false;
}

std::string MediaPlayerElementBase::GetCurrentMediaUri() {
  if (impl_->current_media_) {
    return impl_->current_media_->uri_;
  }
  return "";
}

bool MediaPlayerElementBase::PutImage(const void *data,
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

void MediaPlayerElementBase::ClearImage() {
  impl_->image_data_ = NULL;
  QueueDraw();
}

void MediaPlayerElementBase::FireOnPlayStateChangeEvent(PlayState state) {
  impl_->on_play_state_change_event_();
  switch (state) {
    case PLAYSTATE_MEDIAENDED:
      // MEDIAENDED doesn't mean stopping. For example, gstreamer may send out
      // this message in PLAYING state. So if we fail to play the next media,
      // we should stop the current media.
      if (!impl_->PlayNextMedia())
        Stop();
      break;
    case PLAYSTATE_ERROR:
      impl_->CloseCurrentMedia();
      break;
    default:
      break;
  }
}

void MediaPlayerElementBase::FireOnPositionChangeEvent() {
  impl_->on_position_change_event_();
}

void MediaPlayerElementBase::FireOnMediaChangeEvent() {
  ASSERT(impl_->current_media_);
  impl_->current_media_->duration_ = GetDuration();
  impl_->current_media_->author_ = GetTagInfo(TAG_AUTHOR);
  impl_->current_media_->title_ = GetTagInfo(TAG_TITLE);
  impl_->current_media_->album_ = GetTagInfo(TAG_ALBUM);
  impl_->on_media_change_event_();
}

} // namespace ggadget
