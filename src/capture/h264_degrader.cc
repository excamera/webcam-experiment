#include <iostream>
#include <memory>
#include <cstring>
#include <vector>
#include <mutex>
#include <chrono>
#include "h264_degrader.hh"
#include "raster.hh"

extern "C" {
//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
#include <libswscale/swscale.h>
#include "libavcodec/avcodec.h"
#include "libavutil/opt.h"
#include "libavutil/frame.h"
#include "libavutil/imgutils.h"
#include "libavutil/common.h"
#include "libavutil/mathematics.h"
}

void H264_degrader::bgra2yuv422p(uint8_t* input, AVFrame* outputFrame, size_t width, size_t height){
  //std::lock_guard<std::mutex> guard(degrader_mutex);
  uint8_t * inData[1] = { input };
  int inLinesize[1] = { 4*width };

  sws_scale(bgra2yuv422p_context, inData, inLinesize, 0, height, outputFrame->data, outputFrame->linesize);
}

void H264_degrader::yuv422p2bgra(AVFrame* inputFrame, uint8_t* output, size_t width, size_t height){
  //std::lock_guard<std::mutex> guard(degrader_mutex);
  uint8_t * inData[3] = { inputFrame->data[0], inputFrame->data[1], inputFrame->data[2] };
  int inLinesize[3] = { width, width/2, width/2 };
  uint8_t * outputArray[1] = { output };
  int outLinesize[1] = { 4*width };

  sws_scale(yuv422p2bgra_context, inData, inLinesize, 0, height, outputArray, outLinesize);
}

void H264_degrader::yuyv2yuv420p(uint8_t * input, AVFrame * outputFrame, size_t width, size_t height)
{
  uint8_t * inData[1] = { input };
  int inLinesize[1] = { 2*width };

  sws_scale(yuyv2yuv420p_context, inData, inLinesize, 0, height, outputFrame->data, outputFrame->linesize);
}

