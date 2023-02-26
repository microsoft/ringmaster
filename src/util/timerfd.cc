#include "timerfd.hh"
#include "exception.hh"
#include "conversion.hh"

using namespace std;

Timerfd::Timerfd(int clockid, int flags)
  : FileDescriptor(check_syscall(timerfd_create(clockid, flags)))
{}

void Timerfd::set_time(const timespec & initial_expiration,
                       const timespec & interval)
{
  itimerspec its;
  its.it_value = initial_expiration;
  its.it_interval = interval;

  check_syscall(timerfd_settime(fd_num(), 0, &its, nullptr));
}

unsigned int Timerfd::read_expirations()
{
  uint64_t num_exp = 0;

  if (check_syscall(::read(fd_num(), &num_exp, sizeof(num_exp)))
      != sizeof(num_exp)) {
    throw runtime_error("read error in timerfd");
  }

  return narrow_cast<unsigned int>(num_exp);
}
