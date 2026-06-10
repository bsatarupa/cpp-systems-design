/*
 Design a simplified LSM (Log-Structured Merge) Storage Engine Write Path that
 efficiently handles key-value writes while ensuring durability and fast
lookups.

The storage engine should:
Support put(key, value) and get(key) operations.
Append every write to a Write-Ahead Log (WAL) before updating memory.
Store recent writes in an in-memory MemTable.
Flush the MemTable to an immutable SSTable when it reaches a size threshold.
Search for keys in the MemTable first, then in SSTables (newest to oldest).
Recover data after a crash by replaying the WAL.

Architecture:--------------
Write Path:
                  put(key, value)
                         │
                         ▼
               Write-Ahead Log (WAL)
                  (Durability)
                         │
                         ▼
               In-Memory MemTable
                  (Sorted Map)
                         │
          MemTable reaches threshold?
                 No │              │ Yes
                    │              ▼
                    │       Flush to SSTable
                    │              │
                    ▼              ▼
             Continue Writes   Immutable SSTable
                                      │
                                      ▼
                               Disk Storage

Read Path:
             get(key)
                 │
                 ▼
            Search MemTable
                 │
         Found?  │
          Yes    ▼ No
             Return Value
                 │
                 ▼
     Search SSTables (Newest → Oldest)
                 │
         Found?  │
          Yes    ▼ No
             Return Value
                 │
                 ▼
             NOT FOUND

Complexity:
M = Number of entries in the MemTable
N = Number of entries in an SSTable
S = Number of SSTables
Time Complexity ----
WAL Append : O(1)
MemTable Insert (std::map) : O(log M)
Flush MemTable → SSTable : O(M)
Put (Amortized) : O(log M)
Read from MemTable : O(log M)
Read from SSTables : O(S × log N)
Space Complexity ----
MemTable : O(M)
SSTables : O(Total Entries)
WAL : O(Total Writes)
Overall : O(M + Total Entries)

Production Improvement:
1. Immutable MemTable & Background Flush:
Instead of blocking writes while flushing,
MemTable Full
      │
      ▼
Immutable MemTable
      │
Background Flush Thread
      │
      ▼
New SSTable
A new MemTable immediately accepts writes while the old one is flushed
asynchronously.

2. WAL Rotation / Checkpointing
After a successful MemTable flush:
Rotate or truncate the WAL.
Prevent replaying the entire log during recovery.
Reduce recovery time and disk usage.

3. Bloom Filters
Attach a Bloom Filter to every SSTable.
Read Key
    │
    ▼
Bloom Filter
    │
 ┌──┴──┐
 │     │
No    Maybe
 │      │
 ▼      ▼
Skip  Binary Search
This avoids unnecessary SSTable lookups for keys that definitely do not exist.

4. Compaction
Merge multiple SSTables into larger, sorted SSTables.
Benefits:
Remove obsolete versions of keys.
Eliminate deleted records (tombstones).
Reduce the number of SSTables.
Improve read performance.

5. Tombstones
Instead of immediately deleting a key:
DELETE(key)
↓
Write Tombstone
↓
Removed During Compaction
This ensures consistency and enables efficient sequential writes.

6. Block Cache
Cache frequently accessed SSTable blocks in memory to reduce disk I/O and
improve read latency.

7. Compression & Checksums:
Compress SSTable blocks to reduce storage.
Use checksums to detect data corruption during reads.

8. Concurrent Background Workers
Run background threads for:
MemTable flushing
Compaction
Cleanup and maintenance
This keeps foreground reads and writes responsive while maintenance tasks
execute asynchronously.
 */

#include <fstream>
#include <iostream>
#include <map>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <vector>

using namespace std;

class WAL {

  string log_file;
  mutex walMutex;

public:
  WAL(const string &filename) : log_file(filename) {}

  void append(const string &key, const string &value) {

    lock_guard<mutex> lock(walMutex);

    ofstream ofs(log_file, ios::app); // open the file in append mode
    ofs << key << " " << value << endl;
    ofs.flush(); // flush the file to OS/page cache
  }

  vector<pair<string, string>> recover() {

    lock_guard<mutex> lock(walMutex);

    vector<pair<string, string>> logs;
    string key, value;

    ifstream ifs(log_file); // recover KV from log_file and restore

    while (ifs >> key >> value)
      logs.push_back({key, value});

    return logs;
  }
};

class SSTable {

