#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>

using namespace std;

mutex m1, m2;
/*
Deadlock: because of Circular Wait
T1: m1 -> m2
T2: m2 -> m1
*/

void deadlockThread_1() {

  lock_guard<mutex> lock1(m1);
  this_thread::sleep_for(chrono::milliseconds(100));
  lock_guard<mutex> lock2(m2);
  cout << "Thread_1 done" << endl;
}

void deadlockThread_2() {

  lock_guard<mutex> lock2(m2);
  this_thread::sleep_for(chrono::milliseconds(100));
  lock_guard<mutex> lock1(m1);
  cout << "Thread_2 done" << endl;
}

/*
Fix:
1. defer_lock = create lock object, but do not lock immediately
2. std::lock = safely locks multiple mutexes, avoids deadlocks internally
*/
void safeThread_1() {

  unique_lock<mutex> lock1(m1, defer_lock);
  unique_lock<mutex> lock2(m2, defer_lock);

  lock(lock1, lock2); // deadlock-free locking
  cout << "Safe execution_1" << endl;
}

void safeThread_2() {

  unique_lock<mutex> lock2(m2, defer_lock);
  unique_lock<mutex> lock1(m1, defer_lock);

  lock(lock2, lock1); // deadlock-free locking
  cout << "Safe execution_2" << endl;
}

int main() {

  // uncomment to see deadlock
  /*
  cout << "Simulating Deadlock..." << endl;
  thread t1(deadlockThread_1);
  thread t2(deadlockThread_2);

  t1.join();
  t2.join();
  */

  // deadlock-free version
  thread t3(safeThread_1);
  thread t4(safeThread_2);

  t3.join();
  t4.join();

  return 0;
}
/*
Safe execution_1
Safe execution_2
*/
