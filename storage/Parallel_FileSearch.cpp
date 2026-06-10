/*
 Given a root directory and a search pattern, recursively search all files in
the directory tree and print every line containing the pattern. Use multiple
threads to process files in parallel for better performance.

Architecture:
                  Root Directory
                        │
                        ▼
          Recursive Directory Traversal
                 (Producer Thread)
                        │
                        ▼
               Blocking Queue<File>
                        │
          ┌─────────────┼─────────────┐
          ▼             ▼             ▼
      Worker 1      Worker 2      Worker N
          │             │             │
          ▼             ▼             ▼
     Open File     Open File     Open File
          │             │             │
          ▼             ▼             ▼
    Search Pattern Search Pattern Search Pattern
          │             │             │
          └─────────────┼─────────────┘
                        ▼
              Synchronized Console Output

Producer: Recursively traverses the directory and enqueues regular file paths.
Blocking Queue: Shared queue protected by a mutex and condition_variable.
Worker Threads: Dequeue files, search each file for the pattern, and print
matching lines. Output Synchronization: A separate mutex ensures console output
from multiple threads does not interleave.

Concurrency Pattern:
Producer–Consumer with a fixed-size thread pool.

Time Complexity:
F = number of files
S = total size of all files (bytes or characters)

Directory Traversal: O(F)
Pattern Search
Every character is scanned once.
O(S)
Overall: O(F + S)

Space Complexity:
Queue stores file paths waiting to be processed.
Worst case: O(F)
Worker threads: O(T)
where T is the number of worker threads.
Overall: O(F + T)
*/

#include <condition_variable>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

using namespace std;

class ParallelFileSearch {
private:
  mutex queue_mutex, cout_mutex;
  condition_variable cv;

  queue<string> file_queue;
  string pattern;

  bool stopped = false;

  vector<thread> worker_threads;

public:
  ParallelFileSearch(int numThreads, const string &p) : pattern(p) {

    for (int i = 0; i < numThreads; i++)
      worker_threads.emplace_back(&ParallelFileSearch::worker, this);
  }

  ~ParallelFileSearch() {

    {
      lock_guard<mutex> lock(queue_mutex);
      stopped = true;
    }

    cv.notify_all();

    for (auto &t : worker_threads)
      t.join();
  }

  void search_directory(const string &dir) {

    for (const auto &entry : filesystem::recursive_directory_iterator(dir)) {
      // recursively adds all directory under current directory subtree
      if (!entry.is_regular_file())
        continue;

      if (entry.path().extension() != ".cpp") // to skip a.out and .o files
        continue;

      {
        // push only regular_file
        lock_guard<mutex> lock(queue_mutex);
        file_queue.push(entry.path().string());
      }
      cv.notify_one();
    }
  }

private:
  void worker() {

    while (true) {
      string file_name;
      {
        unique_lock<mutex> lock(queue_mutex);
        cv.wait(lock, [&]() { return stopped || !file_queue.empty(); });

        // graceful shutdown
        if (file_queue.empty() && stopped)
          return;

        file_name = file_queue.front();
        file_queue.pop();
      }
      grep_file(file_name);
    }
  }

  void grep_file(const string &file_name) {

    ifstream file_stream(file_name);
    if (!file_stream)
      return;

    string line;
    int line_number = 1;

    while (getline(file_stream, line)) {

      if (line.find(pattern) != string::npos) {

        lock_guard<mutex> lock(cout_mutex);
        cout << "[" << file_name << ":" << line_number << "] " << line << endl;
      }

      line_number++;
    }
  }
};

int main() {

  ParallelFileSearch searcher(4, "TODO");

  searcher.search_directory("./");

  return 0;
}
/*
Output:
[./Parallel_FileSearch.cpp:109]   ParallelFileSearch searcher(4, "TODO");
*/
