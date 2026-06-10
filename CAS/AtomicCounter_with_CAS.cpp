/*
Implement CAS-based Atomic Counter using:
std::atomic
compare_exchange_weak
CAS Retry loop to handle failure
*/
#include <atomic>
#include <iostream>
#include <thread>

using namespace std;

class AtomicCounter {

  atomic<int> count{0};

public:
  void increment() {
    // count.fetch_add(1, std::memory_order_relaxed);
    int old = count.load(std::memory_order_relaxed);

    while (!count.compare_exchange_weak(old, old + 1, std::memory_order_relaxed,
                                        std::memory_order_relaxed))
      ;
    // CAS failed, old has been updated with current value and Retry in a loop
    /* conceptually, The comparison and update happens atomically.
     if (count == old)  //Success
        count = old + 1;
    else                //Failure
        old = count;
     */
  }

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
Strategy:
1. load the current counter value into old.
2. repeatedly attempt a CAS from old to old + 1.
[If another thread changes the counter before my CAS, the operation fails
and updates old with the current value, so retry.
compare_exchange_weak can also fail spuriously, which is another reason
we use a retry loop.]
Since this counter does not synchronize any other data, use
memory_order_relaxed.

1. Why std::atomic?
Because multiple threads concurrently modify count. A normal int would cause a
data race → undefined behavior.

2. Why do we need the retry loop?
Imagine: count = 10
Thread A: old = 10
Thread B: old = 10

A: CAS(10 → 11)  ✓ succeeds

B: CAS(10 → 11)  ✗ fails;
Thread B's old is automatically updated: old = 11 [automatically replaced
with actual atomic value]
Then the loop retries:CAS(11 → 12) ✓

So both increments are preserved: 10 → 11 → 12

3. compare_exchange_weak() may fail spuriously:
Expected == actual
        ↓
CAS can still report failure. That's why it naturally belongs inside a retry
loop. while (!count.compare_exchange_weak(...)) {}

For a single CAS attempt where we do not want spurious failure, use:
compare_exchange_strong()

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
