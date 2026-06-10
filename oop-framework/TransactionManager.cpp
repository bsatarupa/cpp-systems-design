/* Transaction Manager:
Design a Transaction Manager that supports beginning, committing, and rolling
back transactions. A transaction transitions through different states during
its lifetime. New transaction states should be easy to add without modifying
existing code.

Requirements:
- Begin a transaction.
- Commit a transaction.
- Rollback a transaction.
- Maintain transaction state.
- Easy to add new transaction states.

            TransactionManager
      --------------------------------
      transactions
      unordered_map<int, Transaction*>
      --------------------------------
                begin()
                commit()
                rollback()
                |
                |
                v
          ----------------
          | Transaction  |
          ----------------
          txn_id
          state
          ----------------
          commit()
          rollback()
          printState()
                |
                v
            enum State
      ---------------------
                ACTIVE
                COMMITTED
                ABORTED
*/

#include <iostream>
#include <unordered_map>
using namespace std;

enum class State { ACTIVE, COMMITTED, ABORTED };

class Transaction {
  int txn_id;
  State state;

public:
  Transaction(int id) : txn_id(id), state(State::ACTIVE) {}

  void commit() {

    if (state != State::ACTIVE) {
      cout << "Commit Not Allowed\n";
      return;
    }

    cout << "Transaction_" << txn_id << " Committed\n";
    state = State::COMMITTED;
  }

  void rollback() {

    if (state != State::ACTIVE) {
      cout << "Rollback Not Allowed\n";
      return;
    }

    cout << "Transaction_" << txn_id << " Rolled Back\n";
    state = State::ABORTED;
  }

  void printState() {

    cout << "Transaction_" << txn_id << " : ";
    switch (state) {
    case State::ACTIVE:
      cout << "ACTIVE";
      break;
    case State::COMMITTED:
      cout << "COMMITTED";
      break;
    case State::ABORTED:
      cout << "ABORTED";
      break;
    }
    cout << endl;
  }
};

class TransactionManager {

  unordered_map<int, Transaction *> transactions;
  //<txn_id, reference to Transaction>
public:
  Transaction *begin(int txn_id) {

    if (transactions.count(txn_id)) {
      cout << "Transaction Already Exists\n";
      return transactions[txn_id];
    }

    cout << "Beginning Transaction : " << txn_id << endl;
    Transaction *txn = new Transaction(txn_id);
    transactions[txn_id] = txn;

    return txn;
  }

  void commit(int txn_id) {

    if (transactions.count(txn_id) == 0) {
      cout << "Transaction Not Found\n";
      return;
    }

    transactions[txn_id]->commit();

    delete transactions[txn_id];
    transactions.erase(txn_id);
  }

  void rollback(int txn_id) {

    if (transactions.count(txn_id) == 0) {
      cout << "Transaction Not Found\n";
      return;
    }

    transactions[txn_id]->rollback();

    delete transactions[txn_id];
    transactions.erase(txn_id);
  }

  void listTransactions() {

    cout << "\nActive Transactions : ";
    for (auto &[txn_id, txn] : transactions)
      cout << txn_id << " ";
    cout << endl;
  }

  ~TransactionManager() {
    for (auto &[txn_id, txn] : transactions)
      delete txn;
  }
};

int main() {

  TransactionManager tm;
  cout << "--------- Transaction 1 ---------\n";
  Transaction *txn1 = tm.begin(1);
  txn1->printState();
  tm.listTransactions();
  tm.commit(1);
  tm.listTransactions();
  cout << "--------- Transaction 2 ---------\n";
  Transaction *txn2 = tm.begin(2);
  txn2->printState();
  tm.listTransactions();
  tm.rollback(2);
  tm.listTransactions();

  return 0;
}
/*
 TransactionManager truly owns the transactions.
Easy to extend with methods like findTransaction(), abortAll(), or
removeTransaction().
 ---------------------------------------
Output:
--------- Transaction 1 ---------
Beginning Transaction : 1
Transaction_1 : ACTIVE

Active Transactions : 1
Transaction_1 Committed

Active Transactions :
--------- Transaction 2 ---------
Beginning Transaction : 2
Transaction_2 : ACTIVE

Active Transactions : 2
Transaction_2 Rolled Back

Active Transactions :
-----------------------------------------
Q. How would you support Two-Phase Commit (2PC)?
Add another state.
ACTIVE
   |
PREPARED
   |
COMMITTED

enum class State {
    ACTIVE,
    PREPARED,
    COMMITTED,
    ABORTED
};
Add prepare() before commit().

Q. How would you support MVCC or Two-Phase Locking?
Keep transaction lifecycle independent from concurrency control.
            TransactionManager
                    |
         -----------------------
         |                     |
      State         ConcurrencyControl
                             ^
                     -------------------
                     |                 |
                   MVCC              TwoPL
*/
