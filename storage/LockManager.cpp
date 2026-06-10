#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>

using namespace std;

enum LockType { SHARED, EXCLUSIVE };

class LockManager {

  struct LockInfo { // lockinfo per resource_id

    int activeReaders = 0;
    bool AnyActiveWriter = false;
    condition_variable cv; // per resource
  };

  unordered_map<string, LockInfo>
      lock_table; //<resource_id, LockInfo for individual resource>
  mutex mtx;

public:
  void acquire_shared_lock(const string &resource) {

    unique_lock<mutex> lock(mtx);

    LockInfo &info = lock_table[resource]; // lock_state of given resource
    info.cv.wait(lock, [&]() { return info.AnyActiveWriter == false; });

    info.activeReaders++;

    cout << "SHARED Lock acquired on Resource : " << resource << endl;
  }

  void release_shared_lock(const string &resource) {

    unique_lock<mutex> lock(mtx);

    LockInfo &info = lock_table[resource];

    info.activeReaders--;
    if (info.activeReaders == 0)
      info.cv.notify_all();

    cout << "SHARED Lock released from resource : " << resource << endl;
  }

  void acquire_exclusive_lock(const string &resource) {

    unique_lock<mutex> lock(mtx);

    LockInfo &info = lock_table[resource]; // lock_state of given resource
    info.cv.wait(lock, [&]() {
      return info.AnyActiveWriter == false && info.activeReaders == 0;
    });

    info.AnyActiveWriter = true;

    cout << "EXCLUSIVE Lock acquired on Resource : " << resource << endl;
  }

  void release_exclusive_lock(const string &resource) {

    unique_lock<mutex> lock(mtx);

    LockInfo &info = lock_table[resource];

    info.AnyActiveWriter = false;
    info.cv.notify_all();

    cout << "EXCLUSIVE Lock released from resource : " << resource << endl;
  }
};

int main() {

  LockManager lm;

  thread reader1([&]() {
    lm.acquire_shared_lock("row1");
    this_thread::sleep_for(chrono::seconds(2));
    lm.release_shared_lock("row1");
  });

  thread reader2([&]() {
    lm.acquire_shared_lock("row1");
    this_thread::sleep_for(chrono::seconds(2));
    lm.release_shared_lock("row1");
  });

  thread writer1([&]() {
    this_thread::sleep_for(chrono::milliseconds(500));

    lm.acquire_exclusive_lock("row1");
    this_thread::sleep_for(chrono::seconds(1));
    lm.release_exclusive_lock("row1");
  });

  reader1.join();
  reader2.join();
  writer1.join();

  return 0;
}
/*
SHARED Lock acquired on Resource : row1
SHARED Lock acquired on Resource : row1
SHARED Lock released from resource : row1
SHARED Lock released from resource : row1
EXCLUSIVE Lock acquired on Resource : row1
EXCLUSIVE Lock released from resource : row1
*/
