#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ), available_capacity_( capacity ) {}

bool Writer::is_closed() const
{
  return is_closed_;
}

void Writer::push( string data )
{
  auto len = min( available_capacity_, data.size() );
  if ( len < data.size() ) {
    data = data.substr( 0, len );
  }
  if ( len > 0 ) {
    bytes_pushed_ += len;
    bytes_buffered_ += len;
    available_capacity_ -= len;
    buffer_.emplace_back( move( data ) );
    buffer_view_.emplace_back( buffer_.back() );
  }
}

void Writer::close()
{
  is_closed_ = true;
}

uint64_t Writer::available_capacity() const
{
  return available_capacity_;
}

uint64_t Writer::bytes_pushed() const
{
  return bytes_pushed_;
}

bool Reader::is_finished() const
{
  return is_closed_ && bytes_buffered_ == 0;
}

uint64_t Reader::bytes_popped() const
{
  return bytes_popped_;
}

string_view Reader::peek() const
{
  return buffer_view_.front();
}

void Reader::pop( uint64_t len )
{
  len = min( len, bytes_buffered_ );

  auto sz = len;

  while ( len > 0 ) {
    auto size = buffer_view_.front().size();
    if ( len >= size ) {
      buffer_view_.pop_front();
    } else {
      buffer_view_.front().remove_prefix( len );
      break;
    }
    len -= size;
  }

  bytes_buffered_ -= sz;
  bytes_popped_ += sz;
  available_capacity_ += sz;
}

uint64_t Reader::bytes_buffered() const
{
  return bytes_buffered_;
}
