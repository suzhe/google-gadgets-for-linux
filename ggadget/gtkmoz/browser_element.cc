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

#include <cmath>
#include <unistd.h>
#include <fcntl.h>
#include <vector>
#include <gtk/gtk.h>
#include <ggadget/logger.h>
#include <ggadget/main_loop_interface.h>
#include <ggadget/scriptable_array.h>
#include <ggadget/string_utils.h>
#include <ggadget/view.h>

#include "browser_element.h"
#include "browser_child.h"

namespace ggadget {
namespace gtkmoz {

class BrowserElement::Impl {
 public:
  Impl(BrowserElement *owner)
      : owner_(owner),
        main_loop_(owner->GetView()->GetMainLoop()),
        content_type_("text/html"),
        container_(NULL),
        container_x_(0), container_y_(0),
        socket_(NULL),
        child_pid_(0),
        down_fd_(0), up_fd_(0), ret_fd_(0),
        up_fd_watch_(0) {
  }
  ~Impl() {
    QuitChild();
  }

  bool EnsureSocket() {
    if (socket_)
      return true;

    socket_ = gtk_socket_new();
    owner_->GetView()->GetNativeWidgetInfo(
        reinterpret_cast<void **>(&container_), &container_x_, &container_y_);
    if (!GTK_IS_FIXED(container_)) {
      LOG("BrowserElement needs a GTK_FIXED parent. Actual type: %s",
          G_OBJECT_TYPE_NAME(container_));
      gtk_widget_destroy(socket_);
      socket_ = NULL;
      return false;
    }
    gtk_fixed_put(GTK_FIXED(container_), socket_,
                  container_x_ + static_cast<gint>(round(owner_->GetPixelX())),
                  container_y_ + static_cast<gint>(round(owner_->GetPixelY())));
    gtk_widget_set_size_request(
        socket_,
        static_cast<gint>(ceil(owner_->GetPixelWidth())),
        static_cast<gint>(ceil(owner_->GetPixelHeight())));
    gtk_widget_show(socket_);

    if (GTK_WIDGET_REALIZED(container_)) {
      EnsurePipeAndChild();
      return true;
    } else {
      g_signal_connect(socket_, "realize", G_CALLBACK(OnSocketRealize), this);
      return false;
    }
  }

  static void OnSocketRealize(GtkWidget *widget, gpointer user_data) {
    Impl *impl = static_cast<Impl *>(user_data);
    impl->EnsurePipeAndChild();
    impl->SendCommand(kSetContentCommand, impl->content_type_.c_str(),
                      impl->content_.c_str(), NULL);
  }

  void EnsurePipeAndChild() {
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
      // Convert GdkNativeWindow to intmax_t to ensure the printf format
      // to match the data type and not to loose accuracy.
      std::string socket_id_str = StringPrintf("0x%jx",
          static_cast<intmax_t>(gtk_socket_get_id(GTK_SOCKET(socket_))));
      std::string down_fd_str = StringPrintf("%d", down_pipe_fds[0]);
      std::string up_fd_str = StringPrintf("%d", up_pipe_fds[1]); 
      std::string ret_fd_str = StringPrintf("%d", ret_pipe_fds[0]); 
      // TODO: Deal with the situtation that the main program is not run from
      // the directory it is in.
      execl("browser_child", "browser_child", socket_id_str.c_str(),
            down_fd_str.c_str(), up_fd_str.c_str(), ret_fd_str.c_str(), NULL);
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
      up_fd_watch_ = main_loop_->AddIOReadWatch(
          up_fd_, new WatchCallbackSlot(NewSlot(this, &Impl::OnUpReady)));
    }
  }

  void Layout() {
    gtk_fixed_move(
        GTK_FIXED(container_), socket_,
        container_x_ + static_cast<gint>(round(owner_->GetPixelX())),
        container_y_ + static_cast<gint>(round(owner_->GetPixelY())));
    gtk_widget_set_size_request(
        socket_,
        static_cast<gint>(ceil(owner_->GetPixelWidth())),
        static_cast<gint>(ceil(owner_->GetPixelHeight())));
  }

  bool OnUpReady(int fd) {
    // ASSERT(fd == up_fd_);
    char buffer[4096];
    ssize_t read_bytes;
    while ((read_bytes = read(up_fd_, buffer, sizeof(buffer))) > 0) {
      up_buffer_.append(buffer, read_bytes);
      if (read_bytes < static_cast<ssize_t>(sizeof(buffer)))
        break;
    }
    ProcessUpMessages();
    return true;
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
      ProcessUpMessage(params);
    }
    up_buffer_.erase(0, curr_pos);
  }

