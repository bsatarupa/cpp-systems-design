/*
Top 100 IPs in 500 GB logs [Top K Huge File]:
Phase1: hash-based partition (Partition file into NUM_BUCKETS. Same IP always
goes to same bucket. TC: O(N)) Phase2: per-bucket frequency map (count
frequency. TC: O(N)) Phase3: global min heap of size K (TC: UlogK, U: uniqueIPs)

For simplicity I'm using ifstream, which is already buffered. In production, for
a 500GB file I would switch to chunked I/O using read() with a large buffer
(e.g. 64KB–1MB) to reduce parsing overhead.

Q: what if each bucket is still too large?
Increase NUM_BUCKETS or Recursively partition that specific bucket.

Q.Why does counting inside each bucket work?
Same IP always hashes to the same bucket. Therefore all occurrences of an IP
are counted in exactly one bucket.

$ clang++ top_100IP_in_500GB_logs.cpp -o top_ip && ./top_ip input.txt 3
10.0.0.1 -> 6
10.0.0.2 -> 5
10.0.0.3 -> 2

Huge Log Processing: Streaming + Hash-based Partition + Per-bucket HashMap
Top K Huge Logs: Hash-based Partition + Per-bucket HashMap + Global Min Heap
*/

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <queue>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

const string BUCKET_DIR = "top_buckets";
constexpr int NUM_BUCKETS = 100;
// ulimit -n : max #file descriptors = 256

struct Entry {

  string ip;
  int count;
};

struct Compare {

  bool operator()(const Entry &a, const Entry &b) const {
    return a.count > b.count;
  }
};

void partition_file(const string &input_file) {

  filesystem::create_directories(BUCKET_DIR);

  vector<ofstream> buckets(NUM_BUCKETS);
  for (int i = 0; i < NUM_BUCKETS; i++)
    buckets[i].open(BUCKET_DIR + "/bucket_" + to_string(i) + ".txt");

  ifstream in(input_file);
  string ip;

  while (in >> ip) {
    buckets[hash<string>{}(ip) % NUM_BUCKETS] << ip << '\n';
  }
}

vector<Entry> top_K_IPs(int K) {

  priority_queue<Entry, vector<Entry>, Compare> pq;

  for (int i = 0; i < NUM_BUCKETS; i++) {

    ifstream in(BUCKET_DIR + "/bucket_" + to_string(i) + ".txt");

    unordered_map<string, int> freq;
    string ip;
    while (in >> ip)
      freq[ip]++;

    for (auto &[ip, count] : freq) {

      pq.push({ip, count});
      if (pq.size() > K)
        pq.pop();
    }
  }

  vector<Entry> result;
  while (!pq.empty()) {
    result.push_back(pq.top());
    pq.pop();
  }

  reverse(result.begin(), result.end());
  return result;
}

int main(int argc, char *argv[]) {
  if (argc != 3) {

    cerr << "usage: ./a.out <log_file> <K> \n";
    return -1;
  }
  partition_file(argv[1]);

  auto result = top_K_IPs(stoi(argv[2]));
  for (auto &[ip, count] : result)
    cout << ip << " -> " << count << '\n';

  return 0;
}
