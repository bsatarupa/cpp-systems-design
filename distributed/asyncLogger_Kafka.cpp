/*
 * Async Logger

Goal: Async logging decouples application execution from slow disk IO using a
background consumer thread. Avoids blocking application threads on slow disk IO.

Architecture:

Producer Threads
       ↓
Thread-safe Queue
       ↓
Condition Variable
       ↓
Background Worker Thread
       ↓
Append-only Log File

Thread Types:

1. Producer Threads (t1, t2)
- application/request threads
- call logger.log(...)
- only push messages into in-memory queue
- do NOT perform disk IO
- return quickly

2. WorkerThread
- continuously waits for logs
- wakes up using condition_variable
- dequeues messages
- writes logs to file
- flushes periodically to file (OS/page cache)

Why Async?
- disk IO is slow
- memory queue operations are fast
- application threads avoid blocking
- improves throughput and latency

Important Synchronization:
- mutex protects shared queue
- condition_variable avoids busy waiting
- graceful shutdown ensures no logs lost

Key Optimization:
- disk IO happens outside lock
- minimizes lock contention

Real Systems Using Similar Design:
- Kafka
- RocksDB WAL
- nginx logging
- databases
- distributed tracing systems

Core Pattern:
multi-producer + single-consumer
producer-consumer architecture
*/

#include <condition_variable>
#include <fstream>
#include <ios>
#include <iostream>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

using namespace std;

class AsyncLogger {

  condition_variable cv;
  mutex mtx;

  bool stop = false;
  queue<string> logs;

  thread workerThread;
  ofstream ofs;

public:
  AsyncLogger(const string &log_file) {

    ofs.open(log_file, ios::app);

    // keep on running process_logs() as soon as logfile is created
    workerThread = thread(&AsyncLogger::process_logs, this);
  }

  void log(const string &msg) { // keeps data in in-memory queue

    { // critical section
      lock_guard<mutex> lock(mtx);
      logs.push(msg);
    }
    cv.notify_one();
  }

  void process_logs() { // flush all queue data from in-memory queue to OS/ page
                        // cache

    while (true) {

      string msg;

      { // critical section
        unique_lock<mutex> lock(mtx);
        cv.wait(lock, [&]() { return !logs.empty() || stop == true; });

        // graceful shutdown
        if (logs.empty() && stop)
          break;

        msg = logs.front();
        logs.pop();
      }

      // IO outside critical section and lock
      ofs << msg << endl;
      ofs.flush();
      // flushed to OS or page cache, fsync() only flushes to disk periodically

      cout << "Logged msg : " << msg << endl;
    }
  }

  ~AsyncLogger() {

    { // critical section
      lock_guard<mutex> lock(mtx);
      stop = true;
    }

    cv.notify_all();

    if (workerThread.joinable())
      workerThread.join();

    ofs.close();
  }
};

int main() {

  AsyncLogger logger("app.log");

  thread t1([&]() {
    for (int i = 1; i <= 5; i++)
      logger.log("Thread1 log : " + to_string(i));
  });

  thread t2([&]() {
    for (int i = 1; i <= 5; i++)
      logger.log("Thread2 log : " + to_string(i));
  });

  t1.join();
  t2.join();

  return 0;
}
/*
Logged msg : Thread1 log : 1
Logged msg : Thread2 log : 1
Logged msg : Thread2 log : 2
Logged msg : Thread2 log : 3
Logged msg : Thread2 log : 4
Logged msg : Thread2 log : 5
Logged msg : Thread1 log : 2
Logged msg : Thread1 log : 3
Logged msg : Thread1 log : 4
Logged msg : Thread1 log : 5
*/
