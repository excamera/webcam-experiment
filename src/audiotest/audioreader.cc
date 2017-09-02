#include <unistd.h>
#include <cstdio>
#include <iostream>
#include <stdexcept>
#include <string>

#include <pulse/simple.h>
#include <pulse/error.h>

#include "file_descriptor.hh"
#include "audio.hh"

using namespace std;

#define DEVNAME "alsa_input.usb-046d_C922_Pro_Stream_Webcam_6BB339DF-02.analog-stereo"
#define BUFSIZE 1024

int main( int, char * [] )
{
  pa_sample_spec ss;
  ss.format = PA_SAMPLE_S16LE;
  ss.rate = 44100;
  ss.channels = 2;

  pa_buffer_attr ba;
  ba.maxlength = 1 << 16;
  ba.prebuf = 1024;

  AudioReader ar { DEVNAME, ss, ba };
  FileDescriptor stdout_fd { STDOUT_FILENO };

  while ( true ) {
    string data = ar.read( BUFSIZE );

    if ( data.length() > 0 ) {
      stdout_fd.write( data, true );
    }
  }

  return 0;
}
