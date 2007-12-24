#ifndef GGADGET_GTK_GTK_MAIN_LOOP_H__
#define GGADGET_GTK_GTK_MAIN_LOOP_H__
#include <ggadget/main_loop_interface.h>

namespace ggadget {
namespace gtk {

// GtkMainLoop is a wrapper around gtk's main loop functions to realize
// MainLoopInterface interface.
class GtkMainLoop : public MainLoopInterface {
 public:
  GtkMainLoop();
  virtual ~GtkMainLoop();
  virtual int AddIOReadWatch(int fd, WatchCallbackInterface *callback);
  virtual int AddIOWriteWatch(int fd, WatchCallbackInterface *callback);
  virtual int AddTimeoutWatch(int interval, WatchCallbackInterface *callback);
  virtual WatchType GetWatchType(int watch_id);
  virtual int GetWatchData(int watch_id);
  virtual void RemoveWatch(int watch_id);
  // This function just call gtk_main(). So you can use either gtk_main() or
  // this function.
  virtual void Run();
  // This function just call gtk_main_iteration_do(). So you can use either
  // gtk_main_iteration_do() or this function.
  virtual bool DoIteration(bool may_block);
  // This function just call gtk_main_quit().
  virtual void Quit();
  virtual bool IsRunning() const;
  virtual uint64_t GetCurrentTime() const;

 private:
  class Impl;
  Impl *impl_;
};

} // namespace gtk
} // namespace ggadget
#endif  // GGADGET_GTK_GTK_MAIN_LOOP_H__
