#ifndef TIMERFD_HH
#define TIMERFD_HH

#include <time.h>
#include <sys/timerfd.h>

#include "file_descriptor.hh"

class Timerfd : public FileDescriptor
{
public:
  Timerfd(int clockid = CLOCK_MONOTONIC, int flags = TFD_NONBLOCK);

  void set_time(const timespec & initial_expiration,
                const timespec & interval);

  unsigned int read_expirations();
};

#endif /* TIMERFD_HH */
