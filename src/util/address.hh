#ifndef ADDRESS_HH
#define ADDRESS_HH

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <string>
#include <utility>

class Address
{
public:
  // constructors
  Address(const std::string & ip, const uint16_t port);
  Address(const sockaddr & addr, const socklen_t size);

  // accessors
  const sockaddr & sock_addr() const { return addr_; }
  socklen_t size() const { return size_; }

  // get IP and port
  std::pair<std::string, uint16_t> ip_port() const;
  std::string ip() const { return ip_port().first; }
  uint16_t port() const { return ip_port().second; }
  std::string str() const;

  // comparison operator
  bool operator==(const Address & other) const;

private:
  sockaddr addr_ {};
  socklen_t size_ {};
};

#endif /* ADDRESS_HH */
