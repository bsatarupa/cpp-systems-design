/*
Implement Atomic Counter using:
std::atomic,
memory_order_relaxed,
No CAS
*/
#include <atomic>
#include <iostream>
#include <thread>

using namespace std;

class AtomicCounter {

  atomic<int> count{0};

public:
  void increment() { count.fetch_add(1, std::memory_order_relaxed); }

  int get() const { return count.load(std::memory_order_relaxed); }
};

int main() {

  AtomicCounter counter;
  vector<thread> threads;

  for (int i = 0; i < 4; i++) {
    threads.emplace_back([&counter]() {
      for (int j = 0; j < 100000; j++)
        counter.increment();
    });
  }

  for (auto &t : threads)
    t.join();

  cout << counter.get() << '\n';
  return 0;
}
/*
Output:
400000

Why std::atomic?
Because multiple threads concurrently modify count. A normal int would cause a
data race → undefined behavior.

Why fetch_add()? count.fetch_add(1, std::memory_order_relaxed);
fetch_add() performs increment atomically. Two threads cannot lose an increment.

Why memory_order_relaxed?
For a pure counter, we only need:Atomicity, not synchronization.
We don't need the counter increment to establish ordering of other memory
operations. So, fetch_add(1, relaxed) is sufficient and cheaper than stronger
ordering.

Atomic Counter
    │
    ├── std::atomic<int>
    │
    ├── fetch_add()
    │
    ├── memory_order_relaxed
    │
    └── NO CAS
          ↓
      CAS-based counter
          ↓
      compare_exchange_weak() in a while loop
          ↓
      acquire/release
          ↓
      lock-free stack
*/
