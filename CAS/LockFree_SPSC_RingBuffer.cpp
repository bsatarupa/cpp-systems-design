

#include <atomic>
#include <cstddef>
#include <iostream>
using namespace std;

class SPSC_Queue {
  static constexpr size_t BUFFER_SIZE = 8;
  int buffer[BUFFER_SIZE];

  atomic<size_t> write_pos{0};
  atomic<size_t> read_pos{0};

public:
  bool push(int val) {
    auto write = write_pos.load(memory_order_relaxed);
    auto next = (write + 1) % BUFFER_SIZE;

    if (next == read_pos.load(memory_order_acquire)) {
      return false; // queue is FULL
    }

    buffer[write] = val;

    // publish the new write position
    write_pos.store(next, memory_order_release);

    return true;
  }

  bool pop(int &val) {
    auto read = read_pos.load(memory_order_relaxed);
    auto next = (read + 1) % BUFFER_SIZE;

    if (read == write_pos.load(memory_order_acquire)) {
      return false; // queue is EMPTY
    }

    val = buffer[read];

    // publish that current slot is free
    read_pos.store(next, memory_order_release);

    return true;
  }
};

int main() {
  SPSC_Queue queue;

  queue.push(10);
  queue.push(20);
  queue.push(30);

  int val;
  while (queue.pop(val)) {
    cout << "Popped : " << val << '\n';
  }
  return 0;
}
/*
Output:
Popped : 10
Popped : 20
Popped : 30

There are 2 threads.The producer must publish the data before publishing
write_pos. Producer                         Consumer

buffer[write] = value

       |
       | release
       ↓
write_pos.store()
       |
       |              acquire
       +----------→ write_pos.load()

                              |
                              ↓
                         read buffer
Pattern to remember:
             OWN STATE             OTHER THREAD'S STATE

Producer     relaxed               acquire
             write_pos             read_pos

             release
             write_pos.store()

Consumer     relaxed               acquire
             read_pos              write_pos

             release
             read_pos.store()

Q1. Why relaxed here?
const auto write = write_pos.load(std::memory_order_relaxed);
The producer is the only thread modifying write_pos. It does not need
synchronization to read its own position.

Likewise: const auto read = read_pos.load(std::memory_order_relaxed);
The consumer owns read_pos.
So, for Own index → relaxed is sufficient.

Q2. Why release?
Producer: buffer[write] = value;
write_pos.store(next, std::memory_order_release);

The release store means:
All writes before this store become visible to a thread that performs a
corresponding acquire operation. So the producer effectively says: I have
written the item. Now I am publishing that fact.

Q3. Why acquire?
Consumer: write_pos.load(std::memory_order_acquire);
If it observes the producer's released write_pos, it can safely see the data
that the producer wrote before that release.

Producer:
    write data
        ↓
    release

Consumer:
    acquire
        ↓
    read data
This is the release → acquire synchronization pair.

Q4. Why not relaxed everywhere?
Suppose producer does:buffer[write] = value;
write_pos.store(next, std::memory_order_relaxed);

And consumer does:
if (read != write_pos.load(std::memory_order_relaxed)) {
    value = buffer[read];
}
The atomic write_pos itself is still atomic, but it does not establish the
required synchronization for the non-atomic buffer access.

Q5. Why not seq_cst everywhere?
You could write: std::memory_order_seq_cst
everywhere and make the ordering stronger than necessary.
But the algorithm only needs: relaxed + acquire/release
Using the weakest ordering that correctly expresses the synchronization
requirements is an important low-level C++ skill.

Relaxed gives atomicity without synchronization;
Acquire/Eelease is needed when 1 thread publishes data and another thread must
safely observe/consume that data.
*/
