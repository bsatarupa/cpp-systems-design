#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

enum class CloudProvider { AWS, AZURE, GCP };

struct UploadRequest {

  CloudProvider provider;
  string sourcePath;
  string destinationBlob;
  bool async;
};

class BlobStore { // pure abstract class
public:
  virtual void upload(const string &blob_name, const string &file_path) = 0;
  virtual string download(const string &blob_name) = 0;
  virtual void delete_blob(const string &blob_name) = 0;
  virtual vector<string> list(const string &prefix) = 0;

  virtual ~BlobStore() = default;
};

// Similarly implement AzureBlobSore and GCPBlobStore, similar interfaces
class S3BlobStore : public BlobStore {

  unordered_map<string, string> storage; //<blob_name, file_path>
public:
  void upload(const string &blob_name, const string &file_path) override {
    storage[blob_name] = file_path;
    cout << "Uploaded to S3\n";
  }

  string download(const string &blob_name) override {
    if (!storage.count(blob_name))
      return "";

    return storage[blob_name];
  }

  void delete_blob(const string &blob_name) override {
    storage.erase(blob_name);
  }

  vector<string> list(const string &prefix) override {
    vector<string> result;

    for (auto &[blob_name, file_path] : storage) {
      if (blob_name.find(prefix) == 0) // match found at 0th position
        result.push_back(blob_name);
    }
    return result;
  }
};
/*
class BlobStoreFactory {
public:
  static BlobStore *create_blob_store(CloudProvider provider) {

    switch (provider) {

    case CloudProvider::AWS:
      return new S3BlobStore();

    case CloudProvider::AZURE:
        return new AzureBlobSore();

    case CloudProvider::GCP:
        return new GCPBlobStore();

    default:
      return nullptr;
    }
  }
};
*/
class BlobService {

  unordered_map<CloudProvider, BlobStore *>
      stores; //<provider, reference to BlobStore>

public:
  // Once BlobService owns the provider instances, the BlobStoreFactory becomes
  // unnecessary If provider creation becomes more complex, I'd move it into a
  // Factory. For simplicity, BlobService currently owns singleton provider
  // instances.
  BlobService() {

    stores[CloudProvider::AWS] = new S3BlobStore();
    // stores[CloudProvider::AZURE] = new AzureBlobSore();
    // stores[CloudProvider::GCP] = new GCPBlobStore();
  }

  ~BlobService() {

    for (auto &[provider, store] : stores)
      delete store;
  }

  void upload(const UploadRequest &req) {

    // BlobStore *store = BlobStoreFactory::create_blob_store(req.provider);
    BlobStore *store = stores[req.provider];

    if (req.async) {

      cout << "Schedule async upload\n";
      // TaskScheduler.submit(uploadTask);
    } else {
      // for sync request, upload instantly, as part of client's write path
      store->upload(req.destinationBlob, req.sourcePath);
    }
  }

  string download(CloudProvider provider, const string &blob_name) {

    // BlobStore *store = BlobStoreFactory::create_blob_store(provider);
    BlobStore *store = stores[provider];
    return store->download(blob_name);
  }

  void delete_blob(CloudProvider provider, const string &blob_name) {

    // BlobStore *store = BlobStoreFactory::create_blob_store(provider);
    BlobStore *store = stores[provider];
    store->delete_blob(blob_name);
  }

  /*
      My current list() scans the entire unordered_map and is O(N). To optimize
     prefix listing, I'd maintain a Trie alongside the hash map. Upload inserts
     into both structures, delete removes from both, download still uses the
     hash map for O(1) lookup, and list(prefix) traverses the Trie in
     O(prefix_length + result_size). For very large result sets, I'd add
     pagination or a streaming iterator. In production systems with billions of
     objects, metadata is usually stored in B+Trees or LSM trees rather than an
     in-memory Trie. This naturally supports efficient prefix range scans.
   */
  vector<string> list(CloudProvider provider, const string &prefix) {
    BlobStore *store = stores[provider];
    return store->list(prefix);
  }
};

