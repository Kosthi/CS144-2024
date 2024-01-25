#include "reassembler.hh"
#include <algorithm>

using namespace std;

// map version1 合并区间 win 4s 107 lines
void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  if ( is_last_substring && !is_last_substring_ ) {
    is_last_substring_ = true;
    max_index_ = first_index + data.size();
  }

  if ( writer().available_capacity() == 0 ) {
    return;
  }

  if ( data.empty() ) {
    if ( is_last_substring_ && first_unassembled_index_ == max_index_ ) {
      output_.writer().close();
    }
    return;
  }

  uint64_t last_index = first_index + data.size();

  auto first_unaccept_index = first_unassembled_index_ + output_.writer().available_capacity();

  // 字符串已经写入stream或者超出容量，则丢弃，注意左闭右开，这里取等号
  if ( last_index <= first_unassembled_index_ || first_index >= first_unaccept_index ) {
    return;
  }

  uint64_t offset = 0;
  if ( first_index < first_unassembled_index_ ) {
    offset = first_unassembled_index_ - first_index;
    first_index = first_unassembled_index_;
  }

  last_index = min( last_index, first_unaccept_index );

  string_view middle( data.data() + offset, last_index - first_index );

  string res;

  if ( first_index > first_unassembled_index_ ) {

    auto l = lower_bound(
      buffer_.begin(), buffer_.end(), first_index, [&]( const pair<uint64_t, string>& it, uint64_t first_index_ ) {
        return it.first + it.second.size() < first_index_;
      } );
    auto r
      = upper_bound( l, buffer_.end(), last_index, [&]( uint64_t last_index_, const pair<uint64_t, string>& it ) {
          return it.first > last_index_;
        } );

    // 重复
    if ( !buffer_.empty() && l == buffer_.end() && r == buffer_.begin() ) {
      return;
    }

    if ( l != buffer_.end() ) {
      if ( l->first < first_index ) {
        string_view left( l->second.data(), first_index - l->first );
        res.append( left );
      }
      first_index = min( first_index, l->first );
    }

    res.append( middle );

    if ( r != buffer_.begin() ) {
      --r;
      if ( r->first + r->second.size() > last_index ) {
        // 用view的时候注意数据中可能有\0，必须指定长度
        string_view right( r->second.data() + last_index - r->first, r->first + r->second.size() - last_index );
        res.append( right );
      }
      ++r;
      // last_index = max( last_index, r->first + r->second.size() );
    }

    for_each( l, r, [&]( const auto& item ) { bytes_pending_ -= item.second.size(); } );

    bytes_pending_ += res.size();

    buffer_.emplace_hint( buffer_.erase( l, r ), first_index, move( res ) );
    return;
  }

  res.reserve( bytes_pending_ );

  first_unassembled_index_ += middle.size();
  res.append( middle );

  while ( !buffer_.empty() && first_unassembled_index_ >= buffer_.begin()->first ) {
    if ( first_unassembled_index_ < buffer_.begin()->first + buffer_.begin()->second.size() ) {
      // 字符串中可能有\0，需要指定长度
      string_view tail( buffer_.begin()->second.data() + first_unassembled_index_ - buffer_.begin()->first,
                        buffer_.begin()->first + buffer_.begin()->second.size() - first_unassembled_index_ );
      first_unassembled_index_ += tail.size();
      res.append( tail );
    }
    bytes_pending_ -= buffer_.begin()->second.size();
    buffer_.erase( buffer_.begin() );
  }

  output_.writer().push( move( res ) );

  // 仅最后一个字符串来过且等待的字节为0 关闭流
  if ( is_last_substring_ && first_unassembled_index_ == max_index_ ) {
    output_.writer().close();
  }
}

