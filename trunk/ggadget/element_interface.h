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

#ifndef GGADGET_ELEMENT_INTERFACE_H__
#define GGADGET_ELEMENT_INTERFACE_H__

#include <ggadget/event.h>
#include <ggadget/scriptable_interface.h>

namespace ggadget {

class ElementsInterface;
class ViewInterface;

/**
 * ElementInterface defines the properties, methods and events exposed on all
 * elements, with specific elements inheriting what's defined here. When a
 * method is said to return an @c element, that means an element defined in the
 * gadget's XML definition, descended from @c BasicElement.
 */
class ElementInterface : public ScriptableInterface {
 public:
  // TODO: If need this work, perhaps we should define it in ViewHostInterface. 
  enum HitTest {
    HT_DEFAULT,
    HT_TRANSPARENT,
    HT_NOWHERE,
    HT_CLIENT,
    HT_CAPTION,
    HT_SYSMENU,
    HT_SIZE,
    HT_MENU,
    HT_HSCROLL,
    HT_VSCROLL,
    HT_MINBUTTON,
    HT_MAXBUTTON,
    HT_LEFT,
    HT_RIGHT,
    HT_TOP,
    HT_TOPLEFT,
    HT_TOPRIGHT,
    HT_BOTTOM,
    HT_BOTTOMLEFT,
    HT_BOTTOMRIGHT,
    HT_BORDER,
    HT_OBJECT,
    HT_CLOSE,
    HT_HELP
  };

 public:
  CLASS_ID_DECL(0xe863ac4167fa4bba);

 protected:
  virtual ~ElementInterface() { }

 public:
  /** Get the type of the current object. */
  virtual std::string GetTagName() const = 0;

  /** Retrieves the name of the element.  */
  virtual std::string GetName() const = 0;

  /**
   * Retrieves a collection that contains the immediate children of this
   * element.
   */
  virtual const ElementsInterface *GetChildren() const = 0;
  /**
   * Retrieves a collection that contains the immediate children of this
   * element.
   */
  virtual ElementsInterface *GetChildren() = 0;

  /**
   * Retrieves the parent element.
   * @return the pointer to the parent, or @c NULL if the parent is @c View.
   */
  virtual ElementInterface *GetParentElement() = 0;
  /**
   * Retrieves the parent element.
   * @return the pointer to the parent, or @c NULL if the parent is @c View.
   */
  virtual const ElementInterface *GetParentElement() const = 0;

 public:
  /** Retrieves the width in pixels. */
  virtual double GetPixelWidth() const = 0;
  /** Sets the width in pixels. */
  virtual void SetPixelWidth(double width) = 0;
  /** Retrieves the height in pixels. */
  virtual double GetPixelHeight() const = 0;
  /** Sets the height in pixels. */
  virtual void SetPixelHeight(double height) = 0;

  /** Retrieves the width in relative related to the parent. */
  virtual double GetRelativeWidth() const = 0;
  /** Sets the width in relative related to the parent. */
  virtual void SetRelativeWidth(double width) = 0;
  /** Retrieves the height in relative related to the parent. */
  virtual double GetRelativeHeight() const = 0;
  /** Sets the height in relative related to the parent. */
  virtual void SetRelativeHeight(double height) = 0;

  /** Retrieves the horizontal position in pixelds. */
  virtual double GetPixelX() const = 0;
  /** Sets the horizontal position in pixels. */
  virtual void SetPixelX(double x) = 0;
  /** Retrieves the vertical position in pixels. */
  virtual double GetPixelY() const = 0;
  /** Sets the vertical position in pixels. */
  virtual void SetPixelY(double y) = 0;

  /** Retrieves the horizontal position in relative related to the parent. */
  virtual double GetRelativeX() const = 0;
  /** Sets the horizontal position in relative related to the parent. */
  virtual void SetRelativeX(double x) = 0;
  /** Retrieves the vertical position in relative related to the parent. */
  virtual double GetRelativeY() const = 0;
  /** Sets the vertical position in relative related to the parent. */
  virtual void SetRelativeY(double y) = 0;

  /** Retrieves the horizontal pin in pixels. */
  virtual double GetPixelPinX() const = 0;
  /** Sets the horizontal pin in pixels. */
  virtual void SetPixelPinX(double pin_x) = 0;
  /** Retrieves the vertical pin in pixels. */
  virtual double GetPixelPinY() const = 0;
  /** Sets the vertical pin in pixels. */
  virtual void SetPixelPinY(double pin_y) = 0;

  /** Retrieves the horizontal pin in relative related to the child. */
  virtual double GetRelativePinX() const = 0;
  /** Sets the horizontal pin in relative related to the child. */
  virtual void SetRelativePinX(double pin_x) = 0;
  /** Retrieves the vertical pin in relative related to the child. */
  virtual double GetRelativePinY() const = 0;
  /** Sets the vertical pin in relative related to the child. */
  virtual void SetRelativePinY(double pin_y) = 0;

  /** Retrieves the rotation of the element, in degrees. */
  virtual double GetRotation() const = 0;
  /** Sets the rotation of the element, in degrees. */
  virtual void SetRotation(double rotation) = 0;

  /** Retrieve whether x is relative to its parent element. */
  virtual bool XIsRelative() const = 0;
  /** Retrieve whether y is relative to its parent element. */
  virtual bool YIsRelative() const = 0;
  /** Retrieve whether width is relative to its parent element. */
  virtual bool WidthIsRelative() const = 0;
  /** Retrieve whether height is relative to its parent element. */
  virtual bool HeightIsRelative() const = 0;
  /** Retrieve whether pin x is relative to its width. */
  virtual bool PinXIsRelative() const = 0;
  /** Retrieve whether pin y is relative to its height. */
  virtual bool PinYIsRelative() const = 0;

  /** Retrieve whether width is explicitly specified. */
  virtual bool WidthIsSpecified() const = 0;
  /** Clear the specified width value and use the default. */
  virtual void ResetWidthToDefault() = 0;
  /** Retrieve whether height is explicitly specified. */
  virtual bool HeightIsSpecified() const = 0;
  /** Clear the specified height value and use the default. */
  virtual void ResetHeightToDefault() = 0;
  /** Retrieve whether x is explicitly specified. */
  virtual bool XIsSpecified() const = 0;
  /** Clear the specified x value and use the default. */
  virtual void ResetXToDefault() = 0;
  /** Retrieve whether y is explicitly specified. */
  virtual bool YIsSpecified() const = 0;
  /** Clear the specified y value and use the default. */
  virtual void ResetYToDefault() = 0;

  /** Gets the client width (pixel width - width of scrollbar etc. if any). */
  virtual double GetClientWidth() const = 0;
  /** Gets the client height (pixel width - height of scrollbar etc. if any). */
  virtual double GetClientHeight() const = 0;
};

CLASS_ID_IMPL(ElementInterface, ScriptableInterface)

} // namespace ggadget

#endif // GGADGET_ELEMENT_INTERFACE_H__
