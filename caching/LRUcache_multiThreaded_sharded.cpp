#include <chrono>
#include <iostream>
#include <list>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <unordered_map>

using namespace std;

class LRUcache {

  struct Bucket {
    list<pair<int, int>> cache;
    unordered_map<int, list<pair<int, int>>::iterator> mm;
    mutable shared_mutex rwLock;
  };

  vector<Bucket> buckets;
  static const int N_BUCKETS = 8;
  int Tcapacity, per_bucket_capacity;

  Bucket &getBucket(int key) { return buckets[key % N_BUCKETS]; }

public:
  LRUcache(int capacity)
      : Tcapacity(capacity), buckets(N_BUCKETS),
        per_bucket_capacity(max(1, capacity / N_BUCKETS)) {}

  int get(int key) {

    Bucket &b = getBucket(key);
    /*
     * creates an alias/reference to existing bucket.
     * Bucket contains shared_mutex which is non-copyable, bucket can't be
     * copied.
     */
    shared_lock<shared_mutex> readLock(b.rwLock);
    auto found = b.mm.find(key);
    if (found == b.mm.end())
      return -1;
    readLock.unlock();

    unique_lock<shared_mutex> writelock(b.rwLock);
    b.cache.splice(b.cache.begin(), b.cache, found->second);
    return found->second->second;
  }

  void put(int key, int value) {

    Bucket &b = getBucket(key);
    unique_lock<shared_mutex> writelock(b.rwLock);
    auto found = b.mm.find(key);

    if (found != b.mm.end()) {
      b.cache.splice(b.cache.begin(), b.cache, found->second);
      found->second->second = value;
    } else {
      if (b.mm.size() == per_bucket_capacity) {
        int victim_key = b.cache.back().first;
        b.cache.pop_back();
        b.mm.erase(victim_key);
      }
      b.cache.push_front({key, value});
      b.mm[key] = b.cache.begin();
    }
  }
};

int main() {
  LRUcache cache(16);
  mutex printMtx;

  auto writer = [&](int id) {
    for (int i = 1; i <= 25; i++) {
      int key = id * 100 + i;
      cache.put(key, key * 10);

      {
        lock_guard<mutex> lock(printMtx);
        cout << "Writer : " << id << " put key : " << key
             << "; value : " << key * 10 << endl;
      }
      this_thread::sleep_for(chrono ::milliseconds(50));
    }
  };

  auto reader = [&](int id) {
    for (int i = 1; i <= 20; i++) {
      int key = i % 40;
      int val = cache.get(key);

      {
        lock_guard<mutex> lock(printMtx);
        cout << "Reader : " << id << " for key : " << key
             << " got value : " << val << endl;
      }
      this_thread::sleep_for(chrono::milliseconds(50));
    }
  };

  thread t1(writer, 1);
  thread t2(reader, 1);
  thread t3(reader, 2);

  t1.join();
  t2.join();
  t3.join();

  return 0;
}
