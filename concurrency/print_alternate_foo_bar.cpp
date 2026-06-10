#include <condition_variable>
#include <functional>
#include <iostream>
#include <mutex>
#include <thread>

using namespace std;

class FooBarPrinter {
  int max;
  mutex mtx;
  condition_variable cv;
  bool foo_turn = true;

public:
  FooBarPrinter(int n) : max(n) {}

  void printFoo() { cout << "Foo" << endl; }

  void Foo(function<void(void)> printFoo) {

    for (int i = 0; i < max; i++) {
      unique_lock<mutex> lock(mtx);
      cv.wait(lock, [&]() { return foo_turn == true; });

      printFoo();
      foo_turn = false;

      cv.notify_one();
    }
  }

  void printBar() { cout << "Bar" << endl; }

  void Bar(function<void(void)> printBar) {

    for (int i = 0; i < max; i++) {
      unique_lock<mutex> lock(mtx);
      cv.wait(lock, [&]() { return foo_turn == false; });

      printBar();
      foo_turn = true;

      cv.notify_one();
    }
  }
};

int main() {

  int n = 10;
  FooBarPrinter printer(n);

  thread t1(&FooBarPrinter::Foo, &printer, [&]() { printer.printFoo(); });
  thread t2(&FooBarPrinter::Bar, &printer, [&]() { printer.printBar(); });

  t1.join();
  t2.join();

  return 0;
}
