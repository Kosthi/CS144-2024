#include <iostream>

#include "arp_message.hh"
#include "exception.hh"
#include "network_interface.hh"

using namespace std;

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface( string_view name,
                                    shared_ptr<OutputPort> port,
                                    const EthernetAddress& ethernet_address,
                                    const Address& ip_address )
  : name_( name )
  , port_( notnull( "OutputPort", move( port ) ) )
  , ethernet_address_( ethernet_address )
  , ip_address_( ip_address )
{
  cerr << "DEBUG: Network interface has Ethernet address " << to_string( ethernet_address ) << " and IP address "
       << ip_address.ip() << "\n";
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but
//! may also be another host if directly connected to the same network as the destination) Note: the Address type
//! can be converted to a uint32_t (raw 32-bit IP address) by using the Address::ipv4_numeric() method.
void NetworkInterface::send_datagram( const InternetDatagram& dgram, const Address& next_hop )
{
  EthernetFrame ethernetFrame;
  ethernetFrame.header.src = ethernet_address_;

  // not in mp, send arp request datagram
  if ( !mp_.contains( next_hop.ipv4_numeric() ) ) {

    // 如果该IP地址的ARP请求已经发送并且未过期，直接返回
    if ( arp_send_.contains( next_hop.ipv4_numeric() ) ) {
      return;
    }

    ethernetFrame.header.dst = ETHERNET_BROADCAST;
    ethernetFrame.header.type = EthernetHeader::TYPE_ARP;

    ARPMessage arpMessage;
    arpMessage.opcode = ARPMessage::OPCODE_REQUEST;
    arpMessage.sender_ethernet_address = ethernet_address_;
    arpMessage.sender_ip_address = ip_address_.ipv4_numeric();
    arpMessage.target_ip_address = next_hop.ipv4_numeric();

    ethernetFrame.payload = serialize( arpMessage );

    arp_send_.emplace( arpMessage.target_ip_address, std::pair<size_t, EthernetFrame>( cur_time, ethernetFrame ) );

    datagrams_received_.emplace( dgram );

    transmit( ethernetFrame );
    return;
  }

  ethernetFrame.header.dst = mp_[next_hop.ipv4_numeric()].second;
  ethernetFrame.header.type = EthernetHeader::TYPE_IPv4;

  ethernetFrame.payload = serialize( dgram );
  transmit( ethernetFrame );
}

//! \param[in] frame the incoming Ethernet frame
void NetworkInterface::recv_frame( const EthernetFrame& frame )
{
  // 一台主机可能有多个IP
  // 在同一网段，通过路由直接转发
  // 在不同网段，通过路由器确定IP网段来转发给哪个路由，需要该路由的物理地址，以封装成以太网帧
  // 收到以太网帧只接受与自身网卡物理地址相符的数据包
  ARPMessage arpMessage;

  if ( parse( arpMessage, frame.payload ) ) {
    // arp request datagram but not target ip
    if ( arpMessage.target_ip_address != ip_address_.ipv4_numeric() ) {
      return;
    }
    EthernetFrame ethernetFrame;
    if ( arpMessage.opcode == ARPMessage::OPCODE_REQUEST ) {
      // learn from sender to update ip_address
      mp_[arpMessage.sender_ip_address]
        = std::pair<size_t, EthernetAddress>( cur_time, arpMessage.sender_ethernet_address );

      // send arp reply datagram
      arpMessage.opcode = ARPMessage::OPCODE_REPLY;

      arpMessage.target_ethernet_address = ethernet_address_;
      swap( arpMessage.sender_ip_address, arpMessage.target_ip_address );
      swap( arpMessage.sender_ethernet_address, arpMessage.target_ethernet_address );

      ethernetFrame.header.src = ethernet_address_;
      ethernetFrame.header.dst = arpMessage.target_ethernet_address;
      ethernetFrame.header.type = EthernetHeader::TYPE_ARP;

      ethernetFrame.payload = serialize( arpMessage );

      transmit( ethernetFrame );
      return;
    } else if ( arpMessage.opcode == ARPMessage::OPCODE_REPLY ) {
      ethernetFrame.header.src = ethernet_address_;
      ethernetFrame.header.dst = arpMessage.sender_ethernet_address;
      ethernetFrame.header.type = EthernetHeader::TYPE_IPv4;

      // Maybe wrong
      ethernetFrame.payload = serialize( datagrams_received_.front() );

      datagrams_received_.pop();

      // erase
      if ( arp_send_.contains( arpMessage.sender_ip_address ) ) {
        arp_send_.erase( arpMessage.sender_ip_address );
      }

      // learn from sender to update ip_address
      mp_.emplace( arpMessage.sender_ip_address,
                   std::pair<size_t, EthernetAddress>( cur_time, arpMessage.sender_ethernet_address ) );

      transmit( ethernetFrame );
      return;
    }
  }

  // old ethernet address
  if ( frame.header.dst != ethernet_address_ ) {
    return;
  }

  InternetDatagram internetDatagram;
  if ( parse( internetDatagram, frame.payload ) ) {
    datagrams_received_.emplace( internetDatagram );
  }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
  cur_time += ms_since_last_tick;
  for ( auto it = mp_.begin(); it != mp_.end(); ) {
    // IP-to-Ethernet mappings hold 30s
    if ( cur_time - it->second.first >= 30000 ) {
      it = mp_.erase( it );
    } else {
      ++it;
    }
  }

  for ( auto& arp : arp_send_ ) {
    // 5s 超时重传 更新时间戳
    if ( cur_time - arp.second.first >= 5000 ) {
      arp.second.first = cur_time; // may new test
      transmit( arp.second.second );
    }
  }
}
