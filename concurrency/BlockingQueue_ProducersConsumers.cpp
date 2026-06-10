#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>

using namespace std;

template <typename T>

class BlockingQueue {

  mutex mtx;
  condition_variable cv;
  queue<T> Q;

public:
  void push(const T &val) {

    { // critical section
      lock_guard<mutex> lock(mtx);
      Q.push(val);
      cout << "Produced : " << val << endl;
    }
    cv.notify_one(); // Producer notifies waiting consumer
  }

  T pop() {

    unique_lock<mutex> lock(mtx);
    cv.wait(lock, [&]() {
      return Q.empty() == false;
    }); // Consumer waits if Q is empty

    T val = Q.front();
    Q.pop();
    return val;
  }
};

int main() {

  BlockingQueue<int> bq;

  thread producer([&]() {
    for (int i = 1; i <= 5; i++) {

      this_thread::sleep_for(chrono::milliseconds(500)); // Periodic production
      bq.push(i);
    }
  });

  thread consumer([&]() {
    for (int i = 1; i <= 5; i++) {

      int val = bq.pop();
      cout << "Consumed : " << val << endl;
    }
  });

  producer.join();
  consumer.join();

  return 0;
}
