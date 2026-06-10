/*
Find duplicates in a 500GB file. File does not fit in memory.

For simplicity I'm using ifstream, which is already buffered. In production, for
a 500GB file I would switch to chunked I/O using read() with a large
buffer (e.g. 64KB–1MB) to reduce parsing overhead.

Pass 1: Hash-based partition, same key always goes to same partition.
Pass 2: Detect duplicates inside each bucket.

Q: What if a bucket still doesn't fit memory?
Increase NUM_BUCKETS or Recursively partition that bucket.

Q: What if NUM_BUCKETS is very large?
Cannot keep millions of bucket files open. Need to consider OS file descriptor
limits (ulimit -n). Process buckets in batches or recursively partition.

$ clang++ detect_duplicates_in_500GB_logs.cpp -o dup_ip && ./dup_ip input.txt
10.0.0.8
10.0.0.2
10.0.0.1
10.0.0.9
10.0.0.3
*/
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>

using namespace std;

constexpr int NUM_BUCKETS = 50;
const string BUCKET_DIR = "dup_buckets";

void partitionFile(const string &inputFile) {

  filesystem::create_directories(BUCKET_DIR);

  vector<ofstream> buckets(NUM_BUCKETS);

  for (int i = 0; i < NUM_BUCKETS; i++) {

    buckets[i].open(BUCKET_DIR + "/bucket_" + to_string(i) + ".txt");
    if (!buckets[i]) {
      cerr << "Failed to open bucket : " << i << '\n';
      return;
    }
  }

  ifstream in(inputFile);
  if (!in) {

    cerr << "Failed to open : " << inputFile << '\n';
    return;
  }

  string key;
  while (in >> key) {

    size_t bucket_id = hash<string>{}(key) % NUM_BUCKETS;

    buckets[bucket_id] << key << '\n';
  }
}

void findDuplicates() {

  for (int i = 0; i < NUM_BUCKETS; i++) {

    ifstream in(BUCKET_DIR + "/bucket_" + to_string(i) + ".txt");

    unordered_set<string> seen, printed;

    string key;
    while (in >> key) {

      /*
      duplicate entry already exists in unordered_set seen, but not in printed
      */
      if (seen.insert(key).second == false &&
          printed.insert(key).second == true) {

        cout << key << '\n';
      }
    }
  }
}

int main(int argc, char *argv[]) {

  if (argc != 2) {

    cerr << "usuage : ./a.out <input_file> \n";
    return -1;
  }

  partitionFile(argv[1]);
  findDuplicates();

  return 0;
}
