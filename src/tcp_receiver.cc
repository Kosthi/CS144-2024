#include "tcp_receiver.hh"

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{
  if ( message.RST ) {
    reader().set_error();
    return;
  }

  if ( !zero_point_ ) {
    if ( !message.SYN ) {
      return;
    }
    zero_point_ = message.seqno;
  }

  uint64_t ab_seq = message.seqno.unwrap( zero_point_.value(), writer().bytes_pushed() );
  // if SYN repeated, first_index should be 0.
  reassembler_.insert( message.SYN ? 0 : ab_seq - 1, move( message.payload ), message.FIN );
}

TCPReceiverMessage TCPReceiver::send() const
{
  if ( reassembler_.reader().has_error() ) {
    return { nullopt, 0, true };
  }
  TCPReceiverMessage message;
  message.window_size = writer().available_capacity() >= UINT16_MAX ? UINT16_MAX : writer().available_capacity();
  if ( zero_point_.has_value() ) {
    message.ackno = Wrap32::wrap( writer().bytes_pushed() + writer().is_closed() + 1, zero_point_.value() );
  }
  return message;
}