  void ProcessUpMessage(const std::vector<const char *> &params) {
    std::string result;
    if (params.size() > 0) {
      const char *type = params[0];
      if (strcmp(type, kGetPropertyFeedback) == 0) {
        if (params.size() != 2) {
          LOG("%s feedback needs 2 parameters, but %zd is given",
              kGetPropertyFeedback, params.size());
        } else {
          result = get_property_signal_(JSONString(params[1])).value;
        }
      } else if (strcmp(type, kSetPropertyFeedback) == 0) {
        if (params.size() != 3) {
          LOG("%s feedback needs 3 parameters, but %zd is given",
              kSetPropertyFeedback, params.size());
        } else {
          set_property_signal_(JSONString(params[1]), JSONString(params[2]));
        }
      } else if (strcmp(type, kCallbackFeedback) == 0) {
        if (params.size() < 2) {
          LOG("%s feedback needs at least 2 parameters, but %zd is given",
              kCallbackFeedback, params.size());
        } else {
          size_t callback_params_count = params.size() - 2;
          Variant *callback_params = new Variant[callback_params_count];
          for (size_t i = 0; i < callback_params_count; i++)
            callback_params[i] = Variant(JSONString(params[i + 2]));
          JSONString result_json = callback_signal_(
              JSONString(params[1]),
              ScriptableArray::Create(callback_params, callback_params_count));
          result = result_json.value;
        }
      } else if (strcmp(type, kOpenURLFeedback) == 0) {
        if (params.size() < 2) {
          LOG("%s feedback needs 2 parameters, but %zd is given",
              kOpenURLFeedback, params.size());
        }
        open_url_signal_(JSONString(params[1]));
      } else {
        LOG("Unknown feedback: %s", type);
      }
      DLOG("ProcessUpMessage: %s(%s,%s) result: %s", type,
           params.size() > 1 ? params[1] : "",
           params.size() > 2 ? params[2] : "",
           result.c_str());
    }
    result += '\n';
    write(ret_fd_, result.c_str(), result.size());
  }

  void QuitChild() {
    SendCommand(kQuitCommand, NULL);
    if (socket_) {
      gtk_widget_destroy(socket_);
      socket_ = NULL;
    }
    if (down_fd_) {
      close(down_fd_);
      down_fd_ = 0;
    }
    if (up_fd_) {
      main_loop_->RemoveWatch(up_fd_watch_);
      close(up_fd_);
      up_fd_ = 0;
      up_fd_watch_ = 0;
    }
    if (ret_fd_) {
      close(ret_fd_);
      ret_fd_ = 0;
    }
    child_pid_ = 0;
    up_buffer_.clear();
  }

  void SetContent(const JSONString &content) {
    content_ = content.value;
    if (EnsureSocket()) {
      SendCommand(kSetContentCommand, content_type_.c_str(),
                  content.value.c_str(), NULL);
    }
    // The most common reason that EnsureSocket() returns false is that the
    // container widget has not been realized. The remaining things will be
    // done when the container widget get realized.
  }

  void SendCommand(const char *type, ...) {
    if (down_fd_ > 0) {
      std::string buffer(type);
      va_list ap;
      va_start(ap, type);
      const char *param;
      while ((param = va_arg(ap, const char *)) != NULL) {
        buffer += '\n';
        buffer += param;
      }
      buffer += kEndOfMessageFull;
      write(down_fd_, buffer.c_str(), buffer.size());
    }
  }

  BrowserElement *owner_;
  MainLoopInterface *main_loop_;
  std::string content_type_;
  std::string content_;
  GtkWidget *container_;
  int container_x_, container_y_;
  GtkWidget *socket_;
  pid_t child_pid_;
  int down_fd_, up_fd_, ret_fd_;
  int up_fd_watch_;
  std::string up_buffer_;
  Signal1<JSONString, JSONString> get_property_signal_;
  Signal2<void, JSONString, JSONString> set_property_signal_;
  Signal2<JSONString, JSONString, ScriptableArray *> callback_signal_;
  Signal1<void, JSONString> open_url_signal_;
};

BrowserElement::BrowserElement(BasicElement *parent, View *view,
                               const char *name)
    : BasicElement(parent, view, "browser", name, true),
      impl_(new Impl(this)) {
  RegisterProperty("contentType",
                   NewSlot(this, &BrowserElement::GetContentType),
                   NewSlot(this, &BrowserElement::SetContentType));
  RegisterProperty("innerText", NULL,
                   NewSlot(this, &BrowserElement::SetContent));
  RegisterSignal("onGetProperty", &impl_->get_property_signal_);
  RegisterSignal("onSetProperty", &impl_->set_property_signal_);
  RegisterSignal("onCallback", &impl_->callback_signal_);
  RegisterSignal("onOpenURL", &impl_->open_url_signal_);
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

void BrowserElement::SetContent(const JSONString &content) {
  impl_->SetContent(content);
}

void BrowserElement::Layout() {
  BasicElement::Layout();
  impl_->Layout();
}

void BrowserElement::DoDraw(CanvasInterface *canvas) {
}

BasicElement *BrowserElement::CreateInstance(BasicElement *parent, View *view,
                                             const char *name) {
  return new BrowserElement(parent, view, name);
}

} // namespace gtkmoz
} // namespace ggadget
