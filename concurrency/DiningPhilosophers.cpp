#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>

using namespace std;

class DiningPhilosophers {

  vector<mutex> forks; // Each fork is a shared resource. Only one philosopher
                       // can hold a fork at a time.
  mutex printMtx;

public:
  DiningPhilosophers(int NUM_PHILOSOPHERS) : forks(NUM_PHILOSOPHERS) {}

  void philosopher(int philosopher_id) {

    int leftFork = philosopher_id; // any philosopher needs TWO forks: left fork
                                   // + right fork
    int rightFork = (philosopher_id + 1) % forks.size();

    for (int meal = 1; meal <= 3;
         meal++) { // 3 rounds for each philosopher, to simulate repetition

      think(philosopher_id);

      eat(philosopher_id, leftFork, rightFork);
    }
  }

  void think(int philosopher_id) {

    {
      lock_guard<mutex> printLock(printMtx);
      cout << "Philosopher_" << philosopher_id << " is THINKING..." << endl;
    }
    this_thread::sleep_for(chrono::milliseconds(500));
  }

  void eat(int philosopher_id, int leftFork, int rightFork) {

    unique_lock<mutex> leftLock(forks[leftFork], defer_lock);
    // defer_lock creates lock object/wrapper but does not lock immediately
    unique_lock<mutex> rightLock(forks[rightFork], defer_lock);

    lock(leftLock, rightLock);
    // safely locks(acquires) BOTH mutexes(forks), avoids deadlock(circular
    // wait) and blocks until both available

    {
      lock_guard<mutex> printLock(printMtx);
      cout << "Philosopher_" << philosopher_id << " is EATING!" << endl;
    }

    this_thread::sleep_for(chrono::milliseconds(1000));

    {
      lock_guard<mutex> printLock(printMtx);
      cout << "Philosopher_" << philosopher_id << " finished EATING." << endl;
    }
  }
};

int main() {

  const int NUM_PHILOSOPHERS = 5;
  vector<thread> philosopher_threads;

  DiningPhilosophers table(NUM_PHILOSOPHERS); // object

  for (int i = 0; i < NUM_PHILOSOPHERS; i++)
    philosopher_threads.emplace_back(&DiningPhilosophers::philosopher, &table,
                                     i);

  for (auto &t : philosopher_threads) {

    if (t.joinable())
      t.join();
  }

  return 0;
}
/*
philosopher_0 is THINKING...
Philosopher_1 is THINKING...
Philosopher_2 is THINKING...
Philosopher_3 is THINKING...
Philosopher_4 is THINKING...
Philosopher_2 is EATING!
Philosopher_0 is EATING!
Philosopher_2 finished EATING.
Philosopher_2 is THINKING...
Philosopher_3 is EATING!
Philosopher_0 finished EATING.
Philosopher_0 is THINKING...
Philosopher_1 is EATING!
Philosopher_1 finished EATING.
Philosopher_1 is THINKING...
Philosopher_0 is EATING!
Philosopher_3 finished EATING.
Philosopher_3 is THINKING...
Philosopher_2 is EATING!
Philosopher_0 finished EATING.
Philosopher_0 is THINKING...
Philosopher_4 is EATING!
Philosopher_2 finished EATING.
Philosopher_2 is THINKING...
Philosopher_1 is EATING!
Philosopher_4 finished EATING.
Philosopher_4 is THINKING...
Philosopher_3 is EATING!
Philosopher_1 finished EATING.
Philosopher_1 is THINKING...
Philosopher_0 is EATING!
Philosopher_3 finished EATING.
Philosopher_0 finished EATING.
Philosopher_3 is THINKING...
Philosopher_4 is EATING!
Philosopher_2 is EATING!
Philosopher_2 finished EATING.
Philosopher_1 is EATING!
Philosopher_4 finished EATING.
Philosopher_4 is THINKING...
Philosopher_3 is EATING!
Philosopher_1 finished EATING.
Philosopher_3 finished EATING.
Philosopher_4 is EATING!
Philosopher_4 finished EATING.
*/
