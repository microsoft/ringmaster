#include <sys/ioctl.h>
#include <cassert>
#include <cstring>
#include <iostream>

#include "v4l2.hh"
#include "exception.hh"

using namespace std;

VideoDevice::VideoDevice(const string & video_device_path,
                         const uint16_t display_width,
                         const uint16_t display_height)
  : fd_(check_syscall(open(video_device_path.c_str(), O_RDWR))),
    display_width_(display_width),
    display_height_(display_height)
{
  // query the device about its capability
  v4l2_capability cap {};
  check_syscall(ioctl(fd_.fd_num(), VIDIOC_QUERYCAP, &cap));

  if (not (cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
    throw runtime_error("this device does not handle video capture");
  }

  if (not (cap.capabilities & V4L2_CAP_STREAMING)) {
    throw runtime_error("this device does not support streaming");
  }

  // set pixel format and resolution
  v4l2_format fmt {};
  fmt.type = buffer_type;
  fmt.fmt.pix.pixelformat = pixel_format;
  fmt.fmt.pix.width = display_width;
  fmt.fmt.pix.height = display_height;
  check_syscall(ioctl(fd_.fd_num(), VIDIOC_S_FMT, &fmt));

  if (fmt.fmt.pix.pixelformat != pixel_format) {
    throw runtime_error("cannot set pixel format to be YUYV");
  }

  if (fmt.fmt.pix.width != display_width or
      fmt.fmt.pix.height != display_height) {
    throw runtime_error("cannot set video resolution as specified");
  }

  // inform the device about the buffers about to be allocated
  v4l2_requestbuffers buf_req {};
  buf_req.type = buffer_type;
  buf_req.memory = V4L2_MEMORY_MMAP;
  buf_req.count = NUM_BUFFERS;
  check_syscall(ioctl(fd_.fd_num(), VIDIOC_REQBUFS, &buf_req));

  if (buf_req.count != NUM_BUFFERS) {
    cerr << "Warning: requested " << NUM_BUFFERS << " V4L2 buffers but only "
         << buf_req.count << " were approved" << endl;
  }

  buf_mem_.reserve(buf_req.count);
  for (size_t i = 0; i < buf_req.count; i++) {
    memset(&buf_info_, 0, sizeof(buf_info_));
    buf_info_.index = i;
    buf_info_.type = buffer_type;
    buf_info_.memory = V4L2_MEMORY_MMAP;

    // allocate buffer
    check_syscall(ioctl(fd_.fd_num(), VIDIOC_QUERYBUF, &buf_info_));

    // mmap buffer
    buf_mem_.emplace_back(
      buf_info_.length, PROT_READ | PROT_WRITE, MAP_SHARED,
      fd_.fd_num(), buf_info_.m.offset);

    // enqueue buffer
    check_syscall(ioctl(fd_.fd_num(), VIDIOC_QBUF, &buf_info_));
  }

  // activate streaming
  check_syscall(ioctl(fd_.fd_num(), VIDIOC_STREAMON, &buffer_type));
}

VideoDevice::~VideoDevice()
{
  if (ioctl(fd_.fd_num(), VIDIOC_STREAMOFF, &buffer_type) < 0) {
    cerr << "VideoDevice: unable to deactivate streaming" << endl;
  }
}

void VideoDevice::set_blocking(const bool blocking)
{
  fd_.set_blocking(blocking);
}

bool VideoDevice::read_frame(RawImage & raw_img)
{
  if (raw_img.display_width() != display_width_ or
      raw_img.display_height() != display_height_) {
    throw runtime_error("VideoDevice: image dimensions don't match");
  }

  memset(&buf_info_, 0, sizeof(buf_info_));
  buf_info_.type = buffer_type;
  buf_info_.memory = V4L2_MEMORY_MMAP;

  // try to dequeue a frame
  if (ioctl(fd_.fd_num(), VIDIOC_DQBUF, &buf_info_) < 0) {
    return false;
  }

  // convert pixel format
  const MMap & frame_buf = buf_mem_.at(buf_info_.index);

  // frame_buf.size() is actually greater than the expected YUYV size
  raw_img.copy_from_yuyv({
    reinterpret_cast<const char *>(frame_buf.addr()),
    static_cast<size_t>(display_width_ * display_height_ * 2)
  });

  // enqueue the buffer back
  check_syscall(ioctl(fd_.fd_num(), VIDIOC_QBUF, &buf_info_));

  return true;
}
