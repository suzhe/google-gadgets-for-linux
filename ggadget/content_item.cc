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

#include <cmath>
#include <ctime>
#include "content_item.h"
#include "contentarea_element.h"
#include "details_view.h"
#include "image.h"
#include "string_utils.h"
#include "text_frame.h"
#include "texture.h"
#include "view.h"

namespace ggadget {

static const int kMinWidthToUseLongVersionOfTimeString = 125;
static const int kNormalFontSize = 9;
static const int kExtraInfoFontSize = 8;
static const int kSnippetFontSize = 8;

const Color ScriptableCanvas::kColorNormalBackground(0.984, 0.984, 0.984);
const Color ScriptableCanvas::kColorNormalText(0, 0, 0);
const Color ScriptableCanvas::kColorExtraInfo(0.133, 0.267, 0.6); // #224499
const Color ScriptableCanvas::kColorSnippet(0.4, 0.4, 0.4); // #666666

class ContentItem::Impl {
 public:
  Impl(View *view)
      : ref_count_(0),
        view_(view),
        content_area_(NULL),
        image_(NULL), notifier_image_(NULL),
        time_created_(0),
        heading_text_(NULL, view),
        source_text_(NULL, view),
        time_text_(NULL, view),
        snippet_text_(NULL, view),
        layout_(CONTENT_ITEM_LAYOUT_NOWRAP_ITEMS),
        flags_(CONTENT_ITEM_FLAG_NONE),
        x_(0), y_(0), width_(0), height_(0) {
    ASSERT(view);
    heading_text_.SetTrimming(CanvasInterface::TRIMMING_CHARACTER_ELLIPSIS);
    heading_text_.SetColor(ScriptableCanvas::kColorNormalText, 1.0);
    heading_text_.SetSize(kNormalFontSize);
    source_text_.SetTrimming(CanvasInterface::TRIMMING_CHARACTER_ELLIPSIS);
    source_text_.SetColor(ScriptableCanvas::kColorExtraInfo, 1.0);
    source_text_.SetSize(kExtraInfoFontSize);
    time_text_.SetTrimming(CanvasInterface::TRIMMING_CHARACTER_ELLIPSIS);
    time_text_.SetColor(ScriptableCanvas::kColorExtraInfo, 1.0);
    time_text_.SetAlign(CanvasInterface::ALIGN_RIGHT);
    time_text_.SetSize(kExtraInfoFontSize);
    snippet_text_.SetTrimming(CanvasInterface::TRIMMING_CHARACTER_ELLIPSIS);
    snippet_text_.SetColor(ScriptableCanvas::kColorSnippet, 1.0);
    snippet_text_.SetWordWrap(true);
    snippet_text_.SetSize(kSnippetFontSize);
  }

  ~Impl() {
    delete image_;
    image_ = NULL;
    delete notifier_image_;
    notifier_image_ = NULL;
  }

  void UpdateTimeText() {
    time_text_.SetText(GetTimeDisplayString(
        time_created_,
        flags_ & CONTENT_ITEM_FLAG_TIME_ABSOLUTE ? 0 : view_->GetCurrentTime(),
        width_ < kMinWidthToUseLongVersionOfTimeString).c_str());
  }

  void ScriptSetRect(int x, int y, int width, int height) {
    if (!content_area_ || (content_area_->GetContentFlags() &
                           ContentAreaElement::CONTENT_FLAG_MANUAL_LAYOUT)) {
      x_ = x;
      y_ = y;
      width_ = width;
      height_ = height;
      QueueDraw();
    }
  }

  void QueueDraw() {
    if (content_area_)
      content_area_->QueueDraw();
  }

