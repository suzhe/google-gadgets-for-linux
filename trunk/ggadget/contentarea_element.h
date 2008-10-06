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

#ifndef GGADGET_CONTENT_AREA_ELEMENT_H__
#define GGADGET_CONTENT_AREA_ELEMENT_H__

#include <vector>
#include <ggadget/scrolling_element.h>

namespace ggadget {

class ContentItem;
class ScriptableArray;
class ScriptableImage;

class ContentAreaElement : public ScrollingElement {
 public:
  DEFINE_CLASS_ID(0xa16cc04f24b24cce, BasicElement);

  ContentAreaElement(View *view, const char *name);
  virtual ~ContentAreaElement();

 protected:
  virtual void DoClassRegister();

 public:
  enum ContentFlag {
    CONTENT_FLAG_NONE = 0,
    /** Show details view when user clicks on content items. */
    CONTENT_FLAG_HAVE_DETAILS = 1,
    /** Allow user to pin content items so they will always be displayed. */
    CONTENT_FLAG_PINNABLE = 2,
    /** Items specify their own display position. */
    CONTENT_FLAG_MANUAL_LAYOUT = 4,
    /**
     * Disable the automatic minsize update calculated base on the first item's
     * height.
     */
    CONTENT_FLAG_NO_AUTO_MIN_SIZE = 8,
  };

  enum DisplayOptions {
    /** Display the item in the Sidebar. */
    ITEM_DISPLAY_IN_SIDEBAR = 1,
    /** Display the item in the Sidebar if it is visible. */
    ITEM_DISPLAY_IN_SIDEBAR_IF_VISIBLE = 2,
    /** Display the item in the notification window. */
    ITEM_DISPLAY_AS_NOTIFICATION = 4,
    /** Display the item in the notification window if the Sidebar is hidden. */
    ITEM_DISPLAY_AS_NOTIFICATION_IF_SIDEBAR_HIDDEN = 8,
  };

  /** Combination of one or more @c ContentFlag. */
  int GetContentFlags() const;
  void SetContentFlags(int flags);

  /** Maximum number of allowed content items, defaults to 25. */
  size_t GetMaxContentItems() const;
  void SetMaxContentItems(size_t max_content_items);

  /**
   * The background color of the content area,
   * in "#AARRGGBB" or "#RRGGBB"format.
   */
  std::string GetBackgroundColor() const;
  void SetBackgroundColor(const char *color);

  /**
   * The background color of on mouse down,
   * in "#AARRGGBB" or "#RRGGBB"format.
   */
  std::string GetDownColor() const;
  void SetDownColor(const char *color);

  /**
   * The background color of on mouse over,
   * in "#AARRGGBB" or "#RRGGBB"format.
   */
  std::string GetOverColor() const;
  void SetOverColor(const char *color);

  /** The area's content items. */
  const std::vector<ContentItem *> &GetContentItems();

  /**
   * Gets and sets the pin images used to display the pin status of the items.
   */
  void GetPinImages(ScriptableImage **pinned,
                    ScriptableImage **pinned_over,
                    ScriptableImage **unpinned);
  void SetPinImages(ScriptableImage *pinned,
                    ScriptableImage *pinned_over,
                    ScriptableImage *unpinned);

  void AddContentItem(ContentItem *item, DisplayOptions options);
  void RemoveContentItem(ContentItem *item);
  void RemoveAllContentItems();

  /**
   * For Gadget to register properties into plugin/pluginHelper for
   * historical compatibility.
   */
  ScriptableArray *ScriptGetContentItems();
  void ScriptSetContentItems(ScriptableInterface *array);
  ScriptableArray *ScriptGetPinImages();
  void ScriptSetPinImages(ScriptableInterface *array);

 public:
  virtual bool OnAddContextMenuItems(MenuInterface *menu);

 public:
  static BasicElement *CreateInstance(View *view, const char *name);

 protected:
  virtual void Layout();
  virtual void DoDraw(CanvasInterface *canvas);
  virtual EventResult HandleMouseEvent(const MouseEvent &event);

 public:
  virtual bool HasOpaqueBackground() const;

 private:
  DISALLOW_EVIL_CONSTRUCTORS(ContentAreaElement);

  class Impl;
  Impl *impl_;
};

} // namespace ggadget

#endif // GGADGET_CONTENT_AREA_ELEMENT_H__
