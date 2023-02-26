#ifndef V4L2_HH
#define V4L2_HH

extern "C" {
#include <linux/videodev2.h>
}

#include <string>
#include <vector>

#include "file_descriptor.hh"
#include "mmap.hh"
#include "video_input.hh"

class VideoDevice : public VideoInput
{
public:
  VideoDevice(const std::string & video_device_path,
              const uint16_t display_width,
              const uint16_t display_height);
  ~VideoDevice();

  // video device defaults to blocking mode
  void set_blocking(const bool blocking);

  // try to fetch a video frame from webmcam into raw_img
  bool read_frame(RawImage & raw_img) override;

  // accessors
  FileDescriptor & fd() { return fd_; }
  uint16_t display_width() const override { return display_width_; }
  uint16_t display_height() const override { return display_height_; }

private:
  FileDescriptor fd_;
  uint16_t display_width_;
  uint16_t display_height_;

  v4l2_buffer buf_info_ {}; // allocate a buffer info to reuse
  std::vector<MMap> buf_mem_ {}; // memory region for V4L2 buffers

  // constants
  static constexpr uint32_t pixel_format = V4L2_PIX_FMT_YUYV;
  static constexpr uint32_t buffer_type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  static constexpr size_t NUM_BUFFERS = 4;
};

#endif /* V4L2_HH */
