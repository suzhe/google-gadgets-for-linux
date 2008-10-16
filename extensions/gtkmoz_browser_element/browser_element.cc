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

#include <cstdlib>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <vector>
#include <cmath>
#include <algorithm>
#include <gtk/gtk.h>
#include <ggadget/gadget.h>
#include <ggadget/logger.h>
#include <ggadget/main_loop_interface.h>
#include <ggadget/scriptable_array.h>
#include <ggadget/string_utils.h>
#include <ggadget/view.h>
#include <ggadget/element_factory.h>
#include <ggadget/script_context_interface.h>

#include "browser_element.h"
#include "browser_child.h"

#define Initialize gtkmoz_browser_element_LTX_Initialize
#define Finalize gtkmoz_browser_element_LTX_Finalize
#define RegisterElementExtension \
    gtkmoz_browser_element_LTX_RegisterElementExtension

namespace ggadget {
namespace gtkmoz {
static MainLoopInterface *ggl_main_loop = NULL;
}
}

extern "C" {
  bool Initialize() {
    LOGI("Initialize gtkmoz_browser_element extension.");
    ggadget::gtkmoz::ggl_main_loop = ggadget::GetGlobalMainLoop();
    ASSERT(ggadget::gtkmoz::ggl_main_loop);
    return true;
  }

  void Finalize() {
    LOGI("Finalize gtkmoz_browser_element extension.");
    ggadget::gtkmoz::ggl_main_loop = NULL;
  }

  bool RegisterElementExtension(ggadget::ElementFactory *factory) {
    LOGI("Register gtkmoz_browser_element extension, using name \"_browser\".");
    if (factory) {
      factory->RegisterElementClass(
          "_browser", &ggadget::gtkmoz::BrowserElement::CreateInstance);
    }
    return true;
  }
}

namespace ggadget {
namespace gtkmoz {

static const char *kBrowserChildNames[] = {
#if _DEBUG
  "gtkmoz-browser-child",
#endif
  GGL_LIBEXEC_DIR "/gtkmoz-browser-child",
  NULL
};

class BrowserElement::Impl {
 public:
  Impl(BrowserElement *owner)
      : owner_(owner),
        content_type_("text/html"),
        socket_(NULL),
        controller_(BrowserController::get()),
        browser_id_(controller_->AddBrowserElement(this)),
        x_(0), y_(0), width_(0), height_(0),
        minimized_(false), popped_out_(false),
        minimized_connection_(owner->GetView()->ConnectOnMinimizeEvent(
            NewSlot(this, &Impl::OnViewMinimized))),
        restored_connection_(owner->GetView()->ConnectOnRestoreEvent(
            NewSlot(this, &Impl::OnViewRestored))),
        popout_connection_(owner->GetView()->ConnectOnPopOutEvent(
            NewSlot(this, &Impl::OnViewPoppedOut))),
        popin_connection_(owner->GetView()->ConnectOnPopInEvent(
            NewSlot(this, &Impl::OnViewPoppedIn))),
        dock_connection_(owner->GetView()->ConnectOnDockEvent(
            NewSlot(this, &Impl::OnViewDockUndock))),
        undock_connection_(owner->GetView()->ConnectOnUndockEvent(
            NewSlot(this, &Impl::OnViewDockUndock))) {
  }

  ~Impl() {
    minimized_connection_->Disconnect();
    restored_connection_->Disconnect();
    popout_connection_->Disconnect();
    popin_connection_->Disconnect();
    dock_connection_->Disconnect();
    undock_connection_->Disconnect();
    if (GTK_IS_WIDGET(socket_))
      gtk_widget_destroy(socket_);
    controller_->SendCommand(kCloseBrowserCommand, browser_id_, NULL);
    controller_->RemoveBrowserElement(browser_id_);
  }

  void GetWidgetExtents(gint *x, gint *y, gint *width, gint *height) {
    double widget_x0, widget_y0;
    double widget_x1, widget_y1;
    owner_->SelfCoordToViewCoord(0, 0, &widget_x0, &widget_y0);
    owner_->SelfCoordToViewCoord(owner_->GetPixelWidth(),
                                 owner_->GetPixelHeight(),
                                 &widget_x1, &widget_y1);

    owner_->GetView()->ViewCoordToNativeWidgetCoord(widget_x0, widget_y0,
                                                    &widget_x0, &widget_y0);
    owner_->GetView()->ViewCoordToNativeWidgetCoord(widget_x1, widget_y1,
                                                    &widget_x1, &widget_y1);

    *x = static_cast<gint>(round(widget_x0));
    *y = static_cast<gint>(round(widget_y0));
    *width = static_cast<gint>(ceil(widget_x1 - widget_x0));
    *height = static_cast<gint>(ceil(widget_y1 - widget_y0));
  }

