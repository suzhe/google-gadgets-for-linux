#ifndef GGADGET_QT_QT_MAIN_LOOP_H__
#define GGADGET_QT_QT_MAIN_LOOP_H__
#include <ggadget/main_loop_interface.h>

namespace ggadget {
namespace qt {

// QtMainLoop is a qt4 implementation of MainLoopInterface interface.
class QtMainLoop : public MainLoopInterface {
 public:
  QtMainLoop();
  virtual ~QtMainLoop();
  virtual int AddIOReadWatch(int fd, WatchCallbackInterface *callback);
  virtual int AddIOWriteWatch(int fd, WatchCallbackInterface *callback);
  virtual int AddTimeoutWatch(int interval, WatchCallbackInterface *callback);
  virtual WatchType GetWatchType(int watch_id);
  virtual int GetWatchData(int watch_id);
  virtual void RemoveWatch(int watch_id);
  virtual void Run();
  virtual bool DoIteration(bool may_block);
  virtual void Quit();
  virtual bool IsRunning() const;
  virtual uint64_t GetCurrentTime() const;

 private:
  class Impl;
  Impl *impl_;
};

} // namespace qt
} // namespace ggadget
#endif  // GGADGET_QT_QT_MAIN_LOOP_H__
