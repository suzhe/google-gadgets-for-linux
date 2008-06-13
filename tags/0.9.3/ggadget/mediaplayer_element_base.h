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

#ifndef GGADGET_MEDIAPLAYER_ELEMENT_BASE_H__
#define GGADGET_MEDIAPLAYER_ELEMENT_BASE_H__

#include <string>

#include <ggadget/view.h>
#include <ggadget/signals.h>
#include <ggadget/basic_element.h>

namespace ggadget {

/**
 * This class can be used as either an independent element or an object wrapped
 * in the object element which is for the compatability with gdwin.
 *
 * When it's wrapped in an object element, mediaplayer element won't register
 * those properties that a basic element has. Instead, those properties is
 * provided by the object element.
 *
 * This class is platform-independent. Real mediaplayer element should inherit
 * from this class, and implement all the pure virtual functions.
 */
class MediaPlayerElementBase : public BasicElement {
 public:
  DEFINE_CLASS_ID(0x7C5D2E793806427F, BasicElement);

  enum PlayState {
    PLAYSTATE_UNDEFINED,
    PLAYSTATE_STOPPED,
    PLAYSTATE_PAUSED,
    PLAYSTATE_PLAYING,
    PLAYSTATE_SCANFWD,
    PLAYSTATE_SCANREV,
    PLAYSTATE_BUFFERING,
    PLAYSTATE_WAITING,
    PLAYSTATE_MEDIAENDED,
    PLAYSTATE_TRANSITIONING,
    PLAYSTATE_READY,
    PLAYSTATE_RECONNECTING,
    PLAYSTATE_ERROR
  };

  enum TagType {
    TAG_AUTHOR,
    TAG_TITLE,
    TAG_ALBUM,
    TAG_DATE,
    TAG_GENRE,
    TAG_COMMENT
  };

  enum ErrorCode {
    MEDIA_ERROR_NO_ERROR,
    MEDIA_ERROR_UNKNOWN,
    MEDIA_ERROR_BAD_SRC,
    MEDIA_ERROR_FORMAT_NOT_SUPPORTED,
  };

  /** Ranges of balance and volume. */
  static const int  kMinBalance = -100;
  static const int  kMaxBalance = 100;
  static const int  kMinVolume = 0;
  static const int  kMaxVolume = 100;

 public:
  MediaPlayerElementBase(BasicElement *parent, View *view,
                         const char *tag_name, const char *name,
                         bool children);
  virtual ~MediaPlayerElementBase();

  /** Check whether an action (or a property) @a name can be taken (or set). */
  virtual bool IsAvailable(const std::string& name);

  /** Controls. */
  virtual void Play() = 0;
  virtual void Pause() = 0;
  virtual void Stop() = 0;

  /** Getters and setters. */
  virtual int GetCurrentPosition() = 0;
  virtual void SetCurrentPosition(int position) = 0;

  virtual int GetDuration() = 0;
  virtual std::string GetTagInfo(TagType tag) = 0;

  virtual void SetGeometry(int width, int height) = 0;

  virtual int GetVolume() = 0;
  virtual void SetVolume(int volume) = 0;

  virtual int GetBalance() = 0;
  virtual void SetBalance(int balance) = 0;

  virtual bool GetMute() = 0;
  virtual void SetMute(bool mute) = 0;

  virtual PlayState GetPlayState() = 0;
  virtual ErrorCode GetErrorCode() = 0;

 protected:
  /**
   * Register properties, methods, and signals. The real mediaplayer element
   * doesn't need to do any registation, and should never call this function.
   */
  virtual void DoRegister();

  /**
   * Draw a video frame on the canvas @canvas.
   * The real mediaplayer element should call PutImage to pass in the
   * metadata of an image frame that is ready to be shown. PutImage will do
   * a queue draw, and finally this function will be scheduled to actually
   * show the frame.
   */
  virtual void DoDraw(CanvasInterface *canvas);

  /**
   * Get the uri of current media that is to be played or being played.
   * The real mediaplayer element should call this function in the Play
   * function to get the current media source.
   */
  std::string GetCurrentMediaUri();

  /**
   * Called by the real mediaplayer element to pass in the next video frame. It
   * will do a queuedraw.
   * @param data image buffer with format RGB24.
   * @param x and y start position to draw the image.
   * @param w width of the image.
   * @param h height of the image.
   * @param stride bytes per line for the image buffer (with pads).
   * @see DoDraw.
   * */
  bool PutImage(const void *data, int x, int y, int w, int h, int stride);

  /**
   * Called by the real mediaplayer element to clear the last image frame
   * so that it won't be shown any more. Always be called when a playing
   * media is stopped.
   */
  void ClearImage();

  /**
   * The real mediaplayer element should fire these calls when corresponding
   * events occurs.
   */
  void FireOnPlayStateChangeEvent(PlayState state);
  void FireOnPositionChangeEvent();
  void FireOnMediaChangeEvent();

 private:
  DISALLOW_EVIL_CONSTRUCTORS(MediaPlayerElementBase);

  class Impl;
  Impl *impl_;
};

} // namespace ggadget

#endif // GGADGET_MEDIAPLAYER_ELEMENT_BASE_H__
