Checkpoint 0 Writeup
====================

My name: [Koschei]

My SUNet ID: [210405317]

I collaborated with: [list sunetids here]

I would like to credit/thank these classmates for their help: [list sunetids here]

This lab took me about [3] hours to do. I [did not] attend the lab session.

My secret code from section 2.1 was: [383417]

I was surprised by or edified to learn that: [the power of std::string_view to reduce copy]

Describe ByteStream implementation. [
### ByteStream Implementation Description:

#### Data Structures:
1. **Deques for Buffering:**
  - `buffer_` and `buffer_view_` are deque containers used to store strings and string_views, respectively.
  - `buffer_` is used to store pushed data as strings, and `buffer_view_` contains string_views for efficient peeking.

2. **Capacity Management:**
  - `capacity_` represents the initial capacity of the ByteStream, indicating the maximum allowed bytes.
  - `available_capacity_` tracks the remaining capacity available for pushing data.

#### Approach:
1. **Resizable Buffer:**
  - The implementation utilizes a resizable buffer to store pushed data and supports dynamic resizing of the deque containers as needed.
  - `std::deque` is chosen for its efficient insertion and deletion at both ends.

2. **Reader and Writer Interfaces:**
  - The design separates the Reader and Writer interfaces, allowing independent reading and writing operations on the ByteStream.
  - The `push` operation is implemented in the Writer to add data to the stream, while the `pop` operation is part of the Reader to remove bytes from the buffer.

3. **Error Handling:**
  - The `set_error` function signals that an error has occurred in the stream, and `has_error` is used to check if an error has been set.

#### Alternative Designs Considered:
1. **Single Deque vs. Double Deque:**
  - The choice of using separate deques for strings and string_views could be combined into a single deque. However, separation simplifies the management of different data types.

2. **Alternative Buffering Approaches:**
  - Instead of using deques, other data structures like vectors or lists could be considered. Deques were chosen for their balanced characteristics of fast insertion and deletion at both ends.

#### Benefits and Weaknesses:
##### Benefits:
1. **Simplicity:**
  - The design is relatively simple and easy to understand, separating the concerns of reading and writing.

2. **Dynamic Resizing:**
  - Resizable deques allow efficient management of varying amounts of data, minimizing the need for manual capacity adjustments.

##### Weaknesses:
1. **Memory Overhead:**
  - The use of deques might introduce some memory overhead due to their dynamic resizing behavior.

2. **Potential for Fragmentation:**
  - Frequent resizing of deques could lead to memory fragmentation, impacting long-term performance.

3. **Limited Error Context:**
  - The error handling mechanism is basic, providing a boolean flag for error detection but lacks detailed error information.

#### Measurements (if applicable):
- Empirical performance measurements, such as profiling the memory usage and execution time under different scenarios, could be conducted to assess the impact of dynamic resizing on performance.

#### Conclusion:
The chosen design strikes a balance between simplicity and efficiency, offering a flexible solution for managing byte streams. It allows for dynamic resizing while providing separate interfaces for reading and writing. Consideration of alternative designs ensures that the chosen implementation aligns with the specific requirements of the system.]

- Optional: I had unexpected difficulty with: [describe]

- Optional: I think you could make this lab better by: [describe]

- Optional: I'm not sure about: [describe]

- Optional: I contributed a new test case that catches a plausible bug
  not otherwise caught: [provide Pull Request URL]