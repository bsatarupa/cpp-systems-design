#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <thread>

using namespace std;

class ReadWriteLock {

  mutex mtx;
  condition_variable cv;

  int activeReaders = 0;
  bool anyActiveWriter = false;

public:
  void acquire_read_lock() {

    unique_lock<mutex> lock(mtx);
    cv.wait(lock, [&]() { return anyActiveWriter == false; });

    activeReaders++;
  }

  void release_read_lock() {

    unique_lock<mutex> lock(mtx);

    activeReaders--;
    if (activeReaders == 0)
      cv.notify_all(); // notify blocked writer if any
  }

  void acquire_write_lock() {

    unique_lock<mutex> lock(mtx);
    cv.wait(lock,
            [&]() { return activeReaders == 0 && anyActiveWriter == false; });

    anyActiveWriter = true;
  }

  void release_write_lock() {

    unique_lock<mutex> lock(mtx);

    anyActiveWriter = false;
    cv.notify_all();
  }
};

int main() {

  ReadWriteLock rwLock;
  int sharedData = 0;
  mutex print_mtx;

  vector<thread> readers, writers;

  for (int id = 1; id <= 5; id++) {

    readers.emplace_back([&, id]() {
      rwLock.acquire_read_lock();

      { // critical section
        lock_guard<mutex> lock(print_mtx);
        cout << "Reader : " << id << " reading value : " << sharedData << endl;
      }
      this_thread::sleep_for(chrono::milliseconds(100));

      rwLock.release_read_lock();
    });

    //***stagger reader arrival, to obtain interleaving outcome
    this_thread::sleep_for(chrono::milliseconds(300));

    writers.emplace_back([&, id]() {
      rwLock.acquire_write_lock();
      sharedData++;
      { // critical section
        lock_guard<mutex> lock(print_mtx);
        cout << "Writer : " << id << " updating value to : " << sharedData
             << endl;
      }
      this_thread::sleep_for(chrono::seconds(2));

      rwLock.release_write_lock();
    });
    //***stagger writer arrival, to obtain interleaving outcome
    this_thread::sleep_for(chrono::milliseconds(300));
  }

  for (auto &r : readers)
    r.join();

  for (auto &w : writers)
    w.join();

  return 0;
}
/*
Reader : 1 reading value : 0
Writer : 1 updating value to : 1
Writer : 2 updating value to : 2
Reader : 4 reading value : 2
Reader : 5 reading value : 2
Reader : 2 reading value : 2
Reader : 3 reading value : 2
Writer : 5 updating value to : 3
Writer : 4 updating value to : 4
Writer : 3 updating value to : 5
*/
