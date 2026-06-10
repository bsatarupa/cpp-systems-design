/*
Design a multithreaded database query execution pipeline.
A database receives SQL queries from multiple clients. Each query passes through
the following stages: Parser – Converts SQL into a parse tree. Optimizer –
Generates an efficient execution plan. Executor – Executes the plan and returns
the result. Each stage has a dedicated pool of worker threads, and consecutive
stages communicate through thread-safe blocking queues.

Design a concurrent system that:
Supports multiple client threads submitting queries simultaneously.
Uses the Producer–Consumer pattern between stages.
Processes multiple queries concurrently.
Ensures thread-safe communication using mutexes and condition variables.
Maximizes throughput while avoiding race conditions.

                    Clients
                (Producer Threads)
                       │
                       ▼
            ┌────────────────────┐
            │   Request Queue    │
            └────────────────────┘
                       │
                       ▼
              Parser Thread Pool
            (Consumer + Producer)
                       │
                       ▼
            ┌────────────────────┐
            │   Parsed Queue     │
            └────────────────────┘
                       │
                       ▼
            Optimizer Thread Pool
            (Consumer + Producer)
                       │
                       ▼
            ┌────────────────────┐
            │    Plan Queue      │
            └────────────────────┘
                       │
                       ▼
             Executor Thread Pool
                 (Consumer)
                       │
                       ▼
                Query Results

Each stage is:
Consumer of the previous queue.
Producer for the next queue.

Synchronization:
mutex protects each queue.
condition_variable blocks consumers when a queue is empty.
Multiple workers can process different queries simultaneously.
Each stage has an independent thread pool.
a classic multi-stage producer–consumer pipeline.

Complexity
Time Complexity: O(1) enqueue/dequeue for each queue operation (excluding
processing time at each stage). Space Complexity: O(Q), where Q is the total
number of queries waiting across all pipeline queues. Summary

A Multistage Query Execution Pipeline models how modern database systems process
many queries concurrently. Independent thread pools perform parsing,
optimization, and execution, while thread-safe blocking queues connect adjacent
stages. This architecture improves throughput by allowing different queries to
be processed simultaneously at different stages and naturally demonstrates
producer–consumer synchronization, pipeline parallelism, bottleneck analysis,
and scalable concurrent system design—the core concepts commonly evaluated in
systems, storage, and database interviews.
*/
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>
using namespace std;
//==============Data Structures================
struct Query {

  int query_id;
  string sql;
};

struct ParsedQuery {

  int query_id;
  string parse_tree;
};

struct ExecutionPlan {

  int query_id;
  string plan;
};

//================Blocking Queue=================
template <typename T>

class BlockingQueue {
private:
  queue<T> Q;
  mutex mtx;
  condition_variable cv;

public:
  void push(T &item) {

    {
      lock_guard<mutex> lock(mtx);
      Q.push(item);
    }
    cv.notify_one();
  }

  T pop() {

    unique_lock<mutex> lock(mtx);
    cv.wait(lock, [&] { return !Q.empty(); });

    T item = Q.front();
    Q.pop();

    return item;
  }

  size_t size() {

    lock_guard<mutex> lock(mtx);
    return Q.size();
  }
};

class QueryEngine {
private:
  //================Shared Queues====================
  BlockingQueue<Query> requestQueue;
  BlockingQueue<ParsedQuery> parsedQueue;
  BlockingQueue<ExecutionPlan> planQueue;

  mutex cout_mutex;

  atomic<bool> running{true};

public:
  //===============Client============================
  void clientThread(int client_id) {

    int query_id = client_id * 100;

    while (running) {

      Query q{query_id++, "SELECT * from Employee"};

      {
        lock_guard<mutex> lock(cout_mutex);
        cout << "Client_id" << client_id << " submitted query : " << q.query_id
             << endl;
      }

      requestQueue.push(q);

      this_thread::sleep_for(chrono::milliseconds(500));
    }
  }

