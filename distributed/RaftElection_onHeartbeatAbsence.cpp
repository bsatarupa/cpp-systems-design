#include <atomic>
#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

using namespace std;

enum State {

  FOLLOWER,
  CANDIDATE,
  LEADER
};

class RaftNode {

  int nodeId;

  atomic<int> currTerm; // montonically increasing election epoch/version
                        // number, highest term wins, stale leader steps down
  atomic<State> state;

  vector<RaftNode *> peers;

  int votedFor; // Election state

  chrono::steady_clock::time_point lastHeartbeat; // received from leader

  mutex mtx;

  thread workerThread;

  atomic<bool> stop;

public:
  RaftNode(int id)
      : nodeId(id), currTerm(0), state(State::FOLLOWER), votedFor(-1),
        stop(false) {

    lastHeartbeat = chrono::steady_clock::now(); // lastHeartbeat is initialized
                                                 // to current time
  }

  void setPeers(vector<RaftNode *> &cluster) { peers = cluster; }

  void start() { workerThread = thread(&RaftNode::run, this); }

  void run() {

    while (!stop) {

      int timeout = 1500 + rand() % 1500; // randomized election timeout

      // Followers/Candidates wait for heartbeat
      if (state != State::LEADER) {

        auto elapsed = chrono::duration_cast<chrono::milliseconds>(
                           chrono::steady_clock::now() - lastHeartbeat)
                           .count();

        if (elapsed > timeout)
          startElection();

      } else {
        // Leaders send heartbeats periodically
        sendHeartbeat();

        this_thread::sleep_for(chrono::milliseconds(500));
      }

      this_thread::sleep_for(chrono::milliseconds(100));
    }
  }

  void startElection() {

    lock_guard<mutex> lock(mtx);
    state = State::CANDIDATE; // state upgraded from FOLLOWER to CANDIDATE

    currTerm++;

    lastHeartbeat = chrono::steady_clock::now();
    // reset election timer, else failed candidate can immediately
    // re-trigger elections repeatedly on condition: elapsed > timeout

    votedFor = nodeId; // itself
    cout << "Node_" << nodeId << " starting election for term " << currTerm
         << endl;

    // request votes from peers
    int votes = 1; // vote for itself
    for (auto &peer : peers) {

      if (peer == this)
        continue;

      if (peer->requestVote(currTerm, nodeId) ==
          true) // returns true if currTerm wins over peer's term
        votes++;
    }

    // majority Quorum
    if (votes > peers.size() / 2)
      becomeLeader();
  }

  bool requestVote(int term, int candidateId) {

    lock_guard<mutex> lock(mtx);

    // reject stale term
    if (term < currTerm)
      return false;

    // accept newer term
    if (term > currTerm) {

      currTerm = term;
      votedFor = -1;
      state = State::FOLLOWER; // node mark itself as FOLLOWER
    }

    // vote if not already voted
    if (votedFor == -1 || votedFor == candidateId) {

      votedFor = candidateId; // vote for candidate in both cases

      lastHeartbeat = chrono::steady_clock::now();

      cout << "Node_" << nodeId << " voted for Node_" << candidateId << endl;

      return true;
    }

    // in all other cases, return false
    return false;
  }

  void becomeLeader() {

    state = State::LEADER;
    cout << "Node_" << nodeId << " becomes Leader for term " << currTerm
         << endl;
  }

  void sendHeartbeat() {

    cout << "Leader_" << nodeId << " sending heartbeat " << endl;

    for (auto &peer : peers) {

      if (peer == this)
        continue; // receive heartbeat from all other peers

      peer->receiveHeartbeat(currTerm);
    }
  }

  void receiveHeartbeat(int received_term) {

    lock_guard<mutex> lock(mtx);

    if (received_term >= currTerm) {

      currTerm = received_term;
      state = State::FOLLOWER; // leader turned down to FOLLOWER since node with
                               // greater term found

      votedFor = -1; // no longer voting for itself
      lastHeartbeat = chrono::steady_clock::now();
    }
  }

