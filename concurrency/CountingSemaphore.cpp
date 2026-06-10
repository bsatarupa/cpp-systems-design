#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
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

mutex printMtx;
void worker(CountingSemaphore &sem, int thread_id) {

  sem.wait();

  {
    lock_guard<mutex> lock(printMtx);
    cout << "Thread_" << thread_id << " entered Critical section" << endl;
  }

  this_thread::sleep_for(chrono::seconds(2));

  {
    lock_guard<mutex> lock(printMtx);
    cout << "Thread_" << thread_id << " leaving Critical section" << endl;
  }

  sem.signal();
}

int main() {

  CountingSemaphore sem(2);

  mutex printMtx;
  vector<thread> threads;

  for (int thread_id = 1; thread_id <= 5; thread_id++)

    threads.emplace_back(&worker, ref(sem), thread_id);
  /*
      threads.emplace_back([&, thread_id] {

        sem.wait();

        {
          lock_guard<mutex> lock(printMtx);
          cout << "Thread_" << thread_id << " entered Critical section" << endl;
        }

        this_thread::sleep_for(chrono::seconds(2));

        {
          lock_guard<mutex> lock(printMtx);
          cout << "Thread_" << thread_id << " leaving Critical section" << endl;
        }

        sem.signal();
      });
  */
  for (auto &t : threads)
    t.join();

  return 0;
}
/*
Thread_1 entered Critical section
Thread_2 entered Critical section
Thread_1 leaving Critical section
Thread_3 entered Critical section
Thread_2 leaving Critical section
Thread_4 entered Critical section
Thread_3 leaving Critical section
Thread_4 leaving Critical section
Thread_5 entered Critical section
Thread_5 leaving Critical section
*/
