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
  mutex printmtx;

  vector<thread> producers, consumers;
  const int producer_count = 10, consumer_count = 10, items_per_producer = 5;

  for (int p_id = 1; p_id <= producer_count; p_id++) {

    producers.emplace_back([&, p_id]() {
      for (int j = 1; j <= items_per_producer; j++) {
        this_thread::sleep_for(
            chrono::milliseconds(500)); // Periodic production

        int val = p_id * 100 + j;
        bq.push(val);

        lock_guard<mutex> lock(printmtx);
        cout << "Producer : " << p_id << " produced value : " << val << endl;
      }
    });
  }

  for (int c_id = 1; c_id <= consumer_count; c_id++) {

    consumers.emplace_back([&, c_id]() {
      for (int j = 1; j <= 5; j++) {
        this_thread::sleep_for(
            chrono::milliseconds(500)); // Periodic production

        int val = bq.pop();

        lock_guard<mutex> lock(printmtx);
        cout << "Consumer : " << c_id << " consumed value : " << val << endl;
      }
    });
  }

  for (auto &p : producers)
    p.join();

  for (auto &c : consumers)
    c.join();

  return 0;
}
/*
Producer : 1 produced value : 101
Producer : 7 produced value : 701
Producer : 8 produced value : 801
Producer : 9 produced value : 901
Producer : 10 produced value : 1001
Consumer : 1 consumed value : 101
Consumer : 2 consumed value : 701
Consumer : 3 consumed value : 801
Producer : 4 produced value : 401
Producer : 6 produced value : 601
Consumer : 5 consumed value : 901
Consumer : 4 consumed value : 1001
Consumer : 7 consumed value : 401
Producer : 5 produced value : 501
Consumer : 6 consumed value : 601
Consumer : 9 consumed value : 501
Producer : 2 produced value : 201
Consumer : 8 consumed value : 201
Consumer : 10 consumed value : 301
Producer : 3 produced value : 301
Producer : 7 produced value : 702
Producer : 2 produced value : 202
Consumer : 3 consumed value : 702
Producer : 1 produced value : 102
Producer : 9 produced value : 902
Producer : 4 produced value : 402
Consumer : 6 consumed value : 402
Producer : 10 produced value : 1002
Consumer : 4 consumed value : 602
Consumer : 10 consumed value : 102
Consumer : 5 consumed value : 802
Producer : 8 produced value : 802
Consumer : 7 consumed value : 1002
Producer : 6 produced value : 602
Producer : 5 produced value : 502
Consumer : 9 consumed value : 502
Consumer : 2 consumed value : 202
Consumer : 1 consumed value : 902
Producer : 3 produced value : 302
Consumer : 8 consumed value : 302
Producer : 9 produced value : 903
Producer : 7 produced value : 703
Consumer : 3 consumed value : 703
Producer : 4 produced value : 403
Consumer : 1 consumed value : 403
Producer : 1 produced value : 103
Consumer : 10 consumed value : 903
Producer : 8 produced value : 803
Producer : 10 produced value : 1003
Consumer : 5 consumed value : 103
Producer : 5 produced value : 503
Consumer : 7 consumed value : 1003
Consumer : 2 consumed value : 803
Consumer : 9 consumed value : 503
Producer : 6 produced value : 603
Consumer : 8 consumed value : 603
Producer : 2 produced value : 203
Consumer : 6 consumed value : 203
Producer : 3 produced value : 303
Consumer : 4 consumed value : 303
Consumer : 5 consumed value : 104
Consumer : 1 consumed value : 804
Producer : 6 produced value : 604
Producer : 7 produced value : 704
Producer : 2 produced value : 204
Producer : 9 produced value : 904
Producer : 1 produced value : 104
Producer : 4 produced value : 404
Producer : 8 produced value : 804
Consumer : 10 consumed value : 204
Consumer : 8 consumed value : 304
Consumer : 7 consumed value : 904
Consumer : 9 consumed value : 404
Producer : 10 produced value : 1004
Consumer : 2 consumed value : 1004
Producer : 5 produced value : 504
Consumer : 4 consumed value : 604
Consumer : 6 consumed value : 504
Consumer : 3 consumed value : 704
Producer : 3 produced value : 304
Consumer : 7 consumed value : 405
Consumer : 4 consumed value : 705
Consumer : 5 consumed value : 205
Producer : 5 produced value : 505
Producer : 10 produced value : 1005
Producer : 9 produced value : 905
Producer : 1 produced value : 105
Producer : 7 produced value : 705
Consumer : 6 consumed value : 1005
Producer : 8 produced value : 805
Consumer : 2 consumed value : 605
Consumer : 1 consumed value : 505
Producer : 3 produced value : 305
Consumer : 8 consumed value : 905
Consumer : 9 consumed value : 105
Consumer : 3 consumed value : 805
Consumer : 10 consumed value : 305
Producer : 2 produced value : 205
Producer : 4 produced value : 405
Producer : 6 produced value : 605
*/