H264_degrader::H264_degrader(size_t _width, size_t _height, size_t _bitrate, size_t quantization) :
    width(_width),
    height(_height),
    bitrate(_bitrate),
    frame_count(0),
    quantization(quantization)
{

    avcodec_register_all();

    encoder_codec = avcodec_find_encoder(codec_id);
    if(encoder_codec == NULL){
        std::cout << "encoder_codec: " << codec_id << " not found!" << "\n";
        throw;
    }

    decoder_codec = avcodec_find_decoder(codec_id);
    if(decoder_codec == NULL){
        std::cout << "decoder_codec: " << codec_id << " not found!" << "\n";
        throw;
    }

    encoder_context = avcodec_alloc_context3(encoder_codec);
    if(encoder_context == NULL){
        std::cout << "encoder_context: " << codec_id << " not found!" << "\n";
        throw;
    }

    decoder_context = avcodec_alloc_context3(decoder_codec);
    if(decoder_context == NULL){
        std::cout << "decoder_context: " << codec_id << " not found!" << "\n";
        throw;
    }

    // encoder context parameter
    encoder_context->pix_fmt = pix_fmt;
    encoder_context->width = width;
    encoder_context->height = height;

    encoder_context->bit_rate = bitrate;
    encoder_context->bit_rate_tolerance = 0;

    encoder_context->time_base = (AVRational){1, 20};
    encoder_context->framerate = (AVRational){60, 1};
    encoder_context->gop_size = 0;
    encoder_context->max_b_frames = 0;
    encoder_context->qmin = quantization;
    encoder_context->qmax = quantization;
    encoder_context->qcompress = 0.5;
    av_opt_set(encoder_context->priv_data, "tune", "zerolatency", 0); // forces no frame buffer delay (https://stackoverflow.com/questions/10155099/c-ffmpeg-h264-creating-zero-delay-stream)
    av_opt_set(encoder_context->priv_data, "preset", "veryfast", 0);

    // decoder context parameter
    decoder_context->pix_fmt = pix_fmt;
    decoder_context->width = width;
    decoder_context->height = height;

    decoder_context->bit_rate = encoder_context->bit_rate;
    encoder_context->bit_rate_tolerance = encoder_context->bit_rate_tolerance;

    decoder_context->time_base = encoder_context->time_base;
    decoder_context->framerate = encoder_context->framerate;
    decoder_context->gop_size = encoder_context->gop_size;
    decoder_context->max_b_frames = encoder_context->max_b_frames;
    decoder_context->qmin = encoder_context->qmin;
    decoder_context->qmax = encoder_context->qmax;
    decoder_context->qcompress = encoder_context->qcompress;

    if(avcodec_open2(encoder_context, encoder_codec, NULL) < 0){
        std::cout << "could not open encoder" << "\n";;
        throw;
    }

    if(avcodec_open2(decoder_context, decoder_codec, NULL) < 0){
        std::cout << "could not open decoder" << "\n";;
        throw;
    }

    decoder_parser = av_parser_init(decoder_codec->id);
    if(decoder_parser == NULL){
        std::cout << "Decoder parser could not be initialized" << "\n";
        throw;
    }

    encoder_frame = av_frame_alloc();
    if(encoder_frame == NULL) {
        std::cout << "AVFrame not allocated: encoder" << "\n";
        throw;
    }

    decoder_frame = av_frame_alloc();
    if(decoder_frame == NULL) {
        std::cout << "AVFrame not allocated: decoder" << "\n";
        throw;
    }

    encoder_frame->width = width;
    encoder_frame->height = height;
    encoder_frame->format = pix_fmt;
    encoder_frame->pts = 0;

    decoder_frame->width = width;
    decoder_frame->height = height;
    decoder_frame->format = pix_fmt;
    decoder_frame->pts = 0;

    if(av_frame_get_buffer(encoder_frame, 32) < 0){
        std::cout << "AVFrame could not allocate buffer: encoder" << "\n";
        throw;
    }

    if(av_frame_get_buffer(decoder_frame, 32) < 0){
        std::cout << "AVFrame could not allocate buffer: decoder" << "\n";
        throw;
    }

    encoder_packet = av_packet_alloc();
    if(encoder_packet == NULL) {
        std::cout << "AVPacket not allocated: encoder" << "\n";
        throw;
    }

    decoder_packet = av_packet_alloc();
    if(decoder_packet == NULL) {
        std::cout << "AVPacket not allocated: decoder" << "\n";
        throw;
    }

  bgra2yuv422p_context = sws_getContext(width, height,
				    AV_PIX_FMT_BGRA, width, height,
				    AV_PIX_FMT_YUV422P, 0, 0, 0, 0);

  if (bgra2yuv422p_context == NULL) {
    std::cout << "BGRA to YUV422P context not found\n";
    throw;
  }

  yuv422p2bgra_context = sws_getContext(width, height,
			    AV_PIX_FMT_YUV422P, width, height,
			    AV_PIX_FMT_BGRA, 0, 0, 0, 0);

  if (yuv422p2bgra_context == NULL) {
    std::cout << "BGRA to YUV422P context not found\n";
    throw;
  }

  yuyv2yuv420p_context = sws_getContext(width, height,
				    AV_PIX_FMT_YUYV422, width, height,
				    AV_PIX_FMT_YUV420P, 0, 0, 0, 0);

  if (yuyv2yuv420p_context == NULL) {
    std::cout << "YUYV to YUV420P context not found\n";
    throw;
  }
}

H264_degrader::~H264_degrader(){
    std::lock_guard<std::mutex> guard(degrader_mutex);

    av_parser_close(decoder_parser);

    avcodec_free_context(&decoder_context);
    avcodec_free_context(&encoder_context);

    av_frame_free(&decoder_frame);
    av_frame_free(&encoder_frame);

    av_packet_free(&decoder_packet);
    av_packet_free(&encoder_packet);

    sws_freeContext(bgra2yuv422p_context);
    sws_freeContext(yuv422p2bgra_context);
}