  int ref_count_;
  View *view_;
  ContentAreaElement *content_area_;
  Image *image_, *notifier_image_;
  uint64_t time_created_;
  std::string open_command_, tooltip_;
  TextFrame heading_text_, source_text_, time_text_, snippet_text_;
  Layout layout_;
  int flags_;
  int x_, y_, width_, height_;
  Signal7<void, ContentItem *, GadgetInterface::DisplayTarget,
         ScriptableCanvas *, int, int, int, int> on_draw_item_signal_;
  Signal4<int, ContentItem *, GadgetInterface::DisplayTarget,
          ScriptableCanvas *, int> on_get_height_signal_;
  Signal1<void, ContentItem *> on_open_item_signal_;
  Signal1<void, ContentItem *> on_toggle_item_pinned_state_signal_;
  Signal7<bool, ContentItem *, GadgetInterface::DisplayTarget,
          ScriptableCanvas *, int, int, int, int>
      on_get_is_tooltip_required_signal_;
  Signal1<void /*TODO: DetailsViewInfo * */, ContentItem *> on_details_view_signal_;
  Signal2<void, ContentItem *, int> on_process_details_view_feedback_signal_;
  Signal1<bool, ContentItem *> on_remove_item_signal_;
};

ContentItem::ContentItem(View *view)
    : impl_(new Impl(view)) {
  RegisterProperty("image",
                   NewSlot(this, &ContentItem::GetImage),
                   NewSlot(this, &ContentItem::SetImage));
  RegisterProperty("notifier_image",
                   NewSlot(this, &ContentItem::GetNotifierImage),
                   NewSlot(this, &ContentItem::SetNotifierImage));
  RegisterProperty("time_created",
                   NewSlot(this, &ContentItem::GetTimeCreated),
                   NewSlot(this, &ContentItem::SetTimeCreated));
  RegisterProperty("heading",
                   NewSlot(this, &ContentItem::GetHeading),
                   NewSlot(this, &ContentItem::SetHeading));
  RegisterProperty("source",
                   NewSlot(this, &ContentItem::GetSource),
                   NewSlot(this, &ContentItem::SetSource));
  RegisterProperty("snippet",
                   NewSlot(this, &ContentItem::GetSnippet),
                   NewSlot(this, &ContentItem::SetSnippet));
  RegisterProperty("open_command",
                   NewSlot(this, &ContentItem::GetOpenCommand),
                   NewSlot(this, &ContentItem::SetOpenCommand));
  RegisterProperty("layout",
                   NewSlot(this, &ContentItem::GetLayout),
                   NewSlot(this, &ContentItem::SetLayout));
  RegisterProperty("flags", NULL, // Write only.
                   NewSlot(this, &ContentItem::SetFlags));
  RegisterProperty("tooltip", NULL, // Write only.
                   NewSlot(this, &ContentItem::SetTooltip));
  RegisterMethod("SetRect", NewSlot(impl_, &Impl::ScriptSetRect));

  RegisterSignal("onDrawItem", &impl_->on_draw_item_signal_);
  RegisterSignal("onGetHeight", &impl_->on_get_height_signal_);
  RegisterSignal("onOpenItem", &impl_->on_open_item_signal_);
  RegisterSignal("onToggleItemPinnedState",
                 &impl_->on_toggle_item_pinned_state_signal_);
  RegisterSignal("onGetIsTooltipRequired",
                 &impl_->on_get_is_tooltip_required_signal_);
  RegisterSignal("onDetailsView", &impl_->on_details_view_signal_);
  RegisterSignal("onProcessDetailsViewFeedback",
                 &impl_->on_process_details_view_feedback_signal_);
  RegisterSignal("onRemoveItem", &impl_->on_remove_item_signal_);
}

ContentItem::~ContentItem() {
  ASSERT(impl_->ref_count_ == 0);
  delete impl_;
  impl_ = NULL;
}

ScriptableInterface::OwnershipPolicy ContentItem::Attach() {
  ASSERT(impl_->ref_count_ >= 0);
  impl_->ref_count_++;
  return ScriptableInterface::OWNERSHIP_SHARED;
}

bool ContentItem::Detach() {
  ASSERT(impl_->ref_count_ > 0);
  if (--impl_->ref_count_ == 0) {
    delete this;
    return true;
  }
  return false;
}

void ContentItem::AttachContentArea(ContentAreaElement *content_area) {
  ASSERT(impl_->content_area_ == NULL);
  impl_->content_area_ = content_area;
  Attach();
}

void ContentItem::DetachContentArea(ContentAreaElement *content_area) {
  ASSERT(impl_->content_area_ == content_area);
  impl_->content_area_ = NULL;
  Detach();
}

Variant ContentItem::GetImage() const {
  return Variant(Image::GetSrc(impl_->image_)); 
}

void ContentItem::SetImage(const Variant &image) {
  delete impl_->image_;
  impl_->image_ = impl_->view_->LoadImage(image, false);
  impl_->QueueDraw();
}

Variant ContentItem::GetNotifierImage() const {
  return Variant(Image::GetSrc(impl_->image_));
}

void ContentItem::SetNotifierImage(const Variant &image) {
  delete impl_->notifier_image_;
  impl_->notifier_image_ = impl_->view_->LoadImage(image, false);
  impl_->QueueDraw();
}

Date ContentItem::GetTimeCreated() const {
  return Date(impl_->time_created_);
}

void ContentItem::SetTimeCreated(const Date &time) {
  if (impl_->time_created_ != time.value) {
    impl_->time_created_ = time.value;
    impl_->QueueDraw();
  }
}

const char *ContentItem::GetHeading() const {
  return impl_->heading_text_.GetText();
}

void ContentItem::SetHeading(const char *heading) {
  if (impl_->heading_text_.SetText(heading))
    impl_->QueueDraw();
}

const char *ContentItem::GetSource() const {
  return impl_->source_text_.GetText();
}

void ContentItem::SetSource(const char *source) {
  if (impl_->source_text_.SetText(source))
    impl_->QueueDraw();
}

const char *ContentItem::GetSnippet() const {
  return impl_->snippet_text_.GetText();
}

void ContentItem::SetSnippet(const char *snippet) {
  if (impl_->snippet_text_.SetText(snippet))
    impl_->QueueDraw();
}

const char *ContentItem::GetOpenCommand() const {
  return impl_->open_command_.c_str();
}

void ContentItem::SetOpenCommand(const char *open_command) {
  impl_->open_command_ = open_command;
}

ContentItem::Layout ContentItem::GetLayout() const {
  return impl_->layout_;
}

void ContentItem::SetLayout(Layout layout) {
  if (layout != impl_->layout_) {
    impl_->layout_ = layout;
    impl_->heading_text_.SetWordWrap(layout == CONTENT_ITEM_LAYOUT_NEWS);
    impl_->QueueDraw();
  }
}
 
int ContentItem::GetFlags() const {
  return impl_->flags_;
}

void ContentItem::SetFlags(int flags) {
  if (flags != impl_->flags_) {
    impl_->flags_ = flags;
    impl_->heading_text_.SetBold(flags & CONTENT_ITEM_FLAG_HIGHLIGHTED);
    impl_->QueueDraw();
  }
}

const char *ContentItem::GetTooltip() const {
  return impl_->tooltip_.c_str();
}
  
void ContentItem::SetTooltip(const char *tooltip) {
  impl_->tooltip_ = tooltip;
}

void ContentItem::SetRect(int x, int y, int width, int height) {
  impl_->x_ = x;
  impl_->y_ = y;
  impl_->width_ = width;
  impl_->height_ = height;
  impl_->QueueDraw();
}

void ContentItem::GetRect(int *x, int *y, int *width, int *height) {
  ASSERT(x && y && width && height);
  *x = impl_->x_;
  *y = impl_->y_;
  *width = impl_->width_;
  *height = impl_->height_;
}

void ContentItem::Draw(GadgetInterface::DisplayTarget target,
                       CanvasInterface *canvas,
                       int x, int y, int width, int height) {
  // Try script handler first.
  if (impl_->on_draw_item_signal_.HasActiveConnections()) {
    ScriptableCanvas scriptable_canvas(canvas, impl_->view_);
    impl_->on_draw_item_signal_(this, target, &scriptable_canvas,
                                x, y, width, height);
  }

  // Then the default logic.
  int heading_space_width = width;
  int heading_left = x;
  int image_height = 0;
  if (impl_->image_) {
    int image_width = static_cast<int>(impl_->image_->GetWidth());
    heading_space_width -= image_width;
    image_height = static_cast<int>(impl_->image_->GetHeight());
    if (impl_->flags_ & CONTENT_ITEM_FLAG_LEFT_ICON) {
      impl_->image_->Draw(canvas, x, y);
      heading_left += image_width;
    } else {
      impl_->image_->Draw(canvas, x + width - image_width, y);
    }
  }

  impl_->UpdateTimeText();
  double heading_width = 0, heading_height = 0;
  impl_->heading_text_.GetSimpleExtents(&heading_width, &heading_height);
  if (impl_->layout_ == CONTENT_ITEM_LAYOUT_NEWS &&
      heading_width > heading_space_width) {
    // Heading can wrap up to 2 lines under news layout mode.
    heading_height *= 2;
  }
  impl_->heading_text_.Draw(canvas, heading_left, y,
                            heading_space_width, heading_height);
  if (impl_->layout_ == CONTENT_ITEM_LAYOUT_NOWRAP_ITEMS ||
      impl_->layout_ > CONTENT_ITEM_LAYOUT_EMAIL) {
    return;
  }

  y += std::max(static_cast<int>(ceil(heading_height)), image_height);
  double source_width = 0, source_height = 0;
  impl_->source_text_.GetSimpleExtents(&source_width, &source_height);
  double time_width = 0, time_height = 0;
  impl_->time_text_.GetSimpleExtents(&time_width, &time_height);
  time_width += 3;
  if (time_width > width) time_width = width;

  impl_->time_text_.Draw(canvas, x + width - time_width, y,
                         time_width, time_height);
  if (width > time_width)
    impl_->source_text_.Draw(canvas, x, y, width - time_width, source_height);

  if (impl_->layout_ == CONTENT_ITEM_LAYOUT_EMAIL) {
    y += static_cast<int>(ceil(std::max(source_height, time_height)));
    double snippet_width = 0, snippet_height = 0;
    impl_->snippet_text_.GetSimpleExtents(&snippet_width, &snippet_height);
    if (snippet_width > width)
      snippet_height *= 2;
    impl_->snippet_text_.Draw(canvas, x, y, width, snippet_height);
  }
}

Connection *ContentItem::ConnectOnDrawItem(
    Slot7<void, ContentItem *, GadgetInterface::DisplayTarget,
          ScriptableCanvas *, int, int, int, int> *handler) {
  return impl_->on_draw_item_signal_.Connect(handler);
}

int ContentItem::GetHeight(GadgetInterface::DisplayTarget target,
                           CanvasInterface *canvas, int width) {
  // Try script handler first.
  if (impl_->on_get_height_signal_.HasActiveConnections()) {
    ScriptableCanvas scriptable_canvas(canvas, impl_->view_);
    return impl_->on_get_height_signal_(this, target, &scriptable_canvas,
                                        width);
  }

  int heading_space_width = width;
  int image_height = 0;
  if (impl_->image_) {
    heading_space_width -= impl_->image_->GetWidth();
    image_height = static_cast<int>(impl_->image_->GetHeight());
  }

  // Then the default logic.
  impl_->UpdateTimeText();
  double heading_width = 0, heading_height = 0;
  impl_->heading_text_.GetSimpleExtents(&heading_width, &heading_height);
  if (impl_->layout_ == CONTENT_ITEM_LAYOUT_NOWRAP_ITEMS ||
      impl_->layout_ > CONTENT_ITEM_LAYOUT_EMAIL) {
    // Only heading and icon.
    return std::max(static_cast<int>(ceil(heading_height)), image_height);
  }

  double source_width = 0, source_height = 0;
  impl_->source_text_.GetSimpleExtents(&source_width, &source_height);
  double time_width = 0, time_height = 0;
  impl_->time_text_.GetSimpleExtents(&time_width, &time_height);
  int extra_info_height = static_cast<int>(ceil(std::max(source_height,
                                                         time_height)));
  if (impl_->layout_ == CONTENT_ITEM_LAYOUT_NEWS) {
    // Heading can wrap up to 2 lines. Show extra info.
    if (heading_width > heading_space_width)
      heading_height *= 2;
    return std::max(static_cast<int>(ceil(heading_height)), image_height) +
           extra_info_height;
  }

  // Heading doesn't wrap. Show extra info. Snippet can wrap up to 2 lines.
  double snippet_width = 0, snippet_height = 0;
  impl_->snippet_text_.GetSimpleExtents(&snippet_width, &snippet_height);
  if (snippet_width > width)
    snippet_height *= 2;
  return std::max(static_cast<int>(ceil(heading_height)), image_height) +
         extra_info_height +
         static_cast<int>(ceil(snippet_height));
}

Connection *ContentItem::ConnectOnGetHeight(
    Slot4<int, ContentItem *, GadgetInterface::DisplayTarget,
          ScriptableCanvas *, int> *handler) {
  return impl_->on_get_height_signal_.Connect(handler);
}

void ContentItem::OpenItem() {
  // TODO:
}

Connection *ContentItem::ConnectOnOpenItem(
    Slot1<void, ContentItem *> *handler) {
  return impl_->on_open_item_signal_.Connect(handler);
}

void ContentItem::ToggleItemPinnedState() {
  // TODO:
}

Connection *ContentItem::ConnectOnToggleItemPinnedState(
    Slot1<void, ContentItem *> *handler) {
  return impl_->on_toggle_item_pinned_state_signal_.Connect(handler);
}

bool ContentItem::IsTooltipRequired(GadgetInterface::DisplayTarget target,
                                    CanvasInterface *canvas,
                                    int x, int y, int width, int height) {
  // TODO:
  return true;
}

Connection *ContentItem::ConnectOnGetIsTooltipRequired(
    Slot7<bool, ContentItem *, GadgetInterface::DisplayTarget,
          ScriptableCanvas *, int, int, int, int> *handler) {
  return impl_->on_get_is_tooltip_required_signal_.Connect(handler);
}

/*  DetailsViewInfo *OnDetailsView(ContentItem *item);
typedef Slot1<DetailsViewInfo *, ContentItem *> OnDetailsViewHandler;
Connection *ConnectOnDetailsView(OnDetailsViewHandler *handler); */

Connection *ContentItem::ConnectOnProcessDetailsViewFeedback(
    Slot2<void, ContentItem *, int> *handler) {
  return impl_->on_process_details_view_feedback_signal_.Connect(handler);
}

bool ContentItem::OnUserRemove() {
  return impl_->on_remove_item_signal_.HasActiveConnections() ?
         impl_->on_remove_item_signal_(this) : true;
}

Connection *ContentItem::ConnectOnRemoveItem(
    Slot1<bool, ContentItem *> *handler) {
  return impl_->on_remove_item_signal_.Connect(handler);
}

std::string ContentItem::GetTimeDisplayString(uint64_t time,
                                              uint64_t current_time,
                                              bool short_form) {
  static const unsigned int kMsPerDay = 86400000U;
  static const unsigned int kMsPerHour = 3600000U;
  static const unsigned int kMsPerMinute = 60000U;

  if (time == 0)
    return "";

  if (current_time == 0) {
    // Show absolute time as HH:MMam/pm.
    // TODO: Localization.
    char buffer[20];
    time_t t = time / 1000;
    strftime(buffer, sizeof(buffer), "%I:%M%p", localtime(&t));
    return buffer;
  }

  uint64_t time_diff = 0;
  if (time < current_time)
    time_diff = current_time - time;

  if (time_diff >= 4 * kMsPerDay) {
    // > 4 days, show like 'Mar 20'.
    // TODO: Localization.
    char buffer[20];
    time_t t = static_cast<time_t>(time / 1000);
    strftime(buffer, sizeof(buffer), "%b %d", localtime(&t));
    return buffer;
  }
  if (time_diff >= kMsPerDay) {
    // TODO: long format, plural/singular.
    return StringPrintf("%dd ago", static_cast<int>(time_diff / kMsPerDay));
  }
  if (time_diff >= kMsPerHour) {
    return StringPrintf("%dh ago", static_cast<int>(time_diff / kMsPerHour));
  }
  return StringPrintf("%dm ago", static_cast<int>(time_diff / kMsPerMinute));
}

ScriptableCanvas::ScriptableCanvas(CanvasInterface *canvas, View *view)
    : canvas_(canvas), view_(view) {
  RegisterMethod("DrawLine",
                 NewSlot(this, &ScriptableCanvas::DrawLineWithColorName));
  RegisterMethod("DrawRect",
                 NewSlot(this, &ScriptableCanvas::DrawRectWithColorName));
  RegisterMethod("DrawImage", NewSlot(this, &ScriptableCanvas::DrawImage));
  RegisterMethod("DrawText",
                 NewSlot(this, &ScriptableCanvas::DrawTextWithColorName));
  RegisterMethod("GetTextWidth",
                 NewSlot(this, &ScriptableCanvas::GetTextWidth));
  RegisterMethod("GetTextHeight",
                 NewSlot(this, &ScriptableCanvas::GetTextHeight));
}

ScriptableCanvas::~ScriptableCanvas() {
}

void ScriptableCanvas::DrawLine(int x1, int y1, int x2, int y2,
                                const Color &color) {
  canvas_->DrawLine(x1, y1, x2, y2, 1, color);
}

void ScriptableCanvas::DrawRect(int x1, int y1, int x2, int y2,
                                const Color &line_color,
                                const Color &fill_color) {
  canvas_->DrawFilledRect(x1, y1, x2, y2, fill_color);
  canvas_->DrawLine(x1, y1, x1, y2, 1, line_color);
  canvas_->DrawLine(x1, y1, x2, y1, 1, line_color);
  canvas_->DrawLine(x2, y1, x2, y2, 1, line_color);
  canvas_->DrawLine(x1, y2, x2, y2, 1, line_color);
}

void ScriptableCanvas::DrawImage(int x, int y, int width, int height,
                                 const Variant &image, int alpha_percent) {
  Image *real_image = view_->LoadImage(image, false);
  if (real_image) {
    canvas_->PushState();
    canvas_->MultiplyOpacity(alpha_percent / 100.0);
    real_image->StretchDraw(canvas_, x, y, width, height);
    canvas_->PopState();
  }
  delete real_image;
}

static void SetupTextFrame(TextFrame *text_frame,
                           const char *text, const Color &color,
                           int flags, ScriptableCanvas::FontID font) {
  text_frame->SetText(text);
  text_frame->SetTrimming(CanvasInterface::TRIMMING_CHARACTER_ELLIPSIS);
  text_frame->SetAlign(flags & ScriptableCanvas::TEXT_FLAG_CENTER ?
                          CanvasInterface::ALIGN_CENTER :
                       flags & ScriptableCanvas::TEXT_FLAG_RIGHT ?
                           CanvasInterface::ALIGN_RIGHT :
                       CanvasInterface::ALIGN_LEFT);
  text_frame->SetVAlign(flags & ScriptableCanvas::TEXT_FLAG_VCENTER ?
                            CanvasInterface::VALIGN_MIDDLE :
                        flags & ScriptableCanvas::TEXT_FLAG_BOTTOM ?
                            CanvasInterface::VALIGN_BOTTOM :
                        CanvasInterface::VALIGN_TOP);
  text_frame->SetColor(color, 1.0);
  // TODO: ScriptableCanvas::TEXT_FLAG_WORD_BREAK
  text_frame->SetWordWrap(!(flags & ScriptableCanvas::TEXT_FLAG_SINGLE_LINE));

  switch (font) {
    case ScriptableCanvas::FONT_NORMAL:
      text_frame->SetSize(kNormalFontSize);
      break;
    case ScriptableCanvas::FONT_BOLD:
      text_frame->SetSize(kNormalFontSize);
      text_frame->SetBold(true);
      break;
    case ScriptableCanvas::FONT_SNIPPET:
      text_frame->SetSize(kSnippetFontSize);
      break;
    case ScriptableCanvas::FONT_EXTRA_INFO:
      text_frame->SetSize(kExtraInfoFontSize);
      break;
    default:
      text_frame->SetSize(kNormalFontSize);
      break;
  }
}

void ScriptableCanvas::DrawText(int x, int y, int width, int height,
                                const char *text, const Color &color,
                                int flags, FontID font) {
  TextFrame text_frame(NULL, view_);
  SetupTextFrame(&text_frame, text, color, flags, font);
  text_frame.Draw(canvas_, x, y, width, height);
}

int ScriptableCanvas::GetTextWidth(const char *text, int flags, FontID font) {
  TextFrame text_frame(NULL, view_);
  SetupTextFrame(&text_frame, text, Color(0, 0, 0), flags, font);
  double width = 0, height = 0;
  text_frame.GetSimpleExtents(&width, &height);
  return static_cast<int>(ceil(width));
}

int ScriptableCanvas::GetTextHeight(const char *text, int width,
                                    int flags, FontID font) {
  TextFrame text_frame(NULL, view_);
  SetupTextFrame(&text_frame, text, Color(0, 0, 0), flags, font);
  double ret_width = 0, height = 0;
  text_frame.GetExtents(width, &ret_width, &height);
  return static_cast<int>(ceil(height));
}

void ScriptableCanvas::DrawLineWithColorName(int x1, int y1, int x2, int y2,
                                             const char *color) {
  DrawLine(x1, y1, x2, y2, Texture(NULL, NULL, color).GetColor());
}

void ScriptableCanvas::DrawRectWithColorName(int x1, int y1, int x2, int y2,
                                             const char *line_color,
                                             const char *fill_color) {
  DrawRect(x1, y1, x2, y2,
           Texture(NULL, NULL, line_color).GetColor(),
           Texture(NULL, NULL, fill_color).GetColor());
}

void ScriptableCanvas::DrawTextWithColorName(int x, int y,
                                             int width, int height,
                                             const char *text,
                                             const char *color,
                                             int flags, FontID font) {
  DrawText(x, y, width, height, text, Texture(NULL, NULL, color).GetColor(),
           flags, font);
}

} // namespace ggadget
