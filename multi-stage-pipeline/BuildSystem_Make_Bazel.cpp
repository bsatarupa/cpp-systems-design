/*
Design a simplified Build System similar to Make, Ninja or Bazel.

Each target may depend on other targets.
Requirements:
Build dependencies before dependents.
Build independent targets concurrently.
Build every target exactly once.
Detect cyclic dependencies.
Simulate compilation using worker threads.

                 Build System
                       │
        +--------------+--------------+
        |                             |
 Dependency Graph               Thread Pool
        │                             │
        │                       Worker Threads
        │                             │
        └──────────► Ready Queue ◄────┘
                      (indegree==0)

Time Complexity (kahn's algo) = Space Complexity = O(V + E)

Concepts Covered:
Directed Graph
Topological Sort (Kahn's Algorithm)
Thread Pool
Producer–Consumer Pattern
Condition Variables
Mutex Synchronization
Parallel Task Scheduling
Cycle Detection
*/
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

using namespace std;

class BuildSystem {
private:
  unordered_map<string, vector<string>> graph;
  unordered_map<string, int> indegree;

  queue<string> ready_queue;

  mutex mtx, cout_mtx;
  condition_variable cv;

  int processed_targets = 0, total_targets = 0, active_workers = 0;

  bool done = false;

  void worker(int id) {

    while (true) {
      string target;
      {
        unique_lock<mutex> lock(mtx);
        cv.wait(lock, [&]() { return done == true || !ready_queue.empty(); });

        // graceful shutdown
        if (done && ready_queue.empty())
          return;

        target = ready_queue.front();
        ready_queue.pop();

        active_workers++;
      }

      // Simulate Build
      {
        lock_guard<mutex> lock(cout_mtx);
        cout << "Worker_" << id << " building : " << target << endl;
      }
      this_thread::sleep_for(chrono::milliseconds(500));
      {
        lock_guard<mutex> lock(cout_mtx);
        cout << "Worker_" << id << " finished : " << target << endl;
      }

      {
        unique_lock<mutex> lock(mtx);

        for (auto &child : graph[target]) {
          // Kahn's Topological sorting
          indegree[child]--;
          if (indegree[child] == 0)
            ready_queue.push(child);
        }

        active_workers--;
        processed_targets++;

        // ***VVIMP: Entire Build finished
        if (processed_targets == total_targets || // all nodes are processed
            ready_queue.empty() &&
                active_workers ==
                    0) { // not all nodes processed but deadlock detected
          done = true;
        }

        cv.notify_all();
      }
    }
  }

public:
  void add_dependency(const string &target, const string &dependency) {

    graph[dependency].push_back(target);

    indegree[target]++;

    //***VVIMP: ensure dependency exists in indegree[]
    if (indegree.find(dependency) == indegree.end())
      indegree[dependency] = 0;
  }

  void build(int numThreads) {

    total_targets = indegree.size(); // based on dependency added so far

    // source for Topological Sort
    for (auto &[target, degree] : indegree) {
      if (indegree[target] == 0)
        ready_queue.push(target);
    }

    vector<thread> threads;
    for (int i = 1; i <= numThreads; i++)
      threads.emplace_back(&BuildSystem::worker, this, i);

    for (auto &t : threads)
      t.join();

    if (processed_targets == total_targets)
      cout << "Build Completed Successfully.\n";
    else
      cout << "Cycle detected! Build Failed.\n";
  }
};

int main() {

  /*
              main
             /    \
         parser   utils
         /    \
      lexer   ast
 */
  BuildSystem bs;

  bs.add_dependency("parser", "lexer");
  bs.add_dependency("parser", "ast");
  bs.add_dependency("main", "parser");
  bs.add_dependency("main", "utils");

  bs.build(3);

  return 0;
}
/*
Output:
Worker_1 building : utils
Worker_2 building : ast
Worker_3 building : lexer
Worker_1 finished : utils
Worker_3 finished : lexer
Worker_2 finished : ast
Worker_2 building : parser
Worker_2 finished : parser
Worker_2 building : main
Worker_2 finished : main
Build Completed Successfully.
*/
