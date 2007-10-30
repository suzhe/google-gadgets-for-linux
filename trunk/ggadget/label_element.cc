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

#include "label_element.h"
#include "graphics_interface.h"
#include "string_utils.h"
#include "view_interface.h"
#include "texture.h"

namespace ggadget {

static const char *kDefaultColor = "#000000";
static const char *kDefaultFont = "Sans";

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
  "character_ellipsis", 
  "word-ellipsis", 
  "path-ellipsis"
};

class LabelElement::Impl {
 public:
  Impl(ViewInterface *view) : color_texture_(view->LoadTexture(kDefaultColor)), 
                              align_(CanvasInterface::ALIGN_LEFT), 
                              valign_(CanvasInterface::VALIGN_TOP),
                              trimming_(CanvasInterface::TRIMMING_NONE), 
                              bold_(false), italic_(false), strikeout_(false),
                              underline_(false), wrap_(false), size_(10),
                              font_(kDefaultFont), color_(kDefaultColor) { }
  ~Impl() {
    delete color_texture_;
    color_texture_ = NULL;
  }

  Texture *color_texture_;
  CanvasInterface::Alignment align_;
  CanvasInterface::VAlignment valign_;
  CanvasInterface::Trimming trimming_;
  bool bold_, italic_, strikeout_, underline_, wrap_;
  int size_;
  std::string font_, color_, text_;
};

LabelElement::LabelElement(ElementInterface *parent,
                       ViewInterface *view,
                       const char *name)
    : BasicElement(parent, view, "label", name, false),
      impl_(new Impl(view)) {
  RegisterProperty("bold",
                   NewSlot(this, &LabelElement::IsBold),
                   NewSlot(this, &LabelElement::SetBold));
  RegisterProperty("color",
                   NewSlot(this, &LabelElement::GetColor),
                   NewSlot(this, &LabelElement::SetColor));
  RegisterProperty("font",
                   NewSlot(this, &LabelElement::GetFont),
                   NewSlot(this, &LabelElement::SetFont));
  RegisterProperty("innerText",
                   NewSlot(this, &LabelElement::GetInnerText),
                   NewSlot(this, &LabelElement::SetInnerText));
  RegisterProperty("italic",
                   NewSlot(this, &LabelElement::IsItalic),
                   NewSlot(this, &LabelElement::SetItalic));
  RegisterProperty("size",
                   NewSlot(this, &LabelElement::GetSize),
                   NewSlot(this, &LabelElement::SetSize));
  RegisterProperty("strikeout",
                   NewSlot(this, &LabelElement::IsStrikeout),
                   NewSlot(this, &LabelElement::SetStrikeout));
  RegisterProperty("underline",
                   NewSlot(this, &LabelElement::IsUnderline),
                   NewSlot(this, &LabelElement::SetUnderline));
  RegisterProperty("wordWrap",
                   NewSlot(this, &LabelElement::IsWordWrap),
                   NewSlot(this, &LabelElement::SetWordWrap));
     
  RegisterStringEnumProperty("align", 
                             NewSlot(this, &LabelElement::GetAlign),
                             NewSlot(this, &LabelElement::SetAlign), 
                             kAlignNames, arraysize(kAlignNames));
  RegisterStringEnumProperty("valign", 
                             NewSlot(this, &LabelElement::GetVAlign),
                             NewSlot(this, &LabelElement::SetVAlign), 
                             kVAlignNames, arraysize(kVAlignNames));
  RegisterStringEnumProperty("trimming", 
                             NewSlot(this, &LabelElement::GetTrimming),
                             NewSlot(this, &LabelElement::SetTrimming), 
                             kTrimmingNames, arraysize(kTrimmingNames));  
}

LabelElement::~LabelElement() {
  delete impl_;
}

