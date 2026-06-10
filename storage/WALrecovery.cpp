/*
Design a write-ahead logging system that records operations before applying
them to storage. Logs should support recovery after failures. Recovery should
replay uncommitted changes.

Implemented a simplified REDO Write-Ahead Logging (WAL) system where every
update is appended to the log before being applied to the in-memory MemTable,
ensuring recoverability after crashes by log replay. During startup, the WAL is
replayed to rebuild the MemTable, and the design also supports DELETE operations
and a checkpoint stub for future optimization. The design is extensible to
support transactions (BEGIN/COMMIT), checkpoints, and durable fsync() semantics
in a production storage engine.

Write Ahead Logging:
WAL guarantees crash recovery by persisting updates before applying them to
in-memory state.

Principle:
1. Append update to WAL.
2. Flush WAL (fsync in production).
3. Apply update to MemTable.

On restart:
Replay WAL to rebuild MemTable. This is a simplified REDO WAL.

                 Client
                    |
                 put(key,val)
                    |
         +----------+----------+
         |                     |
      Append WAL          MemTable
         |
      fsync()
         |
      success
---------------------------------------
Checkpoint
    ↓
Flush MemTable to SSTable/Disk
    ↓
Record checkpoint
    ↓
Delete/truncate older WAL
-----------------------------------------
On Crash:

restart
   |
read checkpoint
   |
read WAL
   |
redo committed
   |
undo uncommitted
*/
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

enum class RecordType { PUT, DELETE_OP };

struct LogRecord {

  int lsn; // Log Sequence Number for ordered log records
  RecordType record_type;
  string key;
  string value;
};

class WAL {

  string log_filename;
  int next_lsn = 1;

public:
  WAL(const string &filename) : log_filename(filename), next_lsn(1) {}

  // append log record
  void append(RecordType type, const string &key,
              const string &value = "") { // for DELETE_OP value = ""

    ofstream ofs(log_filename, ios::app);

    ofs << next_lsn++ << " " << (type == RecordType::PUT ? "PUT" : "DELETE")
        << " " << key << " " << value << "\n";

    // In production, use fsync()/fdatasync()
    ofs.flush();

    cout << "WAL Append : " << key << " -> " << value << endl;
  }

  // replay/REDO WAL log
  vector<LogRecord> recover() {

    vector<LogRecord> logs;

    ifstream ifs(log_filename);

    int lsn;
    string key, value = "", op_type;

    while (ifs >> lsn >> op_type >> key) {

      if (op_type == "PUT")
        ifs >> value;

      LogRecord rec;
      rec.lsn = lsn;
      rec.record_type =
          (op_type == "PUT") ? RecordType::PUT : RecordType::DELETE_OP;
      rec.key = key;
      rec.value = value;

      logs.push_back(rec);

      next_lsn = max(next_lsn, lsn + 1);
    }
    return logs;
  }
};

class KVstore {

  unordered_map<string, string> memtable;
  WAL wal;

public:
  KVstore(const string &filename) : wal(filename) { replay(); }

  string get(const string &key) {

    auto it = memtable.find(key);
    if (it == memtable.end())
      return "NOT_FOUND!";

    return it->second;
  }

  void put(const string &key, const string &value) {

    // 1.add to WAL
    wal.append(RecordType::PUT, key, value);

    // 2. add to memory
    memtable[key] = value;
    cout << "Inserted : " << key << " -> " << value << endl;
  }

  void remove(const string &key) { // part of normal user request

    wal.append(RecordType::DELETE_OP, key);

    memtable.erase(key);
    cout << "DELETE : " << key << endl;
  }

  // stub
  void checkPoint() {

    cout << "----checkPoint-----\n";
    cout << "Flush memTable to SSTable\n";
    cout << "Truncate Old WAL\n\n";
  }

private:
  void replay() {

    auto logs = wal.recover(); // 1. get the KV pairs from WAL logfile

    for (auto &rec : logs) {
      // 2. add recovered records to memTable/ delete checkpointed records from
      // memtable
      if (rec.record_type == RecordType::PUT)
        memtable[rec.key] = rec.value;
      else
        memtable.erase(rec.key);
      // replaying the effects of the operations stored in the WAL before crash.
    }

    cout << "WAL Recovery completed! " << logs.size()
         << " log records replayed\n";
  }
};

