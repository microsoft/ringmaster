#include <vector>
#include <stdexcept>

#include "udp_socket.hh"
#include "exception.hh"

using namespace std;

bool UDPSocket::check_bytes_sent(const ssize_t bytes_sent,
                                 const size_t target) const
{
  if (bytes_sent <= 0) {
    if (bytes_sent == -1 and errno == EWOULDBLOCK) {
      return false; // return false to indicate EWOULDBLOCK
    }

    throw unix_error("UDPSocket:send()/sendto()");
  }

  if (static_cast<size_t>(bytes_sent) != target) {
    throw runtime_error("UDPSocket failed to deliver target number of bytes");
  }

  return true;
}

bool UDPSocket::send(const string_view data)
{
  if (data.empty()) {
    throw runtime_error("attempted to send empty data");
  }

  const ssize_t bytes_sent = ::send(fd_num(), data.data(), data.size(), 0);
  return check_bytes_sent(bytes_sent, data.size());
}

bool UDPSocket::sendto(const Address & dst_addr, const string_view data)
{
  if (data.empty()) {
    throw runtime_error("attempted to send empty data");
  }

  const ssize_t bytes_sent = ::sendto(fd_num(), data.data(), data.size(), 0,
                                      &dst_addr.sock_addr(), dst_addr.size());
  return check_bytes_sent(bytes_sent, data.size());
}

bool UDPSocket::check_bytes_received(const ssize_t bytes_received) const
{
  if (bytes_received < 0) {
    if (bytes_received == -1 and errno == EWOULDBLOCK) {
      return false; // return false to indicate EWOULDBLOCK
    }

    throw unix_error("UDPSocket:recv()/recvfrom()");
  }

  if (static_cast<size_t>(bytes_received) > UDP_MTU) {
    throw runtime_error("UDPSocket::recv()/recvfrom(): datagram truncated");
  }

  return true;
}

optional<string> UDPSocket::recv()
{
  // data to receive
  vector<char> buf(UDP_MTU);

  const ssize_t bytes_received = ::recv(fd_num(), buf.data(),
                                        UDP_MTU, MSG_TRUNC);
  if (not check_bytes_received(bytes_received)) {
    return nullopt;
  }

  return string{buf.data(), static_cast<size_t>(bytes_received)};
}

pair<Address, optional<string>> UDPSocket::recvfrom()
{
  // data to receive and its source address
  vector<char> buf(UDP_MTU);
  sockaddr src_addr;
  socklen_t src_addr_len = sizeof(src_addr);

  const ssize_t bytes_received = ::recvfrom(
      fd_num(), buf.data(), UDP_MTU, MSG_TRUNC, &src_addr, &src_addr_len);
  if (not check_bytes_received(bytes_received)) {
    return { Address{src_addr, src_addr_len}, nullopt };
  }

  return { Address{src_addr, src_addr_len},
           string{buf.data(), static_cast<size_t>(bytes_received)} };
}