void LabelElement::DoDraw(CanvasInterface *canvas,
                        const CanvasInterface *children_canvas) {  
  if (!impl_->text_.empty()) {
    FontInterface *f =
      GetView()->GetGraphics()->NewFont(impl_->font_.c_str(), impl_->size_, 
        impl_->italic_ ? FontInterface::STYLE_ITALIC : FontInterface::STYLE_NORMAL,
        impl_->bold_ ? FontInterface::WEIGHT_BOLD : FontInterface::WEIGHT_NORMAL);
    if (!f) {
      return;
    }
    
    CanvasInterface::TextFlag flag = 0;
    if (impl_->underline_) {
      flag |= CanvasInterface::TEXT_FLAGS_UNDERLINE;
    }
    if (impl_->strikeout_) {
      flag |= CanvasInterface::TEXT_FLAGS_STRIKEOUT;
    }
    if (impl_->wrap_) {
      flag |= CanvasInterface::TEXT_FLAGS_WORDWRAP;
    }

    impl_->color_texture_->DrawText(canvas, 0, 0, GetPixelWidth(), 
                                    GetPixelHeight(), impl_->text_.c_str(), f, 
                                    impl_->align_, impl_->valign_, 
                                    impl_->trimming_, flag);
       
    f->Destroy();
  }  
}

CanvasInterface::Alignment LabelElement::GetAlign() const {
  return impl_->align_;
}

void LabelElement::SetAlign(CanvasInterface::Alignment align) {
  if (align != impl_->align_) {
    align = impl_->align_;
    QueueDraw();
  }
}

bool LabelElement::IsBold() const {
  return impl_->bold_;
}

void LabelElement::SetBold(bool bold) {
  if (bold != impl_->bold_) {
    bold = impl_->bold_;
    QueueDraw();
  }
}

const char *LabelElement::GetColor() const {
  return impl_->color_.c_str();
}

void LabelElement::SetColor(const char *color) {
  if (AssignIfDiffer(color, &impl_->color_)) {
    delete impl_->color_texture_;
    impl_->color_texture_ = GetView()->LoadTexture(color);
    QueueDraw();
  }
}

const char *LabelElement::GetFont() const {
  return impl_->font_.c_str();
}

void LabelElement::SetFont(const char *font) {
  if (AssignIfDiffer(font, &impl_->font_)) {
    QueueDraw();
  }
}

const char *LabelElement::GetInnerText() const {
  return impl_->text_.c_str();
}

void LabelElement::SetInnerText(const char *text) {
  if (AssignIfDiffer(text, &impl_->text_)) {
    QueueDraw();
  }
}

bool LabelElement::IsItalic() const {
  return impl_->italic_;
}

void LabelElement::SetItalic(bool italic) {
  if (italic != impl_->italic_) {
    italic = impl_->italic_;
    QueueDraw();
  }
}

int LabelElement::GetSize() const {
  return impl_->size_;
}

void LabelElement::SetSize(int size) {
  if (size != impl_->size_) {
    size = impl_->size_;
    QueueDraw();
  }
}

bool LabelElement::IsStrikeout() const {
  return impl_->strikeout_;
}

void LabelElement::SetStrikeout(bool strikeout) {
  if (strikeout != impl_->strikeout_) {
    strikeout = impl_->strikeout_;
    QueueDraw();
  }
}

CanvasInterface::Trimming LabelElement::GetTrimming() const {
  return impl_->trimming_;
}

void LabelElement::SetTrimming(CanvasInterface::Trimming trimming) {
  if (trimming != impl_->trimming_) {
    trimming = impl_->trimming_;
    QueueDraw();
  }
}

bool LabelElement::IsUnderline() const {
  return impl_->underline_;
}

void LabelElement::SetUnderline(bool underline) {
  if (underline != impl_->underline_) {
    underline = impl_->underline_;
    QueueDraw();
  }
}

CanvasInterface::VAlignment LabelElement::GetVAlign() const {
  return impl_->valign_;
}

void LabelElement::SetVAlign(CanvasInterface::VAlignment valign) {
  if (valign != impl_->valign_) {
    valign = impl_->valign_;
    QueueDraw();
  }
}

bool LabelElement::IsWordWrap() const {
  return impl_->wrap_;
}

void LabelElement::SetWordWrap(bool wrap) {
  if (wrap != impl_->wrap_) {
    wrap = impl_->wrap_;
    QueueDraw();
  }
}

ElementInterface *LabelElement::CreateInstance(ElementInterface *parent,
                                             ViewInterface *view,
                                             const char *name) {
  return new LabelElement(parent, view, name);
}

} // namespace ggadget
