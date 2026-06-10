#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>

using namespace std;

class Sequencer {

  int counter;
  mutex mtx, printMtx;

public:
  Sequencer() : counter(1) {}

  int next() {

    lock_guard<mutex> lock(mtx);

    return counter++;
  }

  void worker(int thread_id) {

    for (int i = 1; i <= 5; i++) {

      int val = next();

      { // critical section
        lock_guard<mutex> lock(printMtx);
        cout << "Thread_" << thread_id << " -> " << val << endl;
      }

      this_thread::sleep_for(chrono::milliseconds(100));
    }
  }
};

int main() {

  Sequencer seq;

  vector<thread> threads;
  for (int thread_id = 1; thread_id <= 3; thread_id++)
    threads.emplace_back(&Sequencer::worker, &seq, thread_id);

  for (auto &t : threads)
    t.join();

  return 0;
}
/*
Thread_1 -> 1
Thread_2 -> 2
Thread_3 -> 3
Thread_1 -> 4
Thread_3 -> 5
Thread_2 -> 6
Thread_1 -> 7
Thread_3 -> 8
Thread_2 -> 9
Thread_1 -> 10
Thread_3 -> 11
Thread_2 -> 12
Thread_2 -> 13
Thread_1 -> 14
Thread_3 -> 15
*/