  void CreateSocket() {
    if (socket_)
      return;

    GtkWidget *container = GTK_WIDGET(owner_->GetView()->GetNativeWidget());
    if (!GTK_IS_FIXED(container)) {
      LOG("BrowserElement needs a GTK_FIXED parent. Actual type: %s",
          G_OBJECT_TYPE_NAME(container));
      return;
    }

    socket_ = gtk_socket_new();
    g_signal_connect_after(socket_, "realize",
                           G_CALLBACK(OnSocketRealize), this);
    g_signal_connect(socket_, "destroy",
                     GTK_SIGNAL_FUNC(gtk_widget_destroyed), &socket_);

    GetWidgetExtents(&x_, &y_, &width_, &height_);
    gtk_fixed_put(GTK_FIXED(container), socket_, x_, y_);
    gtk_widget_set_size_request(socket_, width_, height_);
    gtk_widget_show(socket_);
    gtk_widget_realize(socket_);
  }

  static void OnSocketRealize(GtkWidget *widget, gpointer user_data) {
    Impl *impl = static_cast<Impl *>(user_data);
    std::string browser_id_str = StringPrintf("%zd", impl->browser_id_);
    // Convert GdkNativeWindow to intmax_t to ensure the printf format
    // to match the data type and not to loose accuracy.
    std::string socket_id_str = StringPrintf("0x%jx",
        static_cast<intmax_t>(gtk_socket_get_id(GTK_SOCKET(impl->socket_))));
    impl->controller_->SendCommand(kNewBrowserCommand, impl->browser_id_,
                                   socket_id_str.c_str(), NULL);
    impl->SetChildContent();
  }

  void SetChildContent() {
    controller_->SendCommand(kSetContentCommand, browser_id_,
                             content_type_.c_str(), content_.c_str(), NULL);
  }

  void Layout() {
    GtkWidget *container = GTK_WIDGET(owner_->GetView()->GetNativeWidget());
    if (GTK_IS_FIXED(container) && GTK_IS_SOCKET(socket_)) {
      bool force_layout = false;
      // check if the contain has changed.
      if (gtk_widget_get_parent(socket_) != container) {
        gtk_widget_reparent(socket_, container);
        force_layout = true;
      }

      gint x, y, width, height;
      GetWidgetExtents(&x, &y, &width, &height);

      if (x != x_ || y != y_ || force_layout) {
        x_ = x;
        y_ = y;
        gtk_fixed_move(GTK_FIXED(container), socket_, x, y);
      }
      if (width != width_ || height != height_ || force_layout) {
        width_ = width;
        height_ = height;
        gtk_widget_set_size_request(socket_, width, height);
      }
      if (owner_->IsReallyVisible() && (!minimized_ || popped_out_))
        gtk_widget_show(socket_);
      else
        gtk_widget_hide(socket_);
    }
  }

  void ProcessUpMessage(const std::vector<const char *> &params) {
    std::string result;
    const char *type = params[0];
    if (strcmp(type, kGetPropertyFeedback) == 0) {
      if (params.size() != 3) {
        LOG("%s feedback needs 3 parameters, but %zd is given",
            kGetPropertyFeedback, params.size());
      } else {
        result = get_property_signal_(JSONString(params[2])).value;
      }
    } else if (strcmp(type, kSetPropertyFeedback) == 0) {
      if (params.size() != 4) {
        LOG("%s feedback needs 4 parameters, but %zd is given",
            kSetPropertyFeedback, params.size());
      } else {
        set_property_signal_(JSONString(params[2]), JSONString(params[3]));
      }
    } else if (strcmp(type, kCallbackFeedback) == 0) {
      if (params.size() < 3) {
        LOG("%s feedback needs at least 3 parameters, but %zd is given",
            kCallbackFeedback, params.size());
      } else {
        size_t callback_params_count = params.size() - 3;
        ScriptableArray *callback_params = new ScriptableArray();
        for (size_t i = 0; i < callback_params_count; ++i)
          callback_params->Append(Variant(JSONString(params[i + 3])));
        JSONString result_json =
            callback_signal_(JSONString(params[2]), callback_params);
        result = result_json.value;
      }
    } else if (strcmp(type, kOpenURLFeedback) == 0) {
      if (params.size() != 3) {
        LOG("%s feedback needs 3 parameters, but %zd is given",
            kOpenURLFeedback, params.size());
      } else {
        if (!open_url_signal_.HasActiveConnections() ||
            open_url_signal_(params[2])) {
          Gadget *gadget = owner_->GetView()->GetGadget();
          if (gadget) {
            // Let the gadget allow this OpenURL gracefully.
            bool old_interaction = gadget->SetInUserInteraction(true);
            result = gadget->OpenURL(params[2]) ? '1' : '0';
            gadget->SetInUserInteraction(old_interaction);
          } else {
            result = '0';
          }
        } else {
          result = '0';
        }
      }
    } else {
      LOG("Unknown feedback: %s", type);
    }
    DLOG("ProcessUpMessage: %s(%s,%s,%s,%s) result: %s", type,
         params.size() > 1 ? params[1] : "",
         params.size() > 2 ? params[2] : "",
         params.size() > 3 ? params[3] : "",
         params.size() > 4 ? params[4] : "",
         result.c_str());
    result += '\n';
    controller_->Write(controller_->ret_fd_, result.c_str(), result.size());
  }

