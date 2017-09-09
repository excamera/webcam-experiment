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


BaseRasterQueue::BaseRasterQueue(const size_t capacity,
                                  const uint16_t display_width,
                                  const uint16_t display_height,
                                  const uint16_t width,
                                  const uint16_t height ) :
  _capacity(capacity),
  _occupancy(0),
  back_idx(0),
  front_idx(0),
  capacity(_capacity),
  occupancy(_occupancy),
  array()
{
  
  for(size_t i = 0; i < capacity; i++){
    BaseRaster b{display_width, display_height, width, height};
    array.emplace_back(std::move(b));
  }
}
  
BaseRasterQueue::~BaseRasterQueue()
{}

BaseRaster& BaseRasterQueue::front() {
  return array.at(front_idx);
}

void BaseRasterQueue::pop_front() {
  if(occupancy > 0){
    front_idx = (front_idx + 1) % capacity;
    --_occupancy;
  }
  else{
    throw runtime_error("Queue is empty!");
  }
}

BaseRaster& BaseRasterQueue::back() {
  if(occupancy < capacity){
    BaseRaster &b = array.at(back_idx);
    back_idx = (back_idx + 1) % capacity;
    ++_occupancy;
    return b;
  }
  else{
    throw runtime_error("Queue is full!");
  }
}  