void H264_degrader::degrade(AVFrame *inputFrame, AVFrame *outputFrame){

    std::lock_guard<std::mutex> guard(degrader_mutex);

    bool output_set = false;


    if(av_frame_make_writable(inputFrame) < 0){
        std::cout << "Could not make the frame writable" << "\n";
        throw;
    }

    // copy frame into buffer
    // TODO(jremons) make faster
    /*for(size_t y = 0; y < height; y++){
        for(size_t x = 0; x < width; x++){
            inputFrame->data[0][y*inputFrame->linesize[0] + x] = input[0][y*width + x];
        }
    }
    for(size_t y = 0; y < height/2; y++){
        for(size_t x = 0; x < width/2; x++){
            inputFrame->data[1][y*inputFrame->linesize[1] + x] = input[1][y*width + x];
            inputFrame->data[2][y*inputFrame->linesize[2] + x] = input[2][y*width + x];
        }
    }
    */
    // encode frame
    inputFrame->pts = frame_count;
    int ret = avcodec_send_frame(encoder_context, inputFrame);
    if (ret < 0) {
        std::cout << "error sending a frame for encoding" << "\n";
        throw;
    }
    std::shared_ptr<uint8_t> buffer;
    int buffer_size = 0;
    int count = 0;
    while (ret >= 0) {
        ret = avcodec_receive_packet(encoder_context, encoder_packet);

        if (ret == AVERROR(EAGAIN)){
            break;
            std::cout << "did I make it here?\n";
        }
        else if(ret == AVERROR_EOF){
            break;
        }
        else if (ret < 0) {
            std::cout << "error during encoding" << "\n";
            throw;
        }

        // TODO(jremmons) memory allocation is slow
        buffer = std::move(std::shared_ptr<uint8_t>(new uint8_t[encoder_packet->size + AV_INPUT_BUFFER_PADDING_SIZE]));
        buffer_size = encoder_packet->size;
        std::memcpy(buffer.get(), encoder_packet->data, encoder_packet->size);

        if(count > 0){
            std::cout << "error! multiple parsing passing!" << "\n";
            throw;
        }
        count += 1;
    }
    av_packet_unref(encoder_packet);

    // decode frame
    uint8_t *data = buffer.get();
    int data_size = buffer_size;
    while(data_size > 0){
        size_t ret1 = av_parser_parse2(decoder_parser,
                                       decoder_context,
                                       &decoder_packet->data,
                                       &decoder_packet->size,
                                       data,
                                       data_size,
                                       AV_NOPTS_VALUE,
                                       AV_NOPTS_VALUE,
                                       0);

        if(ret1 < 0){
            std::cout << "error while parsing the buffer: decoding" << "\n";
            throw;
        }

        data += ret1;
        data_size -= ret1;

        if(decoder_packet->size > 0){
            if(avcodec_send_packet(decoder_context, decoder_packet) < 0){
                std::cout << "error while decoding the buffer: send_packet" << "\n";
                throw;
            }

            size_t ret2 = avcodec_receive_frame(decoder_context, outputFrame);
            if (ret2 == AVERROR(EAGAIN) || ret2 == AVERROR_EOF){
                continue;
            }
            else if (ret2 < 0) {
                std::cout << "error during decoding: receive frame" << "\n";
                throw;
            }

            // copy output in output_buffer
            // TODO(jremmons) make faster
            /*for(size_t y = 0; y < height; y++){
                for(size_t x = 0; x < width; x++){
                    output[0][y*width + x] = outputFrame->data[0][y*outputFrame->linesize[0] + x];
                }
            }
            for(size_t y = 0; y < height/2; y++){
                for(size_t x = 0; x < width/2; x++){
                    output[1][y*width + x] = outputFrame->data[1][y*outputFrame->linesize[1] + x];
                    output[2][y*width + x] = outputFrame->data[2][y*outputFrame->linesize[2] + x];
                }
            }
	    */
            output_set = true;
        }
    }
    av_packet_unref(decoder_packet);

    if(!output_set){
        std::memset(outputFrame->data[0], 255, width*height);
        std::memset(outputFrame->data[1], 128, width*height/2);
        std::memset(outputFrame->data[2], 128, width*height/2);

        // make white if output not set
        /*std::memset(output[0], 255, width*height);
        std::memset(output[1], 128, width*height/2);
        std::memset(output[2], 128, width*height/2);
	*/
    }

    frame_count += 1;
}

