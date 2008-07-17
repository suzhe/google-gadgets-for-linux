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

#include "object_videoplayer.h"

#include <vector>

#include <ggadget/basic_element.h>
#include <ggadget/canvas_interface.h>
#include <ggadget/elements.h>
#include <ggadget/element_factory.h>
#include <ggadget/file_manager_interface.h>
#include <ggadget/object_element.h>
#include <ggadget/slot.h>
#include <ggadget/video_element_base.h>

namespace ggadget {

static const char kOnStateChangeEvent[] = "PlayStateChange";
static const char kOnPositionChangeEvent[]  = "PositionChange";
static const char kOnMediaChangeEvent[]  = "MediaChange";
static const char kOnPlaylistChangeEvent[]  = "PlaylistChange";
static const char kOnPlayerDockedStateChangeEvent[]  = "PlayerDockedStateChange";

// Definition of wmplayer play state.
enum wmpState {
  wmpStateUndefined,
  wmpStateStopped,
  wmpStatePaused,
  wmpStatePlaying,
  wmpStateEnded = 8,
  wmpStateReady = 10,
};

class ObjectVideoPlayer::Impl {
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
    double GetDuration() { return duration_; }

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
    double duration_;
  };

  class Playlist : public ScriptableHelperDefault {
   public:
    DEFINE_CLASS_ID(0x209b1644318849d7, ScriptableInterface);

