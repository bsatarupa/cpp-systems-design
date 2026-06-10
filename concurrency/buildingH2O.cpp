#include <condition_variable>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

using namespace std;

class H2O {
  int hydrogenCount;
  mutex mtx;
  condition_variable cv;

public:
  H2O() : hydrogenCount(0) {}

  void releaseHydrogen(void) { cout << "H"; }

  void releaseOxygen(void) { cout << "O"; }

  void hydrogen(function<void(void)> releaseHydrogen) {

    unique_lock<mutex> lock(mtx);

    cv.wait(lock, [&]() { return hydrogenCount < 2; });

    releaseHydrogen();
    hydrogenCount++;

    cv.notify_all();
  }

  void oxygen(function<void(void)> releaseOxygen) {

    unique_lock<mutex> lock(mtx);

    cv.wait(lock, [&]() { return hydrogenCount == 2; });

    releaseOxygen();
    hydrogenCount = 0;

    cv.notify_all();
  }
};

int main() {

  H2O h2o;

  vector<thread> threads;

  for (int i = 0; i < 6; i++)
    threads.emplace_back(&H2O::hydrogen, &h2o,
                         [&]() { h2o.releaseHydrogen(); });

  for (int i = 0; i < 3; i++)
    threads.emplace_back(&H2O::oxygen, &h2o, [&]() { h2o.releaseOxygen(); });

  for (auto &t : threads)
    t.join();

  cout << endl;
  return 0;
}
