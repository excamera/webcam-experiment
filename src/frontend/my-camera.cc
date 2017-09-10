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

  string before_filename = "before.raw";
  string after_filename = "after.raw";
  
  constexpr option options[] = {
    { "camera",       required_argument, NULL, 'c' },
    { "fps",          required_argument, NULL, 'f' },
    { "audio-source", required_argument, NULL, 'a' },
    { "audio-sink",   required_argument, NULL, 'A' },
    { "delay",   required_argument, NULL, 'd' },
    { "beforeFile",   required_argument, NULL, 'x' },
    { "afterFile",   required_argument, NULL, 'y' },
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
  Camera camera { width, height, 1 << 20, 40, before_filename, after_filename, V4L2_PIX_FMT_MJPEG, camera_path };
  
  /* VIDEO DISPLAY */
  BaseRasterQueue video_frames { delay, width, height, width, height };
  list<string> audio_frames {};
  
  atomic<size_t> video_frame_count(0);
  atomic<size_t> audio_frame_count(0);

  mutex video_mtx;
  condition_variable video_cv;

  mutex audio_mtx;
  condition_variable audio_cv;
  
  thread video_read_thread {
    [&]()
      {
        while(true) {      
          unique_lock<mutex> ul { video_mtx };            
          video_cv.wait(ul, [&](){ return video_frames.occupancy < delay; });
          
          BaseRaster &r = video_frames.back();
          camera.get_next_frame( r );
          video_cv.notify_all();
        }
      }
  };
  
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
            
            display.draw( r );

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
          
          audio_writer.write( audio_frame );

        }
      }
  };
  
  while ( true ) { }

  return 0;
}
