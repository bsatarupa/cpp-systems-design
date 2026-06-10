#include <condition_variable>
#include <functional>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>

using namespace std;

class FirstSecondThirdPrinter {
  mutex mtx;
  condition_variable cv;
  int count;

public:
  FirstSecondThirdPrinter() : count(1) {}

  void printString(string str) { cout << str << endl; }

  void first(function<void(string)> printString) {

    unique_lock<mutex> lock(mtx);
    // cv.wait(lock, [&](){return count == 1;});  Redundant

    printString("First");
    count++;

    cv.notify_all();
  }

  void second(function<void(string)> printString) {

    unique_lock<mutex> lock(mtx);
    cv.wait(lock, [&]() { return count == 2; });

    printString("Second");
    count++;

    cv.notify_all();
  }

  void third(function<void(string)> printString) {

    unique_lock<mutex> lock(mtx);
    cv.wait(lock, [&]() { return count == 3; });

    printString("Third");
    // count++;

    // cv.notify_all();   Redundant
  }
};

int main() {
  FirstSecondThirdPrinter printer;
  thread t1(&FirstSecondThirdPrinter::first, &printer,
            [&](string s) { printer.printString(s); });
  thread t2(&FirstSecondThirdPrinter::second, &printer,
            [&](string s) { printer.printString(s); });
  thread t3(&FirstSecondThirdPrinter::third, &printer,
            [&](string s) { printer.printString(s); });

  t1.join();
  t2.join();
  t3.join();

  return 0;
}
