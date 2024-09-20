#ifndef FILE_DESCRIPTOR_HH
#define FILE_DESCRIPTOR_HH

// headers for commonly used system calls: open, close, etc.
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstdint>

#include <string>
#include <string_view>

class FileDescriptor
{
public:
  FileDescriptor(const int fd);
  virtual ~FileDescriptor();

  // move constructor and assignment
  FileDescriptor(FileDescriptor && other);
  FileDescriptor & operator=(FileDescriptor && other);

  // forbid copying or assigning
  FileDescriptor(const FileDescriptor & other) = delete;
  const FileDescriptor & operator=(const FileDescriptor & other) = delete;

  // accessors
  int fd_num() const { return fd_; }
  bool eof() const { return eof_; }

  // close throws exception on failure
  void close();

  // I/O mode
  bool get_blocking() const;
  void set_blocking(const bool blocking);

  // return the actual bytes written; 0 indicates EWOULDBLOCK
  size_t write(const std::string_view data);

  // read and return data up to 'limit' bytes; empty string indicates EOF
  std::string read(const size_t limit = MAX_BUF_SIZE);

  // blocking I/O only: write exactly N bytes of data
  void writen(const std::string_view data, const size_t n);

  // blocking I/O only: write all the data
  void write_all(const std::string_view data);

  // blocking I/O only: read exactly N bytes of data
  std::string readn(const size_t n, const bool allow_partial_read = false);

  // blocking I/O only: read one line of data
  std::string getline();

  // manipulate the file offset
  uint64_t seek(const int64_t offset, const int whence);
  void reset_offset();  // reset file offset to beginning and set EOF to false
  uint64_t file_size(); // get file size without modifying the file offset

protected:
  static constexpr size_t MAX_BUF_SIZE = 1024 * 1024; // 1 MB

private:
  int fd_;
  bool eof_;
};

#endif /* FILE_DESCRIPTOR_HH */
