/* Chunk Store << Content Deduplication Engine:
Design a chunk storage system that stores data blocks identified by chunk IDs.
Chunks should support replication and lookup. Duplicate chunks should not be
stored twice.

          ChunkStorageSystem
                 |
            DedupIndex
                 |
           ChunkMetadata
---------------------------------
data
refCount
replicaAddresses

VVIMP:
refCount → How many logical references (files/objects) point to the same chunk.
replicaAddresses → Where all the physical copies of the chunk are stored.
 */
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

struct ChunkMetadata {

  string data;
  int ref_count = 1;
  vector<uint64_t> replica_addresses;
};

class DedupIndex {

  unordered_map<string, ChunkMetadata> index;
  //<chunk_fingerprint_checksum, ChunkMetadata>

public:
  bool exists(const string &fp) { return index.count(fp); }

  void add(const string &fp, const string &data, uint64_t addr) {

    ChunkMetadata meta;
    meta.data = data;
    meta.replica_addresses.push_back(addr);

    index[fp] = meta;
  }

  ChunkMetadata *get(const string &fp) {

    if (!exists(fp))
      return nullptr;

    return &index[fp];
  }

  void remove(const string &fp) { index.erase(fp); }

  void print() {

    cout << "\nDedup Index : \n";

    for (auto &[fp, meta] : index) {

      cout << "Chunk_ID [finger_print] : " << fp << " Data : " << meta.data
           << " Ref_count : " << meta.ref_count << " Replica Addresses : ";

      for (uint64_t addr : meta.replica_addresses)
        cout << addr << " ";

      cout << "\n\n";
    }
  }
};

class ChunkStorageSystem {

  DedupIndex dedupIndex;

  uint64_t next_replica_address = 1;

  string fingerPrint(const string &chunk_data) {
    return to_string(hash<string>{}(chunk_data));
  }

public:
  void storeChunk(const string &data) {

    string chunk_id = fingerPrint(data);
    ChunkMetadata *meta = dedupIndex.get(chunk_id);
    if (meta) {
      cout << "Duplicate Chunk exists!\n";
      // just increment ref_count
      meta->ref_count++;

      return;
    }

    // new chunk found
    dedupIndex.add(chunk_id, data, next_replica_address++);
  }

  string lookupChunk(const string &data) {

    string chunk_id = fingerPrint(data);
    ChunkMetadata *meta = dedupIndex.get(chunk_id);
    if (!meta)
      return "Chunk NOT FOUND";

    return meta->data;
  }

  void replicateChunk(const string &data) {

    string chunk_id = fingerPrint(data);
    ChunkMetadata *meta = dedupIndex.get(chunk_id);
    if (!meta)
      return;

    return meta->replica_addresses.push_back(next_replica_address++);
    // no change in ref_count because replication doesn't mean another
    // logical user. next_replica_address simulates physical replicas.
  }

  void deleteChunk(const string &data) {

    string chunk_id = fingerPrint(data);
    ChunkMetadata *meta = dedupIndex.get(chunk_id);
    if (!meta)
      return;

    meta->ref_count--;
    if (meta->ref_count == 0) {
      dedupIndex.remove(chunk_id);
    }
  }

  void print() { dedupIndex.print(); }
};

int main() {

  ChunkStorageSystem storage;

  cout << "===== Store Chunks =====\n";

  storage.storeChunk("ABCDEFGH");
  storage.storeChunk("12345678");
  storage.storeChunk("ABCDEFGH"); // duplicate

  cout << "\n===== Lookup =====\n";

  cout << storage.lookupChunk("ABCDEFGH") << endl;

  cout << "\n===== Replicate =====\n";

  storage.replicateChunk("ABCDEFGH");
  storage.replicateChunk("ABCDEFGH");

  storage.print();

  cout << "===== Delete One Reference =====\n";

  storage.deleteChunk("ABCDEFGH");

  storage.print();

  cout << "===== Delete Final Reference =====\n";

  storage.deleteChunk("ABCDEFGH");

  storage.print();

  return 0;
}
/*
Output:--------------------

Step 1
storeChunk("ABCDEFGH")
RefCount = 1
Replicas = [1]

Step 2
storeChunk("12345678")
RefCount = 1
Replicas = [2]

Step 3
storeChunk("ABCDEFGH")

Duplicate found. No new storage. Only RefCount++ becomes RefCount = 2
Replicas = [1]

Step 4
replicateChunk("ABCDEFGH")
replicateChunk("ABCDEFGH")

Now Replica Addresses
1 3 4
meaning:
Replica1 -> Address1
Replica2 -> Address3
Replica3 -> Address4
***VVIMP:RefCount remains 2 because replication doesn't mean another logical
user.

Step 5
deleteChunk("ABCDEFGH")
Only RefCount--
2 -> 1
Replicas stay
1 3 4
because one logical owner still exists.

Step 6
deleteChunk("ABCDEFGH")
RefCount 1 -> 0

Now metadata is erased.
Therefore only 12345678 remains.

Currently replicateChunk() only records
Address 3
Address 4
It doesn't actually create another chunk because we removed ChunkStore.
In this simplified implementation, replicaAddresses simulates physical replicas.
In a production implementation, each replica address would correspond to a
separate copy in a ChunkStore or on another storage node.
*/
