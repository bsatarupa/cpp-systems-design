#include <condition_variable>
#include <iostream>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <utility>

using namespace std;

template <typename T>

class MessageQueue {

  queue<T> Q;
  mutex mtx;
  condition_variable cv;

public:
  void send(T msg) {

    { // critical section
      lock_guard<mutex> lock(mtx);
      Q.push(std::move(msg)); // we transfer ownership/resources(pointer swap)
                              // instead of large data copying.
      // To avoid unnecessary copies when transferring messages into and out of
      // the queue.used in high throughput concurrent systems.
    }
    cv.notify_one();
  }

  T receive() {

    unique_lock<mutex> lock(mtx);
    cv.wait(lock, [&]() { return !Q.empty(); });

    T msg = std::move(Q.front());
    Q.pop();
    return msg;
  }

  // non-blocking receive(), return F if no msg is available in Q
  bool try_receive(T &out_msg) {

    unique_lock<mutex> lock(mtx);
    if (Q.empty())
      return false;

    out_msg = move(Q.front());
    Q.pop();
    return true;
  }
};

int main() {

  MessageQueue<string> mq;

  thread producer([&]() {
    for (int item = 1; item <= 5; item++) // per thread producing 5 items
      mq.send("Message_" + to_string(item));
  });

  thread consumer([&]() {
    for (int item = 1; item <= 5; item++)
      cout << "Received : " << mq.receive() << endl;
  });

  producer.join();
  consumer.join();

  return 0;
}
/*
Received : Message_1
Received : Message_2
Received : Message_3
Received : Message_4
Received : Message_5
*/
