#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

using namespace std;

class RateLimiter_TokenBucket {

  double tokens, maxTokens;
  double refillRate; // tokens received per second

  mutex mtx;
  condition_variable cv;

  chrono::steady_clock::time_point lastRefillTime;

  void refill() {

    auto now = chrono::steady_clock::now();

    double seconds_passed =
        chrono::duration<double>(now - lastRefillTime).count();

    double new_tokens = refillRate * seconds_passed;
    if (new_tokens <= 0)
      return;

    tokens = min(maxTokens, tokens + new_tokens);

    lastRefillTime = now;

    cv.notify_all();
  }

public:
  RateLimiter_TokenBucket(double capacity, double rate)
      : tokens(capacity), maxTokens(capacity), refillRate(rate) {

    lastRefillTime = chrono::steady_clock::now();
  }

  // Non-blocking
  bool try_acquire() {

    lock_guard<mutex> lock(mtx);

    refill();

    if (tokens >= 1.0) {

      tokens -= 1.0;
      return true;
    }
    return false;
  }

  // Blocking
  void acquire() {

    unique_lock<mutex> lock(mtx);

    while (true) {

      refill();

      if (tokens >= 1.0) {

        tokens -= 1.0;
        return;
      }

      // wait until next token arrives
      auto wait_time = chrono::duration<double>(1 / refillRate);
      cv.wait_for(lock, wait_time); // wait from wait_time starting from now
    }
  }
};

int main() {

  RateLimiter_TokenBucket limiter(5, 2);
  vector<thread> threads;
  mutex printMtx;

  for (int thread_id = 1; thread_id <= 3; thread_id++) {

    threads.emplace_back([&, thread_id]() {
      // per thread 5 tokens acquiring attempts
      for (int i = 1; i <= 5; i++) {

        limiter.acquire();

        {
          lock_guard<mutex> printLock(printMtx);
          cout << "Thread : " << thread_id << " acquired Token." << endl;
        }
      }
    });
  }

  for (auto &t : threads)
    t.join();

  return 0;
}
/*
Thread : 1 acquired Token.
Thread : 1 acquired Token.
Thread : 1 acquired Token.
Thread : 3 acquired Token.
Thread : 2 acquired Token.
Thread : 2 acquired Token.
Thread : 3 acquired Token.
Thread : 1 acquired Token.
Thread : 2 acquired Token.
Thread : 3 acquired Token.
Thread : 2 acquired Token.
Thread : 3 acquired Token.
Thread : 2 acquired Token.
Thread : 3 acquired Token.
Thread : 1 acquired Token.
*/
