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

#include "details_view.h"
#include "gadget_consts.h"
#include "string_utils.h"

namespace ggadget {

DetailsView::DetailsView()
    : time_created_(0),
      time_absolute_(false),
      layout_(ContentItem::CONTENT_ITEM_LAYOUT_NOWRAP_ITEMS),
      is_html_(false),
      is_view_(false) {
  RegisterProperty("html_content",
                   NewSlot(this, &DetailsView::ContentIsHTML),
                   NewSlot(this, &DetailsView::SetContentIsHTML));
  RegisterProperty("contentIsView",
                   NewSlot(this, &DetailsView::ContentIsView),
                   NewSlot(this, &DetailsView::SetContentIsView));
  RegisterMethod("SetContent", NewSlot(this, &DetailsView::SetContent));
  RegisterMethod("SetContentFromItem",
                 NewSlot(this, &DetailsView::SetContentFromItem));
}

DetailsView::~DetailsView() {
}

void DetailsView::SetContent(const char *source,
                             Date time_created,
                             const char *text,
                             bool time_absolute,
                             ContentItem::Layout layout) {
  source_ = source ? source : "";
  time_created_ = time_created;
  text_ = text ? text : "";
  time_absolute_ = time_absolute;
  layout_ = layout;

  size_t ext_len = strlen(kXMLExt);
  is_view_ = (text_.length() > ext_len &&
              GadgetStrCmp(text_.c_str() + text_.length() - ext_len,
                           kXMLExt) == 0);
  is_html_ = false;
}

void DetailsView::SetContentFromItem(ContentItem *item) {
  if (item) {
    int flags = item->GetFlags();
    source_ = item->GetSource();
    time_created_ = item->GetTimeCreated();
    text_ = item->GetSnippet();
    layout_ = item->GetLayout();
    time_absolute_ = (flags & ContentItem::CONTENT_ITEM_FLAG_TIME_ABSOLUTE);
    is_html_ = (item->GetFlags() & ContentItem::CONTENT_ITEM_FLAG_HTML);
    is_view_ = false;
  }
}

} // namespace ggadget
