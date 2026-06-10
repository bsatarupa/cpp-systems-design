/*
Design a thread pool that accepts tasks and executes them using a
fixed number of worker threads. Tasks should be queued when all
workers are busy. The pool should support graceful shutdown.

Thread pools improve throughput and reduce thread creation overhead by
reusing a fixed set of worker threads to execute queued tasks.

ThreadPool
 |
 |---- constructor
 |       |
 |       +--- hidden lambda
 |
 |---- submit()
 |
 |---- destructor()*/
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

using namespace std;

class ThreadPool {

  mutex mtx;
  condition_variable cv;

  queue<function<void(void)>> Qtasks; // enqueues functions to be executed
  bool stop;
  vector<thread> worker_threads;

public:
  ThreadPool(int numThreads) : stop(false) {

    for (int i = 1; i <= numThreads; i++) {

      worker_threads.emplace_back([&]() {
        while (true) {

          function<void(void)> task;

          { // critical section
            unique_lock<mutex> lock(mtx);
            cv.wait(lock, [&]() { return stop == true || !Qtasks.empty(); });

            // graceful shutdown
            if (stop == true && Qtasks.empty())
              return;

            task = Qtasks.front();
            Qtasks.pop();
          }

          // executed popped out task
          task();
        }
      });
    }
  }

  void submit(function<void(void)> task) {

    {
      lock_guard<mutex> lock(mtx);
      Qtasks.push(task);
    }
    cv.notify_one();
  }

  ~ThreadPool() {

    {
      lock_guard<mutex> lock(mtx);
      stop = true;
    }

    cv.notify_all();

    for (auto &w : worker_threads) {

      if (w.joinable())
        w.join();
    }
  }
};

int main() {

  ThreadPool pool(3);

  mutex printMutex;

  for (int task_id = 1; task_id <= 10; task_id++) { // similar to producer_id

    pool.submit([&, task_id]() {
      { // critical section
        lock_guard<mutex> printLock(printMutex);

        cout << "Task : " << task_id
             << " executed by thread : " << this_thread::get_id() << endl;
      }

      this_thread::sleep_for(chrono::seconds(2));
    });
  }

  return 0;
}
/*
Task : 1 executed by thread : 0x16f9eb000
Task : 2 executed by thread : 0x16f8d3000
Task : 3 executed by thread : 0x16f95f000
Task : 4 executed by thread : 0x16f9eb000
Task : 5 executed by thread : 0x16f8d3000
Task : 6 executed by thread : 0x16f95f000
Task : 7 executed by thread : 0x16f9eb000
Task : 8 executed by thread : 0x16f95f000
Task : 9 executed by thread : 0x16f8d3000
Task : 10 executed by thread : 0x16f9eb000
*/
