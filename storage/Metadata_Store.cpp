/*
 Metadata Store: Design a metadata service that stores information about files
 and objects. Metadata should be searchable and cached for faster access.
 Frequently accessed metadata should be served efficiently.
 */
#include <cstddef>
#include <iostream>
#include <list>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

struct Metadata {

  long inode_number;
  size_t file_size;
};

class LRUcache {

  int Tcapacity;
  list<pair<string, Metadata>> cache; //<file_pathname, file_metadata>
  unordered_map<string, list<pair<string, Metadata>>::iterator> mm;

  mutable mutex cachelock;

public:
  LRUcache(int capacity) : Tcapacity(capacity) {}

  Metadata get(const string &key) {

    lock_guard<mutex> lock(cachelock);

    auto found = mm.find(key);
    if (found == mm.end())
      return {-1, 0};

    Metadata meta = found->second->second;
    cache.splice(cache.begin(), cache, found->second);
    return meta;
  }

  void put(const string &key, Metadata meta) {

    lock_guard<mutex> lock(cachelock);

    auto found = mm.find(key);

    if (found != mm.end()) {
      cache.splice(cache.begin(), cache, found->second);
      found->second->second = meta;
    } else {
      if (mm.size() == Tcapacity) {
        string victim_key = cache.back().first;
        cache.pop_back();
        mm.erase(victim_key);
      }
      cache.push_front({key, meta});
      mm[key] = cache.begin();
    }
  }

  void erase(const string &key) {

    lock_guard<mutex> lock(cachelock);

    auto found = mm.find(key);
    if (found == mm.end())
      return;

    cache.erase(found->second);
    mm.erase(found);
  }
};

class MetadataService {

  LRUcache cache; // hot metadata cache
  unordered_map<string, Metadata>
      metadata_store; //<file_pathname, file_metadata>

  mutable shared_mutex rwlock;

public:
  MetadataService(int cache_size) : cache(cache_size) {}

  void remove(const string &file_pathname) {

    unique_lock<shared_mutex> writeLock(rwlock);
    metadata_store.erase(file_pathname);
    cache.erase(file_pathname);
  }

  // 1. write-through cache
  void put(const string &file_path, long inode_number, size_t file_size) {

    unique_lock<shared_mutex> writeLock(rwlock);

    Metadata meta = {inode_number, file_size};
    metadata_store[file_path] = meta;
    cache.put(file_path, meta);
  }

  Metadata get(const string &file_pathname) {

    // unique_lock<shared_mutex> writeLock(rwlock);
    //***VVIMP: cache lookup (LRUcache locks itself)
    Metadata meta = cache.get(file_pathname);

    // in case of cache hit
    if (meta.inode_number != -1) {
      cout << "Cache Hit!\n";
      return meta;
    }

    // in case of cache miss
    cout << "Cache Miss!\n";

    // Only metadata_store needs protection
    shared_lock<shared_mutex> readlock(rwlock);

    auto it = metadata_store.find(file_pathname);
    if (it == metadata_store.end()) {
      return {-1, 0};
      // neither found in cache, nor in metadata_store
    }
    meta = it->second;

    //***If found in metadata_store, Time to bring back this data to cache
    //***VVIMP: cache would use its own unique_lock
    cache.put(file_pathname, meta);

    return meta;
  }

  vector<string> searchByPrefix(const string &prefix) {

    shared_lock<shared_mutex> lock(rwlock);

    vector<string> results;
    for (const auto &[file_path, meta] : metadata_store) {
      if (file_path.find(prefix) == 0)
        results.push_back(file_path);
    }
    return results;
  }
};

int main() {

  MetadataService service(2);

  service.put("/a/file1", 1, 1024);
  service.put("/b/file2", 2, 2048);
  auto meta = service.get("/a/file1");
  cout << meta.inode_number << " " << meta.file_size << endl;

  service.put("/a/file3", 3, 4096);
  meta = service.get("/b/file2");
  cout << meta.inode_number << " " << meta.file_size << endl;

  cout << "\nSearching /a\n";
  for (const auto &path : service.searchByPrefix("/a"))
    cout << path << endl;

  service.remove("/a/file1");
  meta = service.get("/a/file1");
  cout << meta.inode_number << " " << meta.file_size << endl;

  return 0;
}
/*
Implemented a thread-safe Metadata Service with a write-through LRU cache.
Metadata is persisted in a backing store and frequently accessed entries are
cached for O(1) lookup. Reads first consult the cache and populate it on misses,
while writes update both cache and backing store to maintain consistency. The
design supports concurrent access using reader-writer locks and resembles
metadata management in distributed storage systems. TC: O(1)
Since it is a *metadata* store, Write-through writes + Cache-aside reads

Output:-------------------
Cache Hit!
1 1024
Cache Miss!
2 2048

Searching /a
/a/file3
/a/file1
Cache Miss!
-1 0

Possible extensions:
Search support: add APIs like searchByPrefix(), searchByOwner(), or
searchByFileType(). A Trie can optimize prefix searches, while secondary indexes
can support attribute-based queries. TTL/expiration: expire stale cache entries
after a configurable duration. Distributed cache: replace the in-process LRU
with a shared cache (e.g., Redis) if multiple metadata service instances are
deployed. Persistence: use a real backing database (RocksDB, LevelDB, etc.)
instead of an unordered_map. Metrics: expose cache hit rate, miss rate, and
eviction count for observability.
*/
