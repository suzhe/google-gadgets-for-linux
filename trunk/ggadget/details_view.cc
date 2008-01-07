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
#include "logger.h"
#include "memory_options.h"
#include "scriptable_options.h"
#include "string_utils.h"

namespace ggadget {

class DetailsView::Impl {
 public:
  Impl()
      : time_created_(0),
        time_absolute_(false),
        layout_(ContentItem::CONTENT_ITEM_LAYOUT_NOWRAP_ITEMS),
        is_html_(false),
        is_view_(false),
        scriptable_data_(&data_, true),
        external_object_(NULL) {
  }

  ~Impl() {
    if (external_object_)
      external_object_->Detach();
  }

  std::string source_;
  Date time_created_;
  std::string text_;
  bool time_absolute_;
  ContentItem::Layout layout_;
  bool is_html_;
  bool is_view_;
  MemoryOptions data_;
  ScriptableOptions scriptable_data_;
  ScriptableInterface *external_object_;
};

DetailsView::DetailsView()
    : impl_(new Impl()) {
  RegisterProperty("html_content",
                   NewSlot(this, &DetailsView::ContentIsHTML),
                   NewSlot(this, &DetailsView::SetContentIsHTML));
  RegisterProperty("contentIsView",
                   NewSlot(this, &DetailsView::ContentIsView),
                   NewSlot(this, &DetailsView::SetContentIsView));
  RegisterMethod("SetContent", NewSlot(this, &DetailsView::SetContent));
  RegisterMethod("SetContentFromItem",
                 NewSlot(this, &DetailsView::SetContentFromItem));
  RegisterConstant("detailsViewData", &impl_->scriptable_data_);
  RegisterProperty("external", NewSlot(this, &DetailsView::GetExternalObject),
                   NewSlot(this, &DetailsView::SetExternalObject));
}

DetailsView::~DetailsView() {
  delete impl_;
  impl_ = NULL;
}

void DetailsView::SetContent(const char *source,
                             Date time_created,
                             const char *text,
                             bool time_absolute,
                             ContentItem::Layout layout) {
  impl_->source_ = source ? source : "";
  impl_->time_created_ = time_created;
  impl_->text_ = text ? text : "";
  impl_->time_absolute_ = time_absolute;
  impl_->layout_ = layout;

  size_t ext_len = strlen(kXMLExt);
  impl_->is_view_ =
      impl_->text_.length() > ext_len &&
      GadgetStrCmp(impl_->text_.c_str() + impl_->text_.length() - ext_len,
                   kXMLExt) == 0;
}

void DetailsView::SetContentFromItem(ContentItem *item) {
  if (item) {
    int flags = item->GetFlags();
    impl_->source_ = item->GetSource();
    impl_->time_created_ = item->GetTimeCreated();
    impl_->text_ = item->GetSnippet();
    impl_->layout_ = item->GetLayout();
    impl_->time_absolute_ =
        (flags & ContentItem::CONTENT_ITEM_FLAG_TIME_ABSOLUTE);
    impl_->is_html_ = (item->GetFlags() & ContentItem::CONTENT_ITEM_FLAG_HTML);
    impl_->is_view_ = false;
  }
}

std::string DetailsView::GetSource() const {
  return impl_->source_;
}

Date DetailsView::GetTimeCreated() const {
  return impl_->time_created_;
}

std::string DetailsView::GetText() const {
  return impl_->text_;
}

bool DetailsView::IsTimeAbsolute() const {
  return impl_->time_absolute_;
}

ContentItem::Layout DetailsView::GetLayout() const {
  return impl_->layout_;
}

bool DetailsView::ContentIsHTML() {
  return impl_->is_html_;
}

void DetailsView::SetContentIsHTML(bool is_html) {
  impl_->is_html_ = is_html;
}

bool DetailsView::ContentIsView() const {
  return impl_->is_view_;
}

void DetailsView::SetContentIsView(bool is_view) {
  impl_->is_view_ = is_view;
}

const ScriptableOptions *DetailsView::GetDetailsViewData() const {
  return &impl_->scriptable_data_;
}

ScriptableOptions *DetailsView::GetDetailsViewData() {
  return &impl_->scriptable_data_;
}

ScriptableInterface *DetailsView::GetExternalObject() const {
  return impl_->external_object_;
}

void DetailsView::SetExternalObject(ScriptableInterface *external_object) {
  if (impl_->external_object_)
    impl_->external_object_->Detach();
  if (external_object)
    external_object->Attach();
  impl_->external_object_ = external_object;
}

DetailsView *DetailsView::CreateInstance() {
  return new DetailsView();
}

} // namespace ggadget
