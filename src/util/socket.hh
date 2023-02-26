#ifndef SOCKET_HH
#define SOCKET_HH

#include <sys/types.h>
#include <sys/socket.h>

#include "address.hh"
#include "file_descriptor.hh"

class Socket : public FileDescriptor
{
protected:
  // constructors
  Socket(const int domain, const int type);
  Socket(FileDescriptor && fd, const int domain, const int type);

  // manipulate socket options
  template<typename OptionType>
  socklen_t getsockopt(const int level, const int option_name,
                       OptionType & option_value) const;

  template<typename OptionType>
  void setsockopt(const int level, const int option_name,
                  const OptionType & option_value);

public:
  // bind socket to a local address
  void bind(const Address & local_addr);

  // connect socket to a peer address
  void connect(const Address & peer_addr);

  // get local and peer addresses
  Address local_address() const;
  Address peer_address() const;

  // allow local address to be reused sooner
  void set_reuseaddr();
};

#endif /* SOCKET_HH */
