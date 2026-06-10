/*
Design and implement a thread-safe Lock Manager that supports Shared (S) and
Exclusive (X) locks on resources.

The lock manager should:
Allow multiple transactions to simultaneously acquire shared locks on the same
resource. Allow only one transaction to hold an exclusive lock on a resource.
Prevent conflicting lock acquisitions.
Give priority to waiting writers to avoid writer starvation.
Support lock upgrades (Shared → Exclusive).
Support lock downgrades (Exclusive → Shared).
Automatically clean up lock metadata when a resource is no longer locked.

                    +--------------------+
                    |    Transactions    |
                    +---------+----------+
                              |
                              |
                    acquire/release/upgrade
                              |
                              ▼
                  +------------------------+
                  |      Lock Manager      |
                  +------------------------+
                  | mutex                  |
                  | lock_table             |
                  +-----------+------------+
                              |
            +-----------------+------------------+
            |                                    |
            ▼                                    ▼
        Resource A                          Resource B
      +-------------+                    +-------------+
      | LockInfo    |                    | LockInfo    |
      |-------------|                    |-------------|
      | SharedOwners|                    | SharedOwners|
      | Exclusive   |                    | Exclusive   |
      | WaitingWtrs |                    | WaitingWtrs |
      | CV          |                    | CV          |
      +-------------+                    +-------------+

Space Complexity: O(R + L)
R = number of resources currently in the lock table
L = total number of shared lock owners across all resources

Time complexity: O(1)

Production Improvements (Discussion):
A production-grade database lock manager would typically extend this design
with: Wait-for graph for deadlock detection. Deadlock victim selection to
resolve cycles. Lock timeouts to avoid indefinite waits. FIFO wait queues for
stronger fairness guarantees. Hierarchical (Intention) locks such as IS, IX, and
SIX for table/page/row locking. Multi-granularity locking across different
resource levels. Transaction manager integration for automatic lock release on
commit or abort.
*/
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>

using namespace std;

enum LockType { SHARED, EXCLUSIVE };

class LockManager {

  struct LockInfo { // lockinfo per resource_id

    unordered_set<int> shared_owners; // Txns holding shared locks
    int exclusive_owner = -1;         // txn holding exclusive lock
    int waitingWriters = 0;           // writer preference

    condition_variable cv; // per resource
  };

  unordered_map<string, LockInfo> lock_table;
  //<key = resource_id, value = LockInfo for individual resource>

  mutex mtx;

  // after every lock Release, check for cleanup conditions
  void cleanup(const string &resource) {

    auto it = lock_table.find(resource);
    if (it == lock_table.end())
      return;

    LockInfo &info = it->second;

    if (info.shared_owners.empty() && info.exclusive_owner == -1 &&
        info.waitingWriters == 0) {
      lock_table.erase(resource);
    }
  }

public:
  void acquire_shared_lock(int txn_id, const string &resource) {

    unique_lock<mutex> lock(mtx);
    LockInfo &info = lock_table[resource]; // lock_state of given resource

    info.cv.wait(lock, [&]() {
      return info.exclusive_owner == -1 && info.waitingWriters == 0;
    });

    info.shared_owners.insert(txn_id);

    cout << "Txn_" << txn_id
         << " acquired SHARED Lock on Resource : " << resource << endl;
  }

  void release_shared_lock(int txn_id, const string &resource) {

    unique_lock<mutex> lock(mtx);

    auto it = lock_table.find(resource);
    if (it == lock_table.end())
      return;

    LockInfo &info = it->second;

    info.shared_owners.erase(txn_id);
    if (info.shared_owners.empty())
      info.cv.notify_all(); // notify waiting writer, if any

    cout << "Txn_" << txn_id
         << " released SHARED Lock from Resource : " << resource << endl;

    cleanup(resource);
  }

  void acquire_exclusive_lock(int txn_id, const string &resource) {

    unique_lock<mutex> lock(mtx);

    LockInfo &info = lock_table[resource]; // lock_state of given resource

    info.waitingWriters++;

    info.cv.wait(lock, [&]() {
      return info.exclusive_owner == -1 && info.shared_owners.empty() == true;
    });

    info.waitingWriters--;
    info.exclusive_owner = txn_id;

    cout << "Txn_" << txn_id
         << " acquired EXCLUSIVE Lock on Resource : " << resource << endl;
  }

