#include <cstring>
#include <stdexcept>

#include "address.hh"

using namespace std;

Address::Address(const std::string & ip, const uint16_t port)
{
  addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;

  // resolved address
  addrinfo * result;

  const int ret_code = getaddrinfo(ip.c_str(), to_string(port).c_str(),
                                   &hints, &result);
  if (ret_code != 0) {
    throw runtime_error(gai_strerror(ret_code));
  }

  size_ = result->ai_addrlen;
  memcpy(&addr_, result->ai_addr, size_);
}

Address::Address(const sockaddr & addr, const socklen_t size)
{
  if (size > sizeof(addr_)) {
    throw runtime_error("invalid sockaddr size");
  }

  size_ = size;
  memcpy(&addr_, &addr, size_);
}

pair<string, uint16_t> Address::ip_port() const
{
  char ip[NI_MAXHOST], port[NI_MAXSERV];

  const int ret_code = getnameinfo(&addr_, size_,
                                   ip, sizeof(ip), port, sizeof(port),
                                   NI_NUMERICHOST | NI_NUMERICSERV);
  if (ret_code != 0) {
    throw runtime_error(gai_strerror(ret_code));
  }

  return {ip, stoi(port)};
}

string Address::str() const
{
  const auto & [ip, port] = ip_port();
  return ip + ":" + to_string(port);
}

bool Address::operator==(const Address & other) const
{
  return size_ == other.size_ and memcmp(&addr_, &other.addr_, size_) == 0;
}
