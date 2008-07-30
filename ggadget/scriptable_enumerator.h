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

#ifndef GGADGET_SCRIPTABLE_ENUMERATOR_H__
#define GGADGET_SCRIPTABLE_ENUMERATOR_H__

#include <ggadget/common.h>
#include <ggadget/scriptable_helper.h>

namespace ggadget {

/**
 * This class is used to reflect an enumerator to script.
 * @param E the native enumerator which supports @c AtEnd(), @c MoveFirst, @c
 *     MoveNext() and @c GetItem().
 * @param ClassId the class id of this scriptable enumerator.
 */
template <typename E, uint64_t ClassId>
class ScriptableEnumeratorBase : public ScriptableHelperDefault {
 public:
  static const uint64_t CLASS_ID = ClassId;
  virtual bool IsInstanceOf(uint64_t class_id) const {
    return class_id == CLASS_ID ||
        ScriptableInterface::IsInstanceOf(class_id);
  }
  virtual uint64_t GetClassId() const { return ClassId; }

  ScriptableEnumeratorBase(ScriptableInterface *owner_,
                           E *enumerator_);
  virtual ~ScriptableEnumeratorBase();

 protected:
  virtual void DoClassRegister();
  E *enumerator_;

 private:
  ScriptableInterface *owner_;
};

template <typename E, uint64_t ClassId>
ScriptableEnumeratorBase<E, ClassId>::ScriptableEnumeratorBase(
    ScriptableInterface *owner,
    E *enumerator)
    : enumerator_(enumerator), owner_(owner) {
  ASSERT(enumerator);
  ASSERT(owner);
  owner_->Ref();
}

template <typename E, uint64_t ClassId>
ScriptableEnumeratorBase<E, ClassId>::~ScriptableEnumeratorBase() {
  delete enumerator_;
  owner_->Unref();
}

template <typename E, uint64_t ClassId>
void ScriptableEnumeratorBase<E, ClassId>::DoClassRegister() {
  RegisterMethod("atEnd",
                 NewSlot(&E::AtEnd,
                         &ScriptableEnumeratorBase<E, ClassId>::enumerator_));
  RegisterMethod("moveFirst",
                 NewSlot(&E::MoveFirst,
                         &ScriptableEnumeratorBase<E, ClassId>::enumerator_));
  RegisterMethod("moveNext",
                 NewSlot(&E::MoveNext,
                         &ScriptableEnumeratorBase<E, ClassId>::enumerator_));
}

template <typename E, typename Wrapper, uint64_t ClassId>
class ScriptableEnumerator : public ScriptableEnumeratorBase<E, ClassId> {
 public:
  /** Creates an empty ScriptableEnumeratorBase object. */
  ScriptableEnumerator(ScriptableInterface *owner,
                       E *enumerator)
      : ScriptableEnumeratorBase<E, ClassId>(owner, enumerator) {
  }

  Wrapper *GetItem() {
    return new Wrapper(this->enumerator_->GetItem());
  }

 protected:
  virtual void DoClassRegister() {
    ScriptableEnumeratorBase<E, ClassId>::DoClassRegister();
    RegisterMethod("item",
                   NewSlot(&ScriptableEnumerator<E, Wrapper, ClassId>
                               ::GetItem));
  }
};

template <typename E, uint64_t ClassId>
class ScriptableEnumerator<E, Variant, ClassId>
    : public ScriptableEnumeratorBase<E, ClassId> {
 public:
  /** Creates an empty ScriptableEnumeratorBase object. */
  ScriptableEnumerator(ScriptableInterface *owner,
                       E *enumerator)
      : ScriptableEnumeratorBase<E, ClassId>(owner, enumerator) {
  }

  Variant GetItem() {
    return Variant(this->enumerator_->GetItem());
  }

 protected:
  virtual void DoClassRegister() {
    ScriptableEnumeratorBase<E, ClassId>::DoClassRegister();
    RegisterMethod("item",
                   NewSlot(&ScriptableEnumerator<E, Variant, ClassId>
                               ::GetItem));
  }
};

} // namespace ggadget

#endif // GGADGET_SCRIPTABLE_ENUMERATOR_H__