  void shutDown() {

    stop = true;

    if (workerThread.joinable())
      workerThread.join();
  }
};

int main() {

  // create cluster
  vector<RaftNode *> cluster;
  for (int id = 1; id <= 5; id++)
    cluster.push_back(new RaftNode(id));

  // set entire cluster as peers of each node
  for (auto &node : cluster)
    node->setPeers(cluster);

  // start nodes
  for (auto &node : cluster)
    node->start();

  // run simulation
  this_thread::sleep_for(chrono::seconds(10));

  // cleanup
  for (auto &node : cluster)
    node->shutDown();

  for (auto &node : cluster)
    delete node;

  return 0;
}
/*
Node_3 starting election for term 1
Node_1 voted for Node_3
Node_2 voted for Node_3
Node_4 voted for Node_3
Node_5 voted for Node_3
Node_3 becomes Leader for term 1
Leader_3 sending heartbeat
Leader_3 sending heartbeat
Leader_3 sending heartbeat
Leader_3 sending heartbeat
Leader_3 sending heartbeat
Leader_3 sending heartbeat
Leader_3 sending heartbeat
Leader_3 sending heartbeat
Leader_3 sending heartbeat
Leader_3 sending heartbeat
Leader_3 sending heartbeat
Leader_3 sending heartbeat
Leader_3 sending heartbeat
Leader_3 sending heartbeat
*/

/*
RAFT LEADER ELECTION + HEARTBEAT FLOW
-------------------------------------

1. Node Initialization
----------------------

RaftNode:

    nodeId
    currentTerm = 0
    state = FOLLOWER
    votedFor = -1

    peers = all other nodes

    lastHeartbeat = current_time


2. Start Background Worker
--------------------------

start():

    create background thread

    run():
        forever:

            if state != LEADER:

                check election timeout

                if timeout expired:
                    startElection()

            else:

                sendHeartbeat()

            sleep small interval


3. Election Timeout Logic
-------------------------

Follower/Candidate:

    elapsed =
        now - lastHeartbeat

    if elapsed > randomized_timeout:

        start election


4. Start Election
-----------------

startElection():

    become CANDIDATE

    currentTerm++

    reset election timer

    vote for self

    votes = 1


5. Request Votes From Peers
---------------------------

for each peer:

    if peer != self:

        if peer grants vote:

            votes++


6. Majority Quorum
------------------

Leader elected only if:

    votes > cluster_size / 2

If majority reached:

    becomeLeader()


7. RequestVote RPC
------------------

requestVote(term, candidateId):

    if candidate term is stale:
        reject vote

    if candidate term is newer:

        update currentTerm

        become FOLLOWER

        clear previous vote

    if not voted yet:

        vote for candidate

        reset heartbeat timer

        return true

    otherwise:

        return false


8. Become Leader
----------------

becomeLeader():

    state = LEADER

    start sending heartbeats


9. Send Heartbeats
------------------

Leader periodically:

    for each peer:

        send heartbeat


10. Receive Heartbeat
---------------------

receiveHeartbeat(term):

    if leader term >= currentTerm:

        update currentTerm

        become FOLLOWER

        reset election timeout

        clear votedFor


11. Leader Failure Handling
---------------------------

If leader crashes:

    followers stop receiving heartbeat

    timeout expires

    new election starts


12. Split Vote Prevention
-------------------------

Raft uses:

    randomized election timeout

so all nodes don't start election simultaneously.


13. State Transition Flow
-------------------------

FOLLOWER
    |
    | heartbeat timeout
    v
CANDIDATE
    |
    | majority votes
    v
LEADER
    |
    | sends heartbeat
    v
FOLLOWERS stay quiet


14. Real Raft Adds (Not Implemented Here)
-----------------------------------------

- AppendEntries RPC
- log replication
- commit index
- persistence
- crash recovery
- log consistency checks
- leader failover recovery
*/
