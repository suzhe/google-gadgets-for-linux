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

#ifndef GGADGET_OPTIONS_FACTORY_H__
#define GGADGET_OPTIONS_FACTORY_H__

#include <ggadget/options_interface.h>

namespace ggadget {

/**
 * Factory class for creating Options instances.
 */
class OptionsFactory {
 public:
  /**
   * Creates an instance of OptionsInterface by using a loaded
   * Options extension.
   *
   * A Options extension must be loaded into the global Extension
   * Manager in advance. If there is no Options extension loaded, NULL
   * will be returned.
   *
   * @param config_file_path the path name of the config file.
   * TODO: This parameter may be a distinct name of the config file.
   */
  OptionsInterface *CreateOptions(const char *config_file_path);

 public:
  /** Get the singleton of OptionsFactory. */
  static OptionsFactory *get();

 private:
  class Impl;
  Impl *impl_;

  /**
   * Private constructor to prevent creating OptionsFactory object
   * directly.
   */
  OptionsFactory();

  /**
   * Private destructor to prevent deleting OptionsFactory object
   * directly.
   */
  ~OptionsFactory();

  DISALLOW_EVIL_CONSTRUCTORS(OptionsFactory);
};

} // namespace ggadget

#endif // GGADGET_OPTIONS_FACTORY_H__
