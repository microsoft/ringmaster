#include <cstdio>
#include <iostream>

#include "epoller.hh"
#include "exception.hh"

using namespace std;

Epoller::Epoller()
  : epfd_(check_syscall(epoll_create1(EPOLL_CLOEXEC)))
{}

Epoller::~Epoller()
{
  // don't throw from destructor
  if (close(epfd_) < 0) {
    perror("failed to close epoll instance");
  }
}

void Epoller::register_event(const int fd,
                             const Flag flag,
                             const Callback callback)
{
  if (not roster_.count(fd)) { // fd is not registered yet
    roster_[fd][flag] = callback;
    active_events_[fd] = flag;
    epoll_add(fd, flag);
  }
  else { // fd is registered but flag should not be registered yet
    if (roster_[fd].count(flag)) {
      throw runtime_error("attempted to register the same event");
    }

    roster_[fd][flag] = callback;
    active_events_[fd] |= flag;
    epoll_mod(fd, active_events_[fd]);
  }
}

void Epoller::register_event(const FileDescriptor & fd,
                             const Flag flag,
                             const Callback callback)
{
  register_event(fd.fd_num(), flag, callback);
}

void Epoller::activate(const int fd, const Flag flag)
{
  uint32_t & active_events = active_events_.at(fd);

  // activate only if not activated yet
  if (not (active_events & flag)) {
    active_events |= flag;
    epoll_mod(fd, active_events);
  }
}

void Epoller::activate(const FileDescriptor & fd, const Flag flag)
{
  activate(fd.fd_num(), flag);
}

void Epoller::deactivate(const int fd, const Flag flag)
{
  uint32_t & active_events = active_events_.at(fd);

  // deactivate only if activated before
  if (active_events & flag) {
    active_events &= ~flag;
    epoll_mod(fd, active_events);
  }
}

void Epoller::deactivate(const FileDescriptor & fd, const Flag flag)
{
  deactivate(fd.fd_num(), flag);
}

void Epoller::deregister(const int fd)
{
  fds_to_deregister_.emplace(fd);
}

void Epoller::deregister(const FileDescriptor & fd)
{
  deregister(fd.fd_num());
}

void Epoller::do_deregister()
{
  for (const int fd : fds_to_deregister_) {
    roster_.erase(fd);
    active_events_.erase(fd);
    check_syscall(epoll_ctl(epfd_, EPOLL_CTL_DEL, fd, nullptr));
  }

  fds_to_deregister_.clear();
}

void Epoller::poll(const int timeout_ms)
{
  static constexpr size_t MAX_EVENTS = 32;
  static epoll_event event_list[MAX_EVENTS];

  // first, deregister the fds that have been scheduled to deregister
  do_deregister();

  int nfds = check_syscall(
    epoll_wait(epfd_, event_list, sizeof(event_list), timeout_ms));

  for (int i = 0; i < nfds; i++) {
    int fd = event_list[i].data.fd;
    uint32_t revents = event_list[i].events;

    for (const auto & [flag, callback] : roster_.at(fd)) {
      if (revents & flag) {
        callback(); // execute the callback function
      }
    }
  }
}

void Epoller::epoll_add(const int fd, const uint32_t events)
{
  epoll_event ev;
  ev.data.fd = fd;
  ev.events = events;

  check_syscall(epoll_ctl(epfd_, EPOLL_CTL_ADD, fd, &ev));
}

void Epoller::epoll_mod(const int fd, const uint32_t events)
{
  epoll_event ev;
  ev.data.fd = fd;
  ev.events = events;

  check_syscall(epoll_ctl(epfd_, EPOLL_CTL_MOD, fd, &ev));
}
