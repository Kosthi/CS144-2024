#include "wrapping_integers.hh"

using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  // return zero_point + n;
  return Wrap32 { static_cast<uint32_t>( n + zero_point.raw_value_ ) }; // faster, reduce function calls
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  constexpr static uint64_t TWO31 = 1UL << 31;
  constexpr static uint64_t TWO32 = 1UL << 32;
  // version1 same offset 0.85s 4lines
  uint64_t dis = raw_value_ - static_cast<uint32_t>( ( checkpoint + zero_point.raw_value_ ) );
  return dis <= TWO31 || checkpoint + dis < TWO32 ? checkpoint + dis : checkpoint + dis - TWO32;

  // version2 cal 0.88s 8lines
  //  uint64_t i = raw_value_ - zero_point.raw_value_;
  //  if ( i < checkpoint ) {
  //    i += ( checkpoint - i ) / ( TWO32 ) * ( TWO32 );
  //    // 如果checkpoint正好在中点上，取左值
  //    return checkpoint <= i + ( TWO31 ) ? i : i + ( TWO32 );
  //  }
  //  return static_cast<uint32_t>( i );
}
