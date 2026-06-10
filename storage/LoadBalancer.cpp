/*
Design a Load Balancer that distributes requests among backend servers.
It should support multiple balancing strategies like round-robin and least
connections. Servers may be added or removed dynamically.

            Strategy
               ^
        ----------------
        |              |
 RoundRobin      LeastConnection

          LoadBalancer
                |
          vector<Server>

             Server

Server: Represents a backend server and tracks active connections.
Strategy: Defines how to choose the next server.
RoundRobin / LeastConnection: Implement different load-balancing algorithms.
LoadBalancer: Maintains the server list and delegates server selection to the
chosen strategy.
*/
#include <iostream>
#include <vector>
using namespace std;

class Server {

  int server_id;
  int num_connections;

public:
  Server(int id) : server_id(id), num_connections(0) {}

  int getId() { return server_id; }

  int getConnectionCount() { return num_connections; }

  void connect() { num_connections++; }

  void disconnect() { num_connections--; }
};

class Strategy {
public:
  virtual Server *nextServer(vector<Server> &servers) = 0;
  virtual ~Strategy() = default;
};

class RoundRobin : public Strategy {

  int next_server_id = 0;

public:
  Server *nextServer(vector<Server> &servers) override {

    if (servers.empty())
      return nullptr;

    Server *server = &servers[next_server_id];
    next_server_id = (next_server_id + 1) % servers.size();
    return server;
  }
};

class LeastConnection : public Strategy {
public:
  Server *nextServer(vector<Server> &servers) override {

    if (servers.empty())
      return nullptr;

    Server *best = &servers[0];
    for (Server &server : servers) {
      if (server.getConnectionCount() < best->getConnectionCount())
        best = &server;
    }
    return best;
  }
};

class LoadBalancer {

  Strategy *strategy;
  vector<Server> servers;

public:
  LoadBalancer(Strategy *strategy) : strategy(strategy) {}

  void addServer(int server_id) {

    servers.emplace_back(server_id);
    /*VVIMP:
      emplace_back() constructs the object directly inside the vector,
      avoiding an extra temporary object.
      Server server(101);
      servers.push_back(server);
    */
  }

  Server *getNextServer() { // select nextServer based on strategy
    return strategy->nextServer(servers);
  }

  void removeServer(int server_id) {

    for (auto it = servers.begin(); it != servers.end();) {
      if (it->getId() == server_id)
        it = servers.erase(it);
      else
        it++;
    }
  }
};

int main() {

  RoundRobin rr;
  LoadBalancer lb(&rr);
  // LeastConnection rr;

  // register backend servers
  lb.addServer(101);
  lb.addServer(102);
  lb.addServer(103);

  cout << "Routing Incoming Requests...\n";
  for (int req_id = 1; req_id <= 10; req_id++) {

    Server *server = lb.getNextServer();

    cout << "Request_" << req_id << " routed to Server_" << server->getId()
         << endl;

    // simulate this server to handle request
    server->connect();
  }

  cout << "\nSwitching to Least Connection Strategy\n\n";
  LeastConnection lc;
  LoadBalancer lb2(&lc);

  lb2.addServer(101);
  lb2.addServer(102);
  lb2.addServer(103);

  // Simulate existing server load
  lb2.getNextServer()->connect(); // Server 101 : 1 connection
  lb2.getNextServer()->connect(); // Server 102 : 1 connection
  lb2.getNextServer()->connect(); // Server 103 : 1 connection

  lb2.getNextServer()->connect(); // Server 101 : 2 connections
  lb2.getNextServer()->connect(); // Server 102 : 2 connections

  cout << "Routing requests using Least Connection\n\n";

  for (int requestId = 1; requestId <= 6; requestId++) {

    Server *server = lb2.getNextServer();

    cout << "Request_" << requestId << " routed to Server_" << server->getId()
         << endl;

    server->connect();
  }

  return 0;
}
/*
Output:---------
Routing Incoming Requests...
Request_1 routed to Server_101
Request_2 routed to Server_102
Request_3 routed to Server_103
Request_4 routed to Server_101
Request_5 routed to Server_102
Request_6 routed to Server_103
Request_7 routed to Server_101
Request_8 routed to Server_102
Request_9 routed to Server_103
Request_10 routed to Server_101

Switching to Least Connection Strategy

Routing requests using Least Connection

Request_1 routed to Server_103
Request_2 routed to Server_101
Request_3 routed to Server_102
Request_4 routed to Server_103
Request_5 routed to Server_101
Request_6 routed to Server_102
*/
