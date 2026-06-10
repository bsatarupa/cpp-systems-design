#include <chrono>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <mutex>
#include <thread>

using namespace std;

class TaskScheduler {

  struct Task {

    function<void(void)> func;
    chrono::steady_clock::time_point executeAt;
  };

  // min heap for prioritizing Tasks
  struct compare {

    bool operator()(const Task &a, const Task &b) const {
      // because of this last const, any value can't be modified inside function
      // body, makes function immutable
      return a.executeAt > b.executeAt;
    }
  };
  priority_queue<Task, vector<Task>, compare> pq;

  mutex mtx;
  condition_variable cv;
  bool stop;

  vector<thread> threads;

public:
  TaskScheduler(int numThreads) : stop(false) {

    for (int thread_id = 1; thread_id <= numThreads; thread_id++)
      threads.emplace_back(&TaskScheduler::worker, this, thread_id);
  }

  ~TaskScheduler() { shutDown(); }

  void worker(int thread_id) {

    while (true) {

      function<void(void)> function;

      { // critical section
        unique_lock<mutex> lock(mtx);

        // 1. wait until task is available
        cv.wait(lock, [&]() { return stop || !pq.empty(); });

        // 2.Graceful shutdown
        if (stop && pq.empty())
          return;

        // 3. ***Wait until execution time
        auto execution_time = pq.top().executeAt;
        if (chrono::steady_clock::now() < execution_time) {
          cv.wait_until(lock, execution_time);
          continue; //****VVIMP, to go back to function top
        }

        // get task ready to get executed
        function = pq.top().func;
        pq.pop();
      }

      // execute the task
      cout << "Worker_thread_" << thread_id << " executing task" << endl;
      function();
    }
  }

  void shutDown() {

    { // critical section
      lock_guard<mutex> lock(mtx);
      stop = true;
    }

    cv.notify_all();

    // always join after notify
    for (auto &t : threads) {

      if (t.joinable())
        t.join();
    }
  }

  void schedule(function<void(void)> func, int delayInMillisecond) {

    auto execution_time =
        chrono::steady_clock::now() + chrono::milliseconds(delayInMillisecond);

    { // critical section
      lock_guard<mutex> lock(mtx);
      pq.push({func, execution_time});
    }
    cv.notify_one();
  }
};

int main() {

  TaskScheduler scheduler(3);

  int task_id = 1;
  scheduler.schedule(function<void(void)>([&, task_id]() {
                       cout << "Task_" << task_id << " executed" << endl;
                     }),
                     1000);

  task_id++;
  scheduler.schedule(function<void(void)>([&, task_id]() {
                       cout << "Task_" << task_id << " executed" << endl;
                     }),
                     3000);

  task_id++;
  scheduler.schedule(function<void(void)>([&, task_id]() {
                       cout << "Task_" << task_id << " executed" << endl;
                     }),
                     2000);

  // allow all tasks to execute
  this_thread::sleep_for(chrono::seconds(5));

  return 0;
}
/*
Worker_thread_1 executing task
Task_1 executed
Worker_thread_2 executing task
Task_3 executed
Worker_thread_3 executing task
Task_2 executed
*/
