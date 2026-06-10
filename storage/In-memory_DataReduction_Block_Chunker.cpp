/*
 Design an In-Memory Data Reduction Block Chunker similar to systems used in
Cohesity, Rubrik, or Data Domain.

Requirements:
Accept a file (byte stream/string) as input.
Split the file into blocks (fixed-size for simplicity).
Generate a fingerprint (hash) for each block.
Perform deduplication:
If an identical block already exists, do not store it again.
Increment the reference count instead.
Store only unique chunks in physical storage.
Maintain metadata mapping fingerprints to physical chunk locations.
Support reading a file by reconstructing it from its chunks.
Support deleting a file:
Decrement reference counts.
Garbage collect chunks whose reference count becomes zero.

Assumptions:
Use in-memory data structures.
Fixed-size chunking (e.g., 8 KB) for simplicity.
Fingerprints are generated using std::hash (SHA256 in production).
Ignore persistence and compression for now.

Optimize storage by eliminating duplicate blocks while allowing efficient write,
read, and delete operations.

 writeFile(filename, data)
Split data into chunks.
Generate fingerprint for each chunk.
If fingerprint exists:
increment refCount.
Else:
write chunk to ChunkStore
add fingerprint to DedupIndex.
Save fingerprint list in fileMap.

readFile(filename)
Get fingerprints from fileMap.
Lookup address from DedupIndex.
Read chunk from ChunkStore.
Concatenate chunks and return file.

deleteFile(filename)
Iterate over fingerprints.
decrementRef().
If refCount becomes zero:
delete chunk from ChunkStore.
remove fingerprint from DedupIndex.
Remove filename from fileMap.

Time Complexity:
writeFile : O(number_of_chunks)
readFile : O(number_of_chunks)
deleteFile: O(number_of_chunks)
Hash lookup: O(1) average.
 */
#include <cstdint>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

class ChunkStore {

  uint64_t next_address;
  unordered_map<uint64_t, string> storage; //<chunk_address, chunk_data>

public:
  ChunkStore() : next_address(0) {}

  uint64_t writeChunk(const string &data) {

    uint64_t addr = next_address++;

    storage[addr] = data;

    return addr;
  }

  string readChunk(uint64_t addr) { return storage[addr]; }

  void deleteChunk(uint64_t addr) { storage.erase(addr); }

  void print() {

    cout << "\nChunk Store\n";
    for (auto &[addr, data] : storage)
      cout << addr << " -> " << data << endl;
  }
};

struct ChunkMetadata {

  uint64_t address;
  int ref_count;
};

class DedupIndex {

  unordered_map<string, ChunkMetadata>
      index; //<chunk_fingerprint_checksum, ChunkMetadata>

public:
  bool exists(const string &fp) { return index.count(fp); }

  void add(const string &fp, uint64_t addr) { index[fp] = {addr, 1}; }

  void incrementRef(const string &fp) { index[fp].ref_count++; }

  void decrementRef(const string &fp) { index[fp].ref_count--; }

  ChunkMetadata get(const string &fp) { return index[fp]; }

  void remove(const string &fp) { index.erase(fp); }

  void print() {
    cout << "\nDedup Index : \n";
    for (auto &[fp, meta] : index)
      cout << "finger_print : " << fp << " address : " << meta.address
           << " ref_count : " << meta.ref_count << endl;
  }
};

class Chunker {

  int chunk_size;

public:
  Chunker(int size = 8) : chunk_size(size) {}

  vector<string> split(const string &data) {
    vector<string> chunks;

    for (int i = 0; i < data.size(); i += chunk_size) {
      chunks.push_back(data.substr(i, min(chunk_size, (int)data.size() - i)));
    }
    return chunks;
  }
};

class DataReductionEngine {

  Chunker chunker;
  ChunkStore chunkStore;
  DedupIndex dedupIndex;

  //<file, chunk finger_prints> mapping
  unordered_map<string, vector<string>> file_fp_map;

  string fingerPrint(const string &chunk_data) {
    return to_string(hash<string>{}(chunk_data));
  }

public:
  void writeFile(const string &file_name, const string &data) {

    vector<string> chunks = chunker.split(data);

    vector<string> finger_prints;
    for (const auto &chunk : chunks) {

      string fp = fingerPrint(chunk);

      if (dedupIndex.exists(fp)) {
        dedupIndex.incrementRef(fp);
      } else {

        uint64_t addr = chunkStore.writeChunk(chunk);
        dedupIndex.add(fp, addr);
      }

      finger_prints.push_back(fp);
    }
    file_fp_map[file_name] =
        finger_prints; //<file_name, finger_prints of all constituent chunks>
  }

