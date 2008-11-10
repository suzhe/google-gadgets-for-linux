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

#include <gtk/gtk.h>
#include <string>
#include <map>

#include "simple_gtk_host.h"

#include <ggadget/common.h>
#include <ggadget/decorated_view_host.h>
#include <ggadget/floating_main_view_decorator.h>
#include <ggadget/docked_main_view_decorator.h>
#include <ggadget/popout_main_view_decorator.h>
#include <ggadget/details_view_decorator.h>
#include <ggadget/gadget.h>
#include <ggadget/gadget_consts.h>
#include <ggadget/gadget_manager_interface.h>
#include <ggadget/gtk/menu_builder.h>
#include <ggadget/gtk/single_view_host.h>
#include <ggadget/gtk/utilities.h>
#include <ggadget/gtk/hotkey.h>
#include <ggadget/locales.h>
#include <ggadget/messages.h>
#include <ggadget/logger.h>
#include <ggadget/script_runtime_manager.h>
#include <ggadget/view.h>
#include <ggadget/main_loop_interface.h>
#include <ggadget/file_manager_factory.h>
#include <ggadget/options_interface.h>
#include <ggadget/string_utils.h>
#include <ggadget/permissions.h>

#include "gadget_browser_host.h"

using namespace ggadget;
using namespace ggadget::gtk;

namespace ggadget {
DECLARE_VARIANT_PTR_TYPE(DecoratedViewHost);
}

namespace hosts {
namespace gtk {

static const char kOptionHotKey[] = "hotkey";
static const char kOptionGadgetsShown[] = "gadgets_shown";
static const char kOptionFontSize[] = "font_size";

static const int kMinFontSize = 4;
static const int kMaxFontSize = 16;

class SimpleGtkHost::Impl {
  struct GadgetInfo {
    GadgetInfo()
      : gadget(NULL), main(NULL), popout(NULL), details(NULL),
        popout_on_right(false), details_on_right(false),
        debug_console(NULL) {
    }

    Gadget *gadget;

    SingleViewHost *main;
    SingleViewHost *popout;
    SingleViewHost *details;
    DecoratedViewHost *main_decorator;

    bool popout_on_right;
    bool details_on_right;
    GtkWidget *debug_console;
  };

 public:
  Impl(SimpleGtkHost *owner, OptionsInterface *options,
       double zoom, bool decorated, int view_debug_mode,
       Gadget::DebugConsoleConfig debug_console_config)
    : gadget_browser_host_(owner, view_debug_mode),
      owner_(owner),
      options_(options),
      zoom_(zoom),
      decorated_(decorated),
      view_debug_mode_(view_debug_mode),
      debug_console_config_(debug_console_config),
      gadgets_shown_(true),
      transparent_(SupportsComposite(NULL)),
      font_size_(kDefaultFontSize),
      gadget_manager_(GetGadgetManager()),
      expanded_original_(NULL),
      expanded_popout_(NULL),
      hotkey_grabber_(NULL) {
    ASSERT(gadget_manager_);
    ASSERT(options_);

    hotkey_grabber_.ConnectOnHotKeyPressed(
        NewSlot(this, &Impl::ToggleAllGadgets));

    if (options_) {
      std::string hotkey;
      if (options_->GetInternalValue(kOptionHotKey).ConvertToString(&hotkey) &&
          hotkey.length()) {
        hotkey_grabber_.SetHotKey(hotkey);
        hotkey_grabber_.SetEnableGrabbing(true);
      }
      options_->GetInternalValue(
          kOptionGadgetsShown).ConvertToBool(&gadgets_shown_);
      options_->GetInternalValue(kOptionFontSize).ConvertToInt(&font_size_);
      font_size_ = std::min(kMaxFontSize, std::max(kMinFontSize, font_size_));
    }

    // Connect gadget related signals.
    gadget_manager_->ConnectOnNewGadgetInstance(
        NewSlot(this, &Impl::NewGadgetInstanceCallback));
    gadget_manager_->ConnectOnRemoveGadgetInstance(
        NewSlot(this, &Impl::RemoveGadgetInstanceCallback));

    // Initializes global permissions.
    // FIXME: Supports customizable global permissions.
    global_permissions_.SetGranted(Permissions::ALL_ACCESS, true);
  }

