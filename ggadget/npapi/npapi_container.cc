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

#include "npapi_container.h"

#include <algorithm>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <dlfcn.h>
#include <unistd.h>
#include <stdlib.h>

#include <ggadget/logger.h>
#include <ggadget/scoped_ptr.h>
#include <ggadget/string_utils.h>
#include <ggadget/system_utils.h>

#include <third_party/npapi/npupp.h>
#include <third_party/npapi/npapi.h>

#include "npapi_impl.h"

namespace ggadget {
namespace npapi {

#define NOT_IMPLEMENTED() \
  LOGW("Unimplemented function %s at line %d\n", __func__, __LINE__)

#define ENV_BROWSER_PLUGINS_DIR "BROWSER_PLUGINS_DIR"

typedef char * (*NP_GetMIMEDescriptionUPP)(void);
typedef NPError (*NP_GetValueUPP)(void *instance, NPPVariable variable, void *value);
typedef NPError (*NP_InitializeUPP)(NPNetscapeFuncs *moz_funcs, NPPluginFuncs *plugin_funcs);
typedef NPError (*NP_ShutdownUPP)(void);

// Every plugin must exports these four APIs.
struct PluginSymbol {
  NP_GetMIMEDescriptionUPP Plugin_NP_GetMIMEDescription_;
  NP_GetValueUPP Plugin_NP_GetValue_;
  NP_InitializeUPP Plugin_NP_Initialize_;
  NP_ShutdownUPP Plugin_NP_Shutdown_;
};

class NPContainer::Impl {
 public:
  class NPPluginWrapper {
   public:
    NPPluginWrapper(void *handle,
                    const std::vector<std::string> &mime_types,
                    PluginSymbol *symbols)
        : handle_(handle), mime_types_(mime_types), symbols_(symbols),
          good_plugin_(true), reference_(0) {
      // FIXME: what's the correct logic?
      // Original code:
      // ASSERT(handle_ && !mime_types.empty() || symbols);
      ASSERT(handle_ && !mime_types.empty() && symbols);

      // Initialize the plugin, get functions exported by the plugin.
      NPNetscapeFuncs container_funcs;
      NPAPIImpl::InitContainerFuncs(&container_funcs);
      plugin_funcs_.size = sizeof(plugin_funcs_);
      NPError ret = symbols_->Plugin_NP_Initialize_(&container_funcs,
                                                    &plugin_funcs_);
      if (ret != NPERR_NO_ERROR) {
        LOGE("Failed to initialize plugin - nperror code %d", ret);
        dlclose(handle);
        good_plugin_ = false;
        return;
      }

      // Get the name and description of the plugin.
      char *name = NULL, *desc = NULL;
      symbols_->Plugin_NP_GetValue_(NULL, NPPVpluginNameString, &name);
      symbols_->Plugin_NP_GetValue_(NULL, NPPVpluginDescriptionString, &desc);
      if (name)
        name_ = name;
      if (desc)
        description_ = desc;
    }

    NPPlugin *NewPluginInstance(const char *mime_type, BasicElement *element,
                                bool xembed, ToolkitType toolkit,
                                int argc, char *argn[], char *argv[]) {
      if (!good_plugin_ || !mime_type)
        return NULL;
      if (std::find(mime_types_.begin(), mime_types_.end(), mime_type) ==
          mime_types_.end()) {
        LOGE("The plugin(%s) cannot handle this MIME type(%s)",
             name_.c_str(), mime_type);
        return NULL;
      }

      // We must construct the new plugin before calling NPP_New, because
      // NPP_New may call back to set properties (e.g. NPN_SetValue) before it
      // returns.
      NPP instance = reinterpret_cast<NPP>(new char[sizeof(*instance)]);
      NPPlugin *plugin = NPContainer::DoNewPlugin(mime_type, element,
                                                  name_.c_str(),
                                                  description_.c_str(),
                                                  instance, &plugin_funcs_,
                                                  xembed, toolkit);
      instance->ndata = plugin;
      NPError ret = plugin_funcs_.newp(const_cast<NPMIMEType>(mime_type),
                                       instance,
                                       // We only use EMBED, but not FULL mode.
                                       NP_EMBED,
                                       argc, argn, argv, NULL);
      if (ret == NPERR_NO_ERROR) {
        reference_++;
        plugin2instance_[plugin] = instance;
        return plugin;
      }
      NPContainer::DoDeletePlugin(plugin);
      delete instance;
      return NULL;
    }

