#ifndef YUV4MPEG_HH
#define YUV4MPEG_HH

#include <string>

#include "file_descriptor.hh"
#include "video_input.hh"

class YUV4MPEG : public VideoInput
{
public:
  YUV4MPEG(const std::string & video_file_path,
           const uint16_t display_width,
           const uint16_t display_height,
           const bool loop = true);

  size_t frame_size() const { return display_width_ * display_height_ * 3 / 2; }
  size_t y_size() const { return display_width_ * display_height_; }
  size_t uv_size() const { return display_width_ * display_height_ / 4; }

  // try to fetch a video frame from video file into raw_img
  bool read_frame(RawImage & raw_img) override;

  // accessors
  FileDescriptor & fd() { return fd_; }
  uint16_t display_width() const override { return display_width_; }
  uint16_t display_height() const override { return display_height_; }

private:
  FileDescriptor fd_;
  uint16_t display_width_;
  uint16_t display_height_;

  // loop over the file infinitely
  bool loop_;
};

#endif /* YUV4MPEG_HH */