  ~Impl() {
    for (GadgetInfoMap::iterator it = gadgets_.begin();
         it != gadgets_.end(); ++it) {
      if (it->second.debug_console)
        gtk_widget_destroy(it->second.debug_console);
      delete it->second.gadget;
    }

    gtk_widget_destroy(host_menu_);
#if GTK_CHECK_VERSION(2,10,0) && defined(GGL_HOST_LINUX)
    g_object_unref(G_OBJECT(status_icon_));
#else
    gtk_widget_destroy(main_widget_);
#endif
  }

  void SetupUI() {
    const int priority = MenuInterface::MENU_ITEM_PRI_HOST;
    host_menu_ = gtk_menu_new();
    MenuBuilder menu_builder(GTK_MENU_SHELL(host_menu_));

    menu_builder.AddItem(GM_("MENU_ITEM_ADD_GADGETS"), 0,
                         MenuInterface::MENU_ITEM_ICON_ADD,
                         NewSlot(this, &Impl::AddGadgetMenuCallback),
                         priority);
    menu_builder.AddItem(GM_("MENU_ITEM_SHOW_ALL"), 0, 0,
                         NewSlot(this, &Impl::ShowAllMenuCallback),
                         priority);
    menu_builder.AddItem(GM_("MENU_ITEM_HIDE_ALL"), 0, 0,
                         NewSlot(this, &Impl::HideAllMenuCallback),
                         priority);
    menu_builder.AddItem(GM_("MENU_ITEM_CHANGE_HOTKEY"), 0, 0,
                         NewSlot(this, &Impl::ChangeHotKeyMenuCallback),
                         priority);

    MenuInterface *sub = menu_builder.AddPopup(GM_("MENU_ITEM_FONT_SIZE"),
                                               priority);
    sub->AddItem(GM_("MENU_ITEM_FONT_SIZE_LARGER"),
                 font_size_ >= kMaxFontSize ?
                 MenuInterface::MENU_ITEM_FLAG_GRAYED : 0,
                 MenuInterface::MENU_ITEM_ICON_ZOOM_IN,
                 NewSlot(this, &Impl::FontSizeMenuHandler, 1), priority);
    sub->AddItem(GM_("MENU_ITEM_FONT_SIZE_DEFAULT"), 0,
                 MenuInterface::MENU_ITEM_ICON_ZOOM_100,
                 NewSlot(this, &Impl::FontSizeMenuHandler, 0), priority);
    sub->AddItem(GM_("MENU_ITEM_FONT_SIZE_SMALLER"),
                 font_size_ <= kMinFontSize ?
                 MenuInterface::MENU_ITEM_FLAG_GRAYED : 0,
                 MenuInterface::MENU_ITEM_ICON_ZOOM_OUT,
                 NewSlot(this, &Impl::FontSizeMenuHandler, -1), priority);

    // Separator
    menu_builder.AddItem(NULL, 0, 0, NULL, MenuInterface::MENU_ITEM_PRI_HOST);

    menu_builder.AddItem(GM_("MENU_ITEM_EXIT"), 0,
                         MenuInterface::MENU_ITEM_ICON_QUIT,
                         NewSlot(this, &Impl::ExitMenuCallback),
                         MenuInterface::MENU_ITEM_PRI_HOST);

#if GTK_CHECK_VERSION(2,10,0) && defined(GGL_HOST_LINUX)
    // FIXME:
    std::string icon_data;
    if (GetGlobalFileManager()->ReadFile(kGadgetsIcon, &icon_data)) {
      GdkPixbuf *icon_pixbuf = LoadPixbufFromData(icon_data);
      status_icon_ = gtk_status_icon_new_from_pixbuf(icon_pixbuf);
      g_object_unref(icon_pixbuf);
    } else {
      DLOG("Failed to load Gadgets icon.");
      status_icon_ = gtk_status_icon_new_from_stock(GTK_STOCK_ABOUT);
    }

    g_signal_connect(G_OBJECT(status_icon_), "activate",
                     G_CALLBACK(ToggleAllGadgetsHandler), this);
    g_signal_connect(G_OBJECT(status_icon_), "popup-menu",
                     G_CALLBACK(StatusIconPopupMenuHandler), this);
    UpdateStatusIconTooltip();
#else
    GtkWidget *menu_bar = gtk_menu_bar_new();
    gtk_widget_show(menu_bar);
    GtkWidget *item = gtk_menu_item_new_with_label(GM_("GOOGLE_GADGETS"));
    gtk_widget_show(item);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), host_menu_);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), item);
    main_widget_ = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(main_widget_), GM_("GOOGLE_GADGETS"));
    gtk_window_set_resizable(GTK_WINDOW(main_widget_), FALSE);
    gtk_container_add(GTK_CONTAINER(main_widget_), menu_bar);
    gtk_widget_show(main_widget_);
    g_signal_connect(G_OBJECT(main_widget_), "delete_event",
                     G_CALLBACK(DeleteEventHandler), NULL);
