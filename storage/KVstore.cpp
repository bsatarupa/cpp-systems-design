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

    for (auto &[k, v] : logs)
      memTable[k] = v;

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