// map version2 缝隙填充 win 2.9s 106 lines
// void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
//{
//  if ( is_last_substring && !is_last_substring_ ) {
//    is_last_substring_ = true;
//    max_index_ = first_index + data.size();
//  }
//
//  if ( data.empty() ) {
//    if ( is_last_substring_ && first_unassembled_index_ == max_index_ ) {
//      output_.writer().close();
//    }
//    return;
//  }
//
//  if ( writer().available_capacity() == 0 ) {
//    return;
//  }
//
//  uint64_t last_index = first_index + data.size();
//
//  auto first_unaccept_index = first_unassembled_index_ + output_.writer().available_capacity();
//
//  // 字符串已经写入stream或者超出容量，则丢弃，注意左闭右开，这里取等号
//  if ( last_index <= first_unassembled_index_ || first_index >= first_unaccept_index ) {
//    return;
//  }
//
//  uint64_t offset = 0;
//  if ( first_index < first_unassembled_index_ ) {
//    offset = first_unassembled_index_ - first_index;
//    first_index = first_unassembled_index_;
//  }
//
//  last_index = min( last_index, first_unaccept_index );
//
//  string_view middle( data.data() + offset, last_index - first_index );
//
//  string res;
//
//  auto find_pos = first_index;
//  auto it = buffer_.upper_bound( find_pos );
//  auto begin_index = first_index;
//  auto end_index = last_index;
//
//  if ( first_index > first_unassembled_index_ ) {
//
//    if ( buffer_.empty() ) {
//      buffer_.emplace( first_index, data.substr( offset, last_index - first_index ) );
//      bytes_pending_ += last_index - first_index;
//      return;
//    }
//
//    auto prev = it;
//    --prev;
//
//    // 如果it为头 prev--后还是头
//    begin_index = it == buffer_.begin() ? first_index : max( begin_index, prev->first + prev->second.size() );
//
//    uint64_t right_index, len;
//
//    while ( it != buffer_.end() && begin_index < end_index ) {
//      right_index = max( begin_index, min( end_index, it->first ) );
//      len = right_index - begin_index;
//      if ( len > 0 ) {
//        buffer_.emplace_hint(
//          it, begin_index, data.substr( begin_index - first_index - offset, right_index - begin_index ) );
//        bytes_pending_ += len;
//      }
//      begin_index = right_index == end_index ? end_index : max( begin_index, it->first + it->second.size() );
//      ++it;
//    }
//
//    if ( begin_index < end_index ) {
//      buffer_.emplace_hint(
//        it, begin_index, data.substr( begin_index - first_index + offset, end_index - begin_index ) );
//      bytes_pending_ += end_index - begin_index;
//    }
//
//    //    auto prev = bytes_pending_list.begin();
//    //    for ( auto it = ++bytes_pending_list.begin();
//    //          it != bytes_pending_list.end() && prev->first + prev->second.size() == it->first; ) {
//    //      prev->second.append( it->second );
//    //      it = bytes_pending_list.erase( it );
//    //    }
//
//    return;
//  }
//
//  // 如果buffer里面前面的字节都送过来了，写入bytestream
//
//  res.reserve( bytes_pending_ );
//
//  first_unassembled_index_ += middle.size();
//  res.append( middle );
//
//  while ( !buffer_.empty() && first_unassembled_index_ >= buffer_.begin()->first ) {
//    if ( first_unassembled_index_ < buffer_.begin()->first + buffer_.begin()->second.size() ) {
//      // 字符串中可能有\0，需要指定长度
//      string_view tail( buffer_.begin()->second.data() + first_unassembled_index_ - buffer_.begin()->first,
//                        buffer_.begin()->first + buffer_.begin()->second.size() - first_unassembled_index_ );
//      first_unassembled_index_ += tail.size();
//      res.append( tail );
//    }
//    bytes_pending_ -= buffer_.begin()->second.size();
//    buffer_.erase( buffer_.begin() );
//  }
//
//  output_.writer().push( move( res ) );
//
//  // 仅最后一个字符串来过且到达上限index 关闭流
//  if ( is_last_substring_ && first_unassembled_index_ == max_index_ ) {
//    output_.writer().close();
//  }
//}

