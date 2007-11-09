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

#include "text_frame.h"
#include "basic_element.h"
#include "graphics_interface.h"
#include "view_interface.h"
#include "texture.h"
#include "string_utils.h"

namespace ggadget{

static const char *const kDefaultColor = "#000000";
static const char *const kDefaultFont = "Sans";

static const char *kAlignNames[] = {
  "left", "center", "right"
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

TextFrame::TextFrame(BasicElement *owner, ViewInterface *view)
  : owner_(owner), view_(view), font_(NULL),
    color_texture_(view->LoadTexture(kDefaultColor)), 
    align_(CanvasInterface::ALIGN_LEFT), 
    valign_(CanvasInterface::VALIGN_TOP),
    trimming_(CanvasInterface::TRIMMING_NONE), 
    bold_(false), italic_(false), flag_(0), size_(10),
    font_name_(kDefaultFont), color_(kDefaultColor) { 
  // Register Properties
  // All properties are registered except for GetText/SetText
  // since some elements call it "caption" while others call it "innerText."
  // Elements may also want to do special handling on SetText.
  owner_->RegisterProperty("bold",
                           NewSlot(this, &TextFrame::IsBold),
                           NewSlot(this, &TextFrame::SetBold));
  owner_->RegisterProperty("color",
                           NewSlot(this, &TextFrame::GetColor),
                           NewSlot(this, &TextFrame::SetColor));
  owner_->RegisterProperty("font",
                           NewSlot(this, &TextFrame::GetFont),
                           NewSlot(this, &TextFrame::SetFont));
  owner_->RegisterProperty("italic",
                           NewSlot(this, &TextFrame::IsItalic),
                           NewSlot(this, &TextFrame::SetItalic));
  owner_->RegisterProperty("size",
                           NewSlot(this, &TextFrame::GetSize),
                           NewSlot(this, &TextFrame::SetSize));
  owner_->RegisterProperty("strikeout",
                           NewSlot(this, &TextFrame::IsStrikeout),
                           NewSlot(this, &TextFrame::SetStrikeout));
  owner_->RegisterProperty("underline",
                           NewSlot(this, &TextFrame::IsUnderline),
                           NewSlot(this, &TextFrame::SetUnderline));
  owner_->RegisterProperty("wordWrap",
                           NewSlot(this, &TextFrame::IsWordWrap),
                           NewSlot(this, &TextFrame::SetWordWrap));
     
  owner_->RegisterStringEnumProperty("align", 
                                     NewSlot(this, &TextFrame::GetAlign),
                                     NewSlot(this, &TextFrame::SetAlign), 
                                     kAlignNames, arraysize(kAlignNames));
  owner_->RegisterStringEnumProperty("valign", 
                                     NewSlot(this, &TextFrame::GetVAlign),
                                     NewSlot(this, &TextFrame::SetVAlign), 
                                     kVAlignNames, arraysize(kVAlignNames));
  owner_->RegisterStringEnumProperty("trimming", 
                                     NewSlot(this, &TextFrame::GetTrimming),
                                     NewSlot(this, &TextFrame::SetTrimming), 
                                     kTrimmingNames, arraysize(kTrimmingNames)); 
}

TextFrame::~TextFrame() {
  delete color_texture_;
  color_texture_ = NULL;
  
  ClearFont();
}

void TextFrame::ClearFont() {
  if (font_) {
    font_->Destroy();
    font_ = NULL;
  }
}

CanvasInterface::Alignment TextFrame::GetAlign() const {
  return align_;
}

void TextFrame::SetAlign(CanvasInterface::Alignment align) {
  if (align != align_) {
    align_ = align;
    owner_->QueueDraw();
  }
}

bool TextFrame::IsBold() const {
  return bold_;
}

void TextFrame::SetBold(bool bold) {
  if (bold != bold_) {
    ClearFont();
    bold_ = bold;
    owner_->OnDefaultSizeChange();
    owner_->QueueDraw();
  }
}

const char *TextFrame::GetColor() const {
  return color_.c_str();
}

void TextFrame::SetColor(const char *color) {
  if (AssignIfDiffer(color, &color_)) {
    delete color_texture_;
    color_texture_ = view_->LoadTexture(color);
    owner_->QueueDraw();
  }
}

const char *TextFrame::GetFont() const {
  return font_name_.c_str();
}

void TextFrame::SetFont(const char *font) {
  if (AssignIfDiffer(font, &font_name_)) {
    ClearFont();
    owner_->OnDefaultSizeChange();
    owner_->QueueDraw();
  }
}

bool TextFrame::IsItalic() const {
  return italic_;
}

void TextFrame::SetItalic(bool italic) {
  if (italic != italic_) {
    ClearFont();
    italic_ = italic;
    owner_->OnDefaultSizeChange();
    owner_->QueueDraw();
  }
}

int TextFrame::GetSize() const {
  return size_;
}

void TextFrame::SetSize(int size) { 
  if (size != size_) {
    ClearFont();
    size_ = size;
    owner_->OnDefaultSizeChange();
    owner_->QueueDraw();
  } 
}

bool TextFrame::IsStrikeout() const {
  return flag_ & CanvasInterface::TEXT_FLAGS_STRIKEOUT;
}

void TextFrame::SetStrikeout(bool strikeout) {
  if (strikeout != !!(flag_ & CanvasInterface::TEXT_FLAGS_STRIKEOUT)) {
    flag_ ^= CanvasInterface::TEXT_FLAGS_STRIKEOUT;
    owner_->QueueDraw();
  }
}

CanvasInterface::Trimming TextFrame::GetTrimming() const {
  return trimming_;
}

void TextFrame::SetTrimming(CanvasInterface::Trimming trimming) {
  if (trimming != trimming_) {
    trimming_ = trimming;
    owner_->QueueDraw();
  }
}

bool TextFrame::IsUnderline() const {
  return flag_ & CanvasInterface::TEXT_FLAGS_UNDERLINE;
}

void TextFrame::SetUnderline(bool underline) {
  if (underline != !!(flag_ & CanvasInterface::TEXT_FLAGS_UNDERLINE)) {
    flag_ ^= CanvasInterface::TEXT_FLAGS_UNDERLINE;
    owner_->OnDefaultSizeChange();
    owner_->QueueDraw();
  }
}

CanvasInterface::VAlignment TextFrame::GetVAlign() const {
  return valign_;
}

void TextFrame::SetVAlign(CanvasInterface::VAlignment valign) {
  if (valign != valign_) {
    valign_ = valign;
    owner_->QueueDraw();
  }
}

bool TextFrame::IsWordWrap() const {
  return flag_ & CanvasInterface::TEXT_FLAGS_WORDWRAP;
}

void TextFrame::SetWordWrap(bool wrap) {
  if (wrap != !!(flag_ & CanvasInterface::TEXT_FLAGS_WORDWRAP)) {
    flag_ ^= CanvasInterface::TEXT_FLAGS_WORDWRAP;
    owner_->OnDefaultSizeChange();
    owner_->QueueDraw();
  }
}

const char *TextFrame::GetText() const {
  return text_.c_str();
}

void TextFrame::SetText(const char *text) {  
  if (AssignIfDiffer(text, &text_)) {
    owner_->OnDefaultSizeChange();
    owner_->QueueDraw();
  } 
}

bool TextFrame::SetUpFont() {
  // The FontInterface object is cached on draw.
  if (!font_) {
    font_ = view_->GetGraphics()->NewFont(font_name_.c_str(), 
        size_, 
        italic_ ? FontInterface::STYLE_ITALIC : FontInterface::STYLE_NORMAL,
        bold_ ? FontInterface::WEIGHT_BOLD : FontInterface::WEIGHT_NORMAL);
    if (!font_) {
      return false;
    }
  }
  return true;
}

void TextFrame::DrawWithTexture(CanvasInterface *canvas, double x, double y, 
                                double width, double height, Texture *texture) {
  if (!text_.empty()) {
    if (!SetUpFont()) {
      return;
    }

    texture->DrawText(canvas, x, y, width, height, text_.c_str(), font_, 
                      align_, valign_, trimming_, flag_);
  }  
}

void TextFrame::Draw(CanvasInterface *canvas, double x, double y, 
                     double width, double height) {
  DrawWithTexture(canvas, x, y, width, height, color_texture_);
}

bool TextFrame::GetSimpleExtents(CanvasInterface *canvas, 
                                 double *width, double *height) {
  if (!SetUpFont()) {
    return false;
  }

  bool result = canvas->GetTextExtents(text_.c_str(), font_, flag_,
                                       width, height);
  DLOG("text extents: %f %f", *width, *height);
  return result;
}

}; // namespace ggadget
