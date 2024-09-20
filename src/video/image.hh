#ifndef IMAGE_HH
#define IMAGE_HH

extern "C" {
#include <vpx/vpx_image.h>
}

#include <cstdint>
#include <string_view>

// wrapper class for vpx_image of format I420
class RawImage
{
public:
  // allocate a new vpx_image and own it
  RawImage(const uint16_t display_width, const uint16_t display_height);

  // hold a non-owning pointer to an existing vpx_image
  RawImage(vpx_image_t * const vpx_img);

  // free the vpx_image only if the class owns it
  ~RawImage();

  // return the underlying vpx_image
  vpx_image * get_vpx_image() const { return vpx_img_; }

  // image dimensions
  uint16_t display_width() const { return display_width_; }
  uint16_t display_height() const { return display_height_; }

  // plane size
  size_t y_size() const { return display_width_ * display_height_; }
  size_t uv_size() const { return display_width_ * display_height_ / 4; }

  // pointers to the top left pixel for each plane
  uint8_t * y_plane() const { return vpx_img_->planes[VPX_PLANE_Y]; }
  uint8_t * u_plane() const { return vpx_img_->planes[VPX_PLANE_U]; }
  uint8_t * v_plane() const { return vpx_img_->planes[VPX_PLANE_V]; }

  // stride between rows for each plane
  int y_stride() const { return vpx_img_->stride[VPX_PLANE_Y]; }
  int u_stride() const { return vpx_img_->stride[VPX_PLANE_U]; }
  int v_stride() const { return vpx_img_->stride[VPX_PLANE_V]; }

  // copy image data from a YUYV-formatted buffer
  void copy_from_yuyv(const std::string_view src);

  // copy plane data from a buffer
  void copy_y_from(const std::string_view src);
  void copy_u_from(const std::string_view src);
  void copy_v_from(const std::string_view src);

  // forbid copy and move operators
  RawImage(const RawImage & other) = delete;
  const RawImage & operator=(const RawImage & other) = delete;
  RawImage(RawImage && other) = delete;
  RawImage & operator=(RawImage && other) = delete;

private:
  // underlying vpx_image
  vpx_image * vpx_img_;

  // if the class owns the vpx_image
  bool own_vpx_img_;

  // image display dimensions
  uint16_t display_width_;
  uint16_t display_height_;
};

#endif /* IMAGE_HH */
