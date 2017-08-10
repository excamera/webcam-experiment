#include <iostream>
#include <string>
#include <unistd.h>

#include "raster.hh"
#include "display.hh"
#include "camera.hh"

using namespace std;

int main( int argc, char const * argv[] )
{
  const uint16_t width = 1280;
  const uint16_t height = 720;

  BaseRaster raster( width, height, width, height );
  VideoDisplay display( raster );
  Camera camera( width, height, V4L2_PIX_FMT_MJPEG, "/dev/video1" );

  while ( true ) {
    camera.get_next_frame( raster );
    display.draw( raster );
  }
  return 0;
}
