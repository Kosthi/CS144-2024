#include "router.hh"

#include <iostream>
#include <limits>

using namespace std;

// route_prefix: The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
// prefix_length: For this route to be applicable, how many high-order (most-significant) bits of
//    the route_prefix will need to match the corresponding bits of the datagram's destination address?
// next_hop: The IP address of the next hop. Will be empty if the network is directly attached to the router (in
//    which case, the next hop address should be the datagram's final destination).
// interface_num: The index of the interface to send the datagram out on.
void Router::add_route( const uint32_t route_prefix,
                        const uint8_t prefix_length,
                        const optional<Address> next_hop,
                        const size_t interface_num )
{
  cerr << "DEBUG: adding route " << Address::from_ipv4_numeric( route_prefix ).ip() << "/"
       << static_cast<int>( prefix_length ) << " => " << ( next_hop.has_value() ? next_hop->ip() : "(direct)" )
       << " on interface " << interface_num << "\n";

  route_table_.emplace( prefix_length, std::make_tuple( route_prefix, next_hop, interface_num ) );
}

// Go through all the interfaces, and route every incoming datagram to its proper outgoing interface.
void Router::route()
{
  for ( auto& interface : _interfaces ) {
    while ( !interface->datagrams_received().empty() ) {
      auto& datagram = interface->datagrams_received().front();

      // decrements the datagram’s TTL optimize
      if ( datagram.header.ttl <= 1 ) {
        interface->datagrams_received().pop();
        continue;
      }

      // if (datagram.header)
      for ( auto& route : route_table_ ) {
        uint32_t prefix;
        // 默认路由
        if ( route.first == 0 ) {
          prefix = 0;
        } else {
          // ！右移32位或以上是未定义行为，不会发送改变
          prefix = ( datagram.header.dst >> ( 32 - route.first ) ) << ( 32 - route.first );
        }
        // 前缀匹配
        if ( get<0>( route.second ) == prefix ) {
          --datagram.header.ttl;
          datagram.header.compute_checksum();

          if ( get<1>( route.second ) ) {
            _interfaces[get<2>( route.second )]->send_datagram( datagram, get<1>( route.second ).value() );
          } else {
            _interfaces[get<2>( route.second )]->send_datagram( datagram,
                                                                Address::from_ipv4_numeric( datagram.header.dst ) );
          }

          break;
        }
      }

      // drop or already send
      interface->datagrams_received().pop();
    }
  }
}
