Checkpoint 2 Writeup
====================

My name: [Koschei]

My SUNet ID: [210405317]

I collaborated with: [list sunetids here]

I would like to thank/reward these classmates for their help: [list sunetids here]

This lab took me about [6] hours to do. I [did not] attend the lab session.

Describe Wrap32 and TCPReceiver structure and design. [
## Warp
### Warp function
64位绝对序列号加上zero_point，然后对UINT32_MAX取余就行，直接强制转化截断高位，相当于取余 
### UnWrap function
The wrap/unwrap operations should preserve offsets—two seqnos that differ by 17 will correspond to two absolute seqnos that also differ by 17.
文档中极为关键的提示，直接利用Wrap方法直接算出checkpoint对应的seqno，然后计算与自身seqno的偏移量（如果偏移量下溢，则会由于数据是以补码存储的特点，相当于a + 2^32 - b），有了偏移量，我们再以checkpoint为基点，就可以直接算出绝对序列号：
在32位空间中，从一个点到另一个点，既可以向右移动X距离，也可以向左移动(2^32 - X)，对应到64位空间中，就是checkpoint 右边：checkpoint + X, 左边：checkpoint - (2^32 - X)，只需要判断 X 和 2^32 - X 哪个更小即可。还有一种特殊情况，
即如果checkpoint 不够左移怎么办（checkpoint < 2^32 - X），那么只有选右边的点呗

## TCP receiver
### receive
根据文档提示，checkpoint可以用first_unassembled_index，即bytes_pushed，然后注意如果是SYN发过来，就要把ISN记录下来作为zero_point，并且此时first_index一定是0
### send
滑动窗口最大一定是65535，如果buffer里面的容量大于这个值，就要取65535。如果连接关闭了，FIN要消耗一个序列号，则ack的seqno应该是first_unassembled_index + 1 + writer().is_closed()
]

Implementation Challenges:
[UnWrap，没有仔细看hint，导致一开始的实现的既复杂又慢，用好offset实现地非常容易]

Remaining Bugs:
[Maybe not.]

- Optional: I had unexpected difficulty with: [describe]

- Optional: I think you could make this lab better by: [describe]

- Optional: I was surprised by: [describe]

- Optional: I'm not sure about: [describe]

- Optional: I made an extra test I think will be helpful in catching bugs: [
- 第一次发来SYN，没有数据，第二次发来SYN，有数据，这种情况该不该考虑？
- FIN发过来连接关闭以后，收到上次连接的数据包（滞留了很久才终于发过来），恰好seqno==first_assembled_index，会不会插入尾部？
]
