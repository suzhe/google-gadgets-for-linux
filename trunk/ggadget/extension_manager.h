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

#ifndef GGADGET_EXTENSION_MANAGER_H__
#define GGADGET_EXTENSION_MANAGER_H__

#include <string>
#include <ggadget/slot.h>

namespace ggadget {

class ElementFactory;
class ScriptContextInterface;
class MainLoopInterface;

/**
 * class ExtensionManager is used to manage extension modules.
 *
 * An extension module can be used to provide additional Elements and Script
 * objects to gadgets.
 *
 * In addition to Initialize() and Finalize(), an extension module shall
 * provide following function:
 *
 * - bool RegisterExtension(ElementFactory *factory,
 *                          ScriptContextInterface *context);
 *   This function may be called multiple times for different gadgets during
 *   the life time of the extension module.
 *   Extension module shall register its Element classes and Script objects to
 *   the specified ElementFactory instance and ScriptContext instance.
 *   The function shall return true upon success.
 *   A symbol prefix modulename_LTX_ shall be used to avoid possible symbol
 *   conflicts. So the real function name shall be:
 *   modulename_LTX_RegisterExtension
 *   modulename is the name of the extension module, with all characters
 *   other than [0-9a-zA-Z_] replaced to '_' (underscore).
 */
class ExtensionManager {
 public:

  /**
   * Destroy a ExtensionManager object.
   *
   * @return true upon success. It's only possible to return false when trying
   * to destroy the global ExtensionManager instance, which can't be destroyed.
   */
  bool Destroy();

  /**
   * Loads a specified extension module.
   * @param name Name of the extension module. Can be a full path to the module
   * file.
   * @param resident Indicates if the extension should be resident in memory.
   *        A resident extension module can't be unloaded.
   * @return true if the extension was loaded and initialized successfully.
   */
  bool LoadExtension(const char *name, bool resident);

  /**
   * Unloads a loaded extension.
   * A resident extension can't be unloaded.
   *
   * @param name The exactly same name used to load the module.
   *
   * @return true if the extension was unloaded successfully.
   */
  bool UnloadExtension(const char *name);

  /**
   * Enumerate all loaded extensions.
   *
   * The caller shall not unload any extension during enumeration.
   *
   * @param callback A slot which will be called for each loaded extension,
   * the first parameter is the name used to load the extension. the second
   * parameter is the normalized name of the extension, without any file path
   * information. If the callback returns false, then the enumeration process
   * will be interrupted.
   *
   * @return true if all extensions have been enumerated. If there is no
   * loaded extension or the callback returns false, then returns false.
   */
  bool EnumerateLoadedExtensions(
      Slot2<bool, const char *, const char *> *callback) const;

  /**
   * Registers Element classes and Script objects provided by a specified
   * extension.
   *
   * If the specified extension has not been loaded, then try to load it first.
   *
   * @param name The name of the extension to be registered. It must be the
   * same name used to load the extension.
   * @param factory A ElementFactory instance to which the Element classes
   * provided by the extension shall be registered.
   * @param context A ScriptContext instance to which the Script objects
   * provided by the extension shall be registered.
   *
   * @return true if the extension has been loaded and registered successfully.
   */
  bool RegisterExtension(const char *name, ElementFactory *factory,
                         ScriptContextInterface *context) const;

  /**
   * Registers Element classes and Script objects provided by all loaded
   * extensions.
   *
   * @param factory A ElementFactory instance to which the Element classes
   * provided by the extensions shall be registered.
   * @param context A ScriptContext instance to which the Script objects
   * provided by the extensions shall be registered.
   *
   * @return true if all loaded extensions have been registered successfully.
   */
  bool RegisterLoadedExtensions(ElementFactory *factory,
                                ScriptContextInterface *context) const;

 public:
  /**
   * Set an ExtensionManager instance as the global singleton for all gadgets.
   *
   * The global ExtensionManager instance can only be set once, and after
   * setting a manager as the global singleton, no extensions can be added or
   * removed from the manager anymore.
   *
   * @return true upon success. If the global ExtensionManager is already set,
   * returns false.
   */
  static bool SetGlobalExtensionManager(ExtensionManager *manager);

  /**
   * Returns the global ExtensionManager singleton previously set by calling
   * SetGlobalExtensionManager().
   * The global ExtensionManager instance is is shared by all gadgets.
   * The caller shall not destroy it.
   */
  static const ExtensionManager *GetGlobalExtensionManager();

  static ExtensionManager *CreateExtensionManager(MainLoopInterface *main_loop);

 private:
  class Impl;
  Impl *impl_;

  /**
   * Private constructor to prevent creating ExtensionManager object directly.
   */
  ExtensionManager();

  /**
   * Private destructor to prevent deleting ExtensionManager object directly.
   */
  ~ExtensionManager();

  DISALLOW_EVIL_CONSTRUCTORS(ExtensionManager);
};

} // namespace ggadget

#endif // GGADGET_EXTENSION_MANAGER_H__
