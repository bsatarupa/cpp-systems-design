/*
Design and implement a Two-Phase Commit (2PC) Coordinator that coordinates
multiple participants. Ignore networking and assume participants are local
objects.

                  Coordinator
             ----------------------
             participants
             state
             ----------------------
             prepare()
             commit()
             abort()
                    |
      ----------------------------
      |            |            |
Participant1  Participant2  Participant3
*/
#include <iostream>
#include <vector>
using namespace std;

enum class Txnstate { INIT, PREPARED, COMMITTED, ABORTED };

class Participant {

  int id;

public:
  Participant(int id) : id(id) {}

  bool prepare() {
    cout << "Participant_" << id << " prepared\n";
    return true; // vote YES
  }

  bool commit() {
    cout << "Participant_" << id << " committed\n";
    return true;
  }

  bool abort() {
    cout << "Participant_" << id << " aborted\n";
    return true;
  }
};

class Coordinator {

  vector<Participant *> participants;

  Txnstate state = Txnstate::INIT;

public:
  void addParticipant(Participant *p) { participants.push_back(p); }

  bool prepare() {
    cout << "\n----Prepare Phase----\n";

    for (auto &p : participants) {
      if (p->prepare() == false) { // if atleast 1 participant voted NO

        abort();
        return false;
      }
    }

    state = Txnstate::PREPARED;
    return true;
  }

  void commit() {

    if (state != Txnstate::PREPARED) {

      cout << "Not reached Prepared state! Cannot commit!\n";
      return;
    }

    cout << "\n----Commit Phase----\n";

    for (auto &p : participants)
      p->commit();

    state = Txnstate::COMMITTED;
  }

  void abort() {

    cout << "\n----Abort Phase----\n";

    for (auto &p : participants)
      p->abort();

    state = Txnstate::ABORTED;
  }
};

int main() {

  Participant p1(1);
  Participant p2(2);
  Participant p3(3);

  Coordinator coordinator;

  coordinator.addParticipant(&p1);
  coordinator.addParticipant(&p2);
  coordinator.addParticipant(&p3);

  if (coordinator.prepare())
    coordinator.commit();

  return 0;
}
/* Output:
----Prepare Phase----
Participant_1 prepared
Participant_2 prepared
Participant_3 prepared

----Commit Phase----
Participant_1 committed
Participant_2 committed
Participant_3 committed

Complexity:-------
Prepare / Commit / Abort : O(n), n = #participants

Follow-up Q&A:
1. Why is it called Two-Phase Commit?
Phase 1:
Coordinator
↓
Prepare
↓
Participants vote
------------------
Phase 2:
Commit
or
Abort

2. What if one participant votes NO?
Abort everyone.

3. What if the coordinator crashes after PREPARE?
This is the classic weakness of 2PC.
Participants are in the PREPARED state and must wait until the coordinator
recovers. This is why 2PC is a blocking protocol.

4. How do databases recover?
Persist the coordinator state using a Write-Ahead Log (WAL):
INIT
↓
PREPARED
↓
COMMITTED
On restart, the coordinator reads the log and resumes.

5. How is 3PC different?
3PC adds another phase to reduce blocking, but it is rarely used in production.

6. Why do systems like Spanner or CockroachDB often avoid classic 2PC?
Because they combine consensus protocols (like Paxos or Raft) with transaction
coordination to provide fault tolerance without the same blocking behavior as
classic 2PC.
*/
