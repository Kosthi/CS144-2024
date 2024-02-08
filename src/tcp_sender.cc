#include "tcp_sender.hh"
#include "tcp_config.hh"

using namespace std;

uint64_t TCPSender::sequence_numbers_in_flight() const
{
  return sequence_numbers_in_flight_;
}

uint64_t TCPSender::consecutive_retransmissions() const
{
  return consecutive_retransmissions_;
}

void TCPSender::push( const TransmitFunction& transmit )
{
  // 1.达到最大传输字节数（窗口大小）
  // 2.仅传输中的序列号没了且window_size=0才需要发送假消息，如果还有序列号才传输中，可以利用这些得到ack更新size（传输失败就重传）
  if ( ( window_size_ && sequence_numbers_in_flight_ >= window_size_ )
       || ( window_size_ == 0 && sequence_numbers_in_flight_ >= 1 ) ) {
    return;
  }

  auto seqno = Wrap32::wrap( abs_ackno_, isn_ );

  // 限制从buffer中取出来的字节数
  auto win
    = window_size_ == 0 ? 1 : window_size_ - sequence_numbers_in_flight_ - static_cast<uint16_t>( seqno == isn_ );

  string out;
  while ( reader().bytes_buffered() and out.size() < win ) {
    auto view = reader().peek();

    if ( view.empty() ) {
      throw std::runtime_error( "Reader::peek() returned empty string_view" );
    }

    view = view.substr( 0, win - out.size() ); // Don't return more bytes than desired.
    out += view;
    input_.reader().pop( view.size() );
  }

  size_t len;
  string_view view( out );

  while ( !view.empty() || seqno == isn_ || ( !FIN_ && writer().is_closed() ) ) {
    len = min( view.size(), TCPConfig::MAX_PAYLOAD_SIZE );

    string payload( view.substr( 0, len ) );

    TCPSenderMessage message { seqno, seqno == isn_, move( payload ), false, writer().has_error() };

    // 1.当前窗口大小限制携带不了FIN，留着以后发，没有新的消息了直接退出，否则携带
    // 2.zero窗口仅当message为0时才能携带（因为视为窗口大小为1）
    if ( !FIN_ && writer().is_closed() && len == view.size()
         && ( sequence_numbers_in_flight_ + message.sequence_length() < window_size_
              || ( window_size_ == 0 && message.sequence_length() == 0 ) ) ) {
      FIN_ = message.FIN = true;
    }

    transmit( message );

    abs_ackno_ += message.sequence_length();
    sequence_numbers_in_flight_ += message.sequence_length();
    msg_queue_.emplace( move( message ) );

    // 当前窗口大小限制携带不了FIN，留着以后发，没有新的消息了直接退出
    if ( !FIN_ && writer().is_closed() && len == view.size() ) {
      break;
    }

    seqno = Wrap32::wrap( abs_ackno_, isn_ );
    view.remove_prefix( len );
  }
}

TCPSenderMessage TCPSender::make_empty_message() const
{
  return { Wrap32::wrap( abs_ackno_, isn_ ), false, "", false, writer().has_error() };
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  if ( msg.RST ) {
    writer().set_error();
    return;
  }

  // treat a '0' window size as equal to '1' but don't back off RTO
  window_size_ = msg.window_size;
  uint64_t abs_seq_k = msg.ackno ? msg.ackno.value().unwrap( isn_, abs_old_ackno_ ) : 0;

  // 1.当前消息如果未确认，要始终接受前一个消息的窗口信息
  // 2.只要有ack发过来，就得更新窗口？
  //  if ( ( msg.ackno.has_value() && abs_seq_k == abs_old_ackno_ ) || !msg.ackno.has_value() ) {
  //    // treat a '0' window size as equal to '1' but don't back off RTO
  //    window_size_ = msg.window_size;
  //    return;
  //  }

  // ack之间的比较应该使用绝对序列号，如果传输数据大于4GB，seqno就无法直接比较了
  // 此处不能与old_ackno_相同，否则计时器会被清0，并且不会弹出任何msg
  if ( abs_seq_k > abs_old_ackno_ && abs_seq_k <= abs_ackno_ ) {
    abs_old_ackno_ = abs_seq_k;

    // 计时器清0
    timer_ms = 0;
    RTO_ms_ = initial_RTO_ms_;
    consecutive_retransmissions_ = 0;

    uint64_t abs_seq = 0;
    while ( !msg_queue_.empty() && abs_seq <= abs_seq_k ) {
      abs_seq = msg_queue_.front().seqno.unwrap( isn_, abs_old_ackno_ ) + msg_queue_.front().sequence_length();
      if ( abs_seq <= abs_seq_k ) {
        sequence_numbers_in_flight_ -= msg_queue_.front().sequence_length();
        msg_queue_.pop();
      }
    }
  }
}

void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{
  timer_ms += ms_since_last_tick;
  // 如果在重传之前收到ack并且队列为空，则不需要重传了，此时timer相关信息已经清0
  if ( timer_ms >= RTO_ms_ ) {
    if ( !msg_queue_.empty() ) {
      transmit( msg_queue_.front() );
      if ( window_size_ > 0 ) {
        ++consecutive_retransmissions_;
        RTO_ms_ <<= 1;
      }
    }
    timer_ms = 0;
  }
}