  vector<pair<string, string>> sorted_entries; //<key, value>

public:
  // build immutable SSTable from already sorted MemTable
  SSTable(map<string, string> &memTable) {

    for (auto &entry : memTable)
      sorted_entries.push_back(entry);
  }

  // Binary search lookup inside SSTable
  string searchKey(const string &key) {

    int left = 0, right = sorted_entries.size() - 1;
    while (left <= right) {

      int mid = left + (right - left) / 2;
      if (sorted_entries[mid].first == key)
        return sorted_entries[mid].second;

      else if (sorted_entries[mid].first < key)
        left = mid + 1;

      else
        right = mid - 1;
    }
    return "NOT FOUND!";
  }

  void printSSTable() {

    cout << "-------SSTable--------" << endl;

    for (auto &[k, v] : sorted_entries)
      cout << k << " -> " << v << endl;
  }
};

class KVstore {

  map<string, string> memTable;
  vector<SSTable> sstables;

  static const int memTable_Flush_Threshold = 3;

  WAL wal;
  mutable shared_mutex rwMutex;

public:
  KVstore(const string &wal_file) : wal(wal_file) { recover(); }

  void recover() { // load data from WAL to Memtable as part of crash recovery

    auto logs = wal.recover();

    for (auto &[k, v] : logs) {
      memTable[k] = v;

      // VVIMP: In case memTable is FULL during Recovery, flush out instantly
      if (memTable.size() >= memTable_Flush_Threshold)
        flush_memTable_toDisk();
    }

    cout << "WAL Log Recovery completed!" << endl;
  }

  void insertKeyValue(const string &key, const string &value) {

    unique_lock<shared_mutex> lock(rwMutex);

    // 1. WAL first
    wal.append(key, value);

    // 2. MemTable update
    memTable[key] = value;
    cout << "Inserted key : " << key << endl;

    // 3. flush memTable if threshold is reached
    if (memTable.size() >= memTable_Flush_Threshold)
      flush_memTable_toDisk();
  }

  void flush_memTable_toDisk() {

    cout << "----Flushing MemTable to latest SSTable----" << endl;
    sstables.emplace_back(memTable);
    // constructs an SSTable object directly inside the vector, calling
    // SSTable() equivalent to: [SSTable temp(memTable);
    // sstable.push_back(temp);]
    memTable.clear();
  }

  string readKey(const string &key) {

    shared_lock<shared_mutex> lock(rwMutex);
    // search in memtable first
    auto it = memTable.find(key);
    if (it != memTable.end())
      return it->second;

    //****Search SSTables newest to oldest
    for (auto rev_it = sstables.rbegin(); rev_it != sstables.rend(); rev_it++) {

      string val = rev_it->searchKey(key);
      if (val != "NOT FOUND!")
        return val;
    }
    return "NOT FOUND!";
  }

  void print_MemTable() {

    shared_lock<shared_mutex> lock(rwMutex);

    cout << "------MemTable-------" << endl;
    for (auto &entry : memTable)
      cout << entry.first << " -> " << entry.second << endl;
  }

  void print_all_SSTables() {

    shared_lock<shared_mutex> lock(rwMutex);

    for (auto &sstable : sstables)
      sstable.printSSTable(); // print each SSTable
  }
};

int main() {

  {
    KVstore db("wal.log");

    db.insertKeyValue("apple", "red");
    db.insertKeyValue("banana", "yellow");
    db.insertKeyValue("cat", "animal");
    db.insertKeyValue("dog", "pet");
    db.insertKeyValue("elephant", "wild");

    cout << "Read banana : " << db.readKey("banana") << endl;

    db.print_MemTable();
    db.print_all_SSTables();
  }

  cout << "Restarting After Crash Recovery, for WAL Replay" << endl;

  {
    KVstore db("wal.log");

    cout << "Recovered dog : " << db.readKey("dog") << endl;
    cout << "Recovered mango : " << db.readKey("mango") << endl;
  }

  return 0;
}
/*
WAL Log Recovery completed!
Inserted key : apple
----Flushing MemTable to latest SSTable----
Inserted key : banana
Inserted key : cat
Inserted key : dog
----Flushing MemTable to latest SSTable----
Inserted key : elephant
Read banana : yellow
------MemTable-------
elephant -> wild
-------SSTable--------
apple -> red
banana -> yellow
cat -> animal
dog -> pet
elephant -> wild
-------SSTable--------
banana -> yellow
cat -> animal
dog -> pet
Restarting After Crash Recovery, for WAL Replay
WAL Log Recovery completed!
Recovered dog : pet
Recovered mango : NOT FOUND!
*/
