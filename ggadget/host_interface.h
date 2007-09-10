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

#ifndef GGADGET_HOST_INTERFACE_H__
#define GGADGET_HOST_INTERFACE_H__

namespace ggadget {

class GraphicsInterface;
class ViewInterface;

/**
 * Interface for providing host services to the gadget library. Each view 
 * maintains it's own pointer to a HostInterface object. The HostInterface 
 * implementation should depend on the host. Each HostInterface should be 
 * dedicated to the view that is attached to it in order to handle things like
 * redraw, in which it needs to know which view is requesting the draw. 
 * The services provided by HostInterface are unidirectional. That is, the 
 * View calls methods in the HostInterface, and not the other way around.
 * All methods, except the destructor of this class should be called by a 
 * view attaching it.
 */
class HostInterface {   
 public:
   
  /** Returns the GraphicsInterface associated with this host. */
  virtual const GraphicsInterface *GetGraphics() const = 0;
   
  /** Asks the host to redraw the given view. */
  virtual void QueueDraw() = 0;
   
  /** Asks the host to deliver keyboard events to the view. */
  virtual bool GrabKeyboardFocus() = 0;
   
  /**
   * Detach the current HostInterface object from the view. When a HostInterface
   * is removed from a view (such as when a new HostInterface is attached), the 
   * view receives this notification from ViewInterface::AttachHost(). The view
   * then announces to the HostInterface this detachment. This solves the 
   * problem of the old (detached) HostInterface not knowing that it is being
   * detached, since this action may be initiated by a different host that does
   * not know about the old host. No more HostInterface methods should be called 
   * by the view after its detached its HostInterface.
   * @return true on success, false on failure.
   */ 
  virtual bool DetachFromView() = 0;   

  virtual ~HostInterface() {}   
};

} // namespace ggadget

#endif // GGADGET_HOST_INTERFACE_H__
