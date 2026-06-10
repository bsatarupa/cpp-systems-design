#include <condition_variable>
#include <iostream>
#include <thread>

using namespace std;

template <typename T>

class BlockingCircularBuffer {

  vector<T> buffer;

  int capacity, count;
  int head, tail;

  mutex mtx;
  condition_variable not_empty, not_full;

public:
  BlockingCircularBuffer(int size)
      : capacity(size), buffer(size), head(0), tail(0), count(0) {}

  void push(T item) {

    unique_lock<mutex> lock(mtx);

    not_full.wait(lock,
                  [&]() { return count < capacity; }); // wait if buffer is full

    buffer[tail] = item;
    tail = (tail + 1) % capacity;
    count++;

    not_empty.notify_one(); // wake 1 waiting consumer
  }

  T pop() {

    unique_lock<mutex> lock(mtx);

    not_empty.wait(lock, [&]() { return count > 0; }); // wait if buffer is
                                                       // empty

    T item = buffer[head];
    head = (head + 1) % capacity;
    count--;

    not_full.notify_one(); // wake 1 waiting producer

    return item;
  }
};

mutex print_mtx;

void producer(BlockingCircularBuffer<int> &Q, int start) {

  for (int i = 0; i < 5; i++) {

    int val = start + i;
    Q.push(val);

    { // critical section
      lock_guard<mutex> lock(print_mtx);
      cout << "Produced : " << val << endl;
    }
  }
}

void consumer(BlockingCircularBuffer<int> &Q) {

  for (int i = 0; i < 5; i++) {

    int val = Q.pop();

    { // critical section
      lock_guard<mutex> lock(print_mtx);
      cout << "Consumed : " << *val << endl;
    }
  }
}

int main() {

  BlockingCircularBuffer<int> bq(3);

  thread p1(producer, ref(bq), 100);
  thread c1(consumer, ref(bq));

  p1.join();
  bq.close(); // unblock consumer after producer is done
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