int main() {

  BlobService service;

  UploadRequest req;
  req.provider = CloudProvider::AWS;
  req.sourcePath = "/tmp/file1.jpg";
  req.destinationBlob = "img1";
  req.async = false;
  service.upload(req);

  service.upload({CloudProvider::AWS, "/tmp/file2.jpg", "img2", false});
  service.upload({CloudProvider::AWS, "/tmp/photo.jpg", "photo1", false});

  vector<string> blobs = service.list(CloudProvider::AWS, "img");

  for (auto &blob : blobs)
    cout << blob << endl;

  cout << service.download(CloudProvider::AWS, "img1") << endl;

  service.delete_blob(CloudProvider::AWS, "img1");

  return 0;
}
/*
 Output:
Uploaded to S3
Uploaded to S3
Uploaded to S3
img2
img1
/tmp/file1.jpg

 Discussion:------------------------------
 1. Why use an interface?
Answer

To decouple the service from provider-specific implementations.

BlobService
     |
 BlobStore interface
   /      |      \
 S3     Azure    GCS

Adding MinIO:

class MinIOBlobStore : public BlobStore {};

requires no changes to BlobService.

Pattern: Strategy Pattern.

2. Why Factory Pattern? (Commented)
Answer

Avoids:

if(provider=="AWS")
...
else if(provider=="AZURE")
...

Factory encapsulates object creation.

BlobStore* store =
    BlobStoreFactory::create(provider);
3. How do you handle large files?
Answer

Multipart upload.

10GB file

chunk0
chunk1
chunk2
chunk3

Upload chunks in parallel.

Retry only failed chunks.

Merge on server side.

4. Sync or async uploads?
Answer

Depends on file size.

Small files:

SYNC

Large files:

ASYNC

Flow:

Client
 ↓
BlobService
 ↓
TaskScheduler
 ↓
ThreadPool
 ↓
UploadTask

Return:

jobId

immediately.

5. How will client know upload completed?
Answer

Several options:

Polling
getJobStatus(jobId)
Callback/Webhook
notify(url)
Observer Pattern
onSuccess()
onFailure()
6. What if upload fails midway?
Answer

Retry failed chunks.

Maintain:

struct UploadSession {

    uploadId;

    completedChunks;
};

Resume from failed chunk.

7. How do you support concurrent uploads?
Answer

ThreadPool.

BlockingQueue
 ↓
Worker Threads
 ↓
UploadTask

Avoid creating one thread per upload.

8. How would you authenticate?
Answer

Introduce:

class Credential {

    string token;

    string accessKey;

    string secret;
};

Support:

JWT
OAuth
AWS access keys

Pass Credential to BlobStore.

9. How to support new providers?
Answer

Implement:

class MinIOBlobStore : public BlobStore {};

Factory:

case MINIO:
    return new MinIOBlobStore();

No change to BlobService.

Open-Closed Principle.

10. How do you avoid repeated object creation?

Commented code:

BlobStoreFactory::create()

creates new stores every request.

Current Better Code:

unordered_map<CloudProvider, BlobStore*> stores;

Initialize once.

Singleton objects.

11. How do you handle metadata?

Maintain:

struct BlobMetadata {

    size_t size;

    time_t creationTime;

    checksum;
};

Separate:

Blob Store
Metadata Store
12. How do you ensure integrity?

Store:

checksum

After upload:

compute hash
compare hash

Detect corruption.

13. How do you make uploads idempotent?

Same request twice:

upload(blobId)
upload(blobId)

should not duplicate data.

Maintain:

uploadId

or overwrite existing blob.

14. How would you add versioning?
blobId -> version list

Example:

img1:v1
img1:v2
img1:v3

Latest pointer:

latestVersion["img1"]
15. How to support delete and restore?

Soft delete.

isDeleted=true

Garbage collector removes later.

16. How to notify multiple consumers?

Observer Pattern.

UploadTask
     ↓
NotificationService
     ↓
------------------
Email
Webhook
SMS
17. How do you improve download performance?

Cache hot blobs.

Client
 ↓
Cache
 ↓ miss
BlobStore

Use LRU.

18. Which SOLID principles are used?
OCP:

Add new providers without changing code.

DIP:

BlobService depends on:

BlobStore

not S3BlobStore.

SRP:

BlobService:

orchestration.

BlobStore:

provider implementation.

Factory:

object creation.

Highest-probability Cohesity follow-up
"Suppose upload is async. How will you notify the user?"

Strong answer:

I would immediately return a jobId, schedule an UploadTask through TaskScheduler
and ThreadPool, and use either polling (getJobStatus(jobId)) or a
webhook/callback mechanism to notify completion. Large files would be uploaded
as chunks, and failed chunks would be retried independently.

That answer naturally ties together:

TaskScheduler ✔
ThreadPool ✔
NotificationSystem ✔
Retry ✔
Chunking ✔

Implemented a cloud-agnostic Blob Storage Wrapper using Strategy and Factory
patterns. A common BlobStore interface abstracts provider-specific
implementations, allowing the service to support multiple cloud backends
transparently. Synchronous uploads are handled directly, while asynchronous
uploads can be delegated to a TaskScheduler and ThreadPool. The design is
extensible and allows adding new providers without changing existing code.
Output:

*/
