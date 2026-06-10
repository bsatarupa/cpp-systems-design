/*
Sort a 1TB file with limited memory.
External Sort----
Phase 1: Read chunk that fits in RAM. Sort it. Write a sorted run.
Phase 2: Merge all sorted runs using a min heap (Merge K Sorted Files).
TC: O(N log M) + O(N log K)
SC: O(chunk_size + K)

Q. What if 10,000 runs? [Can't open 10,000 files because of: ulimit -n]
Apply Multi-level merge:
10000 runs
↓
merge 100 at a time
↓
100 intermediate runs
↓
final merge

Q. Why not quicksort?
Quicksort assumes data is in memory. External sort is designed for disk-resident
data. Why External Sort is efficient? Sequential I/O: Read large chunk, Write
large chunk - instead of random disk seeks.

$ clang++ sort_1TB_file_with_1GB_RAM.cpp -o external_sort && ./external_sort
huge.txt sorted_huge.txt
*/
#include <fstream>
#include <iostream>
#include <queue>
#include <string>
#include <vector>

using namespace std;

constexpr size_t CHUNK_SIZE = 100000;

struct Entry {

  string line;
  int file_index;
};

struct Compare {

  bool operator()(const Entry &a, const Entry &b) const {
    return a.line > b.line;
  }
};

vector<string> create_sorted_runs(const string &input_file) {

  ifstream in(input_file);

  vector<string> run_files;
  vector<string> chunk;

  string line;
  int run_id = 0;

  while (getline(in, line)) {

    chunk.push_back(line);

    // once chunk is full
    if (chunk.size() >= CHUNK_SIZE) {

      sort(chunk.begin(), chunk.end());

      string run_file = "run_" + to_string(run_id++) + ".txt";
      run_files.push_back(run_file); // add only run_file name

      ofstream out(run_file);
      for (string &ln : chunk)
        out << ln << '\n';

      chunk.clear(); //**IMP to clear out the chunk for next iteration
    }
  }
  // for leftover chunk < CHUNK_SIZE
  if (!chunk.empty()) {

    sort(chunk.begin(), chunk.end());

    string run_file = "run_" + to_string(run_id++) + ".txt";
    run_files.push_back(run_file); // add only run_file name

    ofstream out(run_file);
    for (string &ln : chunk)
      out << ln << '\n';
  }

  return run_files;
}

void merge_K_sorted_files(const vector<string> &in_files,
                          const string &out_file) {

  vector<ifstream> inputs(in_files.size());
  for (int i = 0; i < in_files.size(); i++)
    inputs[i].open(in_files[i]);

  ofstream output(out_file);

  priority_queue<Entry, vector<Entry>, Compare> pq;
  for (int i = 0; i < in_files.size(); i++) {
    // insert first string from each file into priority_queue
    string line;
    if (getline(inputs[i], line))
      pq.push({line, i});
  }

  while (!pq.empty()) {

    auto [line, file_index] = pq.top();
    pq.pop();
    output << line << '\n';

    string next_line;
    if (getline(inputs[file_index],
                next_line)) // get next line of the same input file
      pq.push({next_line, file_index});
  }
}

int main(int argc, char *argv[]) {

  if (argc != 3) {

    cerr << "usage: ./a.out <input_file> <output_file>\n";
    return -1;
  }

  auto run_files = create_sorted_runs(argv[1]);

  merge_K_sorted_files(run_files, argv[2]);

  return 0;
}
