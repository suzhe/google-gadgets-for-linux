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

#include <stdint.h>
#include <glib/ghash.h>
#include <gtk/gtk.h>
#include <ggadget/common.h>
#include "main_loop.h"

namespace ggadget {
namespace gtk {

class MainLoop::Impl {
  struct WatchNode {
    MainLoopInterface::WatchType type;

    // Indicates if the watch is being called, thus can't be removed.
    bool calling;

    // Indicates if the watch has been scheduled to be removed.
    bool removing;

    int watch_id;
    int data;      // For IO watch, it's fd, for timeout watch, it's interval.
    WatchCallbackInterface *callback;
    MainLoop::Impl *impl;

    WatchNode()
      : type(MainLoopInterface::INVALID_WATCH),
        calling(false),
        removing(false),
        watch_id(-1),
        data(-1),
        callback(NULL),
        impl(NULL) {
    }
  };

 public:
  Impl(MainLoopInterface *main_loop)
    : main_loop_(main_loop) {
    watches_ = g_hash_table_new_full(g_direct_hash,
                                     g_direct_equal,
                                     NULL,
                                     NodeDestroyCallback);
    ASSERT(watches_);
  }

  ~Impl() {
    RemoveAllWatches();
    g_hash_table_destroy(watches_);
  }

  int AddIOWatch(MainLoopInterface::WatchType type, int fd,
                 WatchCallbackInterface *callback) {
    if (fd < 0 || !callback) return -1;
    GIOCondition cond =
        (type == MainLoopInterface::IO_READ_WATCH ? G_IO_IN : G_IO_OUT);
    GIOChannel *channel = g_io_channel_unix_new(fd);
    WatchNode *node = new WatchNode();
    node->type = type;
    node->data = fd;
    node->callback = callback;
    node->impl = this;
    node->watch_id = g_io_add_watch(channel, cond, IOWatchCallback, node);
    g_hash_table_insert(watches_, GINT_TO_POINTER(node->watch_id), node);
    g_io_channel_unref(channel);
    return node->watch_id;
  }

  int AddTimeoutWatch(int interval, WatchCallbackInterface *callback) {
    if (interval < 0 || !callback) return -1;
    WatchNode *node = new WatchNode();
    node->type = MainLoopInterface::TIMEOUT_WATCH;
    node->data = interval;
    node->callback = callback;
    node->impl = this;
    node->watch_id = (interval <= 1 ?
                      g_idle_add(TimeoutCallback, node) :
                      g_timeout_add(interval, TimeoutCallback, node));
    g_hash_table_insert(watches_, GINT_TO_POINTER(node->watch_id), node);
    return node->watch_id;
  }

  MainLoopInterface::WatchType GetWatchType(int watch_id) {
    WatchNode *node = static_cast<WatchNode *>(
        g_hash_table_lookup(watches_, GINT_TO_POINTER(watch_id)));
    return node ? node->type : MainLoopInterface::INVALID_WATCH;
  }

  int GetWatchData(int watch_id) {
    WatchNode *node = static_cast<WatchNode *>(
        g_hash_table_lookup(watches_, GINT_TO_POINTER(watch_id)));
    return node ? node->data : -1;
  }

  void RemoveWatch(int watch_id) {
    WatchNode *node = static_cast<WatchNode *>(
        g_hash_table_lookup(watches_, GINT_TO_POINTER(watch_id)));
    if (node && !node->removing) {
      node->removing = true;
      if (!node->calling) {
        g_source_remove(watch_id);
        WatchCallbackInterface *callback = node->callback;
        //DLOG("MainLoop::RemoveWatch: id=%d", watch_id);
        callback->OnRemove(main_loop_, watch_id);
        g_hash_table_remove(watches_, GINT_TO_POINTER(watch_id));
      }
    }
  }

  void Run() {
    gtk_main();
  }

  bool DoIteration(bool may_block) {
    gtk_main_iteration_do(may_block);
    // Always returns true here, because the return value of
    // gtk_main_iteration_do() has different meaning.
    return true;
  }

  void Quit() {
    gtk_main_quit();
  }

  bool IsRunning() {
    return gtk_main_level() > 0;
  }

  uint64_t GetCurrentTime() const {
    GTimeVal tv;
    g_get_current_time(&tv);
    return static_cast<uint64_t>(tv.tv_sec) * 1000 + tv.tv_usec / 1000;
  }

 private:
  void RemoveWatchNode(WatchNode *node) {
    if (node->removing) return;
    node->removing = true;
    int watch_id = node->watch_id;
    WatchCallbackInterface *callback = node->callback;
    //DLOG("MainLoop::RemoveWatchNode: id=%d", watch_id);
    callback->OnRemove(main_loop_, watch_id);
    g_hash_table_remove(watches_, GINT_TO_POINTER(watch_id));
  }

