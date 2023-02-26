#include <iostream>
#include <vector>

#include "yuv4mpeg.hh"
#include "exception.hh"
#include "conversion.hh"
#include "split.hh"

using namespace std;

YUV4MPEG::YUV4MPEG(const string & video_file_path,
                   const uint16_t display_width,
                   const uint16_t display_height,
                   const bool loop)
  : fd_(check_syscall(open(video_file_path.c_str(), O_RDONLY))),
    display_width_(display_width),
    display_height_(display_height),
    loop_(loop)
{
  const string y4m_signature = "YUV4MPEG2";
  if (fd_.readn(y4m_signature.size()) != y4m_signature) {
    throw runtime_error("invalid YUV4MPEG2 file signature");
  }

  const string & header = fd_.getline();
  const vector<string> & tokens = split(header, " ");

  for (const auto & token : tokens) {
    if (token.empty()) {
      continue;
    }

    switch (token[0]) {
      case 'W': // width
        if (strict_stoi(token.substr(1)) != display_width) {
          throw runtime_error("wrong YUV4MPEG2 frame width");
        }
        break;

      case 'H': // height
        if (strict_stoi(token.substr(1)) != display_height) {
          throw runtime_error("wrong YUV4MPEG2 frame height");
        }
        break;

      case 'C': // color space
        if (token.substr(0, 4) != "C420") {
          throw runtime_error("only YUV420 color space is supported");
        }
        break;

      default:
        break;
    }
  }
}

bool YUV4MPEG::read_frame(RawImage & raw_img)
{
  if (raw_img.display_width() != display_width_ or
      raw_img.display_height() != display_height_) {
    throw runtime_error("YUV4MPEG: image dimensions don't match");
  }

  string frame_header = fd_.getline();

  if (fd_.eof() and frame_header.empty()) {
    if (loop_) {
      // reset the file offset to the beginning and skip the header line
      fd_.reset_offset();
      fd_.getline();

      // should read "FRAME" again
      frame_header = fd_.getline();
    } else {
      // cannot read past end of file if not set to the 'loop' mode
      return false;
    }
  }

  if (frame_header.substr(0, 5) != "FRAME") {
    throw runtime_error("invalid YUV4MPEG2 input format");
  }

  // read Y, U, V planes in order
  raw_img.copy_y_from(fd_.readn(y_size()));
  raw_img.copy_u_from(fd_.readn(uv_size()));
  raw_img.copy_v_from(fd_.readn(uv_size()));

  return true;
}
