#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace std;

class Multi_threaded_BFS {

  unordered_map<int, vector<int>> graph;

  queue<int> Q; // BFS queue
  unordered_set<int> visited;

  vector<thread> worker_threads;

  mutex mtx;
  condition_variable cv;

  bool stop; // graceful shutdown
  mutex printMtx;

public:
  Multi_threaded_BFS(int num_threads) : stop(false) {

    for (int i = 1; i <= num_threads;
         i++) // worker threads should run inside constructor
      worker_threads.emplace_back(&Multi_threaded_BFS::worker, this, i);
  }

  ~Multi_threaded_BFS() { shutdown(); }

  void worker(int worker_thread_id) {

    int node;
    while (true) {

      { // critical section
        unique_lock<mutex> lock(mtx);
        cv.wait(lock, [&]() { return stop == true || !Q.empty(); });

        if (stop == true && Q.empty())
          return; // graceful shutdown

        node = Q.front();
        Q.pop();
      }

      // process node outside lock
      process_node(node, worker_thread_id);
    }
  }

  void addEdge(int u, int v) { // BFS is always on undirected graph

    graph[u].push_back(v);
    graph[v].push_back(u);
  }

  void startBFS(int source) {

    { // critical section
      lock_guard<mutex> lock(mtx);
      Q.push(source);
      visited.insert(source);
    }
    cv.notify_all();
  }

  void process_node(int node, int worker_thread_id) {

    { // critical section
      lock_guard<mutex> printLock(printMtx);
      cout << "Worker_" << worker_thread_id << " processing Node : " << node
           << endl;
    }
    // Simulating Work
    this_thread::sleep_for(chrono::milliseconds(500));

    for (int neighbour : graph[node]) {

      bool should_notify_worker = false;

      { // critical section
        lock_guard<mutex> lock(mtx);
        if (visited.count(neighbour) == 0) {
          visited.insert(neighbour);
          Q.push(neighbour);

          should_notify_worker = true;
        }
      }

      if (should_notify_worker)
        cv.notify_one();
    }
  }

  void shutdown() {

    {
      lock_guard<mutex> lock(mtx);
      stop = true;
    }
    cv.notify_all();

    // join() is the last option in shutdown(), so sits after notify_all()
    for (auto &t : worker_threads) {
      if (t.joinable())
        t.join();
    }
  }
};

int main() {

  Multi_threaded_BFS bfs(4);

  bfs.addEdge(1, 2);
  bfs.addEdge(1, 3);
  bfs.addEdge(2, 4);
  bfs.addEdge(2, 5);
  bfs.addEdge(3, 7);
  bfs.addEdge(3, 6);

  bfs.startBFS(1);

  // allow workers to process
  this_thread::sleep_for(chrono::seconds(5));

  bfs.shutdown();

  return 0;
}
/*
Worker_2 processing Node : 1
Worker_3 processing Node : 2
Worker_2 processing Node : 3
Worker_2 processing Node : 7
Worker_1 processing Node : 4
Worker_4 processing Node : 6
Worker_3 processing Node : 5
*/
