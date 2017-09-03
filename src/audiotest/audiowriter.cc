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

#define DEVNAME "alsa_output.pci-0000_00_1b.0.analog-stereo"
#define BUFSIZE 1024

int main( int, char * [] )
{
  pa_sample_spec ss;
  ss.format = PA_SAMPLE_S16LE;
  ss.rate = 44100;
  ss.channels = 2;

  pa_buffer_attr ba;
  ba.maxlength = 1 << 24;
  ba.prebuf = 1024;

  AudioWriter aw { DEVNAME, ss, ba };
  FileDescriptor stdin_fd { STDIN_FILENO };

  while ( true ) {
    string data = stdin_fd.read( BUFSIZE );
    aw.write( data );
  }

  return 0;
}