    void DestroyPluginInstance(NPPlugin *plugin) {
      NPP instance = plugin2instance_[plugin];
      ASSERT(instance);
      NPError ret = plugin_funcs_.destroy(instance, NULL);
      if (ret != NPERR_NO_ERROR) {
        LOGE("Failed to destroy plugin instance - nperror code %d.", ret);
      }
      plugin2instance_.erase(plugin);
      NPContainer::DoDeletePlugin(plugin);
      delete instance;
      if (--reference_ == 0) {
        ASSERT(plugin2instance_.empty());
        delete this;
      }
    }

   private:
    // Set private to avoid being deleted directly.
    ~NPPluginWrapper() {
      NPError ret = symbols_->Plugin_NP_Shutdown_();
      if (ret != NPERR_NO_ERROR) {
        LOGE("Failed to shutdown plugin - nperror code %d", ret);
      }
      dlclose(handle_);
      delete symbols_;
    }

    // File handle of the plugin library, while should be closed when the
    // plugin is unloaded.
    void *handle_;

    // MIME types this plugin can handle.
    std::vector<std::string> mime_types_;

    // Basic functions exported by the plugin to initialize itself,
    PluginSymbol *symbols_;

    // Name and description of the plugin.
    std::string name_;
    std::string description_;

    // Plugin-side APIs exported by the plugin.
    NPPluginFuncs plugin_funcs_;

    bool good_plugin_;
    size_t reference_;
    std::map<NPPlugin *, NPP> plugin2instance_;
  };

  std::vector<std::string> GetPluginPaths(const char *mime_type) {
    std::vector<std::string> paths;
    if (type2path_.find(mime_type) != type2path_.end()) {
      // There's already a plugin path cached for this type.
      paths.push_back(type2path_[mime_type]);
    } else {
      // Get paths of all NPAPI-compatible plugins. Check the environment
      // variable first. And then the default directory user specified during
      // compiling.
      std::vector<std::string> dirs;
      char *env_paths = getenv(ENV_BROWSER_PLUGINS_DIR);
      if (env_paths)
         SplitStringList(env_paths, ":", &dirs);
#ifdef GGL_DEFAULT_BROWSER_PLUGINS_DIR
      dirs.push_back(GGL_DEFAULT_BROWSER_PLUGINS_DIR);
#endif
      for (size_t i = 0; i < dirs.size(); i++) {
        struct stat state_value;
        if(::stat(dirs[i].c_str(), &state_value) != 0)
          continue;
        ::DIR *dp = ::opendir(dirs[i].c_str());
        if (dp) {
          struct dirent *dr;
          std::string lib;
          while ((dr = ::readdir(dp))) {
            lib = dr->d_name;
            std::string::size_type pos = lib.find_last_of(".");
            if (pos != lib.npos &&
                lib.compare(pos, 3, ".so") == 0)
              paths.push_back(dirs[i] + "/" + lib);
          }
          ::closedir(dp);
        }
      }
    }
    return paths;
  }

  bool PluginInitialized(const char *mime_type) {
    return (type2wrapper_.find(mime_type) != type2wrapper_.end());
  }

  NPPlugin *NewPlugin(const char *mime_type, BasicElement *element,
                      bool xembed, ToolkitType toolkit,
                      int argc, char *argn[], char *argv[]) {
    if (!PluginInitialized(mime_type)) {
      return NULL;
    }
    NPPluginWrapper *wrapper = type2wrapper_[mime_type];
    NPPlugin *plugin = wrapper->NewPluginInstance(mime_type, element,
                                                  xembed, toolkit,
                                                  argc, argn, argv);
    if (plugin)
      plugin2wrapper_[plugin] = wrapper;
    return plugin;
  }

  NPPlugin *InitAndNewPlugin(void *handle, const char *path,
                             PluginSymbol *symbols,
                             const char *mime_type, BasicElement *element,
                             bool xembed, ToolkitType toolkit,
                             int argc, char *argn[], char *argv[]) {
    if (!handle || !path || !symbols || !mime_type)
      return NULL;

    // Get the set of MIME types that the plugin can handle.
    std::string MIMEDescription = symbols->Plugin_NP_GetMIMEDescription_();
    std::vector<std::string> types;
    SplitStringList(MIMEDescription, ";", &types);
    for (size_t i = 0; i < types.size(); i++) {
      std::string &type = types[i];
      std::string::size_type pos = type.find(':', 0);
      if (pos != type.npos)
        type.erase(pos);
    }

    for (size_t i = 0; i < types.size(); i++) {
      if (types[i].compare(mime_type) == 0) {
        // The plugin is compatible with the target MIME type.
        // Create a plugin wrapper for the plugin library.
        NPPluginWrapper *plugin_wrapper =
            new NPPluginWrapper(handle, types, symbols);

        // Save each <type, pluginwrapper> pair so that we can use the
        // pluginwrapper next time when we meet the corresponding type.
        // Don't overwrite.
        for (size_t j = 0; j < types.size(); j++)
          if (type2wrapper_.find(types[j]) == type2wrapper_.end())
            type2wrapper_[types[j]] = plugin_wrapper;

        NPPlugin *plugin =
            plugin_wrapper->NewPluginInstance(mime_type, element,
                                              xembed, toolkit,
                                              argc, argn, argv);
        plugin2wrapper_[plugin] = plugin_wrapper;
        return plugin;
      } else {
        // Save the plugin path so we can find it immediately next time
        // when we meet the corresponding type.
        if (type2path_.find(types[i]) == type2path_.end())
          type2path_[types[i]] = path;
      }
    }

    return NULL;
  }

