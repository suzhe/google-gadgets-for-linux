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

#include "npapi_plugin_script.h"

#include <ggadget/variant.h>
#include <ggadget/scoped_ptr.h>
#include <ggadget/scriptable_function.h>

#include "npapi_impl.h"

namespace ggadget {
namespace npapi {

Variant ConvertNPToLocal(NPP instance, const NPVariant *np_var) {
  switch (np_var->type) {
    case NPVariantType_Null:
      return Variant(Variant::TYPE_STRING);
    case NPVariantType_Bool:
      return Variant(np_var->value.boolValue);
    case NPVariantType_Int32:
      return Variant(np_var->value.intValue);
    case NPVariantType_Double:
      return Variant(np_var->value.doubleValue);
    case NPVariantType_String:
      return Variant(std::string(np_var->value.stringValue.utf8characters,
                                 np_var->value.stringValue.utf8length));
    case NPVariantType_Object:
     {
      NPPluginObject *obj = new NPPluginObject(instance,
                                               np_var->value.objectValue);
      return Variant(obj);
     }
    default:
      return Variant();
  }
}

void ConvertLocalToNP(NPP instance, const Variant &var, NPVariant *np_var) {
  switch (var.type()) {
    case Variant::TYPE_VOID:
      VOID_TO_NPVARIANT(*np_var);
      break;
    case Variant::TYPE_BOOL:
      BOOLEAN_TO_NPVARIANT(VariantValue<bool>()(var), *np_var);
      break;
    case Variant::TYPE_INT64:
      INT32_TO_NPVARIANT(VariantValue<uint32_t>()(var), *np_var);
      break;
    case Variant::TYPE_DOUBLE:
      DOUBLE_TO_NPVARIANT(VariantValue<double>()(var), *np_var);
      break;
    case Variant::TYPE_STRING:
     {
      const char *s = VariantValue<const char *>()(var);
      size_t size = strlen(s);
      char *new_s = new char[size + 1];
      memcpy(new_s, s, size + 1);
      STRINGZ_TO_NPVARIANT(new_s, *np_var);
      break;
     }
    case Variant::TYPE_SCRIPTABLE:
     {
      NPPluginObject *obj = VariantValue<NPPluginObject *>()(var);
      if (obj) {
        // The object is a scriptable wrapper for a NPObject, returns
        // the NPObject.
        NPObject *np_obj = obj->UnWrap();
        NPAPIImpl::NPN_RetainObject(np_obj);
        OBJECT_TO_NPVARIANT(np_obj, *np_var);
      } else {
        // The object is a native scriptable object, wrap it as a NPObject
        // that can be accessed by Plugin.
        ScriptableInterface *scriptable =
            VariantValue<ScriptableInterface *>()(var);
        NPNativeObject *native_obj = new NPNativeObject(instance, scriptable);
        OBJECT_TO_NPVARIANT(reinterpret_cast<NPObject*>(native_obj), *np_var);
      }
      break;
     }
    case Variant::TYPE_VARIANT:
    case Variant::TYPE_JSON:
    case Variant::TYPE_UTF16STRING:
    case Variant::TYPE_SLOT:
    case Variant::TYPE_DATE:
    case Variant::TYPE_ANY:
    case Variant::TYPE_CONST_ANY:
    default:
      VOID_TO_NPVARIANT(*np_var);
      break;
  }
}

class NPNativeObject::Impl {
 public:
  Impl(NPNativeObject *owner, NPP instance, ScriptableInterface *object)
      : owner_(owner), instance_(instance), native_object_(object)
#ifdef NP_CLASS_STRUCT_VERSION_ENUM
        , property_array_(NULL), property_count_(0)
#endif
  {
    owner->np_obj_._class = new NPClass;
    owner->np_obj_._class->structVersion = NP_CLASS_STRUCT_VERSION;
    owner->np_obj_._class->allocate = NULL;
    owner->np_obj_._class->deallocate = NULL;
    owner->np_obj_._class->invalidate = NULL;
    owner->np_obj_._class->hasMethod = (NPHasMethodFunctionPtr)ScriptableHasMethod;
    owner->np_obj_._class->invoke = (NPInvokeFunctionPtr)ScriptableInvoke;
    owner->np_obj_._class->invokeDefault = NULL;
    owner->np_obj_._class->hasProperty = (NPHasPropertyFunctionPtr)ScriptableHasProperty;
    owner->np_obj_._class->getProperty = (NPGetPropertyFunctionPtr)ScriptableGetProperty;
    owner->np_obj_._class->setProperty = (NPSetPropertyFunctionPtr)ScriptableSetProperty;
    owner->np_obj_._class->removeProperty = (NPRemovePropertyFunctionPtr)ScriptableRemoveProperty;
#ifdef NP_CLASS_STRUCT_VERSION_ENUM
    owner->np_obj_._class->enumerate =(NPEnumerationFunctionPtr)ScriptableEnumerate;
#endif
#ifdef NP_CLASS_STRUCT_VERSION_CTOR
    owner->np_obj_._class->construct = (NPConstructFunctionPtr)ScriptableConstruct;
#endif
    owner->np_obj_.referenceCount = 1;
  }

