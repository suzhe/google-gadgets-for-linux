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

#include "object_element.h"

#include <string>

#include "canvas_interface.h"
#include "elements.h"
#include "element_factory.h"
#include "view.h"

namespace ggadget {

class ObjectElement::Impl {
 public:
  Impl(ObjectElement *owner, View *view)
      : owner_(owner), view_(view), object_(NULL) {
  }
  ~Impl() {
    delete object_;
  }

  void SetObjectClassId(const std::string& classid) {
    delete object_;
    object_ = view_->GetElementFactory()->CreateElement(
        classid.c_str(), owner_, view_, owner_->GetName().c_str());
    if (!object_)
      return;
    classid_ = classid;
  }

  ObjectElement *owner_;
  View *view_;

  BasicElement *object_;
  std::string classid_;
};

ObjectElement::ObjectElement(BasicElement *parent, View *view, const char *name)
    : BasicElement(parent, view, "object", name, false),
      impl_(new Impl(this, view)) {
}

ObjectElement::~ObjectElement() {
  delete impl_;
}

void ObjectElement::DoRegister() {
  BasicElement::DoRegister();
  RegisterProperty("classId",
                   NewSlot(this, &ObjectElement::GetObjectClassId),
                   NewSlot(this, &ObjectElement::SetObjectClassId));
}

BasicElement *ObjectElement::CreateInstance(BasicElement *parent, View *view,
                                            const char *name) {
  return new ObjectElement(parent, view, name);
}

BasicElement *ObjectElement::GetObject() {
  return impl_->object_;
}

const std::string& ObjectElement::GetObjectClassId() const {
  return impl_->classid_;
}

void ObjectElement::SetObjectClassId(const std::string& classid) {
  impl_->SetObjectClassId(classid);
  if (impl_->object_)
    RegisterConstant("object", impl_->object_);
}

void ObjectElement::Layout() {
  BasicElement::Layout();
  if (impl_->object_)
    impl_->object_->Layout();
}

void ObjectElement::DoDraw(CanvasInterface *canvas) {
  if (impl_->object_)
    impl_->object_->Draw(canvas);
}

} // namespace ggadget