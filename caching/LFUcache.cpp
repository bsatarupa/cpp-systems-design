#include <iostream>
#include <list>
#include <unordered_map>

using namespace std;

struct Node {
  int key, value, freq;
};
class LFUCache {
  unordered_map<int, list<Node>>
      same_freq; //<freq -> list of Nodes having same frequency mapping>
  unordered_map<int, list<Node>::iterator> mm; //<key -> Node in list mapping>

  int minFrequency; // need to track for eviction
  int Tcapacity;

  void updateFreq(int key) {

    int freq = mm[key]->freq;
    int value = mm[key]->value;

    same_freq[freq].erase(mm[key]);
    if (same_freq[freq].empty()) {
      same_freq.erase(freq);

      if (minFrequency == freq)
        minFrequency++;
    }

    same_freq[freq + 1].push_front({key, value, freq + 1});
    mm[key] = same_freq[freq + 1].begin();
  }

public:
  LFUCache(int capacity) : Tcapacity(capacity), minFrequency(0) {}

  int get(int key) {
    if (mm.count(key) == 0)
      return -1;

    int value = mm[key]->value;

    updateFreq(key);
    return value;
  }

  void put(int key, int value) {

    if (Tcapacity == 0)
      return;

    if (mm.count(key) != 0) {
      mm[key]->value = value;

      updateFreq(key);
      return;
    }

    if (mm.size() == Tcapacity) { // time to evict
      auto &lfu = same_freq[minFrequency];

      auto victim_key = lfu.back().key;
      lfu.pop_back();

      if (lfu.empty())
        same_freq.erase(minFrequency);

      mm.erase(victim_key);
    }

    minFrequency = 1;
    same_freq[1].push_front({key, value, 1});
    mm[key] = same_freq[1].begin();
  }
};

int main() {
  LFUCache cache(2);

  cout << "put(1,10)\n";
  cache.put(1, 10);

  cout << "put(2,20)\n";
  cache.put(2, 20);

  cout << "\nget(1) = " << cache.get(1)
       << "    // returns 10, freq(1)=2, freq(2)=1\n";

  cout << "\nput(3,30)\n";
  cout << "// cache full, evict LFU key=2\n";
  cache.put(3, 30);

  cout << "get(2) = " << cache.get(2) << "    // expected -1 (evicted)\n";

  cout << "get(3) = " << cache.get(3) << "    // expected 30\n";

  cout << "\nCurrent frequencies:\n";
  cout << "// key 1 -> freq 2\n";
  cout << "// key 3 -> freq 2\n";

  cout << "\nput(4,40)\n";
  cout << "// both keys have freq=2\n";
  cout << "// tie broken by LRU\n";
  cout << "// key 1 is older => evict key 1\n";
  cache.put(4, 40);

  cout << "get(1) = " << cache.get(1) << "    // expected -1\n";

  cout << "get(3) = " << cache.get(3) << "    // expected 30\n";

  cout << "get(4) = " << cache.get(4) << "    // expected 40\n";

  cout << "\nUpdate existing key:\n";
  cout << "put(3,300)\n";
  cache.put(3, 300);

  cout << "get(3) = " << cache.get(3) << "    // expected 300\n";

  return 0;
}
