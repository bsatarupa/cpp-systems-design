#include <iostream>
#include <unordered_map>
#include <vector>

using namespace std;

class Block {
public:
  int id;
  string data;
  int ref_count;

  Block(int id, string data) : id(id), data(data), ref_count(1) {}
};

class Snapshot {
public:
  int version;
  vector<int> block_ids;

  Snapshot(int version, vector<int> blocks)
      : version(version), block_ids(blocks) {}
};

class SnapshotManager {

  int next_block_id = 1;
  int next_version = 1;

  unordered_map<int, Block *> blockStore; // block_id -> block address mapping
  unordered_map<int, Snapshot *>
      snapshots; // version -> Snapshot address mapping

  vector<int> current_blocks; //***VVIMP: current state of file

public:
  void write(string data) { // append block

    Block *block = new Block(next_block_id++, data);
    blockStore[block->id] = block;
    current_blocks.push_back(block->id);
  }

  void readCurrentFile() {

    cout << "Current file : ";
    for (int block_id : current_blocks)
      cout << blockStore[block_id]->data << " ";
    cout << endl;
  }

  int createSnapshot() { // create point-in-time snapshot, returns snapshot
                         // version

    int version = next_version++;
    snapshots[version] = new Snapshot(version, current_blocks);

    // snapshot starts referencing current blocks
    for (int block_id : current_blocks)
      blockStore[block_id]->ref_count++;

    return version;
  }

  void restoreSnapshot(int version) { // restore file to an older version,
                                      // restore block_ids for given snapshots

    if (!snapshots.count(version)) {
      cout << "Snapshot NOT FOUND\n";
      return;
    }

    current_blocks = snapshots[version]->block_ids;
  }

  void deleteSnapshot(int version) { // delete snapshot and free blocks if
                                     // needed

    if (!snapshots.count(version))
      return;

    Snapshot *snapshot = snapshots[version];

    for (int block_id : snapshot->block_ids) {

      blockStore[block_id]->ref_count--;
      if (blockStore[block_id]->ref_count == 0) {

        delete blockStore[block_id]; // deleting refernce of block
        blockStore.erase(block_id);
      }
    }

    delete snapshot; // deleting reference of snapshot
    snapshots.erase(version);
  }

  void readSnapshot(int version) {

    if (!snapshots.count(version)) {
      cout << "Snapshot NOT FOUND\n";
      return;
    }

    cout << "Snapshot " << version << " : ";

    for (int block_id : snapshots[version]->block_ids)
      cout << blockStore[block_id]->data << " ";
    cout << endl;
  }

  void listSnapshots() {

    cout << "Listing Snapshots : ";
    for (auto &[version, snapshot] : snapshots)
      cout << version << " ";
    cout << endl;
  }

  ~SnapshotManager() {

    for (auto &[id, block] : blockStore)
      delete block;

    for (auto &[version, snapshot] : snapshots)
      delete snapshot;
  }
};

int main() {

  SnapshotManager sm;

  sm.write("A");
  sm.write("B");

  cout << "Initial file :\n";
  sm.readCurrentFile();

  int s1 = sm.createSnapshot();

  sm.write("C");
  sm.write("D");

  cout << "\nAfter new writes :\n";
  sm.readCurrentFile();

  int s2 = sm.createSnapshot();

  cout << "\nReading Snapshots :\n";
  sm.readSnapshot(s1);
  sm.readSnapshot(s2);

  cout << "\nRestoring Snapshot : " << s1 << endl;
  sm.restoreSnapshot(s1);

  sm.readCurrentFile();

  cout << "\nDeleting Snapshot : " << s2 << endl;
  sm.deleteSnapshot(s2);

  sm.listSnapshots();
  return 0;
}

/* Snapshot Manager :
 * Implemented a Copy-on-Write (CoW) Snapshot Manager using immutable blocks and
reference counting.
 * Snapshots share blocks instead of copying data, enabling space-efficient
point-in-time versions.
 * Supports write, create snapshot, restore snapshot, delete snapshot, and
snapshot listing operations.
 * Garbage collection is handled via block reference counts, reclaiming blocks
when no snapshot references them.

Architecture
                +----------------+
                | SnapshotManager |
                +----------------+
                   |          |
                   |          |
                   v          v
            +-----------+   +-----------+
            | BlockStore|   | Snapshots |
            +-----------+   +-----------+
                 |                |
                 v                v
             Block(id,data,refCount)
                  ↑
         Shared by multiple snapshots

Initial file :
Current file : A B

After new writes :
Current file : A B C D

Reading Snapshots :
Snapshot 1 : A B
Snapshot 2 : A B C D

Restoring Snapshot : 1
Current file : A B

Deleting Snapshot : 2
Listing Snapshots : 1

Q1. Copy-on-Write :Snapshots don't duplicate blocks.
Current File
A -> B -> C

Snapshot1
A -> B

Snapshot2
A -> B -> C

Q2. Blocks are shared. No physical copy is made.
Reference Counting :
Block A refCount = 3
Block B refCount = 3
Block C refCount = 2

Q3. Delete a snapshot:
refCount--
if refCount == 0:
    reclaim block space

complexity:
write = O(1)
every other function: O(1)
*/
