#include <iostream>
#include <list>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace std;

class LRUcache {

  int Tcapacity;
  list<pair<int, int>> cache;
  unordered_map<int, list<pair<int, int>>::iterator> mm;

public:
  LRUcache(int cap) : Tcapacity(cap) {}

  bool full() { return mm.size() >= Tcapacity; }

  int get(int key) {
    auto found = mm.find(key);
    if (found == mm.end())
      return -1;

    cache.splice(cache.begin(), cache, found->second);
    return found->second->second;
  }

  void insert(int key, int value) {
    cache.push_front({key, value});
    mm[key] = cache.begin();
  }

  void erase(int key) {
    auto found = mm.find(key);
    if (found == mm.end())
      return;

    cache.erase(found->second);
    mm.erase(found);
  }

  pair<int, int> evictLRU() {
    auto victim = cache.back();
    cache.pop_back();
    mm.erase(victim.first);

    return victim;
  }

  optional<int> remove_and_extract_value(int key) {
    auto found = mm.find(key);
    if (found == mm.end())
      return nullopt;

    int value = found->second->second;
    cache.erase(found->second);
    mm.erase(found);

    return value;
  }

  void print(const string &tier_name) {
    cout << tier_name << " : ";
    for (auto &[k, v] : cache)
      cout << "(" << k << "," << v << ") ";
    cout << endl;
  }
};

class MultiTierCache {

  vector<LRUcache> tiers;
  unordered_map<int, int> key_to_tier;

  void insert_into_tier(int level, int key, int value) {

    if (level >= tiers.size()) {

      key_to_tier.erase(key);
      cout << "Dropped key : " << key << endl;
      return;
    }

    auto &currLRU = tiers[level];
    if (!currLRU.full()) {

      currLRU.insert(key, value);
      key_to_tier[key] = level;
      return;
    }

    // when tier is full, evict and insert
    pair<int, int> evicted = currLRU.evictLRU();
    key_to_tier.erase(evicted.first);

    currLRU.insert(key, value);
    key_to_tier[key] = level;

    insert_into_tier(level + 1, evicted.first, evicted.second); // recursive
                                                                // call
  }

public:
  MultiTierCache(const vector<int> &capacities) { // capacity of each tier
    for (auto &cap : capacities)
      tiers.emplace_back(cap);
  }

  void put(int key, int value) { // if already exist, remove from current layer
                                 // and insert into RAM

    auto found = key_to_tier.find(key);
    if (found != key_to_tier.end()) {

      int level = found->second;
      tiers[level].erase(key);
      key_to_tier.erase(key);
    }

    insert_into_tier(0, key, value);
  }

  int get(int key) {

    auto found = key_to_tier.find(key);
    if (found == key_to_tier.end())
      return -1;

    int level = found->second;
    auto value = tiers[level].remove_and_extract_value(key);
    if (!value)
      return -1;

    key_to_tier.erase(key);

    // promote to RAM
    insert_into_tier(0, key,
                     *value); // promote to hottest tier and return immediately
    return *value;
  }

  void print() {
    cout << "\n";
    for (int i = 0; i < tiers.size(); i++)
      tiers[i].print("L" + to_string(i + 1));

    cout << endl;
  }
};

int main() {

  MultiTierCache cache({2, 2, 2}); // RAM-SSD-HDD

  cache.put(1, 100);
  cache.put(2, 200);
  cache.print();

  cache.put(3, 300);
  cache.print();

  cache.put(4, 400);
  cache.print();

  cout << "Fetching value for key = 1 : " << cache.get(1) << endl;
  cache.print();

  cache.put(5, 500);
  cache.print();

  cache.put(6, 600);
  cache.print();

  cache.put(7, 700);
  cache.print();

  return 0;
}
/*
$ clang++ LRUcache_multi-tier_RAM-SSD-HDD.cpp -std=c++20 -o lrucache &&
./lrucache

L1 : (2,200)(1,100)
L2 :
L3 :


L1 : (3,300)(2,200)
L2 : (1,100)
L3 :


L1 : (4,400)(3,300)
L2 : (2,200)(1,100)
L3 :

Fetching value for key = 1 : 100

L1 : (1,100)(4,400)
L2 : (3,300)(2,200)
L3 :


L1 : (5,500)(1,100)
L2 : (4,400)(3,300)
L3 : (2,200)


L1 : (6,600)(5,500)
L2 : (1,100)(4,400)
L3 : (3,300)(2,200)

Dropped key : 2

L1 : (7,700)(6,600)
L2 : (5,500)(1,100)
L3 : (4,400)(3,300)
*/