#endif
  }

#if GTK_CHECK_VERSION(2,10,0) && defined(GGL_HOST_LINUX)
  void UpdateStatusIconTooltip() {
    if (hotkey_grabber_.IsGrabbing()) {
      gtk_status_icon_set_tooltip(status_icon_,
          StringPrintf(GM_("STATUS_ICON_TOOLTIP_WITH_HOTKEY"),
                       hotkey_grabber_.GetHotKey().c_str()).c_str());
    } else {
      gtk_status_icon_set_tooltip(status_icon_, GM_("STATUS_ICON_TOOLTIP"));
    }
  }
#endif

  static bool GetPermissionsDescriptionCallback(int permission,
                                                std::string *msg) {
    if (msg->length())
      msg->append("\n");
    msg->append("  ");
    msg->append(Permissions::GetDescription(permission));
    return true;
  }

  bool ConfirmGadget(int id, Permissions *permissions) {
    std::string path = gadget_manager_->GetGadgetInstancePath(id);
    std::string download_url, title, description;
    if (!gadget_manager_->GetGadgetInstanceInfo(id,
                                                GetSystemLocaleName().c_str(),
                                                NULL, &download_url,
                                                &title, &description))
      return false;

    // Get required permissions description.
    std::string permissions_msg;
    permissions->EnumerateAllRequired(
        NewSlot(GetPermissionsDescriptionCallback, &permissions_msg));

    GtkWidget *dialog = gtk_message_dialog_new(
        NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
        "%s\n\n%s\n%s\n\n%s%s\n\n%s\n%s",
        GM_("GADGET_CONFIRM_MESSAGE"), title.c_str(), download_url.c_str(),
        GM_("GADGET_DESCRIPTION"), description.c_str(),
        GM_("GADGET_REQUIRED_PERMISSIONS"), permissions_msg.c_str());

    GdkScreen *screen;
    gdk_display_get_pointer(gdk_display_get_default(), &screen,
                            NULL, NULL, NULL);
    gtk_window_set_screen(GTK_WINDOW(dialog), screen);
    gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);
    gtk_window_set_title(GTK_WINDOW(dialog), GM_("GADGET_CONFIRM_TITLE"));
    gtk_window_present(GTK_WINDOW(dialog));
    gint result = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);

    if (result == GTK_RESPONSE_YES) {
      // TODO: Is it necessary to let user grant individual permissions
      // separately?
      permissions->GrantAllRequired();
      return true;
    }
    return false;
  }

  bool EnumerateGadgetInstancesCallback(int id) {
    if (!LoadGadgetInstance(id))
      gadget_manager_->RemoveGadgetInstance(id);
    // Return true to continue the enumeration.
    return true;
  }

  bool NewGadgetInstanceCallback(int id) {
    Permissions permissions;
    if (gadget_manager_->GetGadgetDefaultPermissions(id, &permissions)) {
      if (!permissions.HasUngranted() || ConfirmGadget(id, &permissions)) {
        // Save initial permissions.
        std::string options_name =
            gadget_manager_->GetGadgetInstanceOptionsName(id);
        OptionsInterface *options = CreateOptions(options_name.c_str());
        // Don't save required permissions.
        permissions.RemoveAllRequired();
        options->PutInternalValue(kPermissionsOption,
                                  Variant(permissions.ToString()));
        options->Flush();
        delete options;
        return LoadGadgetInstance(id);
      }
    } else {
      ShowAlertDialog(
          GM_("GOOGLE_GADGETS"),
          StringPrintf(
              GM_("GADGET_LOAD_FAILURE"),
              gadget_manager_->GetGadgetInstancePath(id).c_str()).c_str());
    }
    return false;
  }

  bool LoadGadgetInstance(int id) {
    bool result = false;
    std::string options = gadget_manager_->GetGadgetInstanceOptionsName(id);
    std::string path = gadget_manager_->GetGadgetInstancePath(id);
    if (options.length() && path.length()) {
      result =
          LoadGadget(path.c_str(), options.c_str(), id);
      DLOG("SimpleGtkHost: Load gadget %s, with option %s, %s",
           path.c_str(), options.c_str(), result ? "succeeded" : "failed");
    }
    return result;
  }

  bool LoadGadget(const char *path, const char *options_name, int instance_id) {
    if (gadgets_.find(instance_id) != gadgets_.end()) {
      // Gadget is already loaded.
      return true;
    }

    Gadget *gadget = new Gadget(owner_, path, options_name, instance_id,
                                global_permissions_, debug_console_config_);
    GadgetInfoMap::iterator it = gadgets_.find(instance_id);

    if (!gadget->IsValid()) {
      LOG("Failed to load gadget %s", path);
      if (it != gadgets_.end()) {
        if (it->second.debug_console)
          gtk_widget_destroy(it->second.debug_console);
        gadgets_.erase(it);
      }
      delete gadget;
      return false;
    }

    gadget->SetDisplayTarget(Gadget::TARGET_FLOATING_VIEW);
    gadget->GetMainView()->OnOtherEvent(SimpleEvent(Event::EVENT_UNDOCK));

    if (gadgets_shown_)
      gadget->ShowMainView();

    // If debug console is opened during view host creation, the title is
    // not set then because main view is not available. Set the title now.
    if (it->second.debug_console) {
      gtk_window_set_title(GTK_WINDOW(it->second.debug_console),
                           gadget->GetMainView()->GetCaption().c_str());
    }
    return true;
  }

  ViewHostInterface *NewViewHost(Gadget *gadget,
                                 ViewHostInterface::Type type) {
    ASSERT(gadget);
    int gadget_id = gadget->GetInstanceID();
    GadgetInfo *info = &gadgets_[gadget_id];
    ASSERT(info->gadget == NULL || info->gadget == gadget);
    info->gadget = gadget;

    bool decorated =
        (decorated_ || type == ViewHostInterface::VIEW_HOST_OPTIONS);

    SingleViewHost *svh = new SingleViewHost(type, zoom_, decorated, false,
                                             true, view_debug_mode_);

    if (type == ViewHostInterface::VIEW_HOST_OPTIONS)
      return svh;

    DecoratedViewHost *dvh;
    if (type == ViewHostInterface::VIEW_HOST_MAIN) {
      FloatingMainViewDecorator *view_decorator =
          new FloatingMainViewDecorator(svh, transparent_);
      dvh = new DecoratedViewHost(view_decorator);
      ASSERT(!info->main);
      info->main = svh;
      info->main_decorator = dvh;

      svh->ConnectOnShowHide(
          NewSlot(this, &Impl::OnMainViewShowHideHandler, gadget_id));
      svh->ConnectOnResized(
          NewSlot(this, &Impl::OnMainViewResizedHandler, gadget_id));
      svh->ConnectOnMoved(
          NewSlot(this, &Impl::OnMainViewMovedHandler, gadget_id));
      view_decorator->ConnectOnClose(
          NewSlot(this, &Impl::OnCloseHandler, dvh));
      view_decorator->ConnectOnPopOut(
          NewSlot(this, &Impl::OnPopOutHandler, dvh));
      view_decorator->ConnectOnPopIn(
          NewSlot(this, &Impl::OnPopInHandler, dvh));
    } else {
      DetailsViewDecorator *view_decorator = new DetailsViewDecorator(svh);
      dvh = new DecoratedViewHost(view_decorator);
      ASSERT(info->main);
      ASSERT(!info->details);
      info->details = svh;

      svh->ConnectOnShowHide(
          NewSlot(this, &Impl::OnDetailsViewShowHideHandler, gadget_id));
      svh->ConnectOnBeginResizeDrag(
          NewSlot(this, &Impl::OnDetailsViewBeginResizeHandler, gadget_id));
      svh->ConnectOnResized(
          NewSlot(this, &Impl::OnDetailsViewResizedHandler, gadget_id));
      svh->ConnectOnBeginMoveDrag(
          NewSlot(this, &Impl::OnDetailsViewBeginMoveHandler));
      view_decorator->ConnectOnClose(NewSlot(this, &Impl::OnCloseHandler, dvh));
    }

    return dvh;
  }

  void RemoveGadget(Gadget *gadget, bool save_data) {
    ASSERT(gadget);
    ViewInterface *main_view = gadget->GetMainView();

    // If this gadget is popped out, popin it first.
    if (main_view->GetViewHost() == expanded_popout_) {
      OnPopInHandler(expanded_original_);
    }

    int id = gadget->GetInstanceID();
    // If RemoveGadgetInstance() returns false, then means this instance is not
    // installed by gadget manager.
    if (!gadget_manager_->RemoveGadgetInstance(id))
      RemoveGadgetInstanceCallback(id);
  }

  void RemoveGadgetInstanceCallback(int instance_id) {
    GadgetInfoMap::iterator it = gadgets_.find(instance_id);
    if (it != gadgets_.end()) {
      if (it->second.debug_console)
        gtk_widget_destroy(it->second.debug_console);
      delete it->second.gadget;
      gadgets_.erase(it);
    } else {
      LOG("Can't find gadget instance %d", instance_id);
    }
  }

  void LoadGadgets() {
    gadget_manager_->EnumerateGadgetInstances(
        NewSlot(this, &Impl::EnumerateGadgetInstancesCallback));
  }

  void ShowAllMenuCallback(const char *) {
    for (GadgetInfoMap::iterator it = gadgets_.begin();
         it != gadgets_.end(); ++it) {
      it->second.main->ShowView(false, 0, NULL);
    }
    gadgets_shown_ = true;
    if (options_)
      options_->PutInternalValue(kOptionGadgetsShown, Variant(gadgets_shown_));
  }

  void HideAllMenuCallback(const char *) {
    for (GadgetInfoMap::iterator it = gadgets_.begin();
         it != gadgets_.end(); ++it) {
      it->second.main->CloseView();
    }
    gadgets_shown_ = false;
    if (options_)
      options_->PutInternalValue(kOptionGadgetsShown, Variant(gadgets_shown_));
  }

  void ChangeHotKeyMenuCallback(const char *) {
    HotKeyDialog dialog;
    dialog.SetHotKey(hotkey_grabber_.GetHotKey());
    hotkey_grabber_.SetEnableGrabbing(false);
    if (dialog.Show()) {
      std::string hotkey = dialog.GetHotKey();
      hotkey_grabber_.SetHotKey(hotkey);
      // The hotkey will not be enabled if it's invalid.
      hotkey_grabber_.SetEnableGrabbing(true);
      if (options_)
        options_->PutInternalValue(kOptionHotKey, Variant(hotkey));
#if GTK_CHECK_VERSION(2,10,0) && defined(GGL_HOST_LINUX)
      UpdateStatusIconTooltip();
#endif
    }
  }

  void ToggleAllGadgets() {
    if (gadgets_shown_)
      HideAllMenuCallback(NULL);
    else
      ShowAllMenuCallback(NULL);
  }

  void OnThemeChanged() {
    SimpleEvent event(Event::EVENT_THEME_CHANGED);
    for (GadgetInfoMap::iterator it = gadgets_.begin();
         it != gadgets_.end(); ++it) {
      if (it->second.main)
        it->second.main->GetView()->OnOtherEvent(event);
      if (it->second.details)
        it->second.details->GetView()->OnOtherEvent(event);
      if (it->second.popout)
        it->second.popout->GetView()->OnOtherEvent(event);
    }
  }

  void FontSizeMenuHandler(const char *str, int delta) {
    int new_font_size;
    if (delta == 0) {
      new_font_size = kDefaultFontSize;
    } else {
      new_font_size = std::min(std::max(font_size_ + delta, kMinFontSize),
                               kMaxFontSize);
    }
    if (new_font_size != font_size_) {
      font_size_ = new_font_size;
      if (options_)
        options_->PutInternalValue(kOptionFontSize, Variant(font_size_));
      OnThemeChanged();
    }
  }

  void ExitMenuCallback(const char *) {
    // Close the poppped out view, to make sure that the main view decorator
    // can save its states correctly.
    if (expanded_popout_)
      OnPopInHandler(expanded_original_);

    DLOG("Exit...");
    gtk_main_quit();
  }

  void AddGadgetMenuCallback(const char *) {
    gadget_manager_->ShowGadgetBrowserDialog(&gadget_browser_host_);
  }

  void OnCloseHandler(DecoratedViewHost *decorated) {
    ViewInterface *child = decorated->GetView();
    Gadget *gadget = child ? child->GetGadget() : NULL;

    ASSERT(gadget);
    if (!gadget) return;

    GadgetInfo *info = &gadgets_[gadget->GetInstanceID()];

    switch (decorated->GetType()) {
      case ViewHostInterface::VIEW_HOST_MAIN:
        if (decorated == info->main_decorator) {
          gadget->RemoveMe(true);
        } else if (expanded_original_ && expanded_popout_ == decorated) {
          OnPopInHandler(expanded_original_);
        }
        break;
      case ViewHostInterface::VIEW_HOST_DETAILS:
        gadget->CloseDetailsView();
        break;
      default:
        ASSERT_M(false, ("Invalid decorator type."));
    }
  }

  void OnPopOutHandler(DecoratedViewHost *decorated) {
    if (expanded_original_) {
      OnPopInHandler(expanded_original_);
    }

    ViewInterface *child = decorated->GetView();
    ASSERT(child);
    if (child) {
      expanded_original_ = decorated;
      SingleViewHost *svh =
          new SingleViewHost(ViewHostInterface::VIEW_HOST_MAIN, zoom_,
                             false, false, false, view_debug_mode_);
      PopOutMainViewDecorator *view_decorator =
          new PopOutMainViewDecorator(svh);
      expanded_popout_ = new DecoratedViewHost(view_decorator);
      view_decorator->ConnectOnClose(NewSlot(this, &Impl::OnCloseHandler,
                                             expanded_popout_));

      int gadget_id = child->GetGadget()->GetInstanceID();

      GadgetInfo *info = &gadgets_[gadget_id];
      ASSERT(info->main);
      ASSERT(!info->popout);
      info->popout = svh;

      svh->ConnectOnShowHide(
          NewSlot(this, &Impl::OnPopOutViewShowHideHandler, gadget_id));
      svh->ConnectOnBeginResizeDrag(
          NewSlot(this, &Impl::OnPopOutViewBeginResizeHandler, gadget_id));
      svh->ConnectOnResized(
          NewSlot(this, &Impl::OnPopOutViewResizedHandler, gadget_id));
      svh->ConnectOnBeginMoveDrag(
          NewSlot(this, &Impl::OnPopOutViewBeginMoveHandler));

      // Send popout event to decorator first.
      SimpleEvent event(Event::EVENT_POPOUT);
      expanded_original_->GetViewDecorator()->OnOtherEvent(event);

      child->SwitchViewHost(expanded_popout_);
      expanded_popout_->ShowView(false, 0, NULL);
    }
  }

  void OnPopInHandler(DecoratedViewHost *decorated) {
    if (expanded_original_ == decorated && expanded_popout_) {
      ViewInterface *child = expanded_popout_->GetView();
      ASSERT(child);
      if (child) {
        expanded_popout_->CloseView();
        ViewHostInterface *old_host = child->SwitchViewHost(expanded_original_);
        SimpleEvent event(Event::EVENT_POPIN);
        expanded_original_->GetViewDecorator()->OnOtherEvent(event);
        // The old host must be destroyed after sending onpopin event.
        old_host->Destroy();
        expanded_original_ = NULL;
        expanded_popout_ = NULL;

        // Clear the popout info.
        int gadget_id = child->GetGadget()->GetInstanceID();
        gadgets_[gadget_id].popout = NULL;
      }
    }
  }

  void AdjustViewHostPosition(GadgetInfo *info) {
    ASSERT(info && info->main && info->main_decorator);
    int x, y;
    int width, height;
    info->main->GetWindowPosition(&x, &y);
    info->main->GetWindowSize(&width, &height);
    int screen_width = gdk_screen_get_width(
        gtk_widget_get_screen(info->main->GetWindow()));
    int screen_height = gdk_screen_get_height(
        gtk_widget_get_screen(info->main->GetWindow()));

    bool main_dock_right = (x > width);

    if (info->popout && info->popout->IsVisible()) {
      int popout_width, popout_height;
      info->popout->GetWindowSize(&popout_width, &popout_height);
      if (info->popout_on_right && popout_width < x &&
          x + width + popout_width > screen_width)
        info->popout_on_right = false;
      else if (!info->popout_on_right && popout_width > x &&
               x + width + popout_width < screen_width)
        info->popout_on_right = true;

      if (y + popout_height > screen_height)
        y = screen_height - popout_height;

      if (info->popout_on_right) {
        info->popout->SetWindowPosition(x + width, y);
        width += popout_width;
      } else {
        info->popout->SetWindowPosition(x - popout_width, y);
        x -= popout_width;
        width += popout_width;
      }

      main_dock_right = !info->popout_on_right;
    }

    if (info->details && info->details->IsVisible()) {
      int details_width, details_height;
      info->details->GetWindowSize(&details_width, &details_height);
      if (info->details_on_right && details_width < x &&
          x + width + details_width > screen_width)
        info->details_on_right = false;
      else if (!info->details_on_right && details_width > x &&
               x + width + details_width < screen_width)
        info->details_on_right = true;

      if (y + details_height > screen_height)
        y = screen_height - details_height;

      if (info->details_on_right) {
        info->details->SetWindowPosition(x + width, y);
      } else {
        info->details->SetWindowPosition(x - details_width, y);
      }
    }

    MainViewDecoratorBase *view_decorator = down_cast<MainViewDecoratorBase *>(
        info->main_decorator->GetViewDecorator());
    if (view_decorator) {
      view_decorator->SetPopOutDirection(
          main_dock_right ? MainViewDecoratorBase::POPOUT_TO_LEFT :
          MainViewDecoratorBase::POPOUT_TO_RIGHT);
    }
  }

  void OnMainViewShowHideHandler(bool show, int gadget_id) {
    GadgetInfoMap::iterator it = gadgets_.find(gadget_id);
    if (it != gadgets_.end()) {
      if (show) {
        if (it->second.popout && !it->second.popout->IsVisible())
          it->second.popout->ShowView(false, 0, NULL);
        AdjustViewHostPosition(&it->second);
      } else {
        if (it->second.popout) {
          it->second.popout->CloseView();
        }
        if (it->second.details) {
          // The details view won't be shown again.
          it->second.details->CloseView();
          it->second.details = NULL;
        }
      }
    }
  }

  void OnMainViewResizedHandler(int width, int height, int gadget_id) {
    GadgetInfoMap::iterator it = gadgets_.find(gadget_id);
    if (it != gadgets_.end()) {
      AdjustViewHostPosition(&it->second);
    }
  }

  void OnMainViewMovedHandler(int x, int y, int gadget_id) {
    GadgetInfoMap::iterator it = gadgets_.find(gadget_id);
    if (it != gadgets_.end()) {
      AdjustViewHostPosition(&it->second);
    }
  }

  void OnPopOutViewShowHideHandler(bool show, int gadget_id) {
    GadgetInfoMap::iterator it = gadgets_.find(gadget_id);
    if (it != gadgets_.end() && it->second.popout) {
      if (it->second.details) {
        // Close Details whenever the popout view shows or hides.
        it->second.details->CloseView();
        it->second.details = NULL;
      }
      if (show)
        AdjustViewHostPosition(&it->second);
    }
  }

  bool OnPopOutViewBeginResizeHandler(int button, int hittest, int gadget_id) {
    GadgetInfoMap::iterator it = gadgets_.find(gadget_id);
    if (it != gadgets_.end() && it->second.popout) {
      if (it->second.popout_on_right)
        return hittest == ViewInterface::HT_LEFT ||
               hittest == ViewInterface::HT_TOPLEFT ||
               hittest == ViewInterface::HT_BOTTOMLEFT ||
               hittest == ViewInterface::HT_TOP ||
               hittest == ViewInterface::HT_TOPRIGHT;
      else
        return hittest == ViewInterface::HT_RIGHT ||
               hittest == ViewInterface::HT_TOPRIGHT ||
               hittest == ViewInterface::HT_BOTTOMRIGHT ||
               hittest == ViewInterface::HT_TOP ||
               hittest == ViewInterface::HT_TOPLEFT;
    }
    return false;
  }

  void OnPopOutViewResizedHandler(int width, int height, int gadget_id) {
    GadgetInfoMap::iterator it = gadgets_.find(gadget_id);
    if (it != gadgets_.end() && it->second.popout) {
      AdjustViewHostPosition(&it->second);
    }
  }

  bool OnPopOutViewBeginMoveHandler(int button) {
    // User can't move popout view window.
    return true;
  }

  void OnDetailsViewShowHideHandler(bool show, int gadget_id) {
    GadgetInfoMap::iterator it = gadgets_.find(gadget_id);
    if (it != gadgets_.end() && it->second.details) {
      if (show) {
        AdjustViewHostPosition(&it->second);
      } else {
        // The same details view will never shown again.
        it->second.details = NULL;
      }
    }
  }

  bool OnDetailsViewBeginResizeHandler(int button, int hittest, int gadget_id) {
    GadgetInfoMap::iterator it = gadgets_.find(gadget_id);
    if (it != gadgets_.end() && it->second.details) {
      if (it->second.details_on_right)
        return hittest == ViewInterface::HT_LEFT ||
               hittest == ViewInterface::HT_TOPLEFT ||
               hittest == ViewInterface::HT_BOTTOMLEFT ||
               hittest == ViewInterface::HT_TOP ||
               hittest == ViewInterface::HT_TOPRIGHT;
      else
        return hittest == ViewInterface::HT_RIGHT ||
               hittest == ViewInterface::HT_TOPRIGHT ||
               hittest == ViewInterface::HT_BOTTOMRIGHT ||
               hittest == ViewInterface::HT_TOP ||
               hittest == ViewInterface::HT_TOPLEFT;
    }
    return false;
  }

  void OnDetailsViewResizedHandler(int width, int height, int gadget_id) {
    GadgetInfoMap::iterator it = gadgets_.find(gadget_id);
    if (it != gadgets_.end() && it->second.details) {
      AdjustViewHostPosition(&it->second);
    }
  }

  bool OnDetailsViewBeginMoveHandler(int button) {
    // User can't move popout view window.
    return true;
  }

