/* -*-mode:c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */

#include <chrono>
#include <iostream>
#include <string>
#include <unistd.h>
#include <getopt.h>

#include <mutex>
#include <list>
#include <thread>
#include <memory>

#include "raster.hh"
#include "display.hh"
#include "camera.hh"

using namespace std;

int main( int argc, char * argv[] )
{
  const uint16_t width = 1280;
  const uint16_t height = 720;

  string camera_path { "/dev/video0" };
  double fps = 30;

  constexpr option options[] = {
    { "camera", required_argument, NULL, 'c' },
    { "fps",    required_argument, NULL, 'f' },
    { 0, 0, 0, 0 }
  };

  while ( true ) {
    const int opt = getopt_long( argc, argv, "c:f:", options, NULL );

    if ( opt == -1 ) {
      break;
    }

    switch ( opt ) {
    case 'c':
      camera_path = optarg;
      break;

    case 'f':
      fps = stod( optarg );
      break;

    default:
      throw runtime_error( "invalid option" );
    }
  }

  Camera camera( width, height, 1 << 20, 24, V4L2_PIX_FMT_MJPEG, camera_path );

  mutex q_lock;
  list<shared_ptr<BaseRaster>> q;

  thread t(
    [&]()
    {
      BaseRaster raster( width, height, width, height );
      VideoDisplay display( raster );
      shared_ptr<BaseRaster> r;

      while ( true ) {
        bool set = false;

        {
          lock_guard<mutex> lg( q_lock );

          if ( q.size() > 0 ) {
            r = q.front();
            q.pop_front();
            set = true;
          }
        }

        if ( set ) {
          auto display_raster_t1 = chrono::high_resolution_clock::now();
          display.draw( *r.get() );
          auto display_raster_t2 = chrono::high_resolution_clock::now();
          auto display_raster_time = chrono::duration_cast<chrono::duration<double>>( display_raster_t2 - display_raster_t1 );
          cout << "display_raster: " << display_raster_time.count() << "\n";
        }
      }
    }
  );

  const auto interval_between_frames = chrono::microseconds( int( 1.0e6 / fps ) );
  auto next_frame_is_due = chrono::system_clock::now();

  while ( true ) {
    this_thread::sleep_until( next_frame_is_due );
    next_frame_is_due += interval_between_frames;
    auto get_raster_t1 = chrono::high_resolution_clock::now();

    shared_ptr<BaseRaster> r = unique_ptr<BaseRaster>( new BaseRaster { width, height, width, height } );
    camera.get_next_frame( *r.get() );
    {
      lock_guard<mutex> lg( q_lock );
      if ( q.size() < 1 ) {
        q.push_back( r );
      }
    }
    auto get_raster_t2 = chrono::high_resolution_clock::now();
    auto get_raster_time = chrono::duration_cast<chrono::duration<double>>( get_raster_t2 - get_raster_t1 );
    cout << "get_raster: " << get_raster_time.count() << "\n";
    //usleep(1000);
  }

  return 0;
}
