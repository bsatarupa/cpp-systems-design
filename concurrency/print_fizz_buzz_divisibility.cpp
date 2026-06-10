#include <condition_variable>
#include <functional>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>

using namespace std;

class FizzBuzzDivisibilityPrinter {
  mutex mtx;
  condition_variable cv;
  int count, max;

public:
  FizzBuzzDivisibilityPrinter(int n) : count(1), max(n) {}

  void printString(string str) { cout << str << endl; }

  void fizz(function<void(string)> printString) {

    while (count <= max) {

      unique_lock<mutex> lock(mtx);
      cv.wait(lock, [&]() {
        return count > max || (count % 3 == 0 && count % 5 != 0);
      });

      if (count > max)
        break;

      printString("Fizz");
      count++;

      cv.notify_all();
    }
  }

  void buzz(function<void(string)> printString) {

    while (count <= max) {

      unique_lock<mutex> lock(mtx);
      cv.wait(lock, [&]() {
        return count > max || (count % 5 == 0 && count % 3 != 0);
      });

      if (count > max)
        break;

      printString("Buzz");
      count++;

      cv.notify_all();
    }
  }

  void fizzbuzz(function<void(string)> printString) {

    while (count <= max) {

      unique_lock<mutex> lock(mtx);
      cv.wait(lock, [&]() {
        return count > max || (count % 3 == 0 && count % 5 == 0);
      });

      if (count > max)
        break;

      printString("FizzBuzz");
      count++;

      cv.notify_all();
    }
  }

  void remainder(function<void(string)> printString) {

    while (count <= max) {

      unique_lock<mutex> lock(mtx);
      cv.wait(lock, [&]() {
        return count > max || (count % 3 != 0 && count % 5 != 0);
      });

      if (count > max)
        break;

      printString(to_string(count));
      count++;

      cv.notify_all();
    }
  }
};

int main() {

  int limit = 30;
  FizzBuzzDivisibilityPrinter printer(limit);

  thread t1(&FizzBuzzDivisibilityPrinter::fizz, &printer,
            [&](string str) { printer.printString(str); });
  thread t2(&FizzBuzzDivisibilityPrinter::buzz, &printer,
            [&](string str) { printer.printString(str); });
  thread t3(&FizzBuzzDivisibilityPrinter::fizzbuzz, &printer,
            [&](string str) { printer.printString(str); });
  thread t4(&FizzBuzzDivisibilityPrinter::remainder, &printer,
            [&](string str) { printer.printString(str); });

  t1.join();
  t2.join();
  t3.join();
  t4.join();

  cout << endl;

  return 0;
}
