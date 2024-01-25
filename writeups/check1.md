Checkpoint 1 Writeup
====================

My name: [Koschei]

My SUNet ID: [210405317]

I collaborated with: [list sunetids here]

I would like to thank/reward these classmates for their help: [list sunetids here]

This lab took me about [3] hours to do. I [did not] attend the lab session.

I was surprised by or edified to learn that: [describe]

Describe Reassembler structure and design. [
主要：std::map<uint64_t, std::string> 维护first_index和上面的string
bool is_last_substring_ 记录最后一个字符是否来过
uint64_t max_index_ 记录最后一个字符的索引
uint64_t bytes_pending_ 记录缓冲区字节数量
uint64_t first_unassembled_index_ 记录第一个未重组的索引
实际测试插入法的map过win比较块，list速度稍微快一点点
]

Implementation Challenges:
[重组，是合并，还是插入缝隙，合并代码比较简单方便维护，插入方法win耗时比较短，但是代码难写]

Remaining Bugs:
[Maybe not.]

- Optional: I had unexpected difficulty with: [describe]

- Optional: I think you could make this lab better by: [describe]

- Optional: I'm not sure about: [describe]
