#include <condition_variable>
#include <functional>
#include <iostream>
#include <mutex>
#include <thread>

using namespace std;

class ZeroEvenZeroOddPrinter {
  mutex mtx;
  condition_variable cv;
  int count, max;
  bool printZero;

public:
  ZeroEvenZeroOddPrinter(int n) : count(1), max(n), printZero(true) {}

  void printNumber(int num) { cout << num; }

  void zero(function<void(int)> printNumber) {

    while (count <= max) {
      unique_lock<mutex> lock(mtx);
      cv.wait(lock, [&]() { return count > max || printZero == true; });

      if (count > max)
        break;

      printNumber(0);
      printZero = false;

      cv.notify_all();
    }
  }

  void even(function<void(int)> printNumber) {

    while (count <= max) {
      unique_lock<mutex> lock(mtx);
      cv.wait(lock, [&]() {
        return count > max || (printZero == false && count % 2 == 0);
      });

      if (count > max)
        break;

      printNumber(count);
      printZero = true;
      count++;

      cv.notify_all();
    }
  }

  void odd(function<void(int)> printNumber) {

    while (count <= max) {
      unique_lock<mutex> lock(mtx);
      cv.wait(lock, [&]() {
        return count > max || (printZero == false && count % 2 == 1);
      });

      if (count > max)
        break;

      printNumber(count);
      printZero = true;
      count++;

      cv.notify_all();
    }
  }
};

int main() {

  int limit = 10;
  ZeroEvenZeroOddPrinter printer(limit);

  thread t1(&ZeroEvenZeroOddPrinter::zero, &printer,
            [&](int num) { printer.printNumber(num); });
  thread t2(&ZeroEvenZeroOddPrinter::even, &printer,
            [&](int num) { printer.printNumber(num); });
  thread t3(&ZeroEvenZeroOddPrinter::odd, &printer,
            [&](int num) { printer.printNumber(num); });

  t1.join();
  t2.join();
  t3.join();

  cout << endl;

  return 0;
}
