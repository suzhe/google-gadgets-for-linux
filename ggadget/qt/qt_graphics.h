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

#ifndef GGADGET_QT_QT_GRAPHICS_H__
#define GGADGET_QT_QT_GRAPHICS_H__

#include <cmath>
#include <ggadget/common.h>
#include <ggadget/graphics_interface.h>
#include <ggadget/signals.h>
#include <ggadget/slot.h>

namespace ggadget {
namespace qt {

class QtGraphics : public GraphicsInterface {
 public:
  /**
   * Constructs a QtGraphics object
   * @param zoom The zoom level for all new canvases.
   */
  QtGraphics(double zoom);
  virtual ~QtGraphics();

  double GetZoom() const;

  void SetZoom(double zoom);

  Connection *ConnectOnZoom(Slot1<void, double> *slot) const;

 public:
  virtual CanvasInterface *NewCanvas(double w, double h) const;

  virtual ImageInterface *NewImage(const std::string &tag,
                                   const std::string &data,
                                   bool is_mask) const;

  virtual FontInterface *NewFont(const std::string &family,
                                 double pt_size,
                                 FontInterface::Style style,
                                 FontInterface::Weight weight) const;

 private:
  class Impl;
  Impl *impl_;

  DISALLOW_EVIL_CONSTRUCTORS(QtGraphics);
};

inline int D2I(double d) {
  return static_cast<int>(round(d));
}
} // namespace qt
} // namespace ggadget

#endif // GGADGET_QT_QT_GRAPHICS_H__
