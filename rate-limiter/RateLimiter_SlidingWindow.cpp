#include <chrono>
#include <deque>
#include <iostream>
#include <mutex>
#include <thread>
#include <unordered_map>

using namespace std;

class RateLimiter {

  mutex mtx;
  condition_variable cv;

  int globalLimit, clientLimit;

  deque<chrono::steady_clock::time_point> globalQueue;
  unordered_map<string, deque<chrono::steady_clock::time_point>> clientQueues;

  chrono::milliseconds window{1000}; // 1 second window

  void cleanup(deque<chrono::steady_clock::time_point> &dq,
               chrono::steady_clock::time_point now) {

    while (!dq.empty()) {

      auto diff = chrono::duration_cast<chrono::milliseconds>(now - dq.front());
      if (diff >= window)
        dq.pop_front();
      else
        break;
    }
  }

public:
  RateLimiter(int globalLimitPerSec, int clientLimitPerSec)
      : globalLimit(globalLimitPerSec), clientLimit(clientLimitPerSec) {}

  bool allowRequest(const string &clientId) {

    lock_guard<mutex> lock(mtx);

    // cleanup expired requests
    auto now = chrono::steady_clock::now();

    cleanup(globalQueue, now);

    auto &clientQueue = clientQueues[clientId];
    cleanup(clientQueue, now);

    // check Q limits
    if (globalQueue.size() >= globalLimit)
      return false;
    if (clientQueue.size() >= clientLimit)
      return false;

    // accept request
    globalQueue.push_back(now);
    clientQueue.push_back(now);

    return true;
  }
};

int main() {

  RateLimiter limiter(10, 3);

  vector<string> clients = {"Alice", "Bob", "Eva"};
  vector<thread> threads;

  mutex printMtx;

  for (auto &client : clients) {

    threads.emplace_back([&, client]() {
      for (int req = 1; req <= 6; req++) { // per client requests

        bool allowed = limiter.allowRequest(client);

        {
          lock_guard<mutex> printLock(printMtx);
          cout << client << " -> " << (allowed ? "Allowed" : "Rejected")
               << endl;
        }
        this_thread::sleep_for(chrono::milliseconds(200));
      }
    });
  }

  for (auto &t : threads)
    t.join();

  return 0;
}
/*
Alice -> Allowed
Eva -> Allowed
Bob -> Allowed
Eva -> Allowed
Alice -> Allowed
Bob -> Allowed
Eva -> Allowed
Alice -> Allowed
Bob -> Allowed
Eva -> Rejected
Alice -> Rejected
Bob -> Rejected
Eva -> Rejected
Alice -> Rejected
Bob -> Rejected
Eva -> Allowed
Alice -> Allowed
Bob -> Allowed
*/
