#ifndef TCP_SOCKET_HH
#define TCP_SOCKET_HH

#include <string>
#include <string_view>

#include "socket.hh"

class TCPSocket : public Socket
{
public:
  // constructor
  TCPSocket() : Socket(AF_INET, SOCK_STREAM) {};

  // listen for incoming connections
  void listen(const int backlog = 16);

  // accept a new incoming connection
  TCPSocket accept();

  // return the actual bytes sent; 0 indicates EWOULDBLOCK
  inline size_t send(const std::string_view data) { return write(data); }

  // receive and return data up to 'limit' bytes; empty string indicates EOF
  inline std::string recv(const size_t limit = MAX_BUF_SIZE) { return read(limit); }

  // blocking I/O mode only: send exactly N bytes or all of data
  inline void sendn(const std::string_view data, const size_t n) { writen(data, n); }
  inline void send_all(const std::string_view data) { write_all(data); }

  // blocking I/O mode only: *attempt* to receive N bytes of data
  inline std::string recvn(const size_t n, const bool allow_partial_read = true)
    { return readn(n, allow_partial_read); }

private:
  // internal constructor from file descriptor (used by accept())
  TCPSocket(FileDescriptor && fd)
    : Socket(std::move(fd), AF_INET, SOCK_STREAM) {};
};

#endif /* TCP_SOCKET_HH */
