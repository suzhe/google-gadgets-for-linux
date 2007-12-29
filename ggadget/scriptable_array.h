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

#ifndef GGADGET_SCRIPTABLE_ARRAY_H__
#define GGADGET_SCRIPTABLE_ARRAY_H__

#include <ggadget/common.h>
#include <ggadget/scriptable_helper.h>

namespace ggadget {

/**
 * This class is used to reflect a const native array to script.
 * The script can access this object by getting "count" property and "item"
 * method, or with an Enumerator.
 */
class ScriptableArray : public ScriptableHelperOwnershipShared {
 public:
  DEFINE_CLASS_ID(0x65cf1406985145a9, ScriptableInterface);

  virtual ~ScriptableArray();

  /**
   * Creates a @c ScriptableArray with an iterator and count.
   * @param start the start position of an iterator. It can also be the start
   *     address of an array. A copy of this array will be made.
   * @param count number of elements in the array.
   */
  template <typename I>
  static ScriptableArray *Create(I start, size_t count) {
    Variant *variant_array = new Variant[count];
    for (size_t i = 0; i < count; i++)
      variant_array[i] = Variant(*start++);
    return new ScriptableArray(variant_array, count);
  }

  /**
   * Same as above, but accepts a @c NULL terminated array of pointers.
   * A copy of the input array will be made.
   */
  template <typename T>
  static ScriptableArray *Create(T *const *array) {
    size_t size = 0;
    for (; array[size]; size++);
    return Create(array, size);
  }

  /**
   * Create a @c ScriptableArray with a pre-allocated @c Variant array.
   * The created @c ScriptableArray will take the ownership of the input
   * array.
   */
  static ScriptableArray *Create(Variant *array, size_t count) {
    return new ScriptableArray(array, count);
  }

  size_t GetCount() const;
  Variant GetItem(size_t index) const;

 private:
  ScriptableArray(Variant *array, size_t count);

 private:
  DISALLOW_EVIL_CONSTRUCTORS(ScriptableArray);
  class Impl;
  Impl *impl_;
};

} // namespace ggadget

#endif // GGADGET_SCRIPTABLE_ARRAY_H__