  void RemoveAllWatches() {
    g_hash_table_foreach_remove(watches_, ForeachRemoveCallback, main_loop_);
  }

  static gboolean ForeachRemoveCallback(gpointer key, gpointer value,
                                    gpointer data) {
    int watch_id = GPOINTER_TO_INT(key);
    WatchNode *node = static_cast<WatchNode *>(value);
    if (node->removing) return TRUE;
    node->removing = true;
    MainLoopInterface *main_loop = static_cast<MainLoopInterface *>(data);
    WatchCallbackInterface *callback = node->callback;

    g_source_remove(watch_id);
    //DLOG("MainLoop::ForeachRemoveCallback: id=%d", watch_id);
    callback->OnRemove(main_loop, watch_id);
    return TRUE;
  }

  // Delete WatchNode when removing it from the watches_ hash table.
  static void NodeDestroyCallback(gpointer node) {
    delete static_cast<WatchNode *>(node);
  }

  // Callback functions to be registered into gtk's main loop for io watch.
  static gboolean IOWatchCallback(GIOChannel *channel, GIOCondition condition,
                                  gpointer data) {
    WatchNode *node = static_cast<WatchNode *>(data);
    if (node && !node->calling && !node->removing) {
      MainLoop::Impl *impl = node->impl;
      MainLoopInterface *main_loop = impl->main_loop_;
      WatchCallbackInterface *callback = node->callback;
      //DLOG("MainLoop::IOWatchCallback: id=%d fd=%d type=%d",
      //     node->watch_id, node->data, node->type);
      bool ret = false;
      // Only call callback if the condition is correct.
      if ((node->type == MainLoopInterface::IO_READ_WATCH &&
          (condition & G_IO_IN)) ||
          (node->type == MainLoopInterface::IO_WRITE_WATCH &&
          (condition & G_IO_OUT))) {
        node->calling = true;
        ret = callback->Call(main_loop, node->watch_id);
        node->calling = false;
      }

      // node->removing == true means the watch was removed during the Call.
      // However, because node->calling is set to true before invoking Call(),
      // if RemoveWatch() is called during Call() to remove this watch, it
      // won't be removed, instead, removing will be set to true. In this case,
      // the watch must be removed here.
      if (!ret || node->removing) {
        node->removing = false;
        impl->RemoveWatchNode(node);
        ret = false;
      }

      return ret;
    }
    return false;
  }

  // Callback functions to be registered into gtk's main loop for timeout
  // watch.
  static gboolean TimeoutCallback(gpointer data) {
    WatchNode *node = static_cast<WatchNode*>(data);
    if (node && !node->calling && !node->removing) {
      MainLoop::Impl *impl = node->impl;
      MainLoopInterface *main_loop = impl->main_loop_;
      WatchCallbackInterface *callback = node->callback;
      //DLOG("MainLoop::TimeoutCallback: id=%d interval=%d",
      //     node->watch_id, node->data);
      node->calling = true;
      bool ret = callback->Call(main_loop, node->watch_id);
      node->calling = false;

      if (!ret || node->removing) {
        node->removing = false;
        impl->RemoveWatchNode(node);
        ret = false;
      }

      return ret;
    }
    return false;
  }

  MainLoopInterface *main_loop_;
  GHashTable *watches_;
};

MainLoop::MainLoop()
  : impl_(new Impl(this)) {
}
MainLoop::~MainLoop() {
  delete impl_;
}
int MainLoop::AddIOReadWatch(int fd, WatchCallbackInterface *callback) {
  return impl_->AddIOWatch(IO_READ_WATCH, fd, callback);
}
int MainLoop::AddIOWriteWatch(int fd, WatchCallbackInterface *callback) {
  return impl_->AddIOWatch(IO_WRITE_WATCH, fd, callback);
}
int MainLoop::AddTimeoutWatch(int interval,
                                    WatchCallbackInterface *callback) {
  return impl_->AddTimeoutWatch(interval, callback);
}
MainLoopInterface::WatchType MainLoop::GetWatchType(int watch_id) {
  return impl_->GetWatchType(watch_id);
}
int MainLoop::GetWatchData(int watch_id) {
  return impl_->GetWatchData(watch_id);
}
void MainLoop::RemoveWatch(int watch_id) {
  impl_->RemoveWatch(watch_id);
}
void MainLoop::Run() {
  impl_->Run();
}
bool MainLoop::DoIteration(bool may_block) {
  return impl_->DoIteration(may_block);
}
void MainLoop::Quit() {
  impl_->Quit();
}
bool MainLoop::IsRunning() const {
  return impl_->IsRunning();
}
uint64_t MainLoop::GetCurrentTime() const {
  return impl_->GetCurrentTime();
}

} // namespace gtk
} // namespace ggadget
