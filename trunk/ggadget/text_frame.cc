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

#include "text_frame.h"
#include "basic_element.h"
#include "color.h"
#include "gadget_consts.h"
#include "graphics_interface.h"
#include "string_utils.h"
#include "texture.h"
#include "view.h"

namespace ggadget{

static const Color kDefaultColor(0, 0, 0);

static const char *kAlignNames[] = {
  "left", "center", "right", "justify"
};
static const char *kVAlignNames[] = {
  "top", "middle", "bottom"
};
static const char *kTrimmingNames[] = {
  "none",
  "character",
  "word",
  "character-ellipsis",
  "word-ellipsis",
  "path-ellipsis"
};

class TextFrame::Impl {
 public:
  Impl(BasicElement *owner, View *view)
      : owner_(owner), view_(view), font_(NULL),
        color_texture_(new Texture(kDefaultColor, 1.0)),
        align_(CanvasInterface::ALIGN_LEFT),
        valign_(CanvasInterface::VALIGN_TOP),
        trimming_(CanvasInterface::TRIMMING_NONE),
        bold_(false), italic_(false), flags_(0),
        size_(kDefaultFontSize), size_is_default_(true),
        // font_name_ is left blank to indicate default font.
        width_(0.0), height_(0.0) {
  }

  ~Impl() {
    delete color_texture_;
    color_texture_ = NULL;
    ClearFont();
  }

  void ClearFont() {
    if (font_) {
      font_->Destroy();
      font_ = NULL;
    }
  }

  void ResetFont() {
    ClearFont();
    ResetExtents();
  }

  void ResetExtents() {
    width_ = height_ = 0;
    QueueDraw();
  }

  bool SetUpFont() {
    if (size_is_default_) {
      int default_size = view_->GetDefaultFontSize();
      if (default_size != size_) {
        size_ = default_size;
        ResetFont();
      }
    }

    // The FontInterface object is cached on draw.
    if (!font_) {
      font_ = view_->GetGraphics()->NewFont(
          font_name_.empty() ? kDefaultFontName : font_name_.c_str(),
          size_,
          italic_ ? FontInterface::STYLE_ITALIC : FontInterface::STYLE_NORMAL,
          bold_ ? FontInterface::WEIGHT_BOLD : FontInterface::WEIGHT_NORMAL);
      if (!font_) {
        return false;
      }
    }

    if (text_.empty()) {
      width_ = height_ = 0;
    } else if (width_ == 0 && height_ == 0) {
      CanvasInterface *canvas = view_->GetGraphics()->NewCanvas(5, 5);
      canvas->GetTextExtents(text_.c_str(), font_, flags_, 0,
                             &width_, &height_);
      canvas->Destroy();
    }
    return true;
  }

  void QueueDraw() {
    if (owner_)
      owner_->QueueDraw();
  }

  BasicElement *owner_;
  View *view_;

