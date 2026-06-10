/*
Design a Replication Service supporting synchronous and asynchronous
replication. Updates should be propagated to replicas reliably. Replica failures
should be handled gracefully.

Implementation:
Replica encapsulates replica state (alive) and write behavior.
ReplicationStrategy provides the abstraction for different replication modes.
SyncReplication and AsyncReplication implement different propagation policies.
ReplicationService coordinates replication without knowing the concrete
strategy.

              ReplicationStrategy
                     ^
        ----------------------------
        |                          |
 SyncReplication         AsyncReplication
                     ^
                     |
             ReplicationService
                     |
             vector<Replica*>

How would you make async replication real?
Replace the cout << "Queued update..." with a queue<string> of pending updates.
Run a background worker thread that dequeues updates and calls Replica::write().
Retry failed replicas with exponential backoff or maintain a write-ahead log for
durability.
*/
#include <iostream>
#include <vector>
using namespace std;

class Replica {
  int replica_id;
  bool alive = true;

public:
  Replica(int id) : replica_id(id), alive(true) {}

  int getId() { return replica_id; }
  bool isAlive() { return alive; }
  void setAlive(bool status) { alive = status; }
  void write(const string &data) {
    cout << "\nReplica_" << replica_id << " stored data : " << data << '\n';
  }
};

class ReplicationStrategy {
public:
  virtual void replicate(vector<Replica *> &replicas, const string &data) = 0;
  virtual ~ReplicationStrategy() = default;
};

class SyncReplication : public ReplicationStrategy {
public:
  void replicate(vector<Replica *> &replicas, const string &data) override {
    cout << "\nSynchronous Replication\n";

    for (Replica *replica : replicas) {

      if (!replica->isAlive()) {
        cout << "\nReplica_" << replica->getId() << " unavailable\n";
        continue; // can't write into failed replica
      }
      replica->write(data);
    }
  }
};

class AsyncReplication : public ReplicationStrategy {
public:
  void replicate(vector<Replica *> &replicas, const string &data) override {
    cout << "\nAsynchronous Replication\n";

    for (Replica *replica : replicas) {

      if (!replica->isAlive()) {
        cout << "\nReplica_" << replica->getId() << " unavailable\n";
        continue; // can't write into failed replica
      }

      /*
       In production I would enqueue the update into a replication queue and
       a background worker thread would propagate it to replicas.
       I kept it synchronous here to focus on the Strategy pattern and time
       constraint.
       */
      cout << "\nQueued/scheduled update for Replica_" << replica->getId();
      replica->write(data);
    }
  }
};

class ReplicationService {

  ReplicationStrategy *strategy;
  vector<Replica *> replicas;

public:
  ReplicationService(ReplicationStrategy *strategy) : strategy(strategy) {}

  void addReplica(Replica *replica) { replicas.push_back(replica); }

  void replicate(const string &data) { strategy->replicate(replicas, data); }
};

int main() {

  SyncReplication sync;
  AsyncReplication async;

  Replica r1(1);
  Replica r2(2);
  r2.setAlive(false);
  Replica r3(3);

  cout << "---------Sync Replication----------\n";
  ReplicationService sync_service(&sync);
  sync_service.addReplica(&r1);
  sync_service.addReplica(&r2);
  sync_service.addReplica(&r3);

  sync_service.replicate("Hello");

  cout << "\n---------Async Replication----------\n";
  ReplicationService async_service(&async);
  async_service.addReplica(&r1);
  async_service.addReplica(&r2);
  async_service.addReplica(&r3);

  async_service.replicate("World");

  return 0;
}
/*
Output:
---------Sync Replication----------

Synchronous Replication

Replica_1 stored data : Hello

Replica_2 unavailable

Replica_3 stored data : Hello

---------Async Replication----------

Asynchronous Replication

Queued/scheduled update for Replica_1
Replica_1 stored data : World

Replica_2 unavailable

Queued/scheduled update for Replica_3
Replica_3 stored data : World

Q. How would you make this production-ready?

Reliable propagation:
Persist updates in a Write-Ahead Log (WAL) or replication log before sending.
Retry failed replicas with exponential backoff.

True asynchronous replication:
Replace the cout << "Queued..." with a queue.
A background worker thread dequeues updates and sends them to replicas.

Failure handling:
Mark replicas as unhealthy after repeated failures.
Retry or resynchronize them when they recover.

Extensibility: Add new strategies like -
QuorumReplication
ChainReplication
GeoReplication

without modifying ReplicationService.
*/
