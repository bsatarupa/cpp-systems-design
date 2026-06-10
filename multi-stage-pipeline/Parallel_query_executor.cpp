/*
Design a Parallel Query Executor for a database system.
A SQL query has already been parsed and optimized into an execution plan. The
execution engine must execute the query efficiently by processing different
partitions of a table in parallel.

For example: SELECT SUM(salary) FROM Employee WHERE age > 30;
The Employee table contains millions of rows. Instead of scanning the entire
table with a single thread, the table should be partitioned and processed
concurrently by multiple worker threads.

                    Execution Plan
                           │
                           ▼
                 Partition Planner
                           │
      ┌──────────────┬──────────────┬──────────────┬
      ▼              ▼              ▼              ▼
 Partition 1    Partition 2    Partition 3    Partition 4
      │              │              │              │
      ▼              ▼              ▼              ▼
 Worker 1       Worker 2       Worker 3       Worker 4
      │              │              │              │
      └──────────────┴──────────────┴──────────────┘
                           │
                           ▼
                  Result Aggregator
                           │
                           ▼
                     Final Result

Complexity:
N = number of rows
P = number of worker threads

Work: O(N)
Parallel execution time: O(N / P) (ideal case)
Merge cost: O(P)
Extra space: O(P) for partial results

A Parallel Query Executor executes a single query using multiple worker threads.
The execution plan is partitioned into independent tasks, each worker processes
a portion of the data concurrently, computes a local result, and the partial
results are merged to produce the final answer. This demonstrates the Fork–Join
concurrency pattern, efficient data partitioning, parallel computation, and
synchronization.
*/
#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

using namespace std;

struct Row {
  int id;
  int salary;
  int age;
};

struct PartialResult {

  long long sum_salary = 0;
};

class ParallelQueryExecutor {

  vector<Row> table;

  mutex result_mutex;
  long long final_result = 0;

public:
  ParallelQueryExecutor(const vector<Row> &rows) : table(rows) {}

  void worker(int worker_id, int start, int end) {

    PartialResult local_result;

    {
      lock_guard<mutex> lock(result_mutex);
      cout << "Worker_" << worker_id << " processing rows : [" << start << ", "
           << end - 1 << "]\n";
    }

    for (int i = start; i < end; i++) { // row-range
      if (table[i].age > 30)
        local_result.sum_salary += table[i].salary;
    }

    // simulate scan time
    this_thread::sleep_for(chrono::milliseconds(500));

    {
      lock_guard<mutex> lock(result_mutex);

      // need mutex while adding local_result to global_result
      final_result += local_result.sum_salary;

      cout << "Worker_" << worker_id
           << " partial sum : " << local_result.sum_salary << endl;
    }
  }

  long long execute(int numThreads) {

    vector<thread> worker_threads;

    int n = table.size();
    int chunk_size = (n + numThreads - 1) / numThreads;

    for (int i = 0; i < numThreads; i++) {

      int start = i * chunk_size;
      if (start >= n)
        break; //***VVIMP

      int end = min(start + chunk_size, n);

      worker_threads.emplace_back(&ParallelQueryExecutor::worker, this, i + 1,
                                  start, end);
    }

    for (auto &t : worker_threads)
      t.join();

    return final_result;
  }
};

int main() {

  vector<Row> employee_table;

  for (int i = 1; i <= 20; i++)
    employee_table.push_back({i, 1000 * i, 20 + (i % 25)}); // id, salary, age

  ParallelQueryExecutor executor(employee_table);

  long long ans = executor.execute(4);

  cout << "\nFinal SUM(salary) where age > 30 = " << ans << endl;

  return 0;
}
/*
Output:
Worker_1 processing rows : [0, 4]
Worker_2 processing rows : [5, 9]
Worker_3 processing rows : [10, 14]
Worker_4 processing rows : [15, 19]
Worker_2 partial sum : 0
Worker_4 partial sum : 90000
Worker_1 partial sum : 0
Worker_3 partial sum : 65000

Final SUM(salary) where age > 30 = 155000
*/
