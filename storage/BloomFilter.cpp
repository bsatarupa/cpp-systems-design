#include <bitset>
#include <iostream>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <thread>

using namespace std;

class BloomFilter {

  static const int SIZE = 1000;
  bitset<SIZE> bits;
  mutable shared_mutex rwLock;

public:
  void insert(const string &key) {

    unique_lock<shared_mutex> writeLock(rwLock);

    int h1 = hash<string>{}(key) % SIZE;
    int h2 = hash<string>{}(key + "salt") % SIZE;

    // bits[h1] = 1;
    // bits[h2] = 1;

    bits.set(h1);
    bits.set(h2);

    cout << "Inserted : " << key << endl;
  }

  bool possibly_contains(const string &key) {

    shared_lock<shared_mutex> readLock(rwLock);

    int h1 = hash<string>{}(key) % SIZE;
    int h2 = hash<string>{}(key + "salt") % SIZE;

    // return bits[h1] && bits[h2];
    return bits.test(h1) && bits.test(h2);
  }
};

int main() {

  BloomFilter bf;
  vector<thread> producers, consumers;
  mutex printMtx;

  int producer_count = 5, consumer_count = 4;

  string keys[] = {"apple", "banana", "pear", "guava", "mango"};

  for (int p_id = 0; p_id < producer_count; p_id++) {

    producers.emplace_back([&, p_id]() { bf.insert(keys[p_id]); });
  }

  string queries[] = {"apple", "kiwi", "guava", "blackberry"};

  for (int c_id = 0; c_id < consumer_count; c_id++) {

    consumers.emplace_back([&, c_id]() {
      bool found = bf.possibly_contains(queries[c_id]);

      lock_guard<mutex> printLock(printMtx);
      cout << queries[c_id] << " -> ";

      cout << (found ? "Possibly Present" : "Definitely Not Present") << endl;
    });
  }

  for (auto &p : producers)
    p.join();

  for (auto &c : consumers)
    c.join();

  return 0;
}
/*
Inserted : apple
Inserted : pear
Inserted : banana
Inserted : guava
Inserted : mango
apple -> Possibly Present
kiwi -> Definitely Not Present
guava -> Possibly Present
blackberry -> Definitely Not Present
*/
