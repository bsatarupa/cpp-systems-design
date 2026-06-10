/*
A cyclic barrier is a synchronization mechanism where: Multiple threads wait for
each other at a common point before continuing. Think of it like: "Nobody
proceeds until everybody arrives." A cyclic barrier synchronizes phases of
execution across threads. Barrier Action: When the LAST thread reaches the
barrier, execute a callback/function once per generation, then release all
waiting threads.
*/

#include <chrono>
#include <condition_variable>
#include <iostream>
#include <thread>

using namespace std;

class CyclicBarrier {

  mutex mtx, print_mtx;
  condition_variable cv;

  function<void(void)>
      barrierAction; // function to execute when ALL threads reach the barrier

  int count;
  int generation; // stores which cycle this thread belongs to.
  int parties;    // total #threads to reach the barrier

public:
  CyclicBarrier(int n, function<void(void)> action)
      : parties(n), count(n), generation(0), barrierAction(action) {}

  void await() {

    unique_lock<mutex> lock(mtx);

    // a thread has arrived
    int ongoing_generation = generation;

    count--;

    if (count == 0) { // ALL thread has arrived, only execute when the LAST
                      // thread arrives

      // execute callback once
      if (barrierAction)
        barrierAction();

      // reset generations and count
      count = parties;
      generation++; // signals end of current generation/cycle is reached

      // wake up all waiting threads
      cv.notify_all();
    } else { // LAST thread is yet to arrive, for every other threads

      cv.wait(lock, [&]() { return generation != ongoing_generation; });
    }
  }

  void worker(int thread_id) {

    {
      lock_guard<mutex> lock(print_mtx);
      cout << "Thread_" << thread_id << " before barrier" << endl;
    }

    // wait for simulation
    this_thread::sleep_for(
        chrono::milliseconds(100 * thread_id)); // adding random sleeping time
    await();

    {
      lock_guard<mutex> lock(print_mtx);
      cout << "Thread_" << thread_id << " after barrier" << endl;
    }
  }
};

int main() {

  CyclicBarrier barrier(3,
                        [&]() { cout << "---All THREADS arrived---" << endl; });

  vector<thread> threads;
  for (int id = 1; id <= 3; id++)
    threads.emplace_back(&CyclicBarrier::worker, ref(barrier), id);

  for (auto &t : threads)
    t.join();

  return 0;
}
/*
Thread_1 before barrier
Thread_2 before barrier
Thread_3 before barrier
---All THREADS arrived---
Thread_3 after barrier
Thread_1 after barrier
Thread_2 after barrier
*/
