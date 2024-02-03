Checkpoint 3 Writeup
====================

My name: [Koschei]

My SUNet ID: [210405317]

I collaborated with: [list sunetids here]

I would like to thank/reward these classmates for their help: [list sunetids here]

This checkpoint took me about [9] hours to do. I [did not] attend the lab session.

Program Structure and Design of the TCPSender : [
```c++
ByteStream input_;                          // 输入流 底层写入需要发送的字节流并由高层读出
Wrap32 isn_;                                // 初始序列号 随机值
uint64_t initial_RTO_ms_;                   // RTO初始值（毫秒）
uint64_t RTO_ms_;                           // RTO（毫秒）
uint64_t timer_ms { 0 };                    // 计时器（毫秒）
uint16_t window_size_ { 1 };                // 窗体大小（初始化为1，发送无效消息以获取更新的ack）
uint64_t abs_ackno_ { 0 };                  // 发送的最新的消息的绝对序列号
uint64_t abs_old_ackno_ { 0 };              // 上一次收到ack的消息的绝对序列号
uint64_t sequence_numbers_in_flight_ {0};   // 传输中的序列号数量
uint64_t consecutive_retransmissions_ {0};  // 单个消息的重传次数
std::queue<TCPSenderMessage> msg_queue_ {}; // 消息队列，收到ack则去掉小于等于ack的所有消息
bool FIN_ { false };                        // 消息结束标记
```

### push
先检测传输中的序列号数量是不是大于等于窗口大小，是的直接返回，另外特判zero窗口情况，因为要发送一个字节的假消息，序列号数量应该是要大于等于1才直接返回。

先一次性从流中读出窗口大小尺寸的数据（或者更小），然后转为成一条消息，更新元数据信息，存入msg_queue_中，如此往复直到数据转化完。
如果故障发送了，也要发送一条RST为true的故障消息。

流关闭了不是立刻发送FIN，还要根据发送中的序列号数量和窗体大小确定是当前发还是以后发，因为FIN会用掉一个序列号，可能放不下FIN，只能留着以后发。

### make_empty_message
用abs_ackno_返回最好情况下的ack就行，注意故障了要返回RST，其他不需要携带了。

### receive
接收到故障更新流状态后直接返回。 剩下分两种情况：

#### 只更新窗口
消息有ack且等于old_ack或者消息没有ack

#### 更新窗口，计时器清0，移除过期消息
消息有ack且ack>old_ack小于ackno（成员变量），即在有效ack范围内，则接受

### tick
每次被调用就增加timer_ms，如果超时，就重发消息队列里的最早一条消息（即队头），并且如果窗口大小大于0，则更新RTO为原来
的2倍，重传次数加1，同时计时器重新计时；否则为zero窗口，不更新RTO和重传次数，只计时器重新计时。

考虑zero窗口的特殊情况，因为正好可以把重发的消息当作假消息发送，所以这里只要超时就必须发送，但只有实际发送成功（窗口大于0），
才会计入元数据中。这里为什么窗口大于0就有效而不是大于等于消息长度，是因为接受端实现了重组器，即使消息长度大于窗口大小（窗口大于0），
也会存入部分的有效数据。

况且，由push函数知道，假消息也会放入消息队列中，也需要重传，所以即使知道这是假消息计时器也得清0。

这里的设计是让假消息RTO时间重传一次，非假消息指数回避（RTO<<=2）。

]

Implementation Challenges:
[push函数确保SYN和FIN只发送一次，且根据窗口大小确定要不要发FIN，还是留着以后发]

Remaining Bugs:
[Maybe not.]

- Optional: I had unexpected difficulty with: [describe]

- Optional: I think you could make this lab better by: [describe]

- Optional: I was surprised by: [describe]

- Optional: I'm not sure about: [describe]
