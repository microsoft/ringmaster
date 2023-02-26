#ifndef POLLER_HH
#define POLLER_HH

#include <poll.h>

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <functional>

#include "file_descriptor.hh"

class Poller
{
public:
  // type definitions
  enum Flag : short {
    In = POLLIN,
    Out = POLLOUT
  };

  using Callback = std::function<void()>;

  // register a single event (flag) on fd to monitor with a callback function
  void register_event(const int fd,
                      const Flag flag,
                      const Callback callback);
  void register_event(const FileDescriptor & fd,
                      const Flag flag,
                      const Callback callback);

  // activate an event on fd (safe to be called repeatedly)
  void activate(const int fd, const Flag flag);
  void activate(const FileDescriptor & fd, const Flag flag);

  // deactivate an event on fd (safe to be called repeatedly)
  void deactivate(const int fd, const Flag flag);
  void deactivate(const FileDescriptor & fd, const Flag flag);

  // deregister a fd from the interest list
  void deregister(const int fd);
  void deregister(const FileDescriptor & fd);

  // execute the callbacks on the ready fds
  void poll(const int timeout_ms = -1);

private:
  // *actually* deregister fds in fds_to_deregister_
  void do_deregister();

  // fd -> {flag (bit value) -> callback}
  std::unordered_map<int, std::unordered_map<Flag, Callback>> roster_ {};

  // fd -> currently active events (bitmask)
  std::unordered_map<int, short> active_events_ {};

  // fds scheduled to deregister
  std::unordered_set<int> fds_to_deregister_ {};
};

#endif /* POLLER_HH */