MJPEGDecoder::MJPEGDecoder( const size_t width, const size_t height )
  : width( width ), height( height )
{
  codec = avcodec_find_decoder( codec_id );
  if ( codec == NULL ) {
      std::cout << "codec: " << codec_id << " not found!" << "\n";
      throw;
  }

  context = avcodec_alloc_context3( codec );
  if ( context == NULL ) {
      std::cout << "context: " << codec_id << " not found!" << "\n";
      throw;
  }

  context->pix_fmt = pix_fmt;
  context->width = width;
  context->height = height;

  if ( avcodec_open2( context, codec, NULL ) < 0 ) {
      std::cout << "could not open decoder" << "\n";
      throw;
  }

  parser = av_parser_init( codec->id );
  if ( parser == NULL ) {
    std::cout << "Decoder parser could not be initialized" << "\n";
    throw;
  }

  frame = av_frame_alloc();
  if ( frame == NULL ) {
      std::cout << "AVFrame not allocated: decoder" << "\n";
      throw;
  }

  frame->width = width;
  frame->height = height;
  frame->format = AV_PIX_FMT_YUVJ422P;
  frame->pts = 0;

  if ( av_frame_get_buffer( frame, 32 ) < 0 ) {
      std::cout << "AVFrame could not allocate buffer: decoder" << "\n";
      throw;
  }

  out_frame = av_frame_alloc();
  if ( out_frame == NULL ) {
      std::cout << "AVFrame not allocated: decoder" << "\n";
      throw;
  }

  out_frame->width = width;
  out_frame->height = height;
  out_frame->format = AV_PIX_FMT_YUV420P;
  out_frame->pts = 0;

  if ( av_frame_get_buffer( out_frame, 32 ) < 0 ) {
      std::cout << "AVFrame could not allocate buffer: decoder" << "\n";
      throw;
  }

  packet = av_packet_alloc();
  if ( packet == NULL ) {
      std::cout << "AVPacket not allocated: decoder" << "\n";
      throw;
  }

  yuvj422p2yuv420p_context = sws_getContext(width, height,
					    AV_PIX_FMT_YUVJ422P, width, height,
					    AV_PIX_FMT_YUV420P, 0, 0, 0, 0);

  if ( yuvj422p2yuv420p_context == NULL) {
    std::cout << "YUYV to YUV420P context not found\n";
    throw;
  }
}

MJPEGDecoder::~MJPEGDecoder()
{
  av_parser_close( parser );
  avcodec_free_context( &context );
  av_frame_free( &frame );
  av_frame_free( &out_frame );
  av_packet_free( &packet );
  sws_freeContext( yuvj422p2yuv420p_context );
}

void MJPEGDecoder::yuvj422p2yuv420p(AVFrame* inputFrame, AVFrame * output){
  //uint8_t * odata[3] = { &output.Y().at( 0, 0 ), &output.U().at( 0, 0 ), &output.V().at( 0, 0 ) };
  //int olinesize[3] = { width, width / 2, width / 2 };
  sws_scale(yuvj422p2yuv420p_context, inputFrame->data, inputFrame->linesize, 0, height, output->data, output->linesize);
}

void MJPEGDecoder::decode( uint8_t * data, size_t data_size, AVFrame * output )
{
  bool output_set = false;
  while(data_size > 0){
      size_t ret1 = av_parser_parse2(parser,
                                     context,
                                     &packet->data,
                                     &packet->size,
                                     data,
                                     data_size,
                                     AV_NOPTS_VALUE,
                                     AV_NOPTS_VALUE,
                                     0);

      if(ret1 < 0){
          std::cout << "error while parsing the buffer: decoding" << "\n";
          throw;
      }

      data += ret1;
      data_size -= ret1;

      if(packet->size > 0){
          if(avcodec_send_packet(context, packet) < 0){
              std::cout << "error while decoding the buffer: send_packet" << "\n";
              throw;
          }

          size_t ret2 = avcodec_receive_frame(context, frame);
          if (ret2 == AVERROR(EAGAIN) || ret2 == AVERROR_EOF){
              continue;
          }
          else if (ret2 < 0) {
              std::cout << "error during decoding: receive frame" << "\n";
              throw;
          }

          output_set = true;
      }
  }
  av_packet_unref(packet);

  if ( !output_set ) {
    std::memset(frame->data[0], 255, width*height);
    std::memset(frame->data[1], 128, width*height/4);
    std::memset(frame->data[2], 128, width*height/4);
  }

  yuvj422p2yuv420p(frame, output);

  // memcpy( &output.Y().at( 0, 0 ), frame->data[0], width * height );
  //
  // for ( size_t i = 0; i < height / 2; i++ ) {
  //   memcpy( &output.U().at( 0, i ), frame->data[ 1 ] + width * i , width / 2 );
  //   memcpy( &output.V().at( 0, i ), frame->data[ 2 ] + width * i , width / 2 );
  // }
}
