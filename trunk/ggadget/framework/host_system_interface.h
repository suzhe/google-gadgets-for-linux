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

#ifndef GGADGET_FRAMEWORK_HOST_SYSTEM_INTERFACE_H__
#define GGADGET_FRAMEWORK_HOST_SYSTEM_INTERFACE_H__

namespace ggadget {

struct Point {
  Point(int px, int py) : x(px), y(py) {}
  int x;
  int y;
} __attribute__((packed));

struct Size {
  Size(int w, int h) : width(w), height(h) {}
  int width;
  int height;
} __attribute__((packed));

class ImageInterface;
class StreamInterface;

namespace framework {

namespace audio {

class AudioclipInterface;

} // namespace audio

/** Interface for enumerating the files. */
class FilesInterface {
 public:
  virtual void Destroy() = 0;

 public:
  /** Get the number of files. */
  virtual int GetCount() const = 0;
  /**
   * Get the file name according to the index.
   * The caller should not free the pointer this method returned,
   * and the returned pointer may be freed next time when calling to the
   * method in some implementations.
   */
  virtual const char *GetItem(int index) = 0;
};

/**
 * The interface containing methods which should be implemented by the host.
 */
class HostSystemInterface {
 public:
  /**
   * Displays the standard browse for file dialog and returns the name.
   * @param filter in the form "Display Name|List of Types", and multiple
   *     entries can be added to it. For example:
   *     "Music Files|*.mp3;*.wma|All Files|*.*".
   * @return the selected file or an empty string if the dialog is cancelled.
   *     The caller should not free the pointer it returned.
   */
  virtual const char *BrowseForFile(const char *filter) const = 0;
  /**
   * Displays the standard browse for file dialog and returns a collection
   * containing the names of the selected files.
   * @param filter in the form "Display Name|List of Types", and multiple
   *     entries can be added to it. For example:
   *     "Music Files|*.mp3;*.wma|All Files|*.*".
   * @return the selected files or an empty collection if the dialog is
   *     cancelled. The caller should call @c Destroy() to the returned
   *     pointer after use.
   */
  virtual FilesInterface *BrowseForFiles(const char *filter) const = 0;
  /** Load an image from the given file. */
  virtual ImageInterface *LoadImageFromFile(const char *src) const = 0;
  /** Load an image from the stream. */
  virtual ImageInterface *LoadImageFromStream(StreamInterface *stream) = 0;
  /** Retrieves the position of the cursor. */
  virtual Point GetCursorPos() const = 0;
  /** Retrieves the screen size. */
  virtual Size GetSize() const = 0;
  /** Returns the path to the icon associated with the specified file. */
  virtual const char *GetFileIcon(const char *filename) const = 0;
  /** Creates an audio clip from given file. */
  virtual audio::AudioclipInterface *CreateAudioclip(
      const char *filename) const = 0;
};

} // namespace framework

} // namespace ggadget

#endif // GGADGET_FRAMEWORK_HOST_SYSTEM_INTERFACE_H__
