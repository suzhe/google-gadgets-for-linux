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

#ifndef GGADGET_HOST_INTERFACE_H__
#define GGADGET_HOST_INTERFACE_H__

#include <vector>
#include <string>
#include <ggadget/view_host_interface.h>

namespace ggadget {

template <typename R, typename P1> class Slot1;
class Gadget;
class MainLoopInterface;
class OptionsInterface;
class ScriptableInterface;
class Signal;

/**
 * Interface for providing host services to the gadgets.
 * All gadgets may share one HostInterface instance.
 * The @c HostInterface implementation should depend on the host.
 */
class HostInterface {
 public:
  virtual ~HostInterface() { }

  /**
   * Creates a new ViewHost instance.
   * The newly created ViewHost instance must be deleted by the caller
   * afterwards.
   *
   * @param gadget The Gadget instance which will own this ViewHost instance.
   * @param type Type of the new ViewHost instance.
   *
   * @return a new Viewhost instance.
   */
  virtual ViewHostInterface *NewViewHost(Gadget *gadget,
                                         ViewHostInterface::Type type) = 0;

  /**
   * Requests that the gadget be removed from the container (e.g. sidebar).
   * The specified gadget shall be removed in the next main loop cycle,
   * otherwise the behavior is undefined.
   *
   * @param instance_id the id of the gadget instance to be removed.
   * @param save_data if @c true, the gadget's state is saved before the gadget
   *     is removed.
   */
  virtual void RemoveGadget(Gadget *gadget, bool save_data) = 0;

  /** Temporarily install a given font on the system. */
  virtual bool LoadFont(const char *filename) = 0;

  /** Runs the host, start the main loop, etc. */
  virtual void Run() = 0;

  /**
   * Shows an about dialog for a specified gadget.
   */
  virtual void ShowGadgetAboutDialog(Gadget *gadget) = 0;

  /**
   * Shows a debug console that will display all logs for the gadget.
   */
  virtual void ShowGadgetDebugConsole(Gadget *gadget) = 0;
};

} // namespace ggadget

#endif // GGADGET_HOST_INTERFACE_H__
