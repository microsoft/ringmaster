#ifndef MMAP_HH
#define MMAP_HH

#include <sys/mman.h>

class MMap
{
public:
  MMap(const size_t length, const int prot, const int flags,
       const int fd, const off_t offset);
  ~MMap();

  // move constructor and assignment
  MMap(MMap && other);
  MMap & operator=(MMap && other);

  // forbid copying or assigning
  MMap(const MMap & other) = delete;
  const MMap & operator=(const MMap & other) = delete;

  // accessors
  uint8_t * addr() const { return addr_; }
  size_t length() const { return length_; }

private:
  uint8_t * addr_;
  size_t length_;
};

#endif /* MMAP_HH */