  bool DestroyPlugin(NPPlugin *plugin) {
    if (!plugin)
      return false;
    NPPluginWrapper *wrapper = plugin2wrapper_[plugin];
    ASSERT(wrapper);
    plugin2wrapper_.erase(plugin);
    wrapper->DestroyPluginInstance(plugin);
    return true;
  }

 private:
  std::map<std::string, std::string> type2path_;
  std::map<std::string, NPPluginWrapper *> type2wrapper_;
  std::map<NPPlugin *, NPPluginWrapper *> plugin2wrapper_;
};

NPContainer::NPContainer() : impl_(new Impl()) {
}

NPContainer::~NPContainer() {
  delete impl_;
}

NPPlugin *
NPContainer::CreatePlugin(const char *mime_type, BasicElement *element,
                          bool xembed, ToolkitType toolkit,
                          int argc, char *argn[], char *argv[]) {
  if (!mime_type)
    return NULL;

  // If the plugin has already been loaded and initialized, just return
  // a new instance.
  if (impl_->PluginInitialized(mime_type))
    return impl_->NewPlugin(mime_type, element, xembed, toolkit,
                            argc, argn, argv);

  // Get paths of plugin library.
  std::vector<std::string> paths = impl_->GetPluginPaths(mime_type);
  if (paths.empty())
    return NULL;

  // Load plugin.
  void *handle = NULL;
  for (size_t i = 0; i < paths.size(); i++) {
    if ((handle = dlopen(paths[i].c_str(), RTLD_LAZY)) != NULL) {
      const char *error;
      dlerror();
      scoped_ptr<PluginSymbol> symbols(new PluginSymbol());
      symbols->Plugin_NP_GetMIMEDescription_ =
          (NP_GetMIMEDescriptionUPP)dlsym(handle, "NP_GetMIMEDescription");
      if ((error = dlerror()) != NULL)
        continue;
      symbols->Plugin_NP_GetValue_ =
          (NP_GetValueUPP)dlsym(handle, "NP_GetValue");
      if ((error = dlerror()) != NULL)
        continue;
      symbols->Plugin_NP_Initialize_ =
          (NP_InitializeUPP)dlsym(handle, "NP_Initialize");
      if ((error = dlerror()) != NULL)
        continue;
      symbols->Plugin_NP_Shutdown_ =
          (NP_ShutdownUPP)dlsym(handle, "NP_Shutdown");
      if ((error = dlerror()) != NULL)
        continue;

      NPPlugin *plugin = impl_->InitAndNewPlugin(handle, paths[i].c_str(),
                                                 symbols.release(),
                                                 mime_type, element,
                                                 xembed, toolkit,
                                                 argc, argn, argv);
      if (plugin) {
        LOGI("Plugin %s is loaded for MIME type %s",
             paths[i].c_str(), mime_type);
        return plugin;
      }
      dlclose(handle);
    }
  }

  LOGE("Failed to load plugin for MIME type %s", mime_type);
  return NULL;
}

bool NPContainer::DestroyPlugin(NPPlugin *plugin) {
  return impl_->DestroyPlugin(plugin);
}

NPPlugin *NPContainer::DoNewPlugin(const char *mime_type, BasicElement *element,
                                   const char *name, const char *description,
                                   void *instance, void *plugin_funcs,
                                   bool xembed, ToolkitType toolkit) {
  return new NPPlugin(mime_type, element, name, description,
                      instance, plugin_funcs, xembed, toolkit);
}

void NPContainer::DoDeletePlugin(NPPlugin *plugin) {
  delete plugin;
}

NPContainer *GetGlobalNPContainer() {
  static NPContainer *g_np_container = NULL;
  if (!g_np_container)
    g_np_container = new NPContainer();
  return g_np_container;
}

} // namespace npapi
} // namespace ggadget