  void SetContent(const std::string &content) {
    content_ = '\"' + EncodeJavaScriptString(content) + '\"';
    if (!GTK_IS_SOCKET(socket_)) {
      // After the child exited, the socket_ will become an invalid GtkSocket.
      CreateSocket();
    } else {
      SetChildContent();
    }
  }

  void OnViewMinimized() {
    // The browser widget must be hidden when the view is minimized.
    if (GTK_IS_SOCKET(socket_) && !popped_out_) {
      gtk_widget_hide(socket_);
    }
    minimized_ = true;
  }

  void OnViewRestored() {
    if (GTK_IS_SOCKET(socket_) && owner_->IsReallyVisible() && !popped_out_) {
      gtk_widget_show(socket_);
    }
    minimized_ = false;
  }

  void OnViewPoppedOut() {
    popped_out_ = true;
    Layout();
  }

  void OnViewPoppedIn() {
    popped_out_ = false;
    Layout();
  }

  void OnViewDockUndock() {
    // The toplevel window might be changed, so it's necessary to reparent the
    // browser widget.
    Layout();
  }

  class BrowserController {
   public:
    BrowserController()
        : child_pid_(0),
          down_fd_(0), up_fd_(0), ret_fd_(0),
          up_fd_watch_(0),
          ping_timer_watch_(ggl_main_loop->AddTimeoutWatch(
              kPingInterval * 3 / 2,
              new WatchCallbackSlot(
                  NewSlot(this, &BrowserController::PingTimerCallback)))),
          ping_flag_(false),
          removing_watch_(false) {
      StartChild();
    }

    ~BrowserController() {
      StopChild(false);
      instance_ = NULL;
      // This object may live longer than the main loop, so don't remove timer
      // watch here.
    }

    bool PingTimerCallback(int watch) {
      if (!ping_flag_)
        RestartChild();
      ping_flag_ = false;
      return true;
    }

    static BrowserController *get() {
      if (!instance_)
        instance_ = new BrowserController();
      return instance_;
    }

