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

#ifndef GGADGET_SIDEBAR_H__
#define GGADGET_SIDEBAR_H__

#include <ggadget/common.h>
#include <ggadget/variant.h>
#include <ggadget/view_host_interface.h>

namespace ggadget {

class HostInterface;
class MenuInterface;
class ViewElement;
class ViewInterface;

#ifndef DECLARED_VARIANT_PTR_TYPE_MENU_INTERFACE
#define DECLARED_VARIANT_PTR_TYPE_MENU_INTERFACE
DECLARE_VARIANT_PTR_TYPE(MenuInterface);
#endif

/*
 * Object that represent the side bar.
 * SideBar is a container of view element, each view element is combined with a
 * ViewHost.
 */
class SideBar {
 public:
  /*
   * Constructor.
   * @param view_host @c SideBar acts as @c View, @c view_host is the ViewHost
   *        instance that is associated with the @c SideBar instance.
   *        It's not owned by the object, and shall be destroyed after
   *        destroying the object.
   */
  SideBar(ViewHostInterface *view_host);

  /* Destructor. */
  virtual ~SideBar();

 public:
  /**
   * Creates a new ViewHost instance and of curse a new view element
   * hold in the side bar.
   *
   * @param index The index in sidebar of the new ViewHost instance.
   * @return a new Viewhost instance.
   */
  ViewHostInterface *NewViewHost(int index);

  /**
   * @return the ViewHost instance associated with the sidebar instance.
   */
  ViewHostInterface *GetSideBarViewHost() const;

  /**
   * Set the size of the sidebar.
   */
  void SetSize(double width, double height);

  /** Retrieves the width of the side bar in pixels. */
  double GetWidth() const;

  /** Retrieves the height of side bar in pixels. */
  double GetHeight() const;

  /** Retrieves the index of the element in the sidebar that is specified
   *  by the height.
   * @param height the Y-coordination in the sidebar system.
   */
  int GetIndexFromHeight(double height) const;

  /**
   * Insert a place holder in the side bar.
   *
   * @param index The index of position in the sidebar of the place hodler.
   * @param height The height of the position.
   */
  void InsertPlaceholder(int index, double height);

  /**
   * Clear place holder(s)
   */
  void ClearPlaceHolder();

  /**
   * Explicitly let side bar reorganize the layout.
   */
  void Layout();

  /**
   * Explicitly update all elements' index in the sidebar
   */
  void UpdateElememtsIndex();

  /**
   * @return Return the element that is moused over. Return @c NULL if no
   * view element is moused over.
   */
  ViewElement *GetMouseOverElement() const;

  /**
   * Find if any view element in the side bar is the container of the @c view.
   * @param view the queried view.
   * @return @c NULL if no such view element is found.
   */
  ViewElement *FindViewElementByView(ViewInterface *view) const;

  /**
   * Find view element by index.
   */
  ViewElement *GetViewElementByIndex(int index) const;

  /**
   * Set pop out view.
   * When an element is poped out, the @c host associated with
   * the sidebar should let sidebar know it.
   * @param view the view that was poped out
   * @return the view element that contains the @c view.
   */
  ViewElement *SetPopOutedView(ViewInterface *view);

  /**
   * Event connection methods.
   */
  Connection *ConnectOnUndock(Slot2<void, double, double> *slot);
  Connection *ConnectOnPopIn(Slot0<void> *slot);
  Connection *ConnectOnAddGadget(Slot0<void> *slot);
  Connection *ConnectOnMenuOpen(Slot1<bool, MenuInterface *> *slot);
  Connection *ConnectOnClose(Slot0<void> *slot);
  Connection *ConnectOnSizeEvent(Slot0<void> *slot);

 private:
  class Impl;
  Impl *impl_;
  DISALLOW_EVIL_CONSTRUCTORS(SideBar);
};

}  // namespace ggadget

#endif  // GGADGET_SIDEBAR_H__
