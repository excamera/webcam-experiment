/* -*-mode:c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */

#include <chrono>
#include <iostream>
#include <string>
#include <unistd.h>

#include <mutex>
#include <list>
#include <thread>
#include <memory>

#include "raster.hh"
#include "display.hh"
#include "camera.hh"

using namespace std;

int main( int argc, char const * argv[] )
{
  const uint16_t width = 1280;
  const uint16_t height = 720;

  if ( argc != 2 ) {
    cerr << "usage: " << argv[ 0 ] << " camera" << endl;
  }

  Camera camera( width, height, 1 << 20, 24, V4L2_PIX_FMT_MJPEG, argv[ 1 ] );

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

  while ( true ) {
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
