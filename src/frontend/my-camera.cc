/* -*-mode:c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */

#include <getopt.h>
#include <unistd.h>
#include <chrono>
#include <iostream>
#include <string>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <thread>
#include <memory>

#include <pulse/sample.h>

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

  constexpr option options[] = {
    { "camera",       required_argument, NULL, 'c' },
    { "fps",          required_argument, NULL, 'f' },
    { "audio-source", required_argument, NULL, 'a' },
    { "audio-sink",   required_argument, NULL, 'A' },
    { 0, 0, 0, 0 }
  };

  while ( true ) {
    const int opt = getopt_long( argc, argv, "", options, NULL );

    if ( opt == -1 ) {
      break;
    }

    switch ( opt ) {
    case 'c':
      camera_path = optarg;
      break;

    case 'f':
      fps = stoul( optarg );
      break;

    case 'a':
      audio_source = optarg;
      break;

    case 'A':
      audio_sink = optarg;
      break;

    default:
      throw runtime_error( "invalid option" );
    }
  }

  /* AUDIO STUFF */
  pa_sample_spec ss;
  ss.format = PA_SAMPLE_S16LE;
  ss.rate = 44100;
  ss.channels = 2;

  size_t audio_bytes_per_frame = pa_bytes_per_second( &ss ) / fps;

  pa_buffer_attr ba;
  ba.maxlength = 4 * audio_bytes_per_frame;
  ba.prebuf = audio_bytes_per_frame;
  ba.fragsize = audio_bytes_per_frame;

  AudioReader audio_reader { audio_source, ss, ba };

  /* CAMERA */
  Camera camera { width, height, 1 << 20, 40, V4L2_PIX_FMT_NV12, camera_path };

  /* VIDEO DISPLAY */
  BaseRaster current_video_frame { width, height, width, height };
  BaseRaster next_video_frame { width, height, width, height };

  string current_audio_frame;
  string next_audio_frame;

  mutex video_frame_mtx;
  mutex audio_frame_lock;
  condition_variable frame_cv;

  queue<string> audio_frame_queue;

  thread video_display_thread {
    [&]()
    {
      VideoDisplay display { current_video_frame };

      while ( true ) {
        unique_lock<mutex> lock { video_frame_mtx };
        frame_cv.wait( lock );
        display.draw( current_video_frame );
      }
    }
  };

  thread audio_play_thread {
    [&]()
    {
      AudioWriter audio_writer { audio_sink, ss, ba };

      while ( true ) {
        unique_lock<mutex> lock { video_frame_mtx };
        frame_cv.wait( lock );
        audio_writer.write( current_audio_frame );
      }
    }
  };

  const auto interval_between_frames = chrono::microseconds( int( 1.0e6 / fps ) );
  auto next_frame_is_due = chrono::system_clock::now();

  while ( true ) {
    this_thread::sleep_until( next_frame_is_due );
    next_frame_is_due += interval_between_frames;

    camera.get_next_frame( next_video_frame );
    next_audio_frame = audio_reader.read( audio_bytes_per_frame );

    {
      lock_guard<mutex> lg { video_frame_mtx };
      swap( next_video_frame, current_video_frame );
      swap( next_audio_frame, current_audio_frame );
      frame_cv.notify_all();
    }
  }

  return 0;
}

/*
auto get_raster_t1 = chrono::high_resolution_clock::now();
auto get_raster_t2 = chrono::high_resolution_clock::now();
auto get_raster_time = chrono::duration_cast<chrono::duration<double>>( get_raster_t2 - get_raster_t1 );
cout << "get_raster: " << get_raster_time.count() << "\n";
*/
