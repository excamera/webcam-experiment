#include "audio.hh"

#include <stdexcept>

using namespace std;

AudioReader::AudioReader( const string & input_device,
                          const pa_sample_spec & ss,
                          const pa_buffer_attr & ba )
  : source_( nullptr )
{

  int error;
  source_.reset( pa_simple_new( NULL, "source", PA_STREAM_RECORD,
                                input_device.c_str(), "record", &ss, NULL, &ba,
                                &error ) );

  if ( source_ == nullptr ) {
    throw runtime_error( string( "pa_simple_new(): " ) + pa_strerror( error ) );
  }
}

string AudioReader::read( const ssize_t size )
{
  int error;

  static const size_t BUFFER_SIZE = 1048576;
  char buffer[ BUFFER_SIZE ];

  ssize_t rsize = pa_simple_read( source_.get(), buffer, size, &error );

  if ( rsize < 0 or rsize > size ) {
    throw runtime_error( string( "pa_simple_read(): " ) + pa_strerror( error ) );
  }

  return string( buffer, rsize );
}