int main() {

  cout << "===== First Run =====\n";
  // Normal execution
  KVstore db("wal_recovery.log");

  db.put("apple", "red");
  db.put("banana", "yellow");
  db.put("cat", "animal");

  db.remove("banana");

  db.checkPoint();

  cout << "\n----- Simulating Crash -----\n\n";
  cout << "===== Restart =====\n";

  // Recovery after Restart
  KVstore recoveredDb("wal_recovery.log");

  cout << "Recovered apple : " << recoveredDb.get("apple") << endl;
  cout << "Recovered banana : " << recoveredDb.get("banana") << endl;
  cout << "Recovered mango : " << recoveredDb.get("mango") << endl;

  return 0;
}
/*
 ===== First Run =====
WAL Recovery completed! 0 log records replayed
WAL Append : apple -> red
Inserted : apple -> red
WAL Append : banana -> yellow
Inserted : banana -> yellow
WAL Append : cat -> animal
Inserted : cat -> animal
WAL Append : banana ->
DELETE : banana
----checkPoint-----
Flush memTable to SSTable
Truncate Old WAL


----- Simulating Crash -----

===== Restart =====
WAL Recovery completed! 4 log records replayed
Recovered apple : red
Recovered banana : NOT_FOUND!
Recovered mango : NOT_FOUND!

1. Why do we write to WAL before MemTable?

Answer

If the process crashes after updating the MemTable but before persisting the
change, the update is lost.

Writing to the WAL first guarantees we can recover.

Wrong

MemTable
   ↓
Crash
   ↓
WAL

Update lost

Correct

WAL
 ↓
fsync
 ↓
MemTable
2. Why not write directly to disk?

Answer

Random disk writes are slow.

Instead,

Client
   ↓
Sequential WAL append
   ↓
MemTable

Later

MemTable
   ↓
SSTable/Page flush

Sequential writes are much faster than random writes.

3. Why is WAL append-only?

Answer

Appending avoids random seeks.

Benefits

sequential writes
better SSD/HDD throughput
simple crash recovery
easier replication
4. Why replay WAL?

Answer

The MemTable lives in RAM.

After restart

RAM

↓

Empty

Replay WAL

WAL

↓

MemTable rebuilt
5. Why use LSN?

Answer

LSN (Log Sequence Number) uniquely identifies every log record.

Useful for

ordering
checkpoints
replication
page recovery

Example

LSN 100
LSN 101
LSN 102
6. Why is flush() not enough?

Answer

flush()

C++ buffer

↓

Kernel page cache

Power loss can still lose data.

Need

fsync()

↓

Disk
7. What happens if crash occurs while writing the WAL?

Example

PUT apple red
PUT bana

Recovery may read corrupted data.

Production systems add

record length
checksum
CRC

Corrupted tail records are ignored.

8. What if WAL becomes 100GB?

Recovery becomes slow.

Solution

Checkpoint

↓

Flush MemTable

↓

Delete old WAL
9. What is checkpoint?

Checkpoint means

MemTable

↓

Disk

↓

Remember checkpoint LSN

↓

Truncate WAL

Recovery starts after the checkpoint.

10. Difference between WAL and Checkpoint?

WAL

Every write

↓

append

Checkpoint

Occasional

↓

flush everything
11. Why do databases use binary WAL instead of text?

Binary

smaller
faster
fixed-size parsing
less CPU

Text

PUT apple red

Binary

|LSN|TYPE|LEN|KEY|VALUE|CRC|
12. How does DELETE work?

Instead of

memTable.erase(key);

Need

DELETE key

logged first

Recovery

DELETE

↓

erase()
13. How do transactions work?

Current

PUT

↓

PUT

Production

BEGIN

PUT

PUT

COMMIT

Recovery only replays committed transactions.

14. Crash before COMMIT?
BEGIN

PUT A

PUT B

Crash

No COMMIT

↓

Ignore transaction (or UNDO it depending on the recovery algorithm).

15. Crash after COMMIT?
BEGIN

PUT A

PUT B

COMMIT

Crash

Recovery

↓

REDO transaction.

16. Why not flush every write?

Because

fsync()

is expensive.

Databases use

Group Commit

Thread1

Thread2

Thread3

↓

One fsync()

Higher throughput.

17. What is Group Commit?

Instead of

PUT

fsync

PUT

fsync

Use

PUT

PUT

PUT

↓

single fsync

Huge performance gain.

18. How would you make it thread-safe?

Protect

append()

memTable

using

mutex

or

shared_mutex

Readers

shared lock

Writers

exclusive lock
19. Can replay happen in parallel?

Usually yes.

If logs belong to different partitions.

Example

Partition A

↓

Thread1

Partition B

↓

Thread2

Need ordering guarantees.

20. Why does RocksDB still need WAL if it has SSTables?

Because

MemTable

↓

Crash

↓

Lost

WAL recovers MemTable before the next flush.

21. WAL vs Journal?

Generally the same idea.

Journal

Filesystem

WAL

Database

Both log changes before applying them.

22. WAL vs Binlog?

WAL

Crash Recovery

Binlog

Replication

MySQL uses both.

23. Why replay from beginning?

In your implementation,

Start

↓

Read entire WAL

Production

Checkpoint

↓

Replay only after checkpoint
24. How would you support UPDATE?

Treat UPDATE as

PUT key newValue

or

UPDATE key old new

depending on the storage engine.

25. How would you extend this to a distributed database?
Leader

↓

Append WAL

↓

Replicate WAL

↓

Followers ACK

↓

Commit

Consensus algorithms such as Raft use the replicated log as the source of truth.

1. Why write WAL before MemTable?
Interview Answer (30–45 seconds)

The purpose of WAL is to guarantee durability. Before modifying the in-memory
data structure (MemTable), I first append the operation to the WAL and make sure
the log is persisted (using fsync() in a production system). If the system
crashes after updating the MemTable but before flushing it to disk, I can
recover by replaying the WAL during startup. If I updated the MemTable first and
crashed before writing the log, that update would be lost permanently.

Diagram
Correct

Client
   |
Append WAL
   |
fsync()
   |
Update MemTable
   |
Respond to Client

If the order is reversed:

Client
   |
Update MemTable
   |
Crash
   |
Write WAL

❌ Update lost forever
If interviewer asks "Why not write directly to disk?"

Updating the main storage structure (pages or SSTables) involves random writes,
which are expensive. WAL uses sequential appends, which are much faster. The
actual data is flushed to disk later in batches through checkpoints or
background flushes.

2. How would you reduce recovery time?
Interview Answer (45 seconds)

Replaying the entire WAL from the beginning becomes expensive as the log grows.
To reduce recovery time, I'd introduce checkpoints. During a checkpoint, the
MemTable is flushed to persistent storage, and the checkpoint records the latest
LSN. After a successful checkpoint, older WAL entries before that LSN can be
truncated or archived. On restart, recovery starts from the last checkpoint
instead of replaying the entire log.

Diagram
Normal Execution

MemTable
     |
Checkpoint
     |
Flush to Disk
     |
Checkpoint LSN = 1050
     |
Delete WAL < 1050

Recovery becomes:

Restart
   |
Read Checkpoint
   |
Replay WAL after LSN 1050
   |
MemTable Restored
Why is this better?

Without checkpoints:

Recover

↓

Replay 10 million log records

With checkpoints:

Recover

↓

Replay last 5,000 records

Recovery is much faster.

3. How do you support transactions?
Interview Answer (1 minute)

Currently, every operation is treated independently. To support transactions,
I'd introduce transaction boundaries in the WAL using BEGIN and COMMIT records.

Example WAL:

LSN 100 BEGIN   Txn=1
LSN 101 PUT     apple red
LSN 102 PUT     banana yellow
LSN 103 COMMIT  Txn=1

During recovery, I first identify committed transactions and replay only those
operations. If a crash occurs before a COMMIT record is written, that
transaction is considered incomplete and is ignored (or undone, depending on the
recovery algorithm).

Crash before COMMIT
BEGIN
PUT apple
PUT banana
Crash

Recovery:

No COMMIT

↓

Ignore transaction
Crash after COMMIT
BEGIN
PUT apple
PUT banana
COMMIT
Crash

Recovery:

COMMIT exists

↓

Replay transaction

What would you add in a production storage engine?

LSN (Log Sequence Number) to order log records.
Checksums (CRC) to detect partially written or corrupted records.
Binary log format for compact storage and faster parsing.
fsync()/fdatasync() instead of flush() to ensure durability.
Group commit so multiple transactions share a single fsync().
Checkpointing to reduce recovery time.
Concurrent appends protected by synchronization.
REDO + UNDO recovery (or ARIES-style recovery) for transactional consistency.
*/
