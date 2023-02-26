#include <fcntl.h>

#include "socket.hh"
#include "exception.hh"

using namespace std;

Socket::Socket(const int domain, const int type)
  : FileDescriptor(check_syscall(socket(domain, type, 0)))
{}

Socket::Socket(FileDescriptor && fd, const int domain, const int type)
  : FileDescriptor(move(fd))
{
  int actual_value;
  socklen_t len;

  // verify domain and type
  len = getsockopt(SOL_SOCKET, SO_DOMAIN, actual_value);
  if (len != sizeof(actual_value) or actual_value != domain) {
    throw runtime_error("socket domain mismatch");
  }

  len = getsockopt(SOL_SOCKET, SO_TYPE, actual_value);
  if (len != sizeof(actual_value) or actual_value != type) {
    throw runtime_error("socket type mismatch");
  }
}

template<typename OptionType>
socklen_t Socket::getsockopt(const int level, const int option_name,
                             OptionType & option_value) const
{
  socklen_t option_len = sizeof(option_value);
  check_syscall(::getsockopt(fd_num(), level, option_name,
                             &option_value, &option_len));
  return option_len;
}

template<typename OptionType>
void Socket::setsockopt(const int level, const int option_name,
                        const OptionType & option_value)
{
  check_syscall(::setsockopt(fd_num(), level, option_name,
                             &option_value, sizeof(option_value)));
}

void Socket::bind(const Address & local_addr)
{
  check_syscall(::bind(fd_num(), &local_addr.sock_addr(), local_addr.size()));
}

void Socket::connect(const Address & peer_addr)
{
  check_syscall(::connect(fd_num(), &peer_addr.sock_addr(), peer_addr.size()));
}

Address Socket::local_address() const
{
  sockaddr addr;
  socklen_t size = sizeof(addr);

  check_syscall(getsockname(fd_num(), &addr, &size));
  return {addr, size};
}

Address Socket::peer_address() const
{
  sockaddr addr;
  socklen_t size = sizeof(addr);

  check_syscall(getpeername(fd_num(), &addr, &size));
  return {addr, size};
}

void Socket::set_reuseaddr()
{
  setsockopt(SOL_SOCKET, SO_REUSEADDR, int(true));
}
