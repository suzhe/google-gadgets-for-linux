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

#include "unittest/gunit.h"
#include "ggadget/basic_element.h"
#include "ggadget/element_factory.h"
#include "ggadget/elements.h"
#include "ggadget/event.h"
#include "ggadget/slot.h"
#include "ggadget/view.h"
#include "ggadget/xml_utils.h"

ggadget::ElementFactory *gFactory = NULL;

class Muffin : public ggadget::BasicElement {
 public:
  Muffin(ggadget::ElementInterface *parent,
         ggadget::ViewInterface *view,
         const char *name)
      : ggadget::BasicElement(parent, view, name, true) {
    RegisterProperty("tagName", NewSlot(this, &Muffin::GetTagName), NULL);
  }

  virtual ~Muffin() {
  }

 public:
  virtual const char *GetTagName() const {
    return "muffin";
  }

 public:
  virtual void HostChanged() {}
  
 public:
  DEFINE_CLASS_ID(0x6c0dee0e5bbe11dc, ggadget::BasicElement)
  
 public:
  static ggadget::ElementInterface *CreateInstance(
      ggadget::ElementInterface *parent,
      ggadget::ViewInterface *view,
      const char *name) {
    return new Muffin(parent, view, name);
  }
};

class Pie : public ggadget::BasicElement {
 public:
  Pie(ggadget::ElementInterface *parent,
      ggadget::ViewInterface *view,
      const char *name)
      : ggadget::BasicElement(parent, view, name, true) {
    RegisterProperty("tagName", NewSlot(this, &Pie::GetTagName), NULL);
  }

  virtual ~Pie() {
  }

 public:
  virtual const char *GetTagName() const {
    return "pie";
  }
  
 public:
  virtual void HostChanged() {}

 public:
  DEFINE_CLASS_ID(0x829defac5bbe11dc, ggadget::BasicElement)

 public:
  static ggadget::ElementInterface *CreateInstance(
      ggadget::ElementInterface *parent,
      ggadget::ViewInterface *view,
      const char *name) {
    return new Pie(parent, view, name);
  }
};

class EventHandler {
 public:
  EventHandler(ggadget::ViewInterface *view)
      : fired1_(false), fired2_(false), view_(view) {
    signal1_.Connect(ggadget::NewSlot(this, &EventHandler::Handle1));
    signal2_.Connect(ggadget::NewSlot(this, &EventHandler::Handle2));
  }
  void Handle1() {
    ASSERT_FALSE(fired2_);
    fired1_ = true;
    ggadget::Event *current_event = view_->GetEvent();
    ASSERT_EQ(ggadget::Event::EVENT_KEY_DOWN, current_event->GetType());
    ggadget::MouseEvent event(ggadget::Event::EVENT_MOUSE_CLICK, 123, 456);
    view_->FireEvent(&event, signal2_);
    // The current event should be the same as before. 
    ASSERT_EQ(current_event, view_->GetEvent());
    ASSERT_EQ(ggadget::Event::EVENT_KEY_DOWN, current_event->GetType());
  }
  void Handle2() {
    ASSERT_TRUE(fired1_);
    fired2_ = true;
    ggadget::Event *current_event = view_->GetEvent();
    ASSERT_EQ(ggadget::Event::EVENT_MOUSE_CLICK, current_event->GetType());
    ggadget::MouseEvent *mouse_event =
        static_cast<ggadget::MouseEvent *>(current_event);
    ASSERT_EQ(123, mouse_event->GetX());
    ASSERT_EQ(456, mouse_event->GetY());
  }

  ggadget::EventSignal signal1_, signal2_;
  bool fired1_, fired2_;
  ggadget::ViewInterface *view_;
};

TEST(ViewTest, FireEvent) {
  ggadget::View view(NULL, gFactory);
  EventHandler handler(&view);
  ggadget::KeyboardEvent event(ggadget::Event::EVENT_KEY_DOWN);
  view.FireEvent(&event, handler.signal1_);
  ASSERT_TRUE(handler.fired1_);
  ASSERT_TRUE(handler.fired2_);
}

// This test is not merely for View, but mixed test for xml_utils and Elements.
TEST(ViewTest, XMLConstruction) {
  ggadget::View view(NULL, gFactory);
  ASSERT_FALSE(view.GetShowCaptionAlways());
  ASSERT_EQ(ggadget::ViewInterface::RESIZABLE_TRUE, view.GetResizable());
  ASSERT_STREQ("", view.GetCaption());
  ASSERT_EQ(0, view.GetChildren()->GetCount());

  const char *xml =
    "<view width=\"123\" height=\"456\" caption=\"View-Caption\"\n"
    "    resizable=\"zoom\" showCaptionAlways=\"true\">\n"
    "  <pie tooltip=\"pie-tooltip\" x=\"50%\" y=\"100\">\n"
    "    <muffin tagName=\"haha\" name=\"muffin\"/>\n"
    "  </pie>\n"
    "  <pie name=\"pie1\"/>\n"
    "</view>\n";
  ASSERT_TRUE(ggadget::SetupViewFromXML(&view, xml, "filename"));
  ASSERT_STREQ("View-Caption", view.GetCaption());
  ASSERT_EQ(ggadget::ViewInterface::RESIZABLE_ZOOM, view.GetResizable());
  ASSERT_TRUE(view.GetShowCaptionAlways());
  ASSERT_EQ(123, view.GetWidth());
  ASSERT_EQ(456, view.GetHeight());
  ASSERT_EQ(2, view.GetChildren()->GetCount());

  ggadget::ElementInterface *m = view.GetElementByName("muffin");
  ASSERT_TRUE(m != NULL);
  ASSERT_EQ(m, view.GetChildren()->GetItemByIndex(0)->
               GetChildren()->GetItemByIndex(0));

  ggadget::ElementInterface *m1 = view.GetElementByName("pie1");
  ASSERT_TRUE(m1 != NULL);
  ASSERT_EQ(m1, view.GetChildren()->GetItemByIndex(1));

  view.GetChildren()->GetItemByIndex(0)->GetChildren()->RemoveElement(m);
  ASSERT_TRUE(view.GetElementByName("muffin") == NULL);

  ggadget::ElementInterface *m2 = view.GetChildren()->GetItemByIndex(0)->
      GetChildren()->AppendElementFromXML("<muffin name=\"new-muffin\"/>");
  ASSERT_TRUE(m2 != NULL);
  ASSERT_EQ(m2, view.GetChildren()->GetItemByIndex(0)->
                GetChildren()->GetItemByIndex(0));
}

int main(int argc, char *argv[]) {
  testing::ParseGUnitFlags(&argc, argv);
  gFactory = new ggadget::ElementFactory();
  gFactory->RegisterElementClass("muffin", Muffin::CreateInstance);
  gFactory->RegisterElementClass("pie", Pie::CreateInstance);
  int result = RUN_ALL_TESTS();
  delete gFactory;
  return result;
}
