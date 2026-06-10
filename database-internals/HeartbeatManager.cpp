/*
Design a Heartbeat Manager that tracks heartbeats from multiple nodes and
reports which nodes are alive or dead. Ignore networking and timers.

        HeartbeatManager
        -----------------------------
        lastHeartbeat
        timeout
        -----------------------------
        receiveHeartbeat()
        isAlive()
        checkDeadNodes()
*/
#include <iostream>
#include <unordered_map>
#include <vector>
using namespace std;

class HeartbeatManager {

  int timeout;
  unordered_map<string, int> last_Heartbeat; //<node_id, last_timestamp>
public:
  HeartbeatManager(int timeout) : timeout(timeout) {}

  void receive_heartbeat(const string &node_id, int timestamp) {

    last_Heartbeat[node_id] = timestamp;
    cout << "Heartbeat received from : " << node_id
         << " at timestamp : " << timestamp << endl;
  }

  bool is_alive(const string &node_id, int curr_time) {

    auto it = last_Heartbeat.find(node_id);
    if (it == last_Heartbeat.end())
      return false;

    return (curr_time - it->second) <= timeout;
  }

  vector<string> check_dead_nodes(int curr_time) {

    vector<string> dead_nodes;
    for (const auto &[node_id, last_seen] : last_Heartbeat) {

      if (curr_time - last_seen > timeout)
        dead_nodes.push_back(node_id);
    }
    return dead_nodes;
  }
};

int main() {

  HeartbeatManager manager(5); // timeout = 5 seconds

  manager.receive_heartbeat("Node1", 1);
  manager.receive_heartbeat("Node2", 2);
  manager.receive_heartbeat("Node3", 4);

  cout << "\nAt time = 6\n";
  cout << "Node1 Alive: " << manager.is_alive("Node1", 6) << endl;
  cout << "Node2 Alive: " << manager.is_alive("Node2", 6) << endl;
  cout << "Node3 Alive: " << manager.is_alive("Node3", 6) << endl;

  cout << "\nAt time = 10\n";
  vector<string> dead = manager.check_dead_nodes(10);
  cout << "Dead Nodes:\n";
  for (const auto &node : dead)
    cout << node << endl;

  return 0;
}
/*
Output:
At time = 6
Node1 Alive: 1
Node2 Alive: 1
Node3 Alive: 1

At time = 10
Dead Nodes:
Node3
Node2
Node1

Follow-up Q&A:
1. What if a heartbeat is delayed?
Don't immediately declare the node dead. In production systems, we typically:
Allow a timeout window.
Require multiple missed heartbeats before marking a node dead.

2. How would you scale to thousands of nodes?
Current approach:
unordered_map<NodeId, LastHeartbeat>

For very large clusters:
Partition nodes across heartbeat managers. Use a distributed membership service.

3. How is this used in distributed databases?
Heartbeat managers are commonly used to:
Detect leader failure.
Trigger leader election.
Remove failed replicas from service.
Monitor cluster health.

4. Can checkDeadNodes() be optimized?
The current implementation scans every node (O(N)). If checks are very frequent
and the cluster is large, you could maintain a min-heap ordered by heartbeat
expiry time so that expired nodes can be found more efficiently.

*/
