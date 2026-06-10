#include <iostream>
#include <list>
#include <set>
#include <unordered_map>

using namespace std;

struct Node {
  int key, value, priority;
};

class PriorityCache {

private:
  // using list_iterator = list<Node> :: iterator;
  int capacity;
  list<Node> cache;
  unordered_map<int, list<Node>::iterator> mm;

  struct Compare {

    bool operator()(const list<Node>::iterator &a,
                    const list<Node>::iterator &b) const {

      if (a->priority != b->priority)
        return a->priority <
               b->priority; // lower priority one gets evicted first
      else
        return a->key < b->key;
    }
  };
  set<list<Node>::iterator, Compare>
      priority_set; // ordered balanced binary tree, RED-BLACK tree

public:
  PriorityCache(int cap) : capacity(cap) {}

  int get(int key) {
    auto found = mm.find(key);
    if (found == mm.end())
      return -1;
    return found->second->value;
  }

  void put(int key, int value, int priority) {
    auto found = mm.find(key);
    if (found != mm.end()) { // key found
      auto node_iter = found->second;
      priority_set.erase(node_iter);

      node_iter->value = value;
      node_iter->priority = priority;

      priority_set.insert(node_iter);
      return;
    }

    if (capacity == 0) //***A MUST
      return;

    if (mm.size() == capacity && !priority_set.empty()) {
      // when eviction required, priority_set MUST NOT BE EMPTY, evict lowest
      // priority value
      auto victim_iter =
          *priority_set.begin(); // is used to find eviction victim
      cout << "Evicting key = " << victim_iter->key << " Priority "
           << victim_iter->priority << " Value " << victim_iter->value << endl;

      priority_set.erase(priority_set.begin());
      mm.erase(victim_iter->key);
      cache.erase(victim_iter);
    }

    cache.push_front({key, value, priority});
    mm[key] = cache.begin();
    priority_set.insert(cache.begin());
  }

  void update_priority(int key, int new_priority) {
    auto found = mm.find(key);
    if (found == mm.end())
      return;

    auto node_iter = found->second;
    priority_set.erase(node_iter);

    node_iter->priority = new_priority;
    priority_set.insert(node_iter);
  }

  void erase(int key) {
    auto found = mm.find(key);
    if (found == mm.end())
      return;

    auto node_iter = found->second;
    priority_set.erase(node_iter);
    cache.erase(node_iter);
    mm.erase(key);
  }

  void print() {
    cout << "\nCache Contents\n";
    cout << "-----------------\n";
    for (auto &node : cache) {
      cout << "Key = " << node.key << " Value = " << node.value
           << " Priority = " << node.priority << endl;
    }
    cout << endl;
  }
};

int main() {

  PriorityCache cache(3);

  cache.put(1, 100, 5);
  cache.put(2, 200, 2);
  cache.put(3, 300, 8);

  cache.print();

  cout << "Insert(4, 400, p = 1)\n";
  cache.put(4, 400, 1);
  cache.print();

  cout << "Update Priority of key 3 -> 0\n";
  cache.update_priority(3, 0);
  cache.print();

  cout << "Insert(5, 500, p = 10)\n";
  cache.put(5, 500, 10);
  cache.print();

  cout << "Get(5) = " << cache.get(5) << endl;
  cout << "Get(2) = " << cache.get(2) << endl;

  return 0;
}
/*
$ clang++ -std=c++20 LRUcache_priority_based_eviction.cpp -o priority &&
./priority

Cache Contents
-----------------
Key = 3 Value = 300 Priority = 8
Key = 2 Value = 200 Priority = 2
Key = 1 Value = 100 Priority = 5

Insert(4, 400, p = 1)
Evicting key = 2 Priority 2 Value 200

Cache Contents
-----------------
Key = 4 Value = 400 Priority = 1
Key = 3 Value = 300 Priority = 8
Key = 1 Value = 100 Priority = 5

Update Priority of key 3 -> 0

Cache Contents
-----------------
Key = 4 Value = 400 Priority = 1
Key = 3 Value = 300 Priority = 0
Key = 1 Value = 100 Priority = 5

Insert(5, 500, p = 10)
Evicting key = 3 Priority 0 Value 300

Cache Contents
-----------------
Key = 5 Value = 500 Priority = 10
Key = 4 Value = 400 Priority = 1
Key = 1 Value = 100 Priority = 5

Get(5) = 500
Get(2) = -1
*/
