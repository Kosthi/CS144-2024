#include "reassembler.hh"

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  if ( is_last_substring ) {
    is_last_substring_ = true;
    max_index_ = first_index + data.size();
  }

  if ( data.empty() ) {
    if ( is_last_substring_ && first_unassembled_index_ == max_index_ ) {
      output_.writer().close();
    }
    return;
  }

  if ( writer().available_capacity() == 0 ) {
    return;
  }

  uint64_t last_index = first_index + data.size();

  auto first_unaccept_index = first_unassembled_index_ + output_.writer().available_capacity();

  // 字符串已经写入stream或者超出容量，则丢弃，注意左闭右开，这里取等号
  if ( last_index <= first_unassembled_index_ || first_index >= first_unaccept_index ) {
    return;
  }

  uint64_t offset = 0;
  if ( first_index < first_unassembled_index_ && last_index > first_unassembled_index_ ) {
    // data = data.substr( first_unassembled_index_ - first_index );
    offset = first_unassembled_index_ - first_index;
    first_index = first_unassembled_index_;
  }

  if ( first_index < first_unaccept_index && last_index > first_unaccept_index ) {
    // data = data.substr( 0, first_unaccept_index_ - first_index );
    last_index = first_unaccept_index;
  }

  auto find_pos = first_index;
  auto it = buffer_.upper_bound( find_pos );
  auto begin_index = first_index;
  auto end_index = last_index;

  while ( it != buffer_.end() ) {
    if ( end_index <= it->first ) {
      check_left_right( data, it, begin_index, end_index, first_index, offset );
      break;
    } else {
      check_left_right( data, it, begin_index, it->first, first_index, offset );
      if ( end_index <= it->first + it->second.size() ) {
        break;
      }
      find_pos = it->first + it->second.size();
      it = buffer_.upper_bound( find_pos );
      begin_index = find_pos;
    }
  }

  // 找不到比begin_index大的，有两种情况，一是最后一个元素小于begin_index，则有两种情况begin_idx在最后一个元素里面，则分割字符串，否则在末尾直接添加；否则相等，则数据多覆盖数据少的
  if ( it == buffer_.end() ) {
    if ( buffer_.empty() ) {
      buffer_.emplace( begin_index, data.substr( begin_index - first_index + offset, end_index - begin_index ) );
    } else if ( buffer_.rbegin()->first < begin_index ) {
      if ( begin_index >= buffer_.rbegin()->first + buffer_.rbegin()->second.size() ) {
        buffer_.emplace( begin_index, data.substr( begin_index - first_index + offset, end_index - begin_index ) );
      } else if ( begin_index < buffer_.rbegin()->first + buffer_.rbegin()->second.size()
                  && end_index >= buffer_.rbegin()->first + buffer_.rbegin()->second.size() ) {
        begin_index = buffer_.rbegin()->first + buffer_.rbegin()->second.size();
        buffer_.emplace( begin_index, data.substr( begin_index - first_index + offset, end_index - begin_index ) );
      } else {
        begin_index = end_index;
      }
    } else {
      auto d = data.substr( begin_index - first_index + offset, end_index - begin_index );
      if ( buffer_.rbegin()->second.size() < d.size() ) {
        auto sz = d.size() - buffer_.rbegin()->second.size();
        bytes_pending_ -= sz;
        buffer_.rbegin()->second = move( d );
      } else if ( buffer_.rbegin()->second.size() == d.size() ) {
        begin_index = end_index;
      }
    }
    bytes_pending_ += end_index - begin_index;
  }

  // 如果buffer里面前面的字节都送过来了，写入bytestream
  if ( first_index == first_unassembled_index_ ) {
    it = buffer_.begin();

    string s;
    s.reserve( bytes_pending_ );

    do {
      last_index = it->first + it->second.size();
      // bytes_pending_ -= it->second.size();
      s.append( it->second );
      // output_.writer().push( move( it->second ) );
      it = buffer_.erase( it );
    } while ( it->first == last_index );

    bytes_pending_ -= s.size();
    output_.writer().push( move( s ) );

    first_unassembled_index_ = last_index;

    // 仅最后一个字符串来过且等待的字节为0 关闭流
    if ( is_last_substring_ && first_unassembled_index_ == max_index_ ) {
      output_.writer().close();
    }
    return;
  }
}

uint64_t Reassembler::bytes_pending() const
{
  return bytes_pending_;
}

void Reassembler::check_left_right(
  string& data,
  const _Rb_tree_const_iterator<pair<const unsigned long, basic_string<char>>>& it,
  uint64_t& begin_index,
  const uint64_t& end_index,
  uint64_t& first_index,
  uint64_t& offset )
{
  auto d = data.substr( begin_index - first_index + offset, end_index - begin_index );

  auto pos = buffer_.lower_bound( begin_index );
  // 相等的情况未考虑
  if ( pos->first == begin_index ) {
    if ( pos->second.size() < d.size() ) {
      bytes_pending_ += end_index - begin_index - ( d.size() - pos->second.size() );
      pos->second = move( d );
    }
  } else {
    // ft opft de
    // 第一个比begin_index大 右边不用裁剪，左边没有，直接放左边
    if ( pos == buffer_.begin() ) {
      buffer_.emplace_hint( pos, begin_index, move( d ) );
      bytes_pending_ += end_index - begin_index;
    } else {
      // 第一个begin_index小的位置
      auto prev = pos;
      --prev;

      // 裁剪左边
      if ( begin_index < prev->first + prev->second.size() && end_index >= prev->first + prev->second.size() ) {
        begin_index = prev->first + prev->second.size();
        // 可能有空串?
        buffer_.emplace_hint(
          pos, begin_index, data.substr( begin_index - first_index + offset, end_index - begin_index ) );
        bytes_pending_ += end_index - begin_index;
      } else if ( begin_index >= prev->first + prev->second.size() ) {
        buffer_.emplace_hint( it, begin_index, move( d ) );
        bytes_pending_ += end_index - begin_index;
      }
    }
  }
}
