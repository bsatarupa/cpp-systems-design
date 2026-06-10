#include <chrono>
#include <ctime>
#include <iostream>
#include <list>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <thread>
#include <unordered_map>

using namespace std;

class RedisCache_withTTL {

  struct Node {

    string key, value;
    chrono::steady_clock::time_point expiry_time; // per node expiry time
  };

  list<Node> cache;
  unordered_map<string, list<Node>::iterator> mm;

  int Tcapacity;
  mutable shared_mutex rwLock;

public:
  RedisCache_withTTL(int capacity) : Tcapacity(capacity) {}

  string get(const string &key) {

    unique_lock<shared_mutex> writeLock(rwLock);
    auto found = mm.find(key);
    if (found == mm.end())
      return "Key NOT FOUND in Cache!";

    // Lazy expiration: at the time of cache Read
    if (chrono::steady_clock::now() > found->second->expiry_time) {
      cache.erase(mm[key]);
      mm.erase(key);
      return "LAZYily EXPIRED Key!";
    }

    cache.splice(cache.begin(), cache, found->second);
    return found->second->value;
  }

  void put(const string &key, const string &value,
           int TTL_seconds) { // put with TTL

    unique_lock<shared_mutex> writelock(rwLock);
    auto found = mm.find(key);

    if (found != mm.end()) { // remove same old-key entry, if exists
      cache.erase(mm[key]);  // mm[key] : location of node in cache
      mm.erase(key);
    }

    if (cache.size() >= Tcapacity) { // remove oldest entry if cache is full
      string victim_key = cache.back().key;

      cache.pop_back();
      mm.erase(victim_key);

      cout << "Evicting Key : " << victim_key << endl;
    }

    auto expiry = chrono::steady_clock::now() + chrono::seconds(TTL_seconds);
    //***get expiry time for entry to be inserted

    cache.push_front({key, value, expiry});
    mm[key] = cache.begin();

    cout << "Inserted Key : " << key << endl;
  }

  void print() {

    shared_lock<shared_mutex> printLock(rwLock);

    cout << "-----Redis cache content with TTL expiry------" << endl;
    for (auto &node : cache) {

      long long remaining = chrono::duration_cast<chrono::seconds>(
                                node.expiry_time - chrono::steady_clock::now())
                                .count();

      cout << "[" << node.key << " -> " << node.value
           << ", TTL remaining : " << max(remaining, 0LL) << " seconds]"
           << endl;
    }
  }
};

int main() {
  RedisCache_withTTL cache(3);

  cache.put("A", "Apple", 5);
  cache.put("B", "Banana", 5);
  cache.put("C", "Cat", 5);

  cache.print();

  cout << "Get A's value : " << cache.get("A") << endl;

  cache.put("D", "Dog", 5);

  cache.print();

  this_thread::sleep_for(chrono::seconds(6));

  cout << "Get A's value : " << cache.get("A") << endl;

  cache.print();

  return 0;
}
/*
Inserted Key : A
Inserted Key : B
Inserted Key : C
-----Redis cache content with TTL expiry------
[C -> Cat, TTL remaining : 4 seconds]
[B -> Banana, TTL remaining : 4 seconds]
[A -> Apple, TTL remaining : 4 seconds]
Get A's value : Apple
Evicting Key : B
Inserted Key : D
-----Redis cache content with TTL expiry------
[D -> Dog, TTL remaining : 4 seconds]
[A -> Apple, TTL remaining : 4 seconds]
[C -> Cat, TTL remaining : 4 seconds]
Get A's value : LAZYily EXPIRED Key!
-----Redis cache content with TTL expiry------
[D -> Dog, TTL remaining : 0 seconds]
[C -> Cat, TTL remaining : 0 seconds]
*/
