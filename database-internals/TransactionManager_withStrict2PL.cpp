/*
Design and implement a Transaction Manager using Strict 2PL that supports
beginning, reading, writing, committing, and aborting transactions.
Multiple transactions may execute concurrently. Design the system so that
concurrency control mechanisms such as MVCC can be incorporated later.

            TransactionManager
            -----------------------------
            transactions
            LockManager
            -----------------------------
            begin()
            read()
            write()
            commit()
            abort()
                   |
                   |
           -----------------
           |               |
     Transaction      LockManager

Complexity:
beginTransaction, read, write : O(1) for using hash tables.
commit, abort : O(L), L = #locked keys

This code demonstrates the core idea of Strict 2PL:
read() acquires a Shared (S) lock.
write() acquires an Exclusive (X) lock.
No lock is released before commit() or abort(), satisfying the "strict"
property. A transaction can upgrade from S → X if it is the only shared owner.

This implementation intentionally simplifies a few production concerns:
No waiting or blocking: conflicting lock requests fail immediately.
A real implementation would block and wake waiting transactions.
No deadlock detection: production systems use a wait-for graph, timeout, or
deadlock detector.
***No undo logging: writes are applied directly to the in-memory database, so
abort() does not roll back changes. A real transaction manager would buffer
writes or maintain an undo log/WAL.
*/
#include <iostream>
#include <string>
#include <unordered_map>
#include <unordered_set>

using namespace std;

enum class Txnstate { ACTIVE, COMMITTED, ABORTED };

class Transaction {

  int txn_id;
  Txnstate state;

public:
  Transaction(int id) : txn_id(id), state(Txnstate::ACTIVE) {}

  int getID() { return txn_id; }

  Txnstate getState() { return state; }

  void commit() { state = Txnstate::COMMITTED; }

  void abort() { state = Txnstate::ABORTED; }
};

struct Lock {

  unordered_set<int> shared_owners;
  int exclusive_owner = -1;
};

class LockManager {

  unordered_map<string, Lock> lock_table; //<key -> Lock mapping
public:
  bool acquire_shared_lock(int txn_id, const string &key) {

    Lock &lock = lock_table[key];

    if (lock.exclusive_owner == -1 || lock.exclusive_owner == txn_id) {
      // only if there exists no other Writer Lock on key, we can acquire a
      // shared lock

      lock.shared_owners.insert(txn_id);
      return true;
    }
    return false;
  }

  bool acquire_exclusive_lock(int txn_id, const string &key) {

    Lock &lock = lock_table[key];

    if (lock.exclusive_owner == txn_id) // txn already acquired the lock
      return true;

    if (lock.exclusive_owner !=
        -1) // some other txn is holding exclusion lock on same key
      return false;

    // if already acquired shared lock on key
    if (!lock.shared_owners.empty()) {

      if (lock.shared_owners.size() == 1 &&
          lock.shared_owners.count(txn_id) == 1)
        return true; // just convert lock from shared -> exclusion for same
                     // owner txn

      else
        return false; // some other txn is holding shared lock on same key
    }

    // finally txn acquire exclusive lock on given key
    lock.exclusive_owner = txn_id;
    return true;
  }

  void release_all_locks(int txn_id) { // release all locks acquired by txn_id

    for (auto &[key, lock] : lock_table) {

      lock.shared_owners.erase(txn_id);

      if (lock.exclusive_owner == txn_id)
        lock.exclusive_owner = -1;
    }
  }
};

class TransactionManager {

  int next_txn_id = 1;

  unordered_map<int, Transaction *> transactions; //<txn_id, Transaction>
  unordered_map<string, string>
      database; //<key, value> added to DB during write()

  LockManager lockManager;

  bool isActive(int txn_id) { // search for the txn in transactions table

    auto it = transactions.find(txn_id);
    return it != transactions.end() &&
           it->second->getState() == Txnstate::ACTIVE;
  }

public:
  ~TransactionManager() {

    for (auto &[txn_id, txn] : transactions)
      delete txn;
  }

