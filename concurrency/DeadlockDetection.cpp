/*
Design a Deadlock Detector for a database lock manager.
Transactions may wait for other transactions holding locks.
Maintain a Wait-For Graph.
Detect whether adding a new dependency introduces a deadlock.
If a cycle exists, report a deadlock.

          Transaction Manager
                  │
                  ▼
            Lock Manager
                  │
                  ▼
           Wait-For Graph
      (Txn → Waiting For Txn)
                  │
                  ▼
         DFS Cycle Detection
                  │
        Deadlock? (Yes / No)

Complexity:
V = transactions
E = wait dependencies
Add Dependency : O(1)
Remove Transaction : O(E)
Detect Deadlock : DFS traverses every node and edge once. O(V + E)
Space : O(V + E)

Production Improvements:
After coding this, mention ---
Run detection periodically instead of after every edge insertion.
Abort the youngest or lowest-priority transaction as the deadlock victim.
Store edge metadata (lock type, resource ID).
Support shared/exclusive locks.
Integrate with a lock manager so edges are added when transactions block and
removed when locks are released. Use timestamps or transaction priorities to
choose the victim.

In a production lock manager, when a transaction commits or aborts,
I'd remove its outgoing and incoming edges from the wait-for graph.
I omitted that here since the question focuses on cycle detection.
*/
#include <iostream>
#include <unordered_set>

using namespace std;

class DeadlockDetector {

  // wait-for Graph
  unordered_map<int, vector<int>> graph;

  bool isCycle(int txn, unordered_set<int> &visited,
               unordered_set<int> &rec_stack) {

    visited.insert(txn);
    rec_stack.insert(txn);

    for (int next : graph[txn]) {

      if (visited.count(next) == 0 && isCycle(next, visited, rec_stack) == true)
        return true;

      if (visited.count(next) && rec_stack.count(next)) // VVIMP
        return true;
    }

    rec_stack.erase(txn);
    // Backtrack; Erase from currently active recursion path only, not from
    // visited

    return false;
  }

public:
  // waitingTxn waits for holdingTxn
  void addDependency(int waitingTxn, int holdingTxn) {
    graph[waitingTxn].push_back(holdingTxn);
  }

  bool detectDeadlock() {

    unordered_set<int> visited;
    unordered_set<int> rec_stack; // currently active recursion path

    for (auto &[txn, neighbour] : graph) {

      if (visited.count(txn) == 0 && isCycle(txn, visited, rec_stack) == true) {
        return true;
      }
    }
    return false;
  }
};

int main() {

  DeadlockDetector detector;

  // T1 waits for T2
  detector.addDependency(1, 2);

  // T2 waits for T3
  detector.addDependency(2, 3);

  // T3 waits for T1
  detector.addDependency(3, 1);

  if (detector.detectDeadlock())
    cout << "Deadlock Detected\n";
  else
    cout << "No Deadlock\n";

  return 0;
}
// Output: Deadlock Detected
