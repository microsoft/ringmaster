#include <iostream>

#include "mmap.hh"
#include "exception.hh"

using namespace std;

MMap::MMap(const size_t length, const int prot, const int flags,
           const int fd, const off_t offset)
  : addr_(static_cast<uint8_t *>(mmap(nullptr, length, prot, flags, fd, offset))),
    length_(length)
{
  if (addr_ == MAP_FAILED) {
    throw runtime_error("mmap failed");
  }
}

MMap::~MMap()
{
  if (addr_ and length_ > 0 and munmap(addr_, length_) != 0) {
    cerr << "munmap error" << endl;
  }
}

MMap::MMap(MMap && other)
  : addr_(other.addr_), length_(other.length_)
{
  other.addr_ = nullptr;
  other.length_ = 0;
}

MMap & MMap::operator=(MMap && other)
{
  addr_ = other.addr_;
  length_ = other.length_;

  other.addr_ = nullptr;
  other.length_ = 0;

  return *this;
}