  int beginTransaction() {

    int txn_id = next_txn_id++;
    transactions[txn_id] = new Transaction(txn_id);

    cout << "Begin transaction : " << txn_id << endl;
    return txn_id;
  }

  string read(int txn_id, const string &key) {

    if (!isActive(txn_id))
      return "";

    if (!lockManager.acquire_shared_lock(txn_id, key)) {

      cout << "Read lock failed\n";
      return "";
    }

    return database[key];
  }

  bool write(int txn_id, const string &key, const string &value) {

    if (!isActive(txn_id))
      return false;

    if (!lockManager.acquire_exclusive_lock(txn_id, key)) {
      cout << "Write lock failed\n";
      return false;
    }

    database[key] = value;
    return true;
  }

  void commit(int txn_id) {

    if (!isActive(txn_id))
      return;

    transactions[txn_id]->commit(); // changes Txnstate

    lockManager.release_all_locks(txn_id); // locked at the time of write()
    cout << "Committed Transaction : " << txn_id << endl;
  }

  void abort(int txn_id) {

    if (!isActive(txn_id))
      return;

    transactions[txn_id]->abort(); // changes Txnstate

    lockManager.release_all_locks(txn_id); // locked at the time of write()
    cout << "Aborted Transaction : " << txn_id << endl;
  }
};

int main() {

  TransactionManager tm;

  int t1 = tm.beginTransaction();
  tm.write(t1, "A", "100");
  tm.write(t1, "B", "200");

  cout << "Read A = " << tm.read(t1, "A") << endl;

  tm.commit(t1);

  cout << endl;
  int t2 = tm.beginTransaction();

  tm.write(t2, "A", "500");

  cout << "Read A = " << tm.read(t2, "A") << endl;

  tm.abort(t2);

  return 0;
}
/*
Output:
Begin transaction : 1
Read A = 100
Committed Transaction : 1

Begin transaction : 2
Read A = 500
Aborted Transaction : 2

Follow-up Q&A:
1. Two-Phase Locking:
Already has exclusive locking.
Extend LockManager to support shared and exclusive locks.
Hold all locks until commit() or abort().

How do you prevent lost updates?
Growing Phase : Acquire locks
↓
Shrinking Phase : Release locks

Mention:
Shared Lock
Exclusive Lock

2. Lock Table?
unordered_map<Key, Lock>
Exactly like unordered_map<Key, AggregateState>

3. Deadlock?
Wait-for Graph/Timeout

4.MVCC?
Replace unordered_map<string, string> database;
with unordered_map<string, vector<Version>> database;
each version stores a timestamp/transaction ID and value.

Instead of locking
Value
↓
Version1
Version2
Version3
Readers don't block writers. Writers don't block readers.

5. Isolation Levels?
The design can be extended to support four SQL isolation levels:
Read Uncommitted
Read Committed
Repeatable Read
Serializable
by changing how reads interact with locks or versions.

And which anomalies each prevents:
Dirty Read
Non-repeatable Read
Phantom Read

6. Rollback?
Currently, abort() only changes the transaction state and releases locks.
A complete implementation would maintain a write set (or undo log)
per transaction so that uncommitted changes can be reverted.

Maintain a small write set:
Transaction writes
↓
A = 100, B = 200
↓
Commit / Discard

7. WAL: How would you make commits durable?
Before marking a transaction as committed, write its changes to a Write-Ahead
Log (WAL) and flush the log to stable storage. Write Log ↓ fsync() ↓ Commit

writes are applied directly to the database.

For example:

tm.write(t2, "A", "500");
tm.abort(t2);

The above implementation is not Transactionally correct.
After abort(), the value of "A" is still "500" because we never undo the write.
To support proper rollback/ abort, I would maintain a per-transaction write set
(or undo log). Writes would either be buffered until commit() or recorded with
old values so abort() can restore them.

*/