    // For a playlist, always starts playing from the first media, so set
    // previous and next_ to -2 and 0 respectively by default.
    Playlist(const std::string& name) : name_(name), previous_(-2), next_(0) {
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

    Media *GetPreviousMedia() {
      if (previous_ >= 0) {
        previous_--;
        next_--;
        return items_[previous_ + 1];
      } else {
        return NULL;
      }
    }

    Media *GetNextMedia() {
      if (next_ >= 0 && next_ < static_cast<int>(items_.size())) {
        previous_++;
        next_++;
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
    }

    std::string name_;
    std::vector<Media*> items_;
    int previous_, next_;
  };

  Impl(BasicElement *parent, ObjectVideoPlayer *owner, View *view)
      : owner_(owner), view_(view),
        current_media_(NULL),
        current_playlist_(NULL) {

    // Create the video element.
    // Although the ObjectVideoPlayer cannot have any children(otherwise,
    // the children will be exposed to the outside code), it can be (actually
    // must be) the parent of the video element, otherwise, video element has
    // no way to know the size of area in which the video is shown.
    video_element_ = static_cast<VideoElementBase*>(
        view_->GetElementFactory()->CreateElement("video", owner_, view_, "video"));
    if (!video_element_) {
      return;
    }
    video_element_->ConnectOnStateChangeEvent(NewSlot(this, &Impl::OnStateChange));
    video_element_->ConnectOnPositionChangeEvent(NewSlot(this, &Impl::OnPositionChange));
    video_element_->ConnectOnMediaChangeEvent(NewSlot(this, &Impl::OnMediaChange));

    controls_.RegisterProperty(
        "currentPosition",
        NewSlot(video_element_, &VideoElementBase::GetCurrentPosition),
        NewSlot(video_element_, &VideoElementBase::SetCurrentPosition));
    controls_.RegisterMethod("isAvailable", NewSlot(this, &Impl::IsAvailable));
    controls_.RegisterMethod("play", NewSlot(this, &Impl::Play));
    controls_.RegisterMethod("pause", NewSlot(this, &Impl::Pause));
    controls_.RegisterMethod("stop", NewSlot(this, &Impl::Stop));
    controls_.RegisterMethod("previous", NewSlot(this, &Impl::PlayPrevious));
    controls_.RegisterMethod("next", NewSlot(this, &Impl::PlayNext));

    settings_.RegisterMethod("isAvailable", NewSlot(this, &Impl::IsAvailable));
    settings_.RegisterProperty(
        "autoStart",
        NewSlot(video_element_, &VideoElementBase::GetAutoPlay),
        NewSlot(video_element_, &VideoElementBase::SetAutoPlay));
    settings_.RegisterProperty(
        "volume",
        NewSlot(video_element_, &VideoElementBase::GetVolume),
        NewSlot(video_element_, &VideoElementBase::SetVolume));
    settings_.RegisterProperty(
        "balance",
        NewSlot(video_element_, &VideoElementBase::GetBalance),
        NewSlot(video_element_, &VideoElementBase::SetBalance));
    settings_.RegisterProperty(
        "mute",
        NewSlot(video_element_, &VideoElementBase::GetMute),
        NewSlot(video_element_, &VideoElementBase::SetMute));
  }

  ~Impl() {
    if (video_element_)
      delete video_element_;
    if (current_media_)
      current_media_->Unref();
    if (current_playlist_)
      current_playlist_->Unref();
  }

  bool IsAvailable(const std::string &name) {
    if (name.compare("previous") == 0) {
      if (current_playlist_)
        return current_playlist_->previous_ >= 0;
      else
        return false;
    } else if (name.compare("next") == 0) {
      if (current_playlist_)
        return (current_playlist_->next_ >= 0 && current_playlist_->next_ <
                static_cast<int>(current_playlist_->items_.size()));
      else
        return false;
    } else if (name.compare("currentPosition") == 0) {
      return video_element_->IsAvailable("currentTime");
    }
    return video_element_->IsAvailable(name);
  }

  void Play() {
    if (current_media_) {
      if (current_media_->uri_.compare(video_element_->GetSrc()) != 0) {
        video_element_->Stop();
        video_element_->SetSrc(current_media_->uri_);
      }
      video_element_->Play();
    }
  }

  void Pause() {
    video_element_->Pause();
  }

  void Stop() {
    video_element_->Stop();
  }

  void PlayPrevious() {
    Media *previous;
    if (current_playlist_ &&
        (previous = current_playlist_->GetPreviousMedia())) {
      SetCurrentMedia(previous);
      if (video_element_->GetAutoPlay())
        Play();
    }
  }

  void PlayNext() {
    Media *next;
    if (current_playlist_ && (next = current_playlist_->GetNextMedia())) {
      SetCurrentMedia(next);
      if (video_element_->GetAutoPlay())
        Play();
    }
  }

  wmpState GetState() {
    VideoElementBase::State state = video_element_->GetState();
    switch (state) {
      case VideoElementBase::gddVideoStateReady:
        return wmpStateReady;
      case VideoElementBase::gddVideoStatePlaying:
        return wmpStatePlaying;
      case VideoElementBase::gddVideoStatePaused:
        return wmpStatePaused;
      case VideoElementBase::gddVideoStateStopped:
        return wmpStateStopped;
      case VideoElementBase::gddVideoStateEnded:
        return wmpStateEnded;
      default:
        return wmpStateUndefined;
    }
  }

  void OnStateChange() {
    on_state_change_event_();
    // Turn to the next video in the playlist if the current video is ended.
    if (video_element_->GetState() == VideoElementBase::gddVideoStateEnded)
      PlayNext();
  }

  void OnPositionChange() {
    on_position_change_event_();
  }

  void OnMediaChange() {
    ASSERT(impl_->current_media_);
    current_media_->duration_ = video_element_->GetDuration();
    current_media_->author_ =
        video_element_->GetTagInfo(VideoElementBase::tagAuthor);
    current_media_->title_ =
        video_element_->GetTagInfo(VideoElementBase::tagTitle);
    current_media_->album_ =
        video_element_->GetTagInfo(VideoElementBase::tagAlbum);
    on_media_change_event_();
  }

  Media *NewMedia(const char *uri) {
    if (!uri || !*uri) return NULL;

    std::string real_uri;
    if (strstr(uri, "://")) {
      real_uri = uri;
    } else if (*uri == '/') {
      real_uri = std::string("file://") + uri;
    } else {
      // It may be a relative file name under the base path of the gadget.
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
    return true;
  }

  Playlist *NewPlaylist(const char *name, const char *meta_file) {
    // We don't use any meta file for playlist. The parameter exists for
    // interface compatibility.
    if (name) {
      Playlist *new_playlist = new Playlist(name);
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
      video_element_->Stop();
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

  ObjectVideoPlayer *owner_;
  View *view_;

  // The real play backend we wrap.
  VideoElementBase *video_element_;

  NativeOwnedScriptable<UINT64_C(0x42a88e66ff444ba1)> controls_;
  NativeOwnedScriptable<UINT64_C(0xde2169669ebf4b61)> settings_;

  Media *current_media_;
  Playlist *current_playlist_;

  // Signal events
  EventSignal on_state_change_event_;
  EventSignal on_position_change_event_;
  EventSignal on_media_change_event_;
  EventSignal on_playlist_change_event_;
  EventSignal on_player_docked_state_change_event_;
};

ObjectVideoPlayer::ObjectVideoPlayer(BasicElement *parent, View *view,
                                     const char *tag_name, const char *name,
                                     bool children)
    : BasicElement(parent, view, tag_name, name, children) {
  if (!parent || !parent->IsInstanceOf(ObjectElement::CLASS_ID)) {
    LOGE("Video player object can only be used with (i.e. hosted by) object"
         "element.");
    return;
  }

  impl_ = new Impl(parent, this, view);
  if (!impl_->video_element_)
    return;

  // We must call DoRegister here so that the object element can know
  // which properties we have before it can create us.
  // Also, we should set our default relative size, otherwise the object
  // element doesn't know our size.
  DoRegister();
  SetRelativeX(0);
  SetRelativeY(0);
  SetRelativeWidth(1.0);
  SetRelativeHeight(1.0);
}

ObjectVideoPlayer::~ObjectVideoPlayer() {
  delete impl_;
}

BasicElement *ObjectVideoPlayer::CreateInstance(BasicElement *parent,
                                                View *view,
                                                const char *name) {
  ObjectVideoPlayer *self = new ObjectVideoPlayer(parent, view,
                                                  "object", name, false);
  if (!self->impl_->video_element_) {
    delete self;
    return NULL;
  }
  return self;
}

void ObjectVideoPlayer::Layout() {
  BasicElement::Layout();
  if (impl_->video_element_) {
    impl_->video_element_->Layout();
  }
}

void ObjectVideoPlayer::DoRegister() {
  // Don't register properties inherited from BasicElement.
  // These properties are exposed to the outside code by the object element.

  RegisterConstant("controls", &impl_->controls_);
  RegisterConstant("settings", &impl_->settings_);
  RegisterProperty("currentMedia",
                   NewSlot(impl_, &Impl::GetCurrentMedia),
                   NewSlot(impl_, &Impl::SetCurrentMedia));
  RegisterProperty("currentPlaylist",
                   NewSlot(impl_, &Impl::GetCurrentPlaylist),
                   NewSlot(impl_, &Impl::SetCurrentPlaylist));
  RegisterProperty("playState",
                   NewSlot(impl_, &Impl::GetState),
                   NULL);

  RegisterMethod("close", NewSlot(impl_, &Impl::CloseCurrentPlaylist));
  RegisterMethod("newMedia", NewSlot(impl_, &Impl::NewMedia));
  RegisterMethod("newPlaylist", NewSlot(impl_, &Impl::NewPlaylist));

  BasicElement *parent = GetParentElement();
  parent->RegisterSignal(kOnStateChangeEvent,
                         &impl_->on_state_change_event_);
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

void ObjectVideoPlayer::DoDraw(CanvasInterface *canvas) {
  if (canvas && impl_->video_element_) {
    impl_->video_element_->Draw(canvas);
  }
}

} // namespace ggadget
