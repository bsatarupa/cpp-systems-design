/*
Design and implement a Transaction Manager that supports beginning, reading,
writing, committing, and aborting transactions. Multiple transactions may
execute concurrently. Design the system so that concurrency control mechanisms
such as Two-Phase Locking (2PL) or MVCC can be incorporated later.

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
*/
#include <iostream>
#include <string>
#include <unordered_map>

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

class LockManager {
  unordered_map<string, int> locks; //<lock_key -> txn_id>
public:
  bool lock(int txn_id, const string &key) {

    auto it = locks.find(key);
    if (it == locks.end() || it->second == txn_id) {

      locks[key] = txn_id;

      return true; // lock already NOT assigned to another transaction
    }
    return false;
  }

  void unlock(int txn_id) {

    for (auto it = locks.begin(); it != locks.end();) {
      if (it->second == txn_id)
        it = locks.erase(it);
      else
        ++it;
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

    auto it = database.find(key);
    if (it == database.end())
      return "";

    return it->second;
  }

  bool write(int txn_id, const string &key, const string &value) {

    if (!isActive(txn_id))
      return false;

    bool lock_status = lockManager.lock(txn_id, key);
    // lockmanager is trying to acquire lock on key for given txn_id
    // gets unlocked at commit() / abort() stage
    if (!lock_status) {
      cout << "Lock acquisition failed for key : " << key << endl;
      return false;
    }

    database[key] = value;
    return true;
  }

  void commit(int txn_id) {

    if (!isActive(txn_id))
      return;

    transactions[txn_id]->commit(); // changes Txnstate

    lockManager.unlock(txn_id); // locked at the time of write()
    cout << "Committed Transaction : " << txn_id << endl;
  }

  void abort(int txn_id) {

    if (!isActive(txn_id))
      return;

    transactions[txn_id]->abort(); // changes Txnstate

    lockManager.unlock(txn_id); // locked at the time of write()
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
