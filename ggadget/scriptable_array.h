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
class ScriptableArray : public ScriptableHelper<ScriptableInterface> {
 public:
  DEFINE_CLASS_ID(0x65cf1406985145a9, ScriptableInterface);

  virtual ~ScriptableArray();

  /**
   * Creates a @c ScriptableArray.
   * @param array the array source data. A copy will be made before return.
   * @param count number of elements in the array.
   * @param native_owned if @c true, the created @c ScriptableArray is owned by
   *     the native code and the holder of this pointer is responsible to delete
   *     it. if @c false, the ownership will be transferred to script side.
   */
  template <typename T>
  static ScriptableArray *Create(const T *array, size_t count,
                                 bool native_owned) {
    Variant *variant_array = new Variant[count];
    for (size_t i = 0; i < count; i++)
      variant_array[i] = Variant(array[i]);
    return new ScriptableArray(variant_array, count, native_owned);
  }

  /**
   * Same as above, but accepts a @c NULL terminated array of pointers. 
   */
  template <typename T>
  static ScriptableArray *Create(T *const *array, bool native_owned) {
    size_t size = 0;
    for (; array[size]; size++);
    return Create<T *>(array, size, native_owned);
  }

  size_t GetCount() const;
  Variant GetItem(size_t index) const;

  virtual OwnershipPolicy Attach();
  virtual bool Detach();

 private:
  ScriptableArray(Variant *array, size_t count, bool native_owned);
  DISALLOW_EVIL_CONSTRUCTORS(ScriptableArray);
  class Impl;
  Impl *impl_;
};

} // namespace ggadget

#endif // GGADGET_SCRIPTABLE_ARRAY_H__