#if GTK_CHECK_VERSION(2,10,0) && defined(GGL_HOST_LINUX)
  static void StatusIconPopupMenuHandler(GtkWidget *widget, guint button,
                                         guint activate_time,
                                         gpointer user_data) {
    Impl *impl = reinterpret_cast<Impl *>(user_data);
    gtk_menu_popup(GTK_MENU(impl->host_menu_), NULL, NULL,
                   gtk_status_icon_position_menu, impl->status_icon_,
                   button, activate_time);
  }
#endif

  static gboolean DeleteEventHandler(GtkWidget *widget,
                                     GdkEvent *event,
                                     gpointer data) {
    gtk_main_quit();
    return TRUE;
  }

  static void ToggleAllGadgetsHandler(GtkWidget *widget, gpointer user_data) {
    Impl *impl = reinterpret_cast<Impl *>(user_data);
    impl->ToggleAllGadgets();
  }

  void ShowGadgetDebugConsole(Gadget *gadget) {
    if (!gadget)
      return;
    GadgetInfoMap::iterator it = gadgets_.find(gadget->GetInstanceID());
    if (it == gadgets_.end())
      return;
    GadgetInfo *info = &it->second;
    if (info->debug_console) {
      DLOG("Gadget has already debug console opened: %p", info->debug_console);
      return;
    }
    info->debug_console = NewGadgetDebugConsole(gadget);
    g_signal_connect(info->debug_console, "destroy",
                     G_CALLBACK(gtk_widget_destroyed), &info->debug_console);
  }

  typedef std::map<int, GadgetInfo> GadgetInfoMap;
  GadgetInfoMap gadgets_;

  GadgetBrowserHost gadget_browser_host_;
  SimpleGtkHost *owner_;
  OptionsInterface *options_;

  double zoom_;
  bool decorated_;
  int view_debug_mode_;
  Gadget::DebugConsoleConfig debug_console_config_;
  bool gadgets_shown_;
  bool transparent_;
  int font_size_;

  GadgetManagerInterface *gadget_manager_;
