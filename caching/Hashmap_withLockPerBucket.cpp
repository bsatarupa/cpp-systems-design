#include <iostream>
#include <unordered_map>

using namespace std;

template <typename K, typename V>

class Hashmap_PerBucketLock {

  struct Bucket {
    // per bucket hashmap and mutex
    unordered_map<K, V> bucket_map;
    mutex mtx;
  };

  int NUM_BUCKETS;
  vector<Bucket> buckets;

  int getBucketIndex(const K &key) { return hash<K>{}(key) % NUM_BUCKETS; }

public:
  Hashmap_PerBucketLock(int bucket_count)
      : NUM_BUCKETS(bucket_count), buckets(bucket_count) {}

  void put(const K &key, const V &value) {

    int idx = getBucketIndex(key);
    Bucket &bucket = buckets[idx];

    lock_guard<mutex> lock(bucket.mtx);
    bucket.bucket_map[key] = value;
  }

  bool get(const K &key, V &value) {

    int idx = getBucketIndex(key);
    Bucket &bucket = buckets[idx];

    lock_guard<mutex> lock(bucket.mtx);
    auto it = bucket.bucket_map.find(key);
    if (it == bucket.bucket_map.end())
      return false;

    value = it->second;
    return true;
  }

  void remove(const K &key) {

    int idx = getBucketIndex(key);
    Bucket &bucket = buckets[idx];

    lock_guard<mutex> lock(bucket.mtx);
    bucket.bucket_map.erase(key);
  }
};

int main() {

  Hashmap_PerBucketLock<string, int> mp(16);

  mp.put("apple", 10);
  mp.put("banana", 20);

  int value;
  if (mp.get("apple", value) == true)
    cout << value << endl;

  mp.remove("apple");

  return 0;
}