  ~Impl() {
    delete owner_->np_obj_._class;
  }

  static bool ScriptableHasMethod(NPObject *npobj, NPIdentifier name) {
    if (npobj && name) {
      Impl *impl = GetNativeObjectImpl(npobj);
      ScriptableInterface *scriptable = impl->native_object_;
      if (scriptable) {
        if (NPAPIImpl::NPN_IdentifierIsString(name)) {
          NPUTF8 *property_name = NPAPIImpl::NPN_UTF8FromIdentifier(name);
          Variant prototype = scriptable->GetProperty(property_name).v();
          NPAPIImpl::NPN_MemFree(property_name);
          return prototype.type() == Variant::TYPE_SLOT;
        } else {
          int32_t id = NPAPIImpl::NPN_IntFromIdentifier(name);
          Variant prototype = scriptable->GetPropertyByIndex(static_cast<int>(id)).v();
          return prototype.type() == Variant::TYPE_SLOT;
        }
      }
    }
    return false;
  }

  static bool ScriptableInvoke(NPObject *npobj, NPIdentifier name,
                               const NPVariant *args, uint32_t argCount,
                               NPVariant *result) {
    if (npobj && name) {
      Impl *impl = GetNativeObjectImpl(npobj);
      ScriptableInterface *scriptable = impl->native_object_;
      if (scriptable) {
        Variant prototype;
        if (NPAPIImpl::NPN_IdentifierIsString(name)) {
          NPUTF8 *property_name = NPAPIImpl::NPN_UTF8FromIdentifier(name);
          prototype = scriptable->GetProperty(property_name).v();
          NPAPIImpl::NPN_MemFree(property_name);
        } else {
          int32_t id = NPAPIImpl::NPN_IntFromIdentifier(name);
          prototype = scriptable->GetPropertyByIndex(static_cast<int>(id)).v();
        }
        if (prototype.type() == Variant::TYPE_SLOT) {
          scoped_array<Variant> argv(new Variant[argCount]);
          for (uint32_t i = 0; i < argCount; ++i)
            argv[i] = ConvertNPToLocal(impl->instance_, &args[i]);
          Slot *slot = VariantValue<Slot *>()(prototype);
          ResultVariant ret = slot->Call(NULL, argCount, argv.get());
          for (uint32_t i = 0; i < argCount; ++i)
            if (argv[i].type() == Variant::TYPE_SCRIPTABLE)
              delete VariantValue<NPPluginObject *>()(argv[i]);
          if (result)
            ConvertLocalToNP(impl->instance_, ret.v(), result);
          return true;
        }
      }
    }
    return false;
  }

  static bool ScriptableHasProperty(NPObject *npobj, NPIdentifier name) {
    if (npobj && name) {
      Impl *impl = GetNativeObjectImpl(npobj);
      ScriptableInterface *scriptable = impl->native_object_;
      if (scriptable) {
        if (NPAPIImpl::NPN_IdentifierIsString(name)) {
          NPUTF8 *property_name = NPAPIImpl::NPN_UTF8FromIdentifier(name);
          Variant prototype = scriptable->GetProperty(property_name).v();
          NPAPIImpl::NPN_MemFree(property_name);
          return (prototype.type() != Variant::TYPE_VOID &&
                  prototype.type() != Variant::TYPE_SLOT);
        } else {
          int32_t id = NPAPIImpl::NPN_IntFromIdentifier(name);
          Variant prototype = scriptable->GetPropertyByIndex(static_cast<int>(id)).v();
          return (prototype.type() != Variant::TYPE_VOID &&
                  prototype.type() != Variant::TYPE_SLOT);
        }
      }
    }
    return false;
  }

  static bool ScriptableGetProperty(NPObject *npobj, NPIdentifier name,
                                    NPVariant *result) {
    if (npobj && name && result) {
      Impl *impl = GetNativeObjectImpl(npobj);
      ScriptableInterface *scriptable = impl->native_object_;
      if (scriptable) {
        Variant prototype;
        if (NPAPIImpl::NPN_IdentifierIsString(name)) {
          NPUTF8 *property_name = NPAPIImpl::NPN_UTF8FromIdentifier(name);
          prototype = scriptable->GetProperty(property_name).v();
          NPAPIImpl::NPN_MemFree(property_name);
        } else {
          int32_t id = NPAPIImpl::NPN_IntFromIdentifier(name);
          prototype = scriptable->GetPropertyByIndex(static_cast<int>(id)).v();
        }
        if (prototype.type() != Variant::TYPE_VOID) {
          ConvertLocalToNP(impl->instance_, prototype, result);
          return true;
        }
      }
    }
    return false;
  }

