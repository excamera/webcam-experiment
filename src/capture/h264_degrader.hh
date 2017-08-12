#pragma once
//#ifndef __H264_DEGRADER_HH__
//#define __H264_DEGRADER_HH__

extern "C" {
#include <libswscale/swscale.h>
#include "libavcodec/avcodec.h"
#include "libavutil/frame.h"
}

#include <mutex>
#include "raster.hh"

class H264_degrader{
public:

    AVFrame *encoder_frame;
    AVFrame *decoder_frame;

    H264_degrader(size_t _width, size_t _height, size_t _bitrate, size_t quantization);
    ~H264_degrader();

    void bgra2yuv422p(uint8_t* input, AVFrame* outputFrame, size_t width, size_t height);
    void yuv422p2bgra(AVFrame* inputFrame, uint8_t* output, size_t width, size_t height);

    void yuyv2yuv420p(uint8_t * input, AVFrame * outputFrame, size_t width, size_t height);
    //void yuv422p2bgra(AVFrame* inputFrame, uint8_t* output, size_t width, size_t height);

    void degrade(AVFrame *inputFrame, AVFrame *outputFrame);

private:
    std::mutex degrader_mutex;

    const AVCodecID codec_id = AV_CODEC_ID_H264;
    const AVPixelFormat pix_fmt = AV_PIX_FMT_YUV422P;

    const size_t width;
    const size_t height;
    const size_t bitrate;
    const size_t quantization;

    size_t frame_count;

    AVCodec *encoder_codec;
    AVCodec *decoder_codec;

    AVCodecContext *encoder_context;
    AVCodecContext *decoder_context;

    AVCodecParserContext *decoder_parser;

    AVPacket *encoder_packet;
    AVPacket *decoder_packet;

    SwsContext *bgra2yuv422p_context;
    SwsContext *yuv422p2bgra_context;

    SwsContext *yuyv2yuv420p_context;
};

class MJPEGDecoder
{
public:
  AVFrame * frame;
  AVFrame * out_frame;
  MJPEGDecoder( const size_t width, const size_t height );
  ~MJPEGDecoder();

  void decode( uint8_t * data, size_t data_size, AVFrame * output );

private:
  const AVCodecID codec_id = AV_CODEC_ID_MJPEG;
  const AVPixelFormat pix_fmt = AV_PIX_FMT_YUV420P;

  const size_t width;
  const size_t height;

  AVCodec * codec;
  AVCodecContext * context;
  AVCodecParserContext *parser;
  AVPacket * packet;

  SwsContext * yuvj422p2yuv420p_context;
  void yuvj422p2yuv420p(AVFrame* inputFrame, AVFrame * output);
};

//#endif
