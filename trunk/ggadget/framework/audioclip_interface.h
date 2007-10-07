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

#ifndef GGADGET_FRAMEWORK_AUDIO_AUDIOCLIP_INTERFACE_H__
#define GGADGET_FRAMEWORK_AUDIO_AUDIOCLIP_INTERFACE_H__

namespace ggadget {

template <typename R, typename P1, typename P2> class Slot2;

namespace framework {

namespace audio {

/** Used for playing back audio files. */
class AudioclipInterface {
 public:
  enum State {
    STATE_ERROR,
    STATE_STOPPED,
    STATE_PLAYING,
    STATE_PAUSED,
  };

  enum ErrorCode {
    ERROR_NO_ERROR,
    ERROR_UNKNOWN,
    ERROR_BAD_CLIP_SRC,
    ERROR_FORMAT_NOT_SUPPORTED,
  };

 public:
  /**
   * Get the audio signal balance.
   * A number between -10000 ~ 10000.
   * -10000 means that only the left audio channel can be heard;
   * 10000 means that only the right audio channel can be heard.
   */
  virtual int GetBalance() const = 0;
  /**
   * Set the audio signal balance.
   * @see GetBalance().
   */
  virtual void SetBalance(int balance) = 0;
  /**
   * Get the current position within the audio clip.
   * Where 0 represents the beginning of the clip and @c duration is the
   * end + 1.
   */
  virtual int GetCurrentPosition() const = 0;
  /**
   * Set the current position within the audio clip.
   * @see GetCurrentPosition().
   */
  virtual void SetCurrentPosition() = 0;
  /**
   * The length, in seconds, of the sound.
   */
  virtual int GetDuration() const = 0;
  virtual ErrorCode GetError() const = 0;
  virtual const char *GetSrc() const = 0;
  virtual void SetSrc(const char *src) = 0;
  virtual State GetState() const = 0;
  virtual int GetVolume() const = 0;
  virtual void SetVolume(int volume) const = 0;

 public:
  virtual void Play();
  virtual void Pause();
  virtual void Stop();

 public:
  virtual Slot2<void, AudioclipInterface *, State> *
      GetOnStateChange() const = 0;
  virtual void SetOnStateChange(
      Slot2<void, AudioclipInterface *, State> *handle) = 0;
};

} // namespace audio

} // namespace framework

} // namespace ggadget

#endif // GGADGET_FRAMEWORK_AUDIO_AUDIOCLIP_INTERFACE_H__
