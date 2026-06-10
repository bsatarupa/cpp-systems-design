/*
External Sort:
Phase 1: Create sorted runs
Phase 2:Merge K sorted runs [Use: Min Heap + 1 line/word from each file]

Q: Why not load all files?
Files may be GB/TB sized. Need streaming.

Q. What if there are 10,000 files?
Heap size = 10,000, Memory = O(K)
If open-file descriptor limits are reached, merge in batches (e.g. 100 files at
a time) and create intermediate runs. Run: ulimit -n to find opened file
descriptor limits.

Q: What if each file is 100GB?
Still works. Only one record from each file is in memory at any time.
Heap size = K, SC = O(K), TC = O(N log K)

$ clang++ merge_k_sorted_files.cpp -o merge_files && ./merge_files input.txt
input.txt output.txt
*/
#include <fstream>
#include <iostream>
#include <queue>
#include <string>
#include <vector>

using namespace std;

struct Entry {

  string value;
  int file_index;
};

struct Compare {

  bool operator()(const Entry &a, const Entry &b) const {
    return a.value > b.value;
  }
};

void merge_K_sorted_files(const vector<string> &in_files,
                          const string &out_file) {

  vector<ifstream> inputs(in_files.size());
  for (int i = 0; i < in_files.size(); i++)
    inputs[i].open(in_files[i]);

  ofstream output(out_file);

  priority_queue<Entry, vector<Entry>, Compare> pq;
  for (int i = 0; i < in_files.size(); i++) {
    // insert first string from each file into priority_queue
    string value;
    if (inputs[i] >> value)
      pq.push({value, i});
  }

  while (!pq.empty()) {

    auto [value, file_index] = pq.top();
    pq.pop();
    output << value << '\n';

    if (inputs[file_index] >> value)
      pq.push({value, file_index});
  }
}

int main(int argc, char *argv[]) {

  if (argc < 4) {

    cerr << "usage: ./a.out <input_files> <output_file>\n";
    return -1;
  }

  vector<string> input_files;
  string output_file = argv[argc - 1];

  for (int i = 1; i < argc - 1; i++)
    input_files.push_back(argv[i]);

  merge_K_sorted_files(input_files, output_file);
  return 0;
}
