#include <atomic>
#include <functional>
#include <iostream>
#include <thread>

using namespace std;

class AtomicCounter {

  atomic<int> count;

public:
  AtomicCounter() : count(0) {}

  void worker() {
    for (int i = 1; i <= 100000; i++)
      increment();
  }

  int get() {

    return count.load(std::__1::memory_order_relaxed);
    // we only need atomicity but no thread synchronization
  }

  void increment() {

    count.fetch_add(1, std::__1::memory_order_relaxed);
    // already atomic, internally implementing CAS(compare-and-swap) loop
  }
};

int main() {

  AtomicCounter counter;

  vector<thread> threads;
  for (int id = 1; id <= 4; id++)
    threads.emplace_back(&AtomicCounter::worker, ref(counter));

  for (auto &t : threads)
    t.join();

  cout << "Final counter : " << counter.get() << endl;
  return 0;
}
// Final counter : 400000