  static bool ScriptableSetProperty(NPObject *npobj, NPIdentifier name,
                                    const NPVariant *value) {
    if (npobj && name && value) {
      Impl *impl = GetNativeObjectImpl(npobj);
      ScriptableInterface *scriptable = impl->native_object_;
      if (scriptable) {
        bool ret;
        Variant param = ConvertNPToLocal(impl->instance_, value);
        if (NPAPIImpl::NPN_IdentifierIsString(name)) {
          NPUTF8 *property_name = NPAPIImpl::NPN_UTF8FromIdentifier(name);
          ret = scriptable->SetProperty(property_name, param);
          NPAPIImpl::NPN_MemFree(property_name);
        } else {
          int32_t id = NPAPIImpl::NPN_IntFromIdentifier(name);
          ret = scriptable->SetPropertyByIndex(static_cast<int>(id), param);
        }
        if (param.type() == Variant::TYPE_SCRIPTABLE)
          delete VariantValue<NPPluginObject *>()(param);
        return ret;
      }
    }
    return false;
  }

  static bool ScriptableRemoveProperty(NPObject *npobj, NPIdentifier name) {
    // Removing property is not supported by ScriptableInterface.
    return false;
  }

#ifdef NP_CLASS_STRUCT_VERSION_ENUM
  static bool ScriptableEnumerate(NPObject *npobj, NPIdentifier **value,
                                  uint32_t *count) {
    if (npobj && value && count) {
      Impl *impl = GetNativeObjectImpl(npobj);
      if (!impl->property_array_) {
        ScriptableInterface *scriptable = impl->native_object_;
        if (scriptable) {
          scriptable->EnumerateProperties(NewSlot(impl, &Impl::EnumerateProperties));
          scriptable->EnumerateElements(NewSlot(impl, &Impl::EnumerateElements));
        }
      }
      *value = impl->property_array_;
      *count = impl->property_count_;
      return true;
    }
    return false;
  }
#endif

#ifdef NP_CLASS_STRUCT_VERSION_CTOR
  static bool ScriptableConstruct(NPObject *npobj,
                                  const NPVariant *args, uint32_t argCount,
                                  NPVariant *result) {
    return false;
  }
#endif

  static Impl *GetNativeObjectImpl(NPObject *npobj) {
    return (reinterpret_cast<NPNativeObject *>(npobj))->impl_;
  }

#ifdef NP_CLASS_STRUCT_VERSION_ENUM
  bool EnumerateProperties(const char *name,
                           ScriptableInterface::PropertyType type,
                           const Variant &prototype) {
    property_count_++;
    NPIdentifier *array = reinterpret_cast<NPIdentifier *>(
        NPAPIImpl::NPN_MemAlloc(sizeof(NPIdentifier) * property_count_));
    memcpy(array, property_array_, sizeof(NPIdentifier) * (property_count_ - 1));
    array[property_count_ - 1] = NPAPIImpl::NPN_GetStringIdentifier(name);
    NPAPIImpl::NPN_MemFree(property_array_);
    property_array_ = array;
    return false;
  }

  bool EnumerateElements(int index, const Variant &prototype) {
    property_count_++;
    NPIdentifier *array = reinterpret_cast<NPIdentifier *>(
        NPAPIImpl::NPN_MemAlloc(sizeof(NPIdentifier) * property_count_));
    memcpy(array, property_array_, sizeof(NPIdentifier) * (property_count_ - 1));
    array[property_count_ - 1] =
        NPAPIImpl::NPN_GetIntIdentifier(static_cast<int32_t>(index));
    NPAPIImpl::NPN_MemFree(property_array_);
    property_array_ = array;
    return false;
  }
#endif

  NPNativeObject *owner_;
  NPP instance_;
  ScriptableInterface *native_object_;
#ifdef NP_CLASS_STRUCT_VERSION_ENUM
  NPIdentifier *property_array_;
  uint32_t property_count_;
#endif
};

NPNativeObject::NPNativeObject(NPP instance, ScriptableInterface *object)
    : impl_(new Impl(this, instance, object)) {
}

NPNativeObject::~NPNativeObject() {
}

ScriptableInterface *NPNativeObject::UnWrap() {
  return impl_->native_object_;
}

class NPPluginObject::Impl {
 public:
  class NPSlot : public Slot {
   public:
    NPSlot(Impl *owner, NPIdentifier id) : owner_(owner), id_(id) { }
    ~NPSlot() {
      NPAPIImpl::NPN_MemFree(id_);
    }

