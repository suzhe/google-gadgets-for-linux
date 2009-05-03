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

#include <ggadget/common.h>
#include <ggadget/element_factory.h>
#include <ggadget/view.h>
#include <ggadget/basic_element.h>
#include <ggadget/scriptable_helper.h>

namespace ggadget {
namespace internal {

static const char kHtmlFlashCode[] =
  "<html>\n"
  "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">\n"
  "<body>\n"
  "<embed src=\"\" quality=\"high\" bgcolor=\"#ffffff\" width=\"100%\" "
  "height=\"100%\" type=\"application/x-shockwave-flash\" "
  "swLiveConnect=\"true\" wmode=\"transparent\" name=\"movieObject\" "
  "pluginspage=\"http://www.macromedia.com/go/getflashplayer\"/>\n"
  "</body>\n"
  "<script language=\"JavaScript\">\n"
  "window.external.movieObject = window.document.movieObject;\n"
  "</script>\n"
  "</html>";

class HtmlFlashElement : public BasicElement {
  class ExternalObject : public ScriptableHelperNativeOwnedDefault {
   public:
    DEFINE_CLASS_ID(0x64eaa63bd2cc4efb, ScriptableInterface);

    ExternalObject(HtmlFlashElement *owner) : owner_(owner) { }

   protected:
    virtual void DoClassRegister() {
      RegisterProperty("movieObject", NULL,
                       NewSlot(&ExternalObject::SetMovieObject));
    }

   private:
    void SetMovieObject(ScriptableInterface *movie_object) {
      DLOG("SetMovieObject: %p, Id=%jx",
           movie_object, movie_object ? movie_object->GetClassId() : 0);
      owner_->movie_object_.Reset(movie_object);
    }

   private:
     HtmlFlashElement *owner_;
  };

 public:
  DEFINE_CLASS_ID(0x2613c535747940a6, BasicElement);

  HtmlFlashElement(View *view, const char *name)
    : BasicElement(view, "flash", name, false),
      browser_(view->GetElementFactory()->CreateElement("_browser", view, "")),
      external_(this) {
    if (browser_) {
      browser_->SetParentElement(this);
      if (!browser_->SetProperty("external", Variant(&external_)) ||
          !browser_->SetProperty("innerText", Variant(kHtmlFlashCode))) {
        DLOG("Invalid browser element.");
        delete browser_;
        browser_ = NULL;
      }
    } else {
      DLOG("Failed to create _browser element.");
    }
  }

  virtual ~HtmlFlashElement() {
    movie_object_.Reset(NULL);
    delete browser_;
  }

  static BasicElement *CreateInstance(View *view, const char *name) {
    return new HtmlFlashElement(view, name);
  }

 public:
  virtual void Layout() {
    BasicElement::Layout();
    if (browser_)
      browser_->Layout();
  }

 protected:
  virtual void DoClassRegister() {
    // It's not necessary to call BasicElement::DoClassRegister(),
    // if it's loaded in object element.
    BasicElement::DoClassRegister();
    RegisterProperty("movie",
                     NewSlot(&HtmlFlashElement::GetSrc),
                     NewSlot(&HtmlFlashElement::SetSrc));
    RegisterProperty("src",
                     NewSlot(&HtmlFlashElement::GetSrc),
                     NewSlot(&HtmlFlashElement::SetSrc));
  }

  virtual void DoRegister() {
    SetDynamicPropertyHandler(NewSlot(this, &HtmlFlashElement::GetProperty),
                              NewSlot(this, &HtmlFlashElement::SetProperty));
  }

  virtual void DoDraw(CanvasInterface *canvas) {
    if (browser_)
      browser_->Draw(canvas);
  }

  virtual EventResult HandleMouseEvent(const MouseEvent &event) {
    BasicElement *fired, *in;
    ViewInterface::HitTest hittest;
    return browser_ ?
        browser_->OnMouseEvent(event, true, &fired, &in, &hittest) :
        EVENT_RESULT_UNHANDLED;
  }

  virtual EventResult HandleDragEvent(const DragEvent &event) {
    BasicElement *fired;
    return browser_ ? browser_->OnDragEvent(event, true, &fired) :
        EVENT_RESULT_UNHANDLED;
  }

  virtual EventResult HandleKeyEvent(const KeyboardEvent &event) {
    return browser_ ? browser_->OnKeyEvent(event) : EVENT_RESULT_UNHANDLED;
  }

  virtual EventResult HandleOtherEvent(const Event &event) {
    return browser_ ? browser_->OnOtherEvent(event) : EVENT_RESULT_UNHANDLED;
  }

 private:
  Variant GetProperty(const std::string &name) {
    if (movie_object_.Get()) {
      Variant value;
      ScriptableInterface *scriptable = NULL;
      { // Life scope of ResultVariant result.
        ResultVariant result = movie_object_.Get()->GetProperty(name.c_str());
        value = result.v();
        if (value.type() == Variant::TYPE_SCRIPTABLE) {
          scriptable = VariantValue<ScriptableInterface *>()(value);
          // Add a reference to prevent ResultVariant from deleting the object.
          if (scriptable)
            scriptable->Ref();
        }
      }
      // Release the temporary reference but don't delete the object.
      if (scriptable)
        scriptable->Unref(true);
      return value;
    }
    return Variant();
  }

  bool SetProperty(const std::string &name, const Variant &value) {
    if (movie_object_.Get())
      return movie_object_.Get()->SetProperty(name.c_str(), value);
    return false;
  }

  void SetSrc(const char *src) {
    if (movie_object_.Get())
      movie_object_.Get()->SetProperty("src", Variant(src));
  }

  std::string GetSrc() {
    if (movie_object_.Get()) {
      ResultVariant result = movie_object_.Get()->GetProperty("src");
      if (result.v().type() == Variant::TYPE_STRING) {
        return VariantValue<std::string>()(result.v());
      }
    }
    return "";
  }

 private:
  DISALLOW_EVIL_CONSTRUCTORS(HtmlFlashElement);

  BasicElement *browser_;
  ScriptableHolder<ScriptableInterface> movie_object_;
  ExternalObject external_;
};

} // namespace internal
} // namespace ggadget

#define Initialize html_flash_element_LTX_Initialize
#define Finalize html_flash_element_LTX_Finalize
#define RegisterElementExtension html_flash_element_LTX_RegisterElementExtension

extern "C" {
  bool Initialize() {
    LOGI("Initialize html_flash_element extension.");
    return true;
  }

  void Finalize() {
    LOGI("Finalize html_flash_element extension.");
  }

  bool RegisterElementExtension(ggadget::ElementFactory *factory) {
    if (factory) {
      LOGI("Register html_flash_element extension, using name \"flash\".");
      factory->RegisterElementClass(
          "clsid:D27CDB6E-AE6D-11CF-96B8-444553540000",
          &ggadget::internal::HtmlFlashElement::CreateInstance);
      factory->RegisterElementClass(
          "progid:ShockwaveFlash.ShockwaveFlash.9",
          &ggadget::internal::HtmlFlashElement::CreateInstance);
      factory->RegisterElementClass(
          "progid:ShockwaveFlash.ShockwaveFlash",
          &ggadget::internal::HtmlFlashElement::CreateInstance);
      factory->RegisterElementClass(
          "flash",
          &ggadget::internal::HtmlFlashElement::CreateInstance);
    }
    return true;
  }
}
