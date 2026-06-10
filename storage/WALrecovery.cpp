/*
WAL guarantees crash recovery by persisting updates before applying them to
in-memory state.
*/
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

class WAL {

  string log_filename;

public:
  WAL(const string &filename) : log_filename(filename) {}

  // append log record
  void append(const string &key, const string &value) {

    ofstream ofs(log_filename, ios::app);
    ofs << key << " " << value << endl;
    ofs.flush();

    cout << "WAL Append : " << key << " -> " << value << endl;
  }

  // replay WAL log
  vector<pair<string, string>> recover() {

    vector<pair<string, string>> logs;

    ifstream ifs(log_filename);
    string key, value;

    while (ifs >> key >> value) {
      logs.push_back({key, value});
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
    wal.append(key, value);

    // 2. add to memTable
    memtable[key] = value;

    cout << "Inserted : " << key << " -> " << value << endl;
  }

private:
  void replay() {

    auto logs = wal.recover(); // 1. get the KV pairs from WAL logfile
    for (auto &[k, v] : logs)  // 2. add recovered KV pairs to memTable
      memtable[k] = v;

    cout << "WAL Recovery completed!" << endl;
  }
};

int main() {

  // Normal execution
  KVstore db("wal_recovery.log");

  db.put("apple", "red");
  db.put("banana", "yellow");
  db.put("cat", "animal");

  cout << "------Simulating Crash-------" << endl;

  // Recovery after Restart
  KVstore recoveredDb("wal_recovery.log");

  cout << "Recovered apple : " << recoveredDb.get("apple") << endl;
  cout << "Recovered banana : " << recoveredDb.get("banana") << endl;
  cout << "Recovered mango : " << recoveredDb.get("mango") << endl;

  return 0;
}
/*
WAL Recovery completed!     //recovery during startup. Since it was an empty
WAL, replay does nothing. WAL Append : apple -> red Inserted : apple -> red WAL
Append : banana -> yellow Inserted : banana -> yellow WAL Append : cat -> animal
Inserted : cat -> animal
------Simulating Crash-------
WAL Recovery completed!
Recovered apple : red
Recovered banana : yellow
Recovered mango : NOT_FOUND!
*/
