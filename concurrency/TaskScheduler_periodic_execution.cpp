/*
 Design a scheduler capable of managing periodic and one-time tasks. Tasks
 should be executed at their scheduled time. The scheduler should support adding
 and cancelling tasks.
                 Scheduler
                     |
      ---------------------------------
      |               |               |
Delayed Scheduler  Periodic Scheduler  Cron Scheduler
(execute once)    (repeat interval)   (calendar based)

I would use a min-heap ordered by execution time and a thread pool. Worker
threads sleep on a condition variable until either a new earlier task arrives or
the top task's execution time is reached. One-time tasks are removed after
execution, while periodic tasks are reinserted into the heap with their next
execution time. Cancellation is implemented via task IDs and lazy deletion using
a hash map. Scheduling and execution are O(log N), and cancellation is O(1).
*/

#include <chrono>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <mutex>
#include <thread>
#include <unordered_set>

using namespace std;

class TaskScheduler {

  struct Task {

    int task_id;
    function<void(void)> func;

    chrono::steady_clock::time_point executeAt;

    bool periodic = false; // by default, execute once
    int interval_milliseconds = 0;
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

  unordered_set<int> cancelledTasks;

  mutex mtx;
  condition_variable cv;
  bool stop;

  vector<thread> threads;
  int next_task_id;

public:
  TaskScheduler(int numThreads) : stop(false), next_task_id(1) {

    for (int thread_id = 1; thread_id <= numThreads; thread_id++)
      threads.emplace_back(&TaskScheduler::worker, this, thread_id);
  }

  ~TaskScheduler() { shutDown(); }

  void worker(int thread_id) {

    while (true) {

      Task task;

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
          cv.wait_until(lock, execution_time, [&]() {
            return stop == true;
          }); // ***avoid spurious wakeups, send back to waiting state unless
              // stop
              // == true

          continue; // ***go back to loop beginning to check if any new task
                    // scheduled with earlier execution start time
        }

        // 4. extract task ready to get executed
        task = pq.top();
        pq.pop();

        // 5.***check if a periodic task is already cancelled, do lazy deletion
        if (cancelledTasks.count(task.task_id))
          continue;
      }

      // 6.Execute the task
      cout << "Worker_thread_" << thread_id << " executing task "
           << task.task_id << endl;
      task.func();

      // 7.***reinsert periodic task and not cancelled yet
      if (task.periodic && cancelledTasks.count(task.task_id) == 0) {

        task.executeAt = chrono::steady_clock::now() +
                         chrono::milliseconds(task.interval_milliseconds);

        {
          lock_guard<mutex> lock(mtx);
          pq.push(task);
        }
        cv.notify_one(); // 8. ***thread waiting for new job submission
      }
    }
  }

  void cancelTask(int task_id) {

    lock_guard<mutex> lock(mtx);
    cancelledTasks.insert(task_id);
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

  int schedule(function<void(void)> func, int delayInMillisecond,
               bool periodic = false, int interval_milliseconds = 0) {

    int task_id = next_task_id++;

    auto execution_time =
        chrono::steady_clock::now() + chrono::milliseconds(delayInMillisecond);

    { // critical section
      lock_guard<mutex> lock(mtx);
      pq.push({task_id, func, execution_time, periodic, interval_milliseconds});
    }
    cv.notify_one();

    return task_id;
  }
};

int main() {

  TaskScheduler scheduler(3);

  // One-time task
  scheduler.schedule([&]() { cout << "One time task executed\n"; }, 2000);

  // Periodic task
  int heartbeat =
      scheduler.schedule([&]() { cout << "Heartbeat\n"; }, 1000, true, 3000);

  // Another one-time task
  scheduler.schedule([&]() { cout << "Backup completed\n"; }, 5000);

  this_thread::sleep_for(
      chrono::seconds(10)); // let the tasks scheduled and executed

  cout << "Cancelling Heartbeat\n";
  scheduler.cancelTask(heartbeat);

  this_thread::sleep_for(chrono::seconds(2));

  return 0;
}
/*
 Worker_thread_2 executing task 2
Heartbeat
Worker_thread_1 executing task 1
One time task executed
Worker_thread_2 executing task 2
Heartbeat
Worker_thread_2 executing task 3
Backup completed
Worker_thread_2 executing task 2
Heartbeat
Cancelling Heartbeat
*/
