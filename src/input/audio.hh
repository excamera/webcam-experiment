#ifndef AUDIO_HH
#define AUDIO_HH

#include <string>
#include <memory>

#include <pulse/simple.h>
#include <pulse/error.h>

struct PADeleter
{
  void operator()( pa_simple * pa ) { if ( pa ) { pa_simple_free( pa ); } }
};

class AudioReader
{
private:
  std::unique_ptr<pa_simple, PADeleter> source_;

public:
  AudioReader( const std::string & input_device,
               const pa_sample_spec & ss,
               const pa_buffer_attr & ba );

  std::string read( const ssize_t buffer_size );
};

class AudioWriter
{
private:
  std::unique_ptr<pa_simple, PADeleter> sink_;

public:
  AudioWriter( const std::string & output_device,
               const pa_sample_spec & ss,
               const pa_buffer_attr & ba );

  void write( const std::string & data );
};

#endif /* AUDIO_HH */
