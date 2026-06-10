#include <chrono>
#include <iostream>
#include <list>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <unordered_map>

using namespace std;

class LRUcache {

  int Tcapacity;
  list<pair<int, int>> cache;
  unordered_map<int, list<pair<int, int>>::iterator> mm;
  mutable shared_mutex rwLock;

public:
  LRUcache(int capacity) : Tcapacity(capacity) {}

  int get(int key) {

    shared_lock<shared_mutex> readLock(rwLock);
    auto found = mm.find(key);
    if (found == mm.end())
      return -1;
    int val = found->second->second;
    readLock.unlock();

    unique_lock<shared_mutex> writelock(rwLock);
    cache.splice(cache.begin(), cache, found->second);
    return val;
  }

  void put(int key, int value) {

    unique_lock<shared_mutex> writelock(rwLock);
    auto found = mm.find(key);

    if (found != mm.end()) {
      cache.splice(cache.begin(), cache, found->second);
      found->second->second = value;
    } else {
      if (mm.size() == Tcapacity) {
        int victim_key = cache.back().first;
        cache.pop_back();
        mm.erase(victim_key);
      }
      cache.push_front({key, value});
      mm[key] = cache.begin();
    }
  }

  void print() {

    shared_lock<shared_mutex> printLock(rwLock);

    cout << "LRU cache content : " << endl;
    for (auto &[k, v] : cache)
      cout << "[" << k << " : " << v << "]" << endl;
  }
};

int main() {
  LRUcache cache(3);

  thread writer([&]() {
    for (int i = 1; i <= 5; i++) {
      cache.put(i, i * 10);
      cout << "Writer wrote value : " << i * 10 << endl;
    }
  });

  thread reader([&]() {
    this_thread::sleep_for(chrono::milliseconds(100));

    for (int i = 1; i <= 3; i++) {
      int val = cache.get(i);
      cout << "Reader got value : " << val << endl;
    }
  });

  writer.join();
  reader.join();

  cache.print();

  return 0;
}
