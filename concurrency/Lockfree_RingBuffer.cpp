#include <atomic>
#include <iostream>
#include <thread>

using namespace std;

template <typename T>

class Lockfree_RingBuffer {

  vector<T> buffer;

  atomic<int> head, tail;

  int capacity;

  /* NOT Needed
  int count;
  mutex mtx;
  condition_variable not_empty, not_full;
  */
public:
  Lockfree_RingBuffer(int size)
      : capacity(size), buffer(size), head(0), tail(0) {}

  bool push(T item) {

    int curr_tail = tail.load(
        memory_order_relaxed); // just atomic read, no synchronization needed
    int next_tail = (curr_tail + 1) % capacity;

    // return if buffer is full in ring buffer
    if (next_tail ==
        head.load(memory_order_acquire)) // ensuring order/synchronization
      return false;

    buffer[curr_tail] = item;

    // publish write
    tail.store(next_tail, memory_order_release);

    return true;
  }

  bool pop(T &item) {

    int curr_head = head.load(memory_order_relaxed);
    int next_head = (curr_head + 1) % capacity;

    // return if buffer is empty
    if (curr_head == tail.load(memory_order_acquire))
      return false;

    item = buffer[curr_head];

    // publish consumption
    head.store(next_head, memory_order_release);

    return true;
  }
};

mutex print_mtx;

void producer(Lockfree_RingBuffer<int> &Q, int start) {

  for (int i = 0; i < 5; i++) {

    int val = start + i;
    while (Q.push(val) == false) {
    };

    { // critical section
      lock_guard<mutex> lock(print_mtx);
      cout << "Produced : " << val << endl;
    }
  }
}

void consumer(Lockfree_RingBuffer<int> &Q) {

  for (int i = 0; i < 5; i++) {

    int val;
    while (Q.pop(val) == false) {
    };

    { // critical section
      lock_guard<mutex> lock(print_mtx);
      cout << "Consumed : " << val << endl;
    }
  }
}

int main() {

  Lockfree_RingBuffer<int> bq(3);

  thread p1(producer, ref(bq), 100);
  thread c1(consumer, ref(bq));

  p1.join();
  c1.join();

  return 0;
}
/*
Produced : 100
Produced : 101
Produced : 102
Produced : 103
Consumed : 100
Consumed : 101
Consumed : 102
Consumed : 103
Produced : 104
Consumed : 104
*/
