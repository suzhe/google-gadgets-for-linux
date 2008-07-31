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
 * @param E the native enumerator.
 * @param ClassId the class id of this scriptable enumerator.
 *
 * An enumerator must at least support the following operations:
 * <code>
 * class NativeEnumerator {
 *   void Destroy();
 *   bool AtEnd();
 *   ItemType GetItem();
 *   bool MoveFirst();
 *   bool MoveNext();
 * };
 * </code>
 */
template <typename E, typename Wrapper, uint64_t ClassId>
class ScriptableEnumerator : public SharedScriptable<ClassId> {
 public:
  ScriptableEnumerator(ScriptableInterface *owner,
                       E *enumerator)
      : enumerator_(enumerator), owner_(owner) {
    ASSERT(enumerator);
    ASSERT(owner);
    owner_->Ref();
  }

  virtual ~ScriptableEnumerator() {
    enumerator_->Destroy();
    owner_->Unref();
  }

  Wrapper *GetItem() {
    return new Wrapper(this->enumerator_->GetItem());
  }

 protected:
  virtual void DoClassRegister() {
    RegisterMethod("atEnd",
                   NewSlot(&E::AtEnd,
                           &ScriptableEnumerator<E, Wrapper, ClassId>
                               ::enumerator_));
    RegisterMethod("moveFirst",
                   NewSlot(&E::MoveFirst,
                           &ScriptableEnumerator<E, Wrapper, ClassId>
                               ::enumerator_));
    RegisterMethod("moveNext",
                   NewSlot(&E::MoveNext,
                           &ScriptableEnumerator<E, Wrapper, ClassId>
                               ::enumerator_));
    RegisterMethod("item",
                   NewSlot(&ScriptableEnumerator<E, Wrapper, ClassId>
                               ::GetItem));
  }
  E *enumerator_;

 private:
  ScriptableInterface *owner_;
};

} // namespace ggadget

#endif // GGADGET_SCRIPTABLE_ENUMERATOR_H__
