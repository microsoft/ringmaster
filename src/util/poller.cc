#include <cassert>
#include <iostream>

#include "poller.hh"
#include "exception.hh"

using namespace std;

void Poller::register_event(const int fd,
                            const Flag flag,
                            const Callback callback)
{
  if (not roster_.count(fd)) { // fd is not registered yet
    roster_[fd][flag] = callback;
    active_events_[fd] = flag;
  }
  else { // fd is registered but flag should not be registered yet
    if (roster_[fd].count(flag)) {
      throw runtime_error("attempted to register the same event");
    }

    roster_[fd][flag] = callback;
    active_events_[fd] |= flag;
  }
}

void Poller::register_event(const FileDescriptor & fd,
                            const Flag flag,
                            const Callback callback)
{
  register_event(fd.fd_num(), flag, callback);
}

void Poller::activate(const int fd, const Flag flag)
{
  active_events_.at(fd) |= flag;
}

void Poller::activate(const FileDescriptor & fd, const Flag flag)
{
  activate(fd.fd_num(), flag);
}

void Poller::deactivate(const int fd, const Flag flag)
{
  active_events_.at(fd) &= ~flag;
}

void Poller::deactivate(const FileDescriptor & fd, const Flag flag)
{
  deactivate(fd.fd_num(), flag);
}

void Poller::deregister(const int fd)
{
  fds_to_deregister_.emplace(fd);
}

void Poller::deregister(const FileDescriptor & fd)
{
  deregister(fd.fd_num());
}

void Poller::do_deregister()
{
  for (const int fd : fds_to_deregister_) {
    roster_.erase(fd);
    active_events_.erase(fd);
  }

  fds_to_deregister_.clear();
}

void Poller::poll(const int timeout_ms)
{
  // first, deregister the fds that have been scheduled to deregister
  do_deregister();

  // construct a list of fds to poll
  vector<pollfd> fds_to_poll;
  for (const auto [fd, events] : active_events_) {
    if (events != 0) {
      fds_to_poll.push_back({fd, events, 0});
    }
  }

  check_syscall(::poll(fds_to_poll.data(), fds_to_poll.size(), timeout_ms));

  for (const auto & it : fds_to_poll) {
    for (const auto & [flag, callback] : roster_.at(it.fd)) {
      if (it.revents & flag) {
        assert(it.events & flag); // returned event must have been requested
        callback(); // execute the callback function
      }
    }
  }
}