  FontInterface *font_;
  Texture *color_texture_;
  CanvasInterface::Alignment align_;
  CanvasInterface::VAlignment valign_;
  CanvasInterface::Trimming trimming_;
  bool bold_, italic_;
  int flags_;
  double size_;
  bool size_is_default_;
  std::string font_name_, color_, text_;
  double width_, height_;
};

TextFrame::TextFrame(BasicElement *owner, View *view)
    : impl_(new Impl(owner, view)) {
}

void TextFrame::RegisterClassProperties(
    TextFrame *(*delegate_getter)(BasicElement *),
    const TextFrame *(*delegate_getter_const)(BasicElement *)) {
  BasicElement *owner = impl_->owner_;
  ASSERT(owner);
  // Register Properties
  // All properties are registered except for GetText/SetText
  // since some elements call it "caption" while others call it "innerText."
  // Elements may also want to do special handling on SetText.
  owner->RegisterProperty("bold",
      NewSlot(&TextFrame::IsBold, delegate_getter_const),
      NewSlot(&TextFrame::SetBold, delegate_getter));
  owner->RegisterProperty("color",
      NewSlot(&TextFrame::GetColor, delegate_getter_const),
      NewSlot<void, const Variant &>(&TextFrame::SetColor, delegate_getter));
  owner->RegisterProperty("font",
      NewSlot(&TextFrame::GetFont, delegate_getter_const),
      NewSlot(&TextFrame::SetFont, delegate_getter));
  owner->RegisterProperty("italic",
      NewSlot(&TextFrame::IsItalic, delegate_getter_const),
      NewSlot(&TextFrame::SetItalic, delegate_getter));
  owner->RegisterProperty("size",
      NewSlot(&TextFrame::GetSize, delegate_getter_const),
      NewSlot(&TextFrame::SetSize, delegate_getter));
  owner->RegisterProperty("strikeout",
      NewSlot(&TextFrame::IsStrikeout, delegate_getter_const),
      NewSlot(&TextFrame::SetStrikeout, delegate_getter));
  owner->RegisterProperty("underline",
      NewSlot(&TextFrame::IsUnderline, delegate_getter_const),
      NewSlot(&TextFrame::SetUnderline, delegate_getter));
  owner->RegisterProperty("wordWrap",
      NewSlot(&TextFrame::IsWordWrap, delegate_getter_const),
      NewSlot(&TextFrame::SetWordWrap, delegate_getter));

  owner->RegisterStringEnumProperty("align",
      NewSlot(&TextFrame::GetAlign, delegate_getter_const),
      NewSlot(&TextFrame::SetAlign, delegate_getter),
      kAlignNames, arraysize(kAlignNames));
  owner->RegisterStringEnumProperty("vAlign",
      NewSlot(&TextFrame::GetVAlign, delegate_getter_const),
      NewSlot(&TextFrame::SetVAlign, delegate_getter),
      kVAlignNames, arraysize(kVAlignNames));
  owner->RegisterStringEnumProperty("trimming",
      NewSlot(&TextFrame::GetTrimming, delegate_getter_const),
      NewSlot(&TextFrame::SetTrimming, delegate_getter),
      kTrimmingNames, arraysize(kTrimmingNames));
}

TextFrame::~TextFrame() {
  delete impl_;
  impl_ = NULL;
}

CanvasInterface::Alignment TextFrame::GetAlign() const {
  return impl_->align_;
}

void TextFrame::SetAlign(CanvasInterface::Alignment align) {
  if (align != impl_->align_) {
    impl_->align_ = align;
    impl_->QueueDraw();
  }
}

bool TextFrame::IsBold() const {
  return impl_->bold_;
}

void TextFrame::SetBold(bool bold) {
  if (bold != impl_->bold_) {
    impl_->bold_ = bold;
    impl_->ResetFont();
  }
}

Variant TextFrame::GetColor() const {
  return Variant(Texture::GetSrc(impl_->color_texture_));
}

void TextFrame::SetColor(const Variant &color) {
  delete impl_->color_texture_;
  impl_->color_texture_ = impl_->view_->LoadTexture(color);
  if (!impl_->color_texture_)
    impl_->color_texture_ = new Texture(kDefaultColor, 1.0);
  impl_->QueueDraw();
}

void TextFrame::SetColor(const Color &color, double opacity) {
  delete impl_->color_texture_;
  impl_->color_texture_ = new Texture(color, opacity);
  impl_->QueueDraw();
}

std::string TextFrame::GetFont() const {
  return impl_->font_name_;
}

void TextFrame::SetFont(const char *font) {
  if (AssignIfDiffer(font, &impl_->font_name_, strcasecmp)) {
    impl_->ResetFont();
  }
}

bool TextFrame::IsItalic() const {
  return impl_->italic_;
}

void TextFrame::SetItalic(bool italic) {
  if (italic != impl_->italic_) {
    impl_->italic_ = italic;
    impl_->ResetFont();
  }
}

double TextFrame::GetSize() const {
  return impl_->size_is_default_ ? -1 : impl_->size_;
}

void TextFrame::SetSize(double size) {
  if (size == -1) {
    impl_->size_is_default_ = true;
    size = impl_->view_->GetDefaultFontSize();
  } else {
    impl_->size_is_default_ = false;
  }
  if (size != impl_->size_) {
    impl_->size_ = size;
    impl_->ResetFont();
  }
}

double TextFrame::GetCurrentSize() const {
  return impl_->size_;
}

bool TextFrame::IsStrikeout() const {
  return impl_->flags_ & CanvasInterface::TEXT_FLAGS_STRIKEOUT;
}

void TextFrame::SetStrikeout(bool strikeout) {
  if (strikeout == !(impl_->flags_ & CanvasInterface::TEXT_FLAGS_STRIKEOUT)) {
    impl_->flags_ ^= CanvasInterface::TEXT_FLAGS_STRIKEOUT;
    impl_->ResetFont();
  }
}

CanvasInterface::Trimming TextFrame::GetTrimming() const {
  return impl_->trimming_;
}

void TextFrame::SetTrimming(CanvasInterface::Trimming trimming) {
  if (trimming != impl_->trimming_) {
    impl_->trimming_ = trimming;
    impl_->QueueDraw();
  }
}

bool TextFrame::IsUnderline() const {
  return impl_->flags_ & CanvasInterface::TEXT_FLAGS_UNDERLINE;
}

void TextFrame::SetUnderline(bool underline) {
  if (underline == !(impl_->flags_ & CanvasInterface::TEXT_FLAGS_UNDERLINE)) {
    impl_->flags_ ^= CanvasInterface::TEXT_FLAGS_UNDERLINE;
    impl_->ResetFont();
  }
}

CanvasInterface::VAlignment TextFrame::GetVAlign() const {
  return impl_->valign_;
}

void TextFrame::SetVAlign(CanvasInterface::VAlignment valign) {
  if (valign != impl_->valign_) {
    impl_->valign_ = valign;
    impl_->QueueDraw();
  }
}

bool TextFrame::IsWordWrap() const {
  return impl_->flags_ & CanvasInterface::TEXT_FLAGS_WORDWRAP;
}

void TextFrame::SetWordWrap(bool wrap) {
  if (wrap == !(impl_->flags_ & CanvasInterface::TEXT_FLAGS_WORDWRAP)) {
    impl_->flags_ ^= CanvasInterface::TEXT_FLAGS_WORDWRAP;
    impl_->ResetFont();
  }
}

std::string TextFrame::GetText() const {
  return impl_->text_;
}

bool TextFrame::SetText(const std::string &text) {
  if (text != impl_->text_) {
    impl_->text_ = text;
    impl_->ResetExtents();
    return true;
  }
  return false;
}

void TextFrame::DrawWithTexture(CanvasInterface *canvas, double x, double y,
                                double width, double height, Texture *texture) {
  ASSERT(texture);
  impl_->SetUpFont();
  if (impl_->font_ && !impl_->text_.empty()) {
    texture->DrawText(canvas, x, y, width, height,
                      impl_->text_.c_str(), impl_->font_,
                      impl_->align_, impl_->valign_,
                      impl_->trimming_, impl_->flags_);
  }
}

void TextFrame::Draw(CanvasInterface *canvas, double x, double y,
                     double width, double height) {
  DrawWithTexture(canvas, x, y, width, height, impl_->color_texture_);
}

void TextFrame::GetSimpleExtents(double *width, double *height) {
  impl_->SetUpFont();
  *width = impl_->width_;
  *height = impl_->height_;
}

void TextFrame::GetExtents(double in_width, double *width, double *height) {
  impl_->SetUpFont();
  if (in_width >= impl_->width_) {
    *width = impl_->width_;
    *height = impl_->height_;
  } else {
    CanvasInterface *canvas = impl_->view_->GetGraphics()->NewCanvas(5, 5);
    canvas->GetTextExtents(impl_->text_.c_str(), impl_->font_, impl_->flags_,
                           in_width, width, height);
    canvas->Destroy();
  }
}

} // namespace ggadget