    // We don't know how many arguments the plugin function can receive.
    // Rely on the plugin to report error if any exists.
    virtual int GetArgCount() const {
      return INT_MAX;
    }

    virtual ResultVariant Call(ScriptableInterface *object,
                               int argc, const Variant argv[]) const {
      NPP instance = owner_->instance_;
      NPObject *np_obj = owner_->np_obj_;

      scoped_array<NPVariant> args(new NPVariant[argc]);
      for (int i = 0; i < argc; ++i) {
        ConvertLocalToNP(instance, argv[i], &args[i]);
      }

      NPVariant result;
      bool ok = NPAPIImpl::NPN_Invoke(instance, np_obj, id_,
                                      args.get(), argc, &result);
      for (int i = 0; i < argc; ++i)
        NPAPIImpl::NPN_ReleaseVariantValue(&args[i]);
      if (ok) {
        ResultVariant ret(ConvertNPToLocal(instance, &result));
        NPAPIImpl::NPN_ReleaseVariantValue(&result);
        return ret;
      }
      return ResultVariant(Variant());
    }

    virtual bool operator==(const Slot &another) const {
      const NPSlot *a = down_cast<const NPSlot *>(&another);
      return a && owner_ == a->owner_ && id_ == a->id_;
    }

   private:
    DISALLOW_EVIL_CONSTRUCTORS(NPSlot);
    Impl *owner_;
    NPIdentifier id_;
  };

  Impl(NPPluginObject *owner, NPP instance, NPObject *np_obj)
      : owner_(owner), instance_(instance), np_obj_(np_obj) {
    NPAPIImpl::NPN_RetainObject(np_obj_);
    owner->SetDynamicPropertyHandler(NewSlot(this, &Impl::GetDynamicProperty),
                                     NewSlot(this, &Impl::SetDynamicProperty));
    owner->SetArrayHandler(NewSlot(this, &Impl::GetArrayProperty),
                           NewSlot(this, &Impl::SetArrayProperty));
  }

  ~Impl() {
    NPAPIImpl::NPN_ReleaseObject(np_obj_);
  }

  Variant GetDynamicProperty(const char *name) {
    if (!instance_ || !np_obj_ )
      return Variant();
    NPIdentifier id = NPAPIImpl::NPN_GetStringIdentifier(name);
    return GetProperty(id);
  }

  bool SetDynamicProperty(const char *name, const Variant &value) {
    if (!instance_ || !np_obj_ )
      return false;
    NPIdentifier id = NPAPIImpl::NPN_GetStringIdentifier(name);
    return SetProperty(id, value);
  }

  Variant GetArrayProperty(int index) {
    if (!instance_ || !np_obj_)
      return Variant();
    NPIdentifier id =
        NPAPIImpl::NPN_GetIntIdentifier(static_cast<int32_t>(index));
    return GetProperty(id);
  }

  bool SetArrayProperty(int index, const Variant &value) {
    if (!instance_ || !np_obj_)
      return false;
    NPIdentifier id =
        NPAPIImpl::NPN_GetIntIdentifier(static_cast<int32_t>(index));
    return SetProperty(id, value);
  }

  Variant GetProperty(NPIdentifier id) {
    scoped_ptr<_NPIdentifier> free_id(id);
    if (np_obj_->_class->hasProperty) {
      if (np_obj_->_class->hasProperty(np_obj_, id)) {
        scoped_ptr<NPVariant> result(new NPVariant);
        if (np_obj_->_class->getProperty &&
            np_obj_->_class->getProperty(np_obj_, id, result.get())) {
          return ConvertNPToLocal(instance_, result.get());
        }
      }
    }
    if (np_obj_->_class->hasMethod) {
      if (np_obj_->_class->hasMethod(np_obj_, id)) {
        // NPSlot owns the identifier.
        free_id.release();
        NPSlot *slot = new NPSlot(this, id);
        return Variant(new ScriptableFunction(slot));
      }
    }
    return Variant();
  }

  bool SetProperty(NPIdentifier id, const Variant &value) {
    scoped_ptr<_NPIdentifier> free_id(id);
    if (np_obj_->_class->hasProperty &&
        np_obj_->_class->hasProperty(np_obj_, id)) {
      NPVariant np_value;
      ConvertLocalToNP(instance_, value, &np_value);
      if (np_obj_->_class->setProperty(np_obj_, id, &np_value)) {
        return true;
      }
    }
    return false;
  }

  NPPluginObject *owner_;
  NPP instance_;
  NPObject *np_obj_;
};

NPPluginObject::NPPluginObject(NPP instance, NPObject *np_obj)
    : impl_(new Impl(this, instance, np_obj)) {
}

NPPluginObject::~NPPluginObject() {
  delete impl_;
}

NPObject *NPPluginObject::UnWrap() {
  return impl_->np_obj_;
}

} // namespace ggadget
} // namespace npapi
