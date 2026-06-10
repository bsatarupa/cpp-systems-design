/* Snapshot Manager = Restore Engine
 Design a Restore Engine capable of reconstructing files from stored chunks.
 Users should be able to restore a specific snapshot or version.
 Restore progress should be trackable. (----Only this part needs to be added)

               RestoreEngine
                    |
         ------------------------
         |                      |
     BlockStore            SnapshotTable

Implementation details:
RestoreEngine maintains immutable blocks using Copy-on-Write semantics.
Snapshots store references to shared blocks instead of duplicating data.
Restoring a version simply switches the current block list to the snapshot's
block list while updating reference counts. Restore progress is tracked by
counting restored blocks over the total number of blocks in the snapshot.
 */
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

class RestoreEngine {

  int next_block_id = 1;
  int next_version = 1;

  unordered_map<int, Block *> blockStore; // block_id -> block address mapping
  unordered_map<int, Snapshot *>
      snapshots; // version -> Snapshot address mapping

  vector<int> current_blocks; //***VVIMP: current state of file

public:
  void write(string data) { // append block

    Block *block =
        new Block(next_block_id++, data); // blocks are never modified in-place
    blockStore[block->id] = block;
    current_blocks.push_back(block->id);
  }

  void readCurrentFile() {

    cout << "Current file : ";
    for (int block_id : current_blocks)
      cout << blockStore[block_id]->data << " ";
    cout << endl;
  }

  void incrementRefCount(const vector<int> &block_ids) {

    for (auto block_id : block_ids)
      blockStore[block_id]->ref_count++;
  }

  void decrementRefCount(const vector<int> &block_ids) {

    for (auto block_id : block_ids) {

      blockStore[block_id]->ref_count--;

      if (blockStore[block_id]->ref_count == 0) {
        delete blockStore[block_id];
        blockStore.erase(block_id);
      }
    }
  }

  int createSnapshot() { // create point-in-time snapshot, returns snapshot
                         // version

    int version = next_version++;
    snapshots[version] = new Snapshot(version, current_blocks);

    // snapshot starts referencing current blocks
    incrementRefCount(current_blocks);

    return version;
  }

  void restoreSnapshot(int version) { // restore file to an older version,
                                      // restore block_ids for given snapshots

    if (!snapshots.count(version)) {
      cout << "Snapshot NOT FOUND\n";
      return;
    }

    // VVIMP: current file will no longer reference blocks of current version
    decrementRefCount(current_blocks);

    // restore snapshot blocks
    current_blocks = snapshots[version]->block_ids;

    // VVIMP: current file will start referencing restored blocks
    incrementRefCount(current_blocks);

    //------------Progress Tracking----------------
    int totalBlocks = current_blocks.size();
    int restoredBlocks = 0;

    for (int block_id : current_blocks) {
      restoredBlocks++;
      cout << "Progress : " << restoredBlocks * 100 / totalBlocks << "%\n";
    }
    // In this in-memory prototype, restore is essentially a metadata operation,
    // so it completes immediately. In a production restore engine, each block
    // would be fetched from disk or object storage, and I'd update the progress
    // after each block is restored. Here I'm simulating that block-by-block
    // progress while keeping the restore logic unchanged.
  }

  void deleteSnapshot(int version) { // delete snapshot and free blocks if
                                     // needed

    if (!snapshots.count(version))
      return;

    Snapshot *snapshot = snapshots[version];

    decrementRefCount(snapshot->block_ids);

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

  void printBlockStore() {

    cout << "\nBlock Store\n";

    for (auto &[id, block] : blockStore) {

      cout << "Block " << id << " : " << block->data
           << " RefCount=" << block->ref_count << endl;
    }
  }

  ~RestoreEngine() {

    for (auto &[id, block] : blockStore)
      delete block;

    for (auto &[version, snapshot] : snapshots)
      delete snapshot;
  }
};

int main() {

  RestoreEngine sm;

  sm.write("A");
  sm.write("B");
  sm.printBlockStore();

  cout << "Initial file :\n";
  sm.readCurrentFile();

  int s1 = sm.createSnapshot();
  sm.printBlockStore();

  sm.write("C");
  sm.write("D");
  sm.printBlockStore();

  cout << "\nAfter new writes :\n";
  sm.readCurrentFile();

  int s2 = sm.createSnapshot();
  sm.printBlockStore();

  cout << "\nReading Snapshots :\n";
  sm.readSnapshot(s1);
  sm.readSnapshot(s2);

  cout << "\nRestoring Snapshot : " << s1 << endl;
  sm.restoreSnapshot(s1);
  sm.printBlockStore();

  sm.readCurrentFile();

  cout << "\nDeleting Snapshot : " << s2 << endl;
  sm.deleteSnapshot(s2);
  sm.printBlockStore();

  sm.listSnapshots();
  return 0;
}

/* Restore Engine :
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
                | RestoreEngine |
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

Output:
Block Store
Block 2 : B RefCount=1
Block 1 : A RefCount=1
Initial file :
Current file : A B

Block Store
Block 2 : B RefCount=2
Block 1 : A RefCount=2

Block Store
Block 4 : D RefCount=1
Block 3 : C RefCount=1
Block 2 : B RefCount=2
Block 1 : A RefCount=2

After new writes :
Current file : A B C D

Block Store
Block 4 : D RefCount=2
Block 3 : C RefCount=2
Block 2 : B RefCount=3
Block 1 : A RefCount=3

Reading Snapshots :
Snapshot 1 : A B
Snapshot 2 : A B C D

Restoring Snapshot : 1
Progress : 50%
Progress : 100%

Block Store
Block 4 : D RefCount=1
Block 3 : C RefCount=1
Block 2 : B RefCount=3
Block 1 : A RefCount=3
Current file : A B

Deleting Snapshot : 2

Block Store
Block 2 : B RefCount=2
Block 1 : A RefCount=2
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
listSnapshots = )(#snapshots)
every other function: O(#blocks in current file/snapshot)

1. Why not copy the whole file?

Copy

100 GB

takes

100 GB

extra.

Your solution

copies only

block ids
2. Why immutable blocks?

Because

Snapshot 1

and

Current

can safely share

Block A

without corruption.

3. Complexity
Operation	Complexity
write	O(1)
snapshot	O(number of block ids)
restore	O(number of block ids)
read	O(number of block ids)
delete snapshot	O(number of block ids)
4. Why reference counting?

Garbage collection.

When

ref_count==0

safe to free.

5. What if block changes?

Don't modify.

Create

New Block

Current

points to it.

Snapshots still point to old.

6. What if multiple snapshots exist?

Example

Snapshot1

↓

A B

Snapshot2

↓

A B C

Current

↓

A B C D

Blocks

A

shared.

7. How to support incremental backup?

Store

Changed Blocks

between snapshots.

Exactly how backup systems work.

8. Thread safety?

Protect

blockStore

snapshots

current_blocks

using

shared_mutex
9. Faster restore?

Current

vector assignment

already

O(number of blocks)

Very efficient.

10. How would NetApp WAFL do this?

Metadata only.

Snapshot copies inode/block pointers.

Blocks are shared.

New writes allocate new blocks.

Exactly your design.

11. How to support deduplication?

Instead of

Block ID

key by

Hash(Block Data)

Reuse identical blocks.

12. Compression?

Compress

Block::data

before storing.

13. Pattern?

Not exactly GoF.

Main ideas:

Copy-on-Write
Immutable objects
Reference Counting
Flyweight (shared blocks)

*/