// list version 缝隙填充 win 3.6s 94 lines
// void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
//{
//  if ( is_last_substring ) {
//    is_last_substring_ = true;
//    max_index_ = first_index + data.size();
//  }
//
//  if ( data.empty() ) {
//    if ( is_last_substring_ ) {
//      output_.writer().close();
//    }
//    return;
//  }
//
//  if ( writer().available_capacity() == 0 ) {
//    return;
//  }
//
//  auto end_index = first_index + data.size();
//
//  auto first_unaccepted_index = first_unassembled_index_ + writer().available_capacity();
//
//  // 已经写入过
//  if ( end_index <= first_unassembled_index_ || first_index >= first_unaccepted_index ) {
//    return;
//  }
//
//  if ( end_index >= first_unaccepted_index ) {
//    data = data.substr( 0, first_unaccepted_index - first_index );
//  }
//
//  // earlier bytes remain unknown, insert into buffer
//  if ( first_index > first_unassembled_index_ ) {
//    auto begin_index = first_index;
//    end_index = first_index + data.size();
//    // c 2 bcd 1
//    for ( auto it = bytes_pending_list.begin(); it != bytes_pending_list.end() && begin_index < end_index; ) {
//      // 取左边界 相同去重，大于比较谁更大
//      if ( it->first <= begin_index ) {
//        begin_index = max( begin_index, it->first + it->second.size() );
//        ++it;
//        continue;
//      }
//      auto right_index = min( end_index, it->first );
//      auto len = right_index - begin_index;
//      bytes_pending_list.emplace(
//        it, begin_index, len == data.size() ? move( data ) : data.substr( begin_index - first_index, len ) );
//      bytes_pending_ += len;
//      begin_index = right_index;
//    }
//
//    if ( begin_index < end_index ) {
//      bytes_pending_list.emplace_back( begin_index, data.substr( begin_index - first_index ) );
//      bytes_pending_ += end_index - begin_index;
//    }
//
//    //    auto prev = bytes_pending_list.begin();
//    //    for ( auto it = ++bytes_pending_list.begin();
//    //          it != bytes_pending_list.end() && prev->first + prev->second.size() == it->first; ) {
//    //      prev->second.append( it->second );
//    //      it = bytes_pending_list.erase( it );
//    //    }
//
//    return;
//  }
//
//  if ( first_index < first_unassembled_index_ ) {
//    data = data.substr( first_unassembled_index_ - first_index );
//  }
//
//  // 不是链表有什么加什么，而是数据包中没有什么加什么，数据包校验正确后就是对的
//  // 把连续的字符串连接一起并清除
//
//  first_unassembled_index_ += data.size();
//  output_.writer().push( move (data) );
//
//  while ( !bytes_pending_list.empty() && first_unassembled_index_ >= bytes_pending_list.begin()->first ) {
//    bytes_pending_ -= bytes_pending_list.begin()->second.size();
//
//    if ( bytes_pending_list.begin()->first + bytes_pending_list.begin()->second.size()
//         > first_unassembled_index_ ) {
//      if ( bytes_pending_list.begin()->first < first_unassembled_index_ ) {
//        bytes_pending_list.begin()->second = bytes_pending_list.begin()->second.substr(
//          first_unassembled_index_ - bytes_pending_list.begin()->first );
//      }
//      first_unassembled_index_ += bytes_pending_list.begin()->second.size();
//      output_.writer().push( move( bytes_pending_list.begin()->second ) );
//    }
//
//    bytes_pending_list.pop_front();
//  }
//
//  if ( is_last_substring_ && first_unassembled_index_ == max_index_ ) {
//    output_.writer().close();
//  }
//}

uint64_t Reassembler::bytes_pending() const
{
  return bytes_pending_;
}
