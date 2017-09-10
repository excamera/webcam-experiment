/* -*-mode:c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */

#include <atomic>
#include <getopt.h>
#include <unistd.h>
#include <chrono>
#include <iostream>
#include <list>
#include <string>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <thread>
#include <memory>

#include <pulse/sample.h>

#include "h264_degrader.hh"
#include "raster.hh"
#include "display.hh"
#include "camera.hh"
#include "audio.hh"

using namespace std;

int main( int argc, char * argv[] )
{
  const uint16_t width = 1280;
  const uint16_t height = 720;

  string camera_path { "/dev/video0" };
  string audio_source = "";
  string audio_sink = "";
  unsigned int fps = 30;
  size_t delay = 1;
  size_t quantizer = 24;

  string before_filename = "before.y4m";
  string after_filename = "after.y4m";

  constexpr option options[] = {
    { "camera",       required_argument, NULL, 'c' },
    { "fps",          required_argument, NULL, 'f' },
    { "audio-source", required_argument, NULL, 'a' },
    { "audio-sink",   required_argument, NULL, 'A' },
    { "delay",        required_argument, NULL, 'd' },
    { "before-file",   required_argument, NULL, 'x' },
    { "after-file",    required_argument, NULL, 'y' },
    { "quantizer",    required_argument, NULL, 'q' },
    { 0, 0, 0, 0 }
  };

  while ( true ) {
    const int opt = getopt_long( argc, argv, "", options, NULL );

    if ( opt == -1 ) {
      break;
    }

    switch ( opt ) {
    case 'c': camera_path = optarg; break;
    case 'f': fps = stoul( optarg ); break;
    case 'a': audio_source = optarg; break;
    case 'A': audio_sink = optarg; break;
    case 'd': delay = stoul( optarg ); break;
    case 'x': before_filename = optarg; break;
    case 'y': after_filename = optarg; break;
    case 'q': quantizer = stoul( optarg ); break;

    default: throw runtime_error( "invalid option" );
    }
  }

  /* AUDIO STUFF */
  pa_sample_spec ss;
  ss.format = PA_SAMPLE_S16LE;
  ss.rate = 44100;
  ss.channels = 2;

  size_t audio_bytes_per_frame = pa_bytes_per_second( &ss ) / fps;
  cout << audio_bytes_per_frame << endl;

  pa_buffer_attr ba;
  ba.maxlength = 4 * audio_bytes_per_frame;
  ba.prebuf = audio_bytes_per_frame;
  ba.fragsize = audio_bytes_per_frame;

  AudioReader audio_reader { audio_source, ss, ba };

  /* CAMERA */
  Camera camera { width, height, 1 << 20, 40, V4L2_PIX_FMT_MJPEG, camera_path };

  /* DEGRADER */
  H264_degrader degrader { width, height, 1 << 20, quantizer };

  /* VIDEO DISPLAY */
  BaseRasterQueue video_frames { delay, width, height, width, height };
  list<string> audio_frames {};

  atomic<size_t> video_frame_count(0);
  atomic<size_t> audio_frame_count(0);

  mutex video_mtx;
  condition_variable video_cv;

  mutex audio_mtx;
  condition_variable audio_cv;

  const string yuv4mpeg_header = "YUV4MPEG2 W1280 H720 F24:1 Ip A0:0 C420\n";
  const string frame_header = "FRAME\n";

  unique_ptr<FILE, decltype( &fclose )> foriginal { fopen( before_filename.c_str(), "wb" ), fclose };
  unique_ptr<FILE, decltype( &fclose )> fdegraded { fopen( after_filename.c_str(), "wb" ),  fclose };

  fwrite( yuv4mpeg_header.c_str(), sizeof( char ), yuv4mpeg_header.size(), foriginal.get() );
  fwrite( yuv4mpeg_header.c_str(), sizeof( char ), yuv4mpeg_header.size(), fdegraded.get() );

  if ( foriginal.get() == nullptr or fdegraded.get() == nullptr ) {
    throw runtime_error( "could not open before or after file" );
  }

  thread video_read_thread {
    [&]()
      {
        while(true) {
          unique_lock<mutex> ul { video_mtx };
          video_cv.wait(ul, [&](){ return video_frames.occupancy < delay; });

          BaseRaster &r = video_frames.back();
          camera.get_next_frame( r );
          video_cv.notify_all();

          fwrite( frame_header.c_str(), sizeof( char ), frame_header.size(), foriginal.get() );
          r.dump( foriginal.get() );
        }
      }
  };

  bool first_degraded_frame = true;

  thread video_play_thread {
    [&]()
      {
        BaseRaster v { width, height, width, height };
        VideoDisplay display { v };

        while ( true ) {
          unique_lock<mutex> ul { video_mtx };
          video_cv.wait(ul, [&](){ return video_frames.occupancy >= delay; });

          if(video_frames.occupancy >= delay){
            BaseRaster &r = video_frames.front();
            ul.unlock();
            video_frame_count.fetch_add(1);
            video_cv.notify_all();

            // TODO(sadjad) call degrader...
            memcpy( degrader.encoder_frame->data[0], &r.Y().at( 0, 0 ), width * height );
            memcpy( degrader.encoder_frame->data[1], &r.U().at( 0, 0 ), width * height / 4 );
            memcpy( degrader.encoder_frame->data[2], &r.V().at( 0, 0 ), width * height / 4 );

            degrader.degrade( degrader.encoder_frame, degrader.decoder_frame );

            memcpy( &r.Y().at( 0, 0 ), degrader.decoder_frame->data[0], width * height );
            memcpy( &r.U().at( 0, 0 ), degrader.decoder_frame->data[1], width * height / 4 );
            memcpy( &r.V().at( 0, 0 ), degrader.decoder_frame->data[2], width * height / 4 );

            while(video_frame_count.load() > audio_frame_count.load()){}
            display.draw( r );

            if ( not first_degraded_frame ) {
              fwrite( frame_header.c_str(), sizeof( char ), frame_header.size(), fdegraded.get() );
              r.dump( fdegraded.get() );
            }
            else {
              first_degraded_frame = false;
            }

            ul.lock();
            video_frames.pop_front();
            ul.unlock();
          }
          video_cv.notify_all();
        }
      }
  };

  thread audio_read_thread {
    [&]()
    {
      while ( true ) {
        string audio_frame = audio_reader.read( audio_bytes_per_frame );

        unique_lock<mutex> ul { audio_mtx };
        audio_cv.wait(ul, [&](){ return audio_frames.size() < delay; } );
        audio_frames.push_back(audio_frame);
        audio_cv.notify_all();
      }
    }
  };

  thread audio_play_thread {
    [&]()
      {
        AudioWriter audio_writer { audio_sink, ss, ba };

        while ( true ) {
          string audio_frame;
          {
            unique_lock<mutex> ul { audio_mtx };
            audio_cv.wait(ul, [&](){ return audio_frames.size() >= delay; });

            audio_frame = audio_frames.front();
            audio_frames.pop_front();
            audio_frame_count.fetch_add(1);
            audio_cv.notify_all();
          }

          while(audio_frame_count.load() > video_frame_count.load()){}
          audio_writer.write( audio_frame );

        }
      }
  };

  while ( true ) { }

  return 0;
}
