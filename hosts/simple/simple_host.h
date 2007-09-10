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

#ifndef HOSTS_SIMPLE_SIMPLE_HOST_H__
#define HOSTS_SIMPLE_SIMPLE_HOST_H__

#include "ggadget/common.h"
#include "ggadget/host_interface.h"

using ggadget::HostInterface;
using ggadget::GraphicsInterface;
using ggadget::ViewInterface;

class GadgetViewWidget;

/**
 * An implementation of HostInterface for the simple gadget host. 
 * In this implementation, there is one instance of SimpleHost per view, 
 * and one instance of GraphicsInterface per SimpleHost.
 */
class SimpleHost : public HostInterface {
 public:
  SimpleHost(GadgetViewWidget *gvw);

  virtual const GraphicsInterface *GetGraphics() const { return gfx_; };
 
  virtual void QueueDraw();
   
  virtual bool GrabKeyboardFocus();

  virtual bool DetachFromView();

  virtual ~SimpleHost();
   
 private:
  GadgetViewWidget *gvw_;
  GraphicsInterface *gfx_;
   
  DISALLOW_EVIL_CONSTRUCTORS(SimpleHost);
};

#endif // HOSTS_SIMPLE_SIMPLE_HOST_H__
