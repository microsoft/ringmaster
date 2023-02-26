#ifndef VIDEO_INPUT_HH
#define VIDEO_INPUT_HH

#include "image.hh"

class VideoInput
{
public:
  VideoInput() {}
  virtual ~VideoInput() {}

  virtual uint16_t display_width() const = 0;
  virtual uint16_t display_height() const = 0;

  // read a raw frame into raw_img
  virtual bool read_frame(RawImage & raw_img) = 0;
};

#endif /* VIDEO_INPUT_HH */
