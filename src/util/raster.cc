/* -*-mode:c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */

#include <boost/functional/hash.hpp>
#include <cstdio>

#include "exception.hh"
#include "raster.hh"

using namespace std;

BaseRaster::BaseRaster( const uint16_t display_width, const uint16_t display_height,
  const uint16_t width, const uint16_t height)
  : display_width_( display_width ), display_height_( display_height ),
    width_( width ), height_( height )
{
  if ( display_width_ > width_ ) {
    throw Invalid( "display_width is greater than width." );
  }

  if ( display_height_ > height_ ) {
    throw Invalid( "display_height is greater than height." );
  }
}

size_t BaseRaster::raw_hash( void ) const
{
  size_t hash_val = 0;

  boost::hash_range( hash_val, Y_.begin(), Y_.end() );
  boost::hash_range( hash_val, U_.begin(), U_.end() );
  boost::hash_range( hash_val, V_.begin(), V_.end() );

  return hash_val;
}

bool BaseRaster::operator==( const BaseRaster & other ) const
{
  return (Y_ == other.Y_) and (U_ == other.U_) and (V_ == other.V_);
}

bool BaseRaster::operator!=( const BaseRaster & other ) const
{
  return not operator==( other );
}

void BaseRaster::copy_from( const BaseRaster & other )
{
  Y_.copy_from( other.Y_ );
  U_.copy_from( other.U_ );
  V_.copy_from( other.V_ );
}

vector<Chunk> BaseRaster::display_rectangle_as_planar() const
{
  vector<Chunk> ret;

  /* write Y */
  for ( uint16_t row = 0; row < display_height(); row++ ) {
    ret.emplace_back( &Y().at( 0, row ), display_width() );
  }

  /* write U */
  for ( uint16_t row = 0; row < chroma_display_height(); row++ ) {
    ret.emplace_back( &U().at( 0, row ), chroma_display_width() );
  }

  /* write V */
  for ( uint16_t row = 0; row < chroma_display_height(); row++ ) {
    ret.emplace_back( &V().at( 0, row ), chroma_display_width() );
  }

  return ret;
}

void BaseRaster::dump( FILE * file ) const
{
  for ( const auto & chunk : display_rectangle_as_planar() ) {
    if ( 1 != fwrite( chunk.buffer(), chunk.size(), 1, file ) ) {
      throw runtime_error( "fwrite returned short write" );
    }
  }
}
