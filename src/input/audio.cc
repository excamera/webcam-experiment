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
                                input_device.length() ? input_device.c_str() : nullptr,
                                "record", &ss, NULL, &ba,
                                &error ) );

  if ( source_ == nullptr ) {
    throw runtime_error( string( "pa_simple_new(): " ) + pa_strerror( error ) );
  }
}

string AudioReader::read( const ssize_t size )
{
  int error;

  static constexpr size_t BUFFER_SIZE = 1048576;
  char buffer[ BUFFER_SIZE ] = {0};

  if ( pa_simple_read( source_.get(), buffer, size, &error ) < 0 ) {
    throw runtime_error( string( "pa_simple_read(): " ) + pa_strerror( error ) );
  }

  return string( buffer, size );
}

AudioWriter::AudioWriter( const string & output_device,
                          const pa_sample_spec & ss,
                          const pa_buffer_attr & ba )
  : sink_( nullptr )
{
  int error;
  sink_.reset( pa_simple_new( NULL, "sink", PA_STREAM_PLAYBACK,
                              output_device.length() ? output_device.c_str() : nullptr,
                              "playback", &ss, NULL, &ba,
                              &error ) );

  if ( sink_ == nullptr ) {
    throw runtime_error( string( "pa_simple_new(): " ) + pa_strerror( error ) );
  }
}

void AudioWriter::write( const string & data )
{
  int error;
  if ( pa_simple_write( sink_.get(), data.data(), data.size(), &error ) < 0 ) {
    throw runtime_error( string( "pa_simple_write(): " ) + pa_strerror(error) );
  }
}
