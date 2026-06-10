#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

using namespace std;

class CountingSemaphore {

  int count;
  mutex mtx;
  condition_variable cv;
  // bool available;     //for binary semaphore

public:
  CountingSemaphore(int n) : count(n) {} // available(is_available)

  void wait() {

    unique_lock<mutex> lock(mtx);
    cv.wait(lock, [&]() { return count > 0; });
    // cv.wait(lock, [&]() { return available == true; });

    count--;
    // available = false;
  }

  void signal() {

    { // critical section
      lock_guard<mutex> lock(mtx);
      count++;
      // available = true;
    }
    cv.notify_one();
  }
};

// Shared buffer
queue<int> buffer;
const int BUFFER_SIZE = 2;
mutex bufferMtx;

void producer(int producer_id, CountingSemaphore &fullSlots,
              CountingSemaphore &emptySlots) {

  for (int item = 1; item <= 5; item++) {

    emptySlots.wait(); // wait for an empty slot

    {
      // add item to buffer
      lock_guard<mutex> lock(bufferMtx);
      buffer.push(item);
      cout << "Producer_" << producer_id << " produced item : " << item << endl;
    }

    fullSlots.signal(); // signal full slot available

    this_thread::sleep_for(chrono::milliseconds(500));
  }
}

void consumer(int consumer_id, CountingSemaphore &fullSlots,
              CountingSemaphore &emptySlots) {

  for (int item = 1; item <= 5; item++) {

    fullSlots.wait(); // wait for an full slot

    {
      // add item to buffer
      lock_guard<mutex> lock(bufferMtx);
      int consumed_item = buffer.front();
      buffer.pop();
      cout << "Consumer_" << consumer_id << " consumed item : " << consumed_item
           << endl;
    }

    emptySlots.signal(); // signal empty slot available

    this_thread::sleep_for(chrono::milliseconds(800));
  }
}

int main() {

  CountingSemaphore emptySlots(BUFFER_SIZE), fullSlots(0);

  // add producer consumer on semaphore
  vector<thread> threads;

  threads.emplace_back(&producer, 1, ref(fullSlots), ref(emptySlots));

  threads.emplace_back(&consumer, 1, ref(fullSlots), ref(emptySlots));

  for (auto &t : threads)
    t.join();

  return 0;
}
/*
Producer_1 produced item : 1
Consumer_1 consumed item : 1
Producer_1 produced item : 2
Consumer_1 consumed item : 2
Producer_1 produced item : 3
Producer_1 produced item : 4
Consumer_1 consumed item : 3
Producer_1 produced item : 5
Consumer_1 consumed item : 4
Consumer_1 consumed item : 5
*/