  void release_exclusive_lock(int txn_id, const string &resource) {

    unique_lock<mutex> lock(mtx);

    auto it = lock_table.find(resource);
    if (it == lock_table.end())
      return;

    LockInfo &info = it->second;

    if (info.exclusive_owner == txn_id)
      info.exclusive_owner = -1;

    info.cv.notify_all();

    cout << "Txn_" << txn_id
         << " released EXCLUSIVE Lock from Resource : " << resource << endl;

    cleanup(resource);
  }

  // upgrade and downgrade locks
  void upgrade(int txn_id, const string &resource) {

    unique_lock<mutex> lock(mtx);
    LockInfo &info = lock_table[resource];

    if (info.shared_owners.count(txn_id) == 0) {
      cout << "Txn_" << txn_id << " does not own SHARED lock\n";
      return;
    }

    // give up shared ownership
    info.shared_owners.erase(txn_id);

    // upgrade to EXCLUSIVE lock
    info.waitingWriters++;

    info.cv.wait(lock, [&]() {
      return info.exclusive_owner == -1 && info.shared_owners.empty();
    });

    info.exclusive_owner = txn_id;
    info.waitingWriters--;

    cout << "Txn_" << txn_id
         << " upgraded to EXCLUSIVE lock on Resource : " << resource << endl;
  }

  void downgrade(int txn_id, const string &resource) {

    unique_lock<mutex> lock(mtx);
    LockInfo &info = lock_table[resource];

    if (info.exclusive_owner != txn_id) {
      cout << "Txn_" << txn_id << " does not own EXCLUSIVE lock\n";
      return;
    }

    // give up EXCLUSIVE ownership
    info.exclusive_owner = -1;

    // downgrade to SHARED lock
    info.shared_owners.insert(txn_id);

    cout << "Txn_" << txn_id
         << " downgraded to SHARED lock on Resource : " << resource << endl;

    info.cv.notify_all(); // notify other READERs
  }
};

int main() {

  LockManager lm;

  thread reader1([&]() {
    lm.acquire_shared_lock(1, "row1");
    this_thread::sleep_for(chrono::seconds(2));
    lm.release_shared_lock(1, "row1");
  });

  thread reader2([&]() {
    lm.acquire_shared_lock(2, "row1");
    this_thread::sleep_for(chrono::seconds(1));
    lm.release_shared_lock(2, "row1");
  });

  thread writer1([&]() {
    this_thread::sleep_for(chrono::milliseconds(500));
    lm.acquire_exclusive_lock(3, "row1");
    this_thread::sleep_for(chrono::seconds(1));
    lm.release_exclusive_lock(3, "row1");
  });

  reader1.join();
  reader2.join();
  writer1.join();

  thread upgrader([&]() {
    lm.acquire_shared_lock(10, "row2");
    this_thread::sleep_for(chrono::seconds(1));
    lm.upgrade(10, "row2");
    this_thread::sleep_for(chrono::seconds(1));
    lm.release_exclusive_lock(10, "row2");
  });
  upgrader.join();

  thread downgrader([&]() {
    lm.acquire_exclusive_lock(20, "row3");
    this_thread::sleep_for(chrono::seconds(1));
    lm.downgrade(20, "row3");
    this_thread::sleep_for(chrono::seconds(1));
    lm.release_shared_lock(20, "row3");
  });
  downgrader.join();

  return 0;
}
/*
Txn_2 acquired SHARED Lock on Resource : row1
Txn_1 acquired SHARED Lock on Resource : row1
Txn_2 released SHARED Lock from Resource : row1
Txn_1 released SHARED Lock from Resource : row1
Txn_3 acquired EXCLUSIVE Lock on Resource : row1
Txn_3 released EXCLUSIVE Lock from Resource : row1
Txn_10 acquired SHARED Lock on Resource : row2
Txn_10 upgraded to EXCLUSIVE lock on Resource : row2
Txn_10 released EXCLUSIVE Lock from Resource : row2
Txn_20 acquired EXCLUSIVE Lock on Resource : row3
Txn_20 downgraded to SHARED lock on Resource : row3
Txn_20 released SHARED Lock from Resource : row3
*/