    void StartChild() {
      removing_watch_ = false;

      int down_pipe_fds[2], up_pipe_fds[2], ret_pipe_fds[2];
      if (pipe(down_pipe_fds) == -1) {
        LOG("Failed to create downwards pipe to browser child");
        return;
      }
      if (pipe(up_pipe_fds) == -1) {
        LOG("Failed to create upwards pipe to browser child");
        close(down_pipe_fds[0]);
        close(down_pipe_fds[1]);
        return;
      }
      if (pipe(ret_pipe_fds) == -1) {
        LOG("Failed to create return value pipe to browser child");
        close(down_pipe_fds[0]);
        close(down_pipe_fds[1]);
        close(up_pipe_fds[0]);
        close(up_pipe_fds[1]);
        return;
      }

      child_pid_ = fork();
      if (child_pid_ == -1) {
        LOG("Failed to fork browser child");
        close(down_pipe_fds[0]);
        close(down_pipe_fds[1]);
        close(up_pipe_fds[0]);
        close(up_pipe_fds[1]);
        close(ret_pipe_fds[0]);
        close(ret_pipe_fds[1]);
        return;
      }

      if (child_pid_ == 0) {
        // This is the child process.
        close(down_pipe_fds[1]);
        close(up_pipe_fds[0]);
        close(ret_pipe_fds[1]);
        std::string down_fd_str = StringPrintf("%d", down_pipe_fds[0]);
        std::string up_fd_str = StringPrintf("%d", up_pipe_fds[1]);
        std::string ret_fd_str = StringPrintf("%d", ret_pipe_fds[0]);
        for (size_t i = 0; kBrowserChildNames[i]; ++i) {
          execl(kBrowserChildNames[i], kBrowserChildNames[i],
                down_fd_str.c_str(), up_fd_str.c_str(),
                ret_fd_str.c_str(), NULL);
        }
        LOG("Failed to execute browser child");
        _exit(-1);
      } else {
        close(down_pipe_fds[0]);
        close(up_pipe_fds[1]);
        close(ret_pipe_fds[0]);
        down_fd_ = down_pipe_fds[1];
        up_fd_ = up_pipe_fds[0];
        ret_fd_ = ret_pipe_fds[1];
        int up_fd_flags = fcntl(up_fd_, F_GETFL);
        up_fd_flags |= O_NONBLOCK;
        fcntl(up_fd_, F_SETFL, up_fd_flags);
        up_fd_watch_ =
            ggl_main_loop->AddIOReadWatch(up_fd_, new UpFdWatchCallback(this));
      }
    }

    void StopChild(bool on_error) {
      if (!removing_watch_) {
        removing_watch_ = true;
        ggl_main_loop->RemoveWatch(up_fd_watch_);
        removing_watch_ = false;
      }
      up_fd_watch_ = 0;
      if (child_pid_) {
        // Don't send QUIT command on error to prevent error loops.
        if (!on_error) {
          std::string quit_command(kQuitCommand);
          quit_command += kEndOfMessageFull;
          Write(down_fd_, quit_command.c_str(), quit_command.size());
        }
        close(down_fd_);
        down_fd_ = 0;
        close(up_fd_);
        up_fd_ = 0;
        close(ret_fd_);
        ret_fd_ = 0;
        child_pid_ = 0;
      }
      browser_elements_.clear();
    }

    void RestartChild() {
      StopChild(true);
      StartChild();
    }

    size_t AddBrowserElement(Impl *impl) {
      // Find an empty slot.
      BrowserElements::iterator it = std::find(browser_elements_.begin(),
                                               browser_elements_.end(),
                                               static_cast<Impl *>(NULL));
      size_t result = it - browser_elements_.begin();
      if (it == browser_elements_.end())
        browser_elements_.push_back(impl);
      else
        *it = impl;
      return result;
    }

    void RemoveBrowserElement(size_t id) {
      browser_elements_[id] = NULL;
    }

    class UpFdWatchCallback : public WatchCallbackInterface {
     public:
      UpFdWatchCallback(BrowserController *controller)
          : controller_(controller) {
      }
      virtual bool Call(MainLoopInterface *main_loop, int watch_id) {
        controller_->OnUpReady();
        return true;
      }
      virtual void OnRemove(MainLoopInterface *main_loop, int watch_id) {
        if (!controller_->removing_watch_) {
          controller_->removing_watch_ = true;
          // Removed by the mainloop when mainloop itself is to be destroyed.
          delete controller_;
        }
        delete this;
      }
     private:
      BrowserController *controller_;
    };

    void OnUpReady() {
      char buffer[4096];
      ssize_t read_bytes;
      while ((read_bytes = read(up_fd_, buffer, sizeof(buffer))) > 0) {
        up_buffer_.append(buffer, read_bytes);
        if (read_bytes < static_cast<ssize_t>(sizeof(buffer)))
          break;
      }
      if (read_bytes < 0)
        RestartChild();
      ProcessUpMessages();
    }

