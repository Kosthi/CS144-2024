Checkpoint 5 Writeup
====================

**Name:** Koschei

This checkpoint took me about _n_ hours to complete. I did not attend the lab session.

## Program Structure

The program uses the following key data structures:

```c++
// Mapping from IP to Ethernet
std::unordered_map<size_t, std::pair<size_t, EthernetAddress>> mp_ {};

// Mapping for ARP requests
std::unordered_map<size_t, std::pair<size_t, EthernetFrame>> arp_send_ {};

// System timer
size_t cur_time { 0 };
```

These data structures are used to manage the IP-to-Ethernet mappings, ARP requests, and the system timing.

## Function Implementation

### Function: `send_datagram`

This function is responsible for sending datagrams. If the IP-to-Ethernet mapping table contains the target IP, the function wraps the message into an Ethernet frame and sends it directly. If the target IP is not in the mapping table, the function checks if an ARP request message has been sent for this IP. If not, it sends an ARP request and stores the current datagram in the queue of datagrams to be sent. If an ARP request has already been sent, the function simply returns without doing anything.

### Function: `recv_frame`

This function handles the received frames. The behavior of this function depends on the type of the received frame:

#### ARP Request Message

- If the target host IP does not match the IP in the request, the function updates the IP-to-Ethernet mapping table.
- If the target host IP matches the IP in the request, the function updates the IP-to-Ethernet mapping table and sends an ARP response message.

#### ARP Response Message

- If the message is explicitly forwarded to the target host IP, the host updates the IP-to-Ethernet mapping table and sends the user datagram.

#### User Datagram Message

- If the target host MAC address matches the MAC address in the message, the function receives the message and stores it in the datagram queue. Otherwise, it rejects the message.

### Function: `tick`

This function is called periodically to perform maintenance tasks:

- It updates the current time.
- It checks the mappings. If an ARP request message is timed out, it updates the timestamp and retransmits the message. If an IP-to-Ethernet mapping is timed out, it removes the mapping.

## Test Passed

![check5.png](https://ph-bed.oss-cn-beijing.aliyuncs.com/cs144/check5.png)
