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

#ifndef GGADGET_CONTENT_ITEM_H__
#define GGADGET_CONTENT_ITEM_H__

#include <ggadget/scriptable_helper.h>

namespace ggadget {

class ContentItem : public ScriptableHelper<ScriptableInterface> {
 public:
  DEFINE_CLASS_ID(0x062fc66bb03640ca, ScriptableInterface);

  enum Layout {
    /** Single line with just the heading and icon. */
    CONTENT_ITEM_LAYOUT_NOWRAP_ITEMS = 0,
    /** A layout displaying the heading, source, and time. */
    CONTENT_ITEM_LAYOUT_NEWS = 1,
    /** A layout displaying the heading, source, time, and snippet. */
    CONTENT_ITEM_LAYOUT_EMAIL = 2,
  };

  enum Flags {
    /** No flags passed. */
    CONTENT_ITEM_FLAG_NONE              = 0x0000,
    /** Item does not take user input. */
    CONTENT_ITEM_FLAG_STATIC            = 0x0001,
    /** Item is highlighted/shown as bold. */
    CONTENT_ITEM_FLAG_HIGHLIGHTED       = 0x0002,
    /** Item is pinned to the top of the list. */
    CONTENT_ITEM_FLAG_PINNED            = 0x0004,
    /** Item's time is shown as absolute time. */
    CONTENT_ITEM_FLAG_TIME_ABSOLUTE     = 0x0008,
    /** Item can take negative feedback from user. */
    CONTENT_ITEM_FLAG_NEGATIVE_FEEDBACK = 0x0010,
    /** Item's icon should be displayed on the left side. */
    CONTENT_ITEM_FLAG_LEFT_ICON         = 0x0020,
    /** Do not show a 'remove' option for this item in the context menu. */
    CONTENT_ITEM_FLAG_NO_REMOVE         = 0x0040,
    /**
     * Item may be shared with friends. This will enable the spedific menu
     * item in the context menu and the button in the details view.
     */
    CONTENT_ITEM_FLAG_SHAREABLE         = 0x0080,
    /** This item was received from another user. */
    CONTENT_ITEM_FLAG_SHARED            = 0x0100,
    /** The user has interfacted (viewed details/opened etc.) with this item. */
    CONTENT_ITEM_FLAG_INTERACTED        = 0x0200,
    /**
     * The content item's text strings (heading, source, snippet) should not be
     * converted to plain text before displaying them on screen. Without this
     * flag, HTML tags and other markup are stripped out. You can use this flag
     * to display HTML code as-is.
     */
    CONTENT_ITEM_FLAG_DISPLAY_AS_IS     = 0x0400,
    /**
     * The @c snippet property of the content item contains HTML text that
     * should be interpreted. Use this flag to make the content in the details
     * view be formatted as HTML. Setting this flag implicitly sets the
     * @c CONTENT_ITEM_FLAG_DISPLAY_AS_IS flag.
     */
    CONTENT_ITEM_FLAG_HTML              = 0x0800,
    /* Hide content items while still having them in the data structures. */
    CONTENT_ITEM_FLAG_HIDDEN            = 0x1000,
  };

  
  // TODO:
#if 0
 public:
  ContentItem();
  virtual ~ContentItem();

 public:
  /** Gets and sets the image to show in the item. */
  Variant GetImage() const;
  void SetImage(const Variant &image);

  /** Gets and sets the image to show in the notifier. */
  Variant GetNotifierImage() const;
  void SetNotifierImage(const Variant &image);

  /** Gets and sets the item's display title. */
  const char *GetHeading() const;
  void SetHeading(const char *heading);

  /** Gets and sets the item's displayed website/news source. */
  const char *GetSource() const;
  void SetSource(const char *source);

  /** Gets and sets the item's displayed snippet. */
  const char *GetSnippet() const;
  void SetSnippet(const char *snippet);

  /**
   * Gets and sets the URL/file path opened when the user opens/double clicks
   * the item.
   */
  const char *GetOpenCommand() const;
  void SetOpenCommand(const char *open_command);

  /** Gets and sets the format in which the item should be displayed. */
  Layout GetLayout() const;
  void SetLayout(Layout layout);

  /**
   * Sets the flags.
   * @param flags combination of Flags.
   */
  void SetFlags(int flags);

  /** Sets the tooltip text, such as full path, full headlines, etc. */
  void SetTooltip(const char *tooltip);

  /**
   * Sets the item's display position. Before setting any item's position, the
   * plug-in's MANUAL_LAYOUT flag should be enabled, otherwise the items will
   * always appear in the default position given by the plug-in.
   */
  void SetRect(double x, double y, double width, double height);

  // TODO:
#endif
};

} // namespace ggadget

#endif  // GGADGET_DETAILS_INTERFACE_H__