    void ProcessUpMessages() {
      size_t curr_pos = 0;
      size_t eom_pos;
      while ((eom_pos = up_buffer_.find(kEndOfMessageFull, curr_pos)) !=
              up_buffer_.npos) {
        std::vector<const char *> params;
        while (curr_pos < eom_pos) {
          size_t end_of_line_pos = up_buffer_.find('\n', curr_pos);
          ASSERT(end_of_line_pos != up_buffer_.npos);
          up_buffer_[end_of_line_pos] = '\0';
          params.push_back(up_buffer_.c_str() + curr_pos);
          curr_pos = end_of_line_pos + 1;
        }
        ASSERT(curr_pos = eom_pos + 1);
        curr_pos += sizeof(kEndOfMessageFull) - 2;

        if (params.size() == 1 && strcmp(params[0], kPingFeedback) == 0) {
          Write(ret_fd_, kPingAckFull, arraysize(kPingAckFull) - 1);
          ping_flag_ = true;
        } else if (params.size() < 2) {
          LOG("No enough feedback parameters");
        } else {
          size_t id = static_cast<size_t>(strtol(params[1], NULL, 0));
          if (id < browser_elements_.size() && browser_elements_[id] != NULL) {
            browser_elements_[id]->ProcessUpMessage(params);
          } else {
            LOG("Invalid browser id: %s", params[1]);
          }
        }
      }
      up_buffer_.erase(0, curr_pos);
    }

    void SendCommand(const char *type, size_t browser_id, ...) {
      if (down_fd_ > 0) {
        std::string buffer(type);
        buffer += '\n';
        buffer += StringPrintf("%zd", browser_id);

        va_list ap;
        va_start(ap, browser_id);
        const char *param;
        while ((param = va_arg(ap, const char *)) != NULL) {
          buffer += '\n';
          buffer += param;
        }
        buffer += kEndOfMessageFull;
        Write(down_fd_, buffer.c_str(), buffer.size());
      }
    }

    static void OnSigPipe(int sig) {
      instance_->RestartChild();
    }

    void Write(int fd, const char *data, size_t size) {
      sig_t old_handler = signal(SIGPIPE, OnSigPipe);
      if (write(fd, data, size) < 0)
        RestartChild();
      signal(SIGPIPE, old_handler);
    }

    static BrowserController *instance_;
    int child_pid_;
    int down_fd_, up_fd_, ret_fd_;
    int up_fd_watch_;
    int ping_timer_watch_;
    bool ping_flag_;
    std::string up_buffer_;
    typedef std::vector<Impl *> BrowserElements;
    BrowserElements browser_elements_;
    bool removing_watch_;
  };

  BrowserElement *owner_;
  std::string content_type_;
  std::string content_;
  GtkWidget *socket_;
  BrowserController *controller_;
  size_t browser_id_;
  gint x_, y_, width_, height_;
  Signal1<JSONString, JSONString> get_property_signal_;
  Signal2<void, JSONString, JSONString> set_property_signal_;
  Signal2<JSONString, JSONString, ScriptableArray *> callback_signal_;
  Signal1<bool, const std::string &> open_url_signal_;
  bool minimized_;
  bool popped_out_;
  Connection *minimized_connection_, *restored_connection_,
             *popout_connection_, *popin_connection_,
             *dock_connection_, *undock_connection_;
};

BrowserElement::Impl::BrowserController *
    BrowserElement::Impl::BrowserController::instance_ = NULL;

BrowserElement::BrowserElement(View *view, const char *name)
    : BasicElement(view, "browser", name, true),
      impl_(new Impl(this)) {
}

void BrowserElement::DoClassRegister() {
  BasicElement::DoClassRegister();
  RegisterProperty("contentType",
                   NewSlot(&BrowserElement::GetContentType),
                   NewSlot(&BrowserElement::SetContentType));
  RegisterProperty("innerText", NULL,
                   NewSlot(&BrowserElement::SetContent));
  RegisterClassSignal("onGetProperty", &Impl::get_property_signal_,
                      &BrowserElement::impl_);
  RegisterClassSignal("onSetProperty", &Impl::set_property_signal_,
                      &BrowserElement::impl_);
  RegisterClassSignal("onCallback", &Impl::callback_signal_,
                      &BrowserElement::impl_);
  RegisterClassSignal("onOpenURL", &Impl::open_url_signal_,
                      &BrowserElement::impl_);
}

BrowserElement::~BrowserElement() {
  delete impl_;
  impl_ = NULL;
}

std::string BrowserElement::GetContentType() const {
  return impl_->content_type_;
}

void BrowserElement::SetContentType(const char *content_type) {
  impl_->content_type_ = content_type && *content_type ? content_type :
                         "text/html";
}

void BrowserElement::SetContent(const std::string &content) {
  impl_->SetContent(content);
}

void BrowserElement::Layout() {
  BasicElement::Layout();
  impl_->Layout();
}

void BrowserElement::DoDraw(CanvasInterface *canvas) {
}

BasicElement *BrowserElement::CreateInstance(View *view, const char *name) {
  return new BrowserElement(view, name);
}

} // namespace gtkmoz
} // namespace ggadget
