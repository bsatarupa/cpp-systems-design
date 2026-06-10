/*
Cache System: Design a cache supporting put() and get() operations. The cache
should support different eviction policies like LRU and LFU. It should be easy
to introduce new policies.

            EvictionPolicy
                  ^
          --------|---------
          |                |
      LRUPolicy       LFUPolicy

                   Cache
                     |
           unordered_map<int,int>
                     |
              EvictionPolicy*
*/
#include <iostream>
#include <list>
#include <unordered_map>
using namespace std;

class EvictionPolicy {
public:
  virtual int get(int key) = 0;
  virtual void put(int key, int value) = 0;
  virtual int evict() = 0;
  virtual void print() = 0;
  virtual ~EvictionPolicy() = default;
};

class LRUPolicy : public EvictionPolicy {

  int capacity;
  list<pair<int, int>> cache;
  unordered_map<int, list<pair<int, int>>::iterator> mm;

public:
  LRUPolicy(int capacity) : capacity(capacity) {}

  int get(int key) override {

    auto found = mm.find(key);

    if (found == mm.end())
      return -1;

    int value = found->second->second;

    cache.splice(cache.begin(), cache, found->second);

    return value;
  }

  void put(int key, int value) override {

    auto found = mm.find(key);

    if (found != mm.end()) {

      cache.splice(cache.begin(), cache, found->second);
      found->second->second = value;
    } else {

      if (cache.size() == capacity)
        evict();

      cache.push_front({key, value});
      mm[key] = cache.begin();
    }
  }

  int evict() override {

    int victim = cache.back().first;

    cache.pop_back();
    mm.erase(victim);

    return victim;
  }

  void print() override {

    cout << "Cache contents\n";

    for (auto &[k, v] : cache)
      cout << "[" << k << " : " << v << "]\n";

    cout << '\n';
  }
};
// can add other eviction policies like LFUPolicy later on

class Cache {

  EvictionPolicy *policy;

public:
  Cache(EvictionPolicy *policy) : policy(policy) {}

  int get(int key) { return policy->get(key); }

  void put(int key, int value) { policy->put(key, value); }

  void print() { policy->print(); }
};

int main() {

  LRUPolicy lru(2);
  // Similarly, LFUPolicy lfu(4);

  Cache cache(&lru);

  cache.put(1, 10);
  cache.put(2, 20);

  cache.print();

  cout << cache.get(1) << endl;

  cache.put(3, 30);

  cache.print();

  cout << cache.get(2) << endl;

  return 0;
}
