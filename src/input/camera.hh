/* -*-mode:c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */

#ifndef CAMERA_HH
#define CAMERA_HH

#include <linux/videodev2.h>

#include <unordered_map>

#include "optional.hh"
#include "file_descriptor.hh"
#include "mmap_region.hh"
#include "raster.hh"
#include "h264_degrader.hh"

static const std::unordered_map<std::string, uint32_t> PIXEL_FORMAT_STRS {
  { "NV12", V4L2_PIX_FMT_NV12 },
  { "YUYV", V4L2_PIX_FMT_YUYV },
  { "YU12", V4L2_PIX_FMT_YUV420 },
  { "MJPEG", V4L2_PIX_FMT_MJPEG }
};

class Camera
{
private:
  uint16_t width_;
  uint16_t height_;

  FileDescriptor camera_fd_;
  std::unique_ptr<MMap_Region> mmap_region_;

  uint32_t pixel_format_;
  v4l2_buffer buffer_info_;
  int type_;

  H264_degrader degrader_;
  MJPEGDecoder mjpeg_decoder_;

  std::string before_filename;
  std::string after_filename;
  FILE* before_file;
  FILE* after_file;
  
public:
  Camera( const uint16_t width, const uint16_t height,
          const size_t bitrate, const size_t quantizer,
          const std::string before_filename,
          const std::string after_filename,
          const uint32_t pixel_format = V4L2_PIX_FMT_NV12,
          const std::string device = "/dev/video0");
       
  ~Camera();

  void get_next_frame( BaseRaster & raster );

  uint16_t display_width() { return width_; }
  uint16_t display_height() { return height_; }

  FileDescriptor & fd() { return camera_fd_; }
};

#endif