  string readFile(const string &file_name) {

    string result;

    for (auto &fp : file_fp_map[file_name]) { // fech chunk fingerPrints for
                                              // given file_name

      ChunkMetadata meta =
          dedupIndex.get(fp); // fetch metadata from dedupIndexStore
      result += chunkStore.readChunk(
          meta.address); // use metadata address to read chunk from chunkStore
    }

    return result;
  }

  void deleteFile(const string &file_name) {

    if (!file_fp_map.count(file_name))
      return;

    for (auto &fp : file_fp_map[file_name]) {

      dedupIndex.decrementRef(fp);

      ChunkMetadata meta = dedupIndex.get(fp);
      if (meta.ref_count == 0) {
        chunkStore.deleteChunk(meta.address);
        dedupIndex.remove(fp);
      }
    }
    file_fp_map.erase(file_name); // erase <file, chunk finger_prints mapping>
  }

  void print() {

    chunkStore.print();
    dedupIndex.print();
  }
};

int main() {

  DataReductionEngine engine;

  engine.writeFile("file1", "ABCDEFGH12345678");
  engine.writeFile("file2", "ABCDEFGH99999999");

  cout << "\nFile1 = " << engine.readFile("file1") << endl;
  cout << "\nFile2 = " << engine.readFile("file2") << endl;

  engine.print();
  cout << "\nDeleting file1...\n";
  engine.deleteFile("file1");

  engine.print();
  cout << "\nDeleting file2...\n";
  engine.deleteFile("file2");

  engine.print();
  return 0;
}
/*In-memory Data Reduction Block Chunker:--------

 Architecture:

                     DataReductionEngine
                           |
          ---------------------------------------
          |                |                    |
       Chunker         DedupIndex           ChunkStore
                            |
                       ChunkMetadata

file_to_fingerprints_Map: fileName -> [fingerprints]

Q1. How to ensure Thread safety?

shared_mutex mutex;

readFile()    -> shared_lock
writeFile()   -> unique_lock
deleteFile()  -> unique_lock
Variable-sized chunking

Q2. Replace: Chunker::split() with Rabin rolling hash (CDC) for Variable-sized
chunking. Production systems use content-defined chunking (Rabin rolling hash).

Q3. Compression during Insert Operation:
Chunker
 ↓
Hash
 ↓
Dedup
 ↓
Compression
 ↓
ChunkStore

Q4.How to Scale to billions of chunks?
Bloom filter -> Sharded fingerprint index (DedupIndex) : (fp % N) ->  SSD
metadata cache

Q5. Parallelism:
Producer
 ↓
Chunk Queue
 ↓
Chunker Threads
 ↓
Hash Threads
 ↓
Dedup Threads
 ↓
Disk Writer

Q6. Persistence?
Metadata can be periodically checkpointed.

Q7. Write Path:

File
↓
Chunker (fixed-size chunks)
↓
Fingerprint (std::hash)
↓
DedupIndex
(fp -> {physical address, refCount}) = (fp -> ChunkMetadata)
↓
ChunkStore
(address -> chunk data)

Q8. Read Path:

fileName
↓
fileMap[fileName]
↓
fingerprints
↓
DedupIndex
↓
ChunkStore
↓
Reconstruct file

Q9. Delete Path:

fileName
↓
fingerprints
↓
DedupIndex.refCount--
↓
refCount == 0 ?
|
Yes
|
ChunkStore.deleteChunk()
DedupIndex.remove()


Output:-------------------------
File1 = ABCDEFGH12345678

File2 = ABCDEFGH99999999

Chunk Store
2 -> 99999999
1 -> 12345678
0 -> ABCDEFGH

Dedup Index :
finger_print : 8673020457155834801 address : 2 ref_count : 1
finger_print : 9934520233202095969 address : 1 ref_count : 1
finger_print : 6278090252846864564 address : 0 ref_count : 2

Deleting file1...

Chunk Store
2 -> 99999999
0 -> ABCDEFGH

Dedup Index :
finger_print : 8673020457155834801 address : 2 ref_count : 1
finger_print : 6278090252846864564 address : 0 ref_count : 1

Deleting file2...

Chunk Store

Dedup Index :
*/
