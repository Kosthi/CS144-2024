Checkpoint 5 Writeup
====================

My name: [Koschei]

My SUNet ID: [210405317]

I collaborated with: [list sunetids here]

I would like to thank/reward these classmates for their help: [list sunetids here]

This checkpoint took me about [n] hours to do. I [did not] attend the lab session.

Program Structure and Design of the NetworkInterface:

## Program Structure

// IP-to-Ethernet mappings
std::unordered_map<size_t, std::pair<size_t, EthernetAddress>> mp_ {};
// arp request mappings 
std::unordered_map<size_t, std::pair<size_t, EthernetFrame>> arp_send_ {};
// system timer
size_t cur_time { 0 };

## Function Implementation

### send_datagram
ip和物理地址映射表包含目标ip，封装成以太网帧直接发送，否则，根据ip判断arp请求报文是否被发送，若无，则发送并将当前数据报文存入待发送数据报队列，否则直接返回。

### recv_frame

#### ARP请求报文
非目标主机ip，更新ip和物理地址映射表。
为目标主机ip，更新ip和物理地址映射表，并发送arp响应报文

#### ARP响应报文
为明确转发给目标主机ip，目标主机更新ip和物理地址映射表，并发送用户数据报

#### 用户数据报文
为目标主机MAC地址，接收并存入数据报队列，否则拒绝

### tick
#### 更新当前时间
#### 检查mappings
arp请求报文超时更新时间戳并重传，IP-to-Ethernet mapping超时删除

Implementation Challenges:
[]

Remaining Bugs:
[]

- Optional: I had unexpected difficulty with: [describe]

- Optional: I think you could make this lab better by: [describe]

- Optional: I was surprised by: [describe]

- Optional: I'm not sure about: [describe]
