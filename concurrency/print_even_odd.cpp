#include <condition_variable>
#include <iostream>
#include <mutex>
#include <thread>

using namespace std;

class EvenOddPrinter {
  int max, current;
  mutex mtx;
  condition_variable cv;

public:
  EvenOddPrinter(int n) : max(n), current(1) {}

  void printEven() {
    while (current <= max) {
      unique_lock<mutex> lock(mtx);
      cv.wait(lock, [&]() { return current > max || current % 2 == 0; });

      if (current > max)
        break;
      cout << "Even : " << current << endl;
      current++;

      cv.notify_one();
    }
  }

  void printOdd() {
    while (current <= max) {
      unique_lock<mutex> lock(mtx);
      cv.wait(lock, [&]() { return current > max || current % 2 == 1; });

      if (current > max)
        break;
      cout << "Odd : " << current << endl;
      current++;

      cv.notify_one();
    }
  }
};

int main() {

  int n = 10;
  EvenOddPrinter printer(n);

  thread t1(&EvenOddPrinter::printEven, &printer);
  thread t2(&EvenOddPrinter::printOdd, &printer);

  t1.join();
  t2.join();

  return 0;
}
