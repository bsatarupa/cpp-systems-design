#include <chrono>
#include <condition_variable>
#include <deque>
#include <iostream>
#include <mutex>
#include <optional>
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

  bool try_push(const T &val, chrono::milliseconds timeout) {

    { // critical section
      unique_lock<mutex> lock(mtx);

      if (not_full.wait_for(lock, timeout,
                            [&]() { return dq.size() < Tcapacity; }) == 0) {
        return false; // timeout
      }

      dq.push_back(val);

      cout << "Produced : " << val << endl;
    }
    not_empty.notify_one(); // Producer notifies waiting consumer
    return true;
  }

  void push(const T &val) {

    { // critical section
      unique_lock<mutex> lock(mtx);

      not_full.wait(lock, [&]() { return dq.size() < Tcapacity; });

      dq.push_back(val);

      cout << "Produced : " << val << endl;
    }
    not_empty.notify_one(); // Producer notifies waiting consumer
  }

  optional<T> try_pop(chrono::milliseconds timeout) {

    T val;
    { // critical section
      unique_lock<mutex> lock(mtx);

      if (not_empty.wait_for(lock, timeout, [&]() {
            return dq.empty() == false;
          }) == 0) {    // Consumer waits if Q is empty
        return nullopt; // timeout
      }

      val = dq.front();
      dq.pop_front();
    }

    not_full.notify_one();
    return val;
  }

  T pop() {

    T val;
    { // critical section
      unique_lock<mutex> lock(mtx);

      not_empty.wait(lock, [&]() {
        return dq.empty() == false;
      }); // Consumer waits if Q is empty

      val = dq.front();
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

      // timeout
      if (bq.try_push(i, chrono::milliseconds(100)))
        cout << "Produced before timeout : " << i << endl;
      else
        cout << "Producer TIMED OUT for : " << i << endl;
    }
  });

  thread consumer([&]() {
    for (int i = 1; i <= 5; i++) {

      this_thread::sleep_for(
          chrono::seconds(2)); // Consumer is slower than Producer
      int val = bq.pop();
      cout << "Consumed : " << val << endl;

      // timeout
      auto timeout_val = bq.try_pop(chrono::milliseconds(150));
      if (timeout_val)
        cout << "Consumed before timeout : " << *timeout_val << endl;
      else
        cout << "Consumer TIMED OUT!" << endl; // got nullopt
    }
  });

  producer.join();
  consumer.join();

  return 0;
}
/*
Trying to produce : 1
Produced : 1
Successfully produced : 1
Produced : 1
Produced before timeout : 1
Trying to produce : 2
Produced : 2
Successfully produced : 2
Producer TIMED OUT for : 2
Trying to produce : 3
Consumed : Produced : 3
Successfully produced : 3
1
Consumed before timeout : 1
Produced : 3
Produced before timeout : 3
Trying to produce : 4
Consumed : 2Produced :
4
Successfully produced : 4
Consumed before timeout : 3
Produced : 4
Produced before timeout : 4
Trying to produce : 5
Consumed : Produced : 35
Successfully produced : 5

Consumed before timeout : 4
Produced : 5
Produced before timeout : 5
Consumed : 4
Consumed before timeout : 5
Consumed : 5
Consumer TIMED OUT!
*/