  //==============Parser==============================
  void parserWorker(int parser_id) {

    while (running) {

      Query q = requestQueue.pop();

      {
        lock_guard<mutex> lock(cout_mutex);
        cout << "Parser_id" << parser_id << " parsing query : " << q.query_id
             << endl;
      }
      this_thread::sleep_for(chrono::milliseconds(800));

      ParsedQuery pq{q.query_id, "Parse Tree"};

      parsedQueue.push(pq);
    }
  }

  //==============Optimizer==============================
  void optimizerWorker(int optimizer_id) {

    while (running) {

      ParsedQuery pq = parsedQueue.pop();

      {

        lock_guard<mutex> lock(cout_mutex);
        cout << "Optimizer_id" << optimizer_id
             << " optimizing query : " << pq.query_id << endl;
      }
      this_thread::sleep_for(chrono::milliseconds(1200));

      ExecutionPlan plan{pq.query_id, "Index Scan"};

      planQueue.push(plan);
    }
  }

  //==============Executor==============================
  void executorWorker(int executor_id) {

    while (running) {

      ExecutionPlan plan = planQueue.pop();

      {
        lock_guard<mutex> lock(cout_mutex);
        cout << "Executor_id" << executor_id
             << " executing query : " << plan.query_id << endl;
      }
      this_thread::sleep_for(chrono::milliseconds(2000));

      {

        lock_guard<mutex> lock(cout_mutex);
        cout << "******Query : " << plan.query_id << " Completed******" << endl;
      }
    }
  }
};
//==============main==================================
int main() {

  QueryEngine engine;

  vector<thread> threads;

  // 2 clients
  for (int i = 0; i < 2; i++)
    threads.emplace_back(&QueryEngine::clientThread, &engine, i);

  // 3 parser threads
  for (int i = 0; i < 3; i++)
    threads.emplace_back(&QueryEngine::parserWorker, &engine, i);

  // 2 optimizer threads
  for (int i = 0; i < 2; i++)
    threads.emplace_back(&QueryEngine::optimizerWorker, &engine, i);

  // 3 executor threads
  for (int i = 0; i < 3; i++)
    threads.emplace_back(&QueryEngine::executorWorker, &engine, i);

  for (auto &t : threads)
    t.join();

  return 0;
}
/*
Output:
Client_id0 submitted query : 0
Client_id1 submitted query : 100
Parser_id0 parsing query : 0
Parser_id1 parsing query : 100
Client_id0 submitted query : 1
Parser_id2 parsing query : 1
Client_id1 submitted query : 101
Parser_id0 parsing query : 101
Optimizer_id0 optimizing query : 0
Optimizer_id1 optimizing query : 100
Client_id0 submitted query : 2
Client_id1 submitted query : 102
Parser_id1 parsing query : 2
Parser_id2 parsing query : 102
Client_id1 submitted query : 103
Client_id0 submitted query : 3
Parser_id0 parsing query : 103
Parser_id1 parsing query : 3
Client_id1 submitted query : 104
Executor_id0 executing query : 0
Executor_id1 executing query : 100
Optimizer_id0 optimizing query : 1
Optimizer_id1 optimizing query : 101
Client_id0 submitted query : 4
Parser_id2 parsing query : 104
Parser_id0 parsing query : 4
Client_id1 submitted query : 105
Client_id0 submitted query : 5
Parser_id1 parsing query : 105
Parser_id2 parsing query : 5
Client_id1 submitted query : 106
Client_id0 submitted query : 6
Optimizer_id0 optimizing query : 2
Executor_id2 executing query : 1
Optimizer_id1 optimizing query : 102
Parser_id0 parsing query : 106
Parser_id1 parsing query : 6
Client_id1 submitted query : 107
Client_id0 submitted query : 7
Parser_id2 parsing query : 107
******Query : 0 Completed******
Executor_id0 executing query : 101
...
*/
