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

#ifndef GGADGET_AUDIOCLIP_H__
#define GGADGET_AUDIOCLIP_H__

#include <string>

#include <ggadget/audioclip_interface.h>
#include <ggadget/slot.h>
#include <ggadget/signals.h>

namespace ggadget {
namespace framework {
namespace linux_os {

/**
 * Linux implementation of @c Audioclip for playing back audio files.
 * Users who is to use this class should be single-threaded for safty,
 * and must run in the default g_main_loop context.
 */
class Audioclip : public AudioclipInterface {
 public:
  Audioclip();
  Audioclip(const char *src);
  virtual ~Audioclip();
  virtual void Destroy();

  /**
   * Get the result of initialization of this audioclip. Should be called
   * before any further operations on the audioclip.
   */
  bool InitIsFailed();

  virtual int GetBalance() const;
  virtual void SetBalance(int balance);
  virtual int GetCurrentPosition() const;
  virtual void SetCurrentPosition(int position);
  virtual int GetDuration() const;
  virtual ErrorCode GetError() const;
  virtual std::string GetSrc() const;
  virtual void SetSrc(const char *src);
  virtual State GetState() const;
  virtual int GetVolume() const;
  virtual void SetVolume(int volume) const;

 public:
  virtual void Play();
  virtual void Pause();
  virtual void Stop();

 public:
  typedef Slot1<void, State> OnStateChangeHandler;
  virtual Connection *ConnectOnStateChange(OnStateChangeHandler *handler);

 private:
  // The following constructors are not allowed
  Audioclip(const Audioclip& audioclip);
  void operator=(const Audioclip& audioclip);

  class Impl;
  Impl *impl_;
};

} // namespace linux_os
} // namespace framework
} // namespace ggadget

#endif // GGADGET_AUDIOCLIP_H__
