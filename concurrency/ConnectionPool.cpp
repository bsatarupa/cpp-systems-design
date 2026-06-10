/*
Design a connection pool that manages reusable database connections. Clients
should acquire and release connections efficiently. The pool should limit the
maximum number of active connections.

Although both reuse expensive resources, they manage different things:
Thread Pool → reuses worker threads to execute tasks.
Connection Pool → reuses database connections.

                ConnectionPool
                     |
        ----------------------------
        |            |             |
    Connection1  Connection2  Connection3

Clients:
Connection* conn = pool.acquire();
// Use database connection
pool.release(conn);
*/
#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>

using namespace std;

class Connection {

  int connection_id;

public:
  Connection(int id) : connection_id(id) {}

  int getId() const { return connection_id; }
};

class ConnectionPool {

  queue<Connection *> available_connections;

  mutex mtx;
  condition_variable cv;

public:
  ConnectionPool(int max_connections) { // fill up queue with #max_connections

    for (int conn_id = 1; conn_id <= max_connections; conn_id++)
      available_connections.push(new Connection(conn_id));
  }

  Connection *acquire() {

    unique_lock<mutex> lock(mtx);
    cv.wait(lock, [&]() { return !available_connections.empty(); });

    Connection *conn = available_connections.front();
    available_connections.pop();
    return conn;
  }

  void release(Connection *conn) {
    {
      lock_guard<mutex> lock(mtx);
      available_connections.push(conn);
    }
    cv.notify_one();
  }

  ~ConnectionPool() {

    while (!available_connections.empty()) {
      Connection *conn = available_connections.front();
      available_connections.pop();
      delete conn;
    }
  }
};

int main() {

  ConnectionPool pool(2);

  vector<thread> clients;
  mutex print_mtx;

  for (int client_id = 1; client_id <= 5; client_id++) {
    clients.emplace_back([&, client_id]() {
      Connection *conn = pool.acquire();
      {
        lock_guard<mutex> lock(print_mtx);
        cout << "Client_" << client_id << " acquired connection_"
             << conn->getId() << endl;
      }
      this_thread::sleep_for(chrono::seconds(2));

      {
        lock_guard<mutex> lock(print_mtx);
        cout << "Client_" << client_id << " released connection_"
             << conn->getId() << endl;
      }
      pool.release(conn);
    });
  }

  for (auto &t : clients)
    t.join();

  return 0;
}

/*
 Output:
Client_1 acquired connection_1
Client_2 acquired connection_2
Client_1 released connection_1
Client_2 released connection_2
Client_3 acquired connection_1
Client_4 acquired connection_2
Client_4 released connection_2
Client_3 released connection_1
Client_5 acquired connection_2
Client_5 released connection_2

1. Why not create a new connection every time?

Creating a database connection is expensive (authentication, TCP handshake, SSL,
etc.). Reusing existing connections improves latency and throughput.

2. What if all connections are busy?

Current solution:

cv.wait(...)

Clients block until one is released.

Alternative: implement

Connection* tryAcquire();

or

Connection* acquire(timeout);
3. Why use a queue?

It provides fair reuse of connections (FIFO). A stack would also work but tends
to reuse the most recently released connection.

4. Is this thread-safe?

Yes. The queue is protected by a mutex, and waiting clients use a
condition_variable.

5. Production improvements
Use std::unique_ptr<Connection> instead of raw pointers.
Validate connections before handing them out.
Recreate broken connections.
Support idle timeout and connection health checks.
Add acquire(timeout) to avoid waiting indefinitely.
*/
