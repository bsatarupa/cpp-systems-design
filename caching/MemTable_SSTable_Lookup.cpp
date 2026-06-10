#include <iostream>
#include <map>
#include <string>
#include <vector>

using namespace std;

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

    for (auto &entry : sorted_entries)
      cout << entry.first << " -> " << entry.second << endl;
  }
};

class LSMstorageEngine {

  map<string, string> memTable;
  vector<SSTable> sstables;

  int memTable_Flush_Threshold;

public:
  LSMstorageEngine(int flush_threshold)
      : memTable_Flush_Threshold(flush_threshold) {}

  void insertKeyValue(const string &key, const string &value) {

    memTable[key] = value;
    cout << "Inserted key : " << key << endl;

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

    cout << "------MemTable-------" << endl;
    for (auto &entry : memTable)
      cout << entry.first << " -> " << entry.second << endl;
  }

  void print_all_SSTables() {

    for (auto &sstable : sstables)
      sstable.printSSTable(); // print each SSTable
  }
};

int main() {

  LSMstorageEngine mem_storage(3);

  mem_storage.insertKeyValue("apple", "red");
  mem_storage.insertKeyValue("banana", "yellow");
  mem_storage.insertKeyValue("cat", "animal");
  mem_storage.insertKeyValue("dog", "pet");
  mem_storage.insertKeyValue("elephant", "wild");

  mem_storage.print_MemTable();
  mem_storage.print_all_SSTables();

  cout << "Read banana : " << mem_storage.readKey("banana") << endl;
  cout << "Read dog : " << mem_storage.readKey("dog") << endl;
  cout << "Read mango : " << mem_storage.readKey("mango") << endl;

  return 0;
}
/*
Inserted key : apple
Inserted key : banana
Inserted key : cat
----Flushing MemTable to latest SSTable----
Inserted key : dog
Inserted key : elephant
------MemTable-------
dog -> pet
elephant -> wild
-------SSTable--------
apple -> red
banana -> yellow
cat -> animal
Read babana : yellow
Read dog : pet
Read mango : NOT FOUND!
*/
