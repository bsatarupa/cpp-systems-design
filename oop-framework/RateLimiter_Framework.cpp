/*
Design a rate limiter that restricts requests from clients. Different algorithms
like Token Bucket and Leaky Bucket should be supported. Limits should be
configurable. New algorithms should be easy to add.

                 RateLimiterStrategy
                        ^
            ----------------------------
            |                          |
       TokenBucket              LeakyBucket
                        ^
                        |
                  RateLimiter
                        |
                client -> strategy

Abstraction → RateLimiterStrategy
Inheritance → TokenBucket, LeakyBucket
Polymorphism → strategy->allowRequest()
Composition → RateLimiter owns a strategy

In Production, I would do the following:
Token Bucket --------------
Track lastRefillTime.
Refill tokens based on elapsed time before processing each request.
Consume one token per accepted request.
Leaky Bucket --------------
Maintain a queue (or current water level).
Drain requests at a fixed rate based on elapsed time.
Reject new requests when the bucket is full.

What is the real difference between these two algorithms?
Token Bucket: controls the rate of request generation by refilling tokens over
time, allowing bursts up to the bucket capacity. Leaky Bucket: controls the rate
of request processing by draining queued requests at a fixed rate, smoothing out
bursts.
*/

#include <iostream>
#include <string>
#include <unordered_map>
using namespace std;

class RateLimiterStrategy {
public:
  virtual bool allowRequest(const string &client) = 0;

  // a timer/background_thread periodically refills/leak bucket for
  // per_client_queue
  virtual void onTimer() = 0;

  virtual ~RateLimiterStrategy() = default;
};

class TokenBucket : public RateLimiterStrategy { // token refilling technique

  unordered_map<string, int> tokens;
  int capacity;

public:
  TokenBucket(int cap) : capacity(cap) {}

  bool allowRequest(const string &client) override {
    // any client can send atmost token-capacity #request successfully
    // there after requests will be rejected.
    if (tokens.count(client) == 0)
      tokens[client] = capacity; // assign capacity #tokens to the new client

    if (tokens[client] == 0)
      return false; // no more requests can be accepted

    tokens[client]--; // after granting the current request
    return true;
  }

  void onTimer() override {
    // Production: refill tokens periodically
    for (auto &[client, token] : tokens)
      token = capacity;
  }
};

class LeakyBucket : public RateLimiterStrategy { // queue draining technique

  unordered_map<string, int> per_client_queue;
  int capacity;

public:
  LeakyBucket(int cap) : capacity(cap) {}

  bool allowRequest(const string &client) override {

    if (per_client_queue[client] == capacity)
      return false;

    per_client_queue[client]++;
    return true;
  }

  void onTimer() override {
    // Production: Leak one request periodically
    for (auto &[client, pending_request] : per_client_queue) {
      if (pending_request > 0)
        pending_request--;
    }
  }
};

class RateLimiterService {
  RateLimiterStrategy *strategy;

public:
  RateLimiterService(RateLimiterStrategy *strategy) : strategy(strategy) {}

  bool allowRequest(const string &client) {
    return strategy->allowRequest(client);
  }

  void onTimer() { strategy->onTimer(); }
};

int main() {

  TokenBucket token_bucket(3);
  RateLimiterService limiter(&token_bucket);
  cout << "--------------Token Bucket----------------\n";
  for (int i = 1; i <= 5; i++)
    cout << "Request_" << i << " : "
         << (limiter.allowRequest("Alice") ? "Allowed" : "Blocked") << endl;

  cout << "Timer Fired\n";
  limiter.onTimer();
  cout << "Request_6 : "
       << (limiter.allowRequest("Alice") ? "Allowed" : "Blocked") << endl;

  cout << "\n\n";

  LeakyBucket leaky_bucket(2);
  RateLimiterService limiter2(&leaky_bucket);
  cout << "--------------Leaky Bucket----------------\n";
  for (int i = 1; i <= 4; i++)
    cout << "Request_" << i << " : "
         << (limiter2.allowRequest("Bob") ? "Allowed" : "Blocked") << endl;

  cout << "Timer2 Fired\n";
  limiter2.onTimer();
  cout << "Request_6 : "
       << (limiter2.allowRequest("Bob") ? "Allowed" : "Blocked") << endl;

  return 0;
}

/* Output:
--------------Token Bucket----------------
Request_1 : Allowed
Request_2 : Allowed
Request_3 : Allowed
Request_4 : Blocked
Request_5 : Blocked
Timer Fired
Request_6 : Allowed


--------------Leaky Bucket----------------
Request_1 : Allowed
Request_2 : Allowed
Request_3 : Blocked
Request_4 : Blocked
Timer2 Fired
Request_6 : Allowed
*/
