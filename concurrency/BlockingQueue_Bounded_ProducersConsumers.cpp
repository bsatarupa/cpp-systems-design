#include <chrono>
#include <condition_variable>
#include <deque>
#include <iostream>
#include <mutex>
#include <thread>

using namespace std;

template <typename T>

class BlockingQueue_Bounded {

  mutex mtx;
  condition_variable not_full, not_empty;

  deque<T> dq;
  size_t Tcapacity;

public:
  BlockingQueue_Bounded(size_t capacity) : Tcapacity(capacity) {}

  void push(T val) {

    { // critical section
      unique_lock<mutex> lock(mtx);

      not_full.wait(lock, [&]() { return dq.size() < Tcapacity; });

      dq.push_back(move(val)); // Faster, no deep copy, only the internal buffer
                               // ownership is transferred.

      cout << "Produced : " << val << endl;
    }
    not_empty.notify_one(); // Producer notifies waiting consumer
  }

  T pop() {

    T val;
    { // critical section
      unique_lock<mutex> lock(mtx);

      not_empty.wait(lock, [&]() {
        return dq.empty() == false;
      }); // Consumer waits if Q is empty

      val = move(dq.front());
      dq.pop_front();
    }

    not_full.notify_one();
    return val;
  }
};

int main() {

  BlockingQueue_Bounded<int> bq(3);

  thread producer([&]() {
    for (int i = 1; i <= 5; i++) {
      cout << "Trying to produce : " << i << endl;

      this_thread::sleep_for(chrono::milliseconds(500)); // Periodic production
      bq.push(i);

      cout << "Successfully produced : " << i << endl;
    }
  });

  thread consumer([&]() {
    for (int i = 1; i <= 5; i++) {

      this_thread::sleep_for(
          chrono::seconds(2)); // Consumer is slower than Producer
      int val = bq.pop();
      cout << "Consumed : " << val << endl;
    }
  });

  producer.join();
  consumer.join();

  return 0;
}

/*
satarupa@Satarupas-MacBook-Air cpp_threads % ./a.out
Trying to produce : 1
Produced : 1
Successfully produced : 1
Trying to produce : 2
Produced : 2
Successfully produced : 2
Trying to produce : 3
Produced : 3
Successfully produced : 3
Trying to produce : 4
Consumed : 1
Produced : 4
Successfully produced : 4
Trying to produce : 5
Consumed : 2Produced :
5
Successfully produced : 5
Consumed : 3
Consumed : 4
Consumed : 5
*/