#if GTK_CHECK_VERSION(2,10,0) && defined(GGL_HOST_LINUX)
  GtkStatusIcon *status_icon_;
#else
  GtkWidget *main_widget_;
#endif
  GtkWidget *host_menu_;

  DecoratedViewHost *expanded_original_;
  DecoratedViewHost *expanded_popout_;

  HotKeyGrabber hotkey_grabber_;
  Permissions global_permissions_;
};

SimpleGtkHost::SimpleGtkHost(OptionsInterface *options, double zoom,
                             bool decorated, int view_debug_mode,
                             Gadget::DebugConsoleConfig debug_console_config)
  : impl_(new Impl(this, options, zoom, decorated, view_debug_mode,
                   debug_console_config)) {
  impl_->SetupUI();
  impl_->LoadGadgets();
}

SimpleGtkHost::~SimpleGtkHost() {
  delete impl_;
  impl_ = NULL;
}

ViewHostInterface *SimpleGtkHost::NewViewHost(Gadget *gadget,
                                              ViewHostInterface::Type type) {
  return impl_->NewViewHost(gadget, type);
}

void SimpleGtkHost::RemoveGadget(Gadget *gadget, bool save_data) {
  return impl_->RemoveGadget(gadget, save_data);
}

bool SimpleGtkHost::LoadFont(const char *filename) {
  return ggadget::gtk::LoadFont(filename);
}

void SimpleGtkHost::ShowGadgetAboutDialog(ggadget::Gadget *gadget) {
  ggadget::gtk::ShowGadgetAboutDialog(gadget);
}

void SimpleGtkHost::ShowGadgetDebugConsole(Gadget *gadget) {
  impl_->ShowGadgetDebugConsole(gadget);
}

int SimpleGtkHost::GetDefaultFontSize() {
  return impl_->font_size_;
}

bool SimpleGtkHost::OpenURL(const Gadget *gadget, const char *url) {
  return ggadget::gtk::OpenURL(gadget, url);
}

} // namespace gtk
} // namespace hosts
