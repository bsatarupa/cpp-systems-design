/*
Design a multithreaded Cloud Backup Pipeline that processes backup requests
concurrently.

Each backup job should go through the following stages:
Read the source data
Chunk the data
Compress it
Encrypt it
Upload it to cloud storage

The system should support:
Multiple worker threads processing backup jobs in parallel.
A thread-safe job queue.
Retrying failed backup jobs up to a configurable limit.
Graceful shutdown after all submitted jobs are processed.

                    Client
                      │
                      ▼
               Backup Scheduler
                      │
                      ▼
          Blocking Queue<BackupJob>
                      │
          ┌───────────┴───────────┐
          ▼                       ▼
      Worker Thread          Worker Thread
          │                       │
          ▼                       ▼
Read → Chunk → Compress → Encrypt → Upload
          │
          ▼
 SUCCESS / FAILED
          │
          ├──────────────► Retry Queue (up to Max Retries)
          │
          ▼
     Backup Complete

Time Complexity:
N = number of backup jobs.
Each job passes through all pipeline stages exactly once (ignoring retries).

Overall Time Complexity: O(N)
If each stage processes data proportional to file size, the total work
is linear in the amount of data processed.

Space Complexity:
Job queue stores pending backup jobs.
Worker threads process jobs independently.

Overall Space Complexity: O(N + T)
where:
N = number of queued jobs.
T = number of worker threads.
*/

#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <ostream>
#include <queue>
#include <string>
#include <thread>

using namespace std;

enum class Status { PENDING, RUNNING, SUCCESS, FAILED };

struct BackupJob {
  int job_id;
  string file_name;
  int retry_count = 0;
  Status status = Status::PENDING;
};

class CloudBackupPipeline {
private:
  mutex queue_mutex, cout_mutex;
  condition_variable cv;

  queue<BackupJob> job_queue;

  bool stopped = false;
  vector<thread> worker_threads;

public:
  CloudBackupPipeline(int numThreads) {

    for (int i = 0; i < numThreads; i++)
      worker_threads.emplace_back(&CloudBackupPipeline::worker, this);
  }

  ~CloudBackupPipeline() {

    {
      lock_guard<mutex> lock(queue_mutex);
      stopped = true;
    }

    cv.notify_all();

    for (auto &t : worker_threads)
      t.join();
  }

  void submit_job(BackupJob job) {

    {
      lock_guard<mutex> lock(queue_mutex);
      job_queue.push(job);
    }
    cv.notify_one();
  }

private:
  // dedicated mutex on console output
  void log(const string &msg) {
    lock_guard<mutex> lock(cout_mutex);
    cout << msg << endl;
  }

  void worker() {

    while (true) {

      BackupJob job;
      {
        unique_lock<mutex> lock(queue_mutex);
        cv.wait(lock, [&]() { return stopped || !job_queue.empty(); });

        // graceful shutdown
        if (job_queue.empty() && stopped)
          return;

        job = job_queue.front();
        job_queue.pop();
      }

      // process BackupJob
      process_backup(job);

      // retry/re-submit failed job for max 3 times
      if (job.status == Status::FAILED && job.retry_count < 3) {

        job.retry_count++;

        log("Retrying " + job.file_name + " (Attempt_" +
            to_string(job.retry_count) + ")");

        submit_job(job); //***VVIMP
      }
    }
  }

  void process_backup(BackupJob &job) {

    job.status = Status::RUNNING;
    log("\nStarting Backup : " + job.file_name);

    if (!read(job) || !chunk(job) || !compress(job) || !encrypt(job) ||
        !upload(job)) {
      job.status = Status::FAILED;

      log("Backup Failed");
      return;
    }

    job.status = Status::SUCCESS;
    log("Backup Completed");
  }

  bool read(const BackupJob &job) {

    log("Reading " + job.file_name);
    return true;
  }

  bool chunk(const BackupJob &job) {

    log("Chunking " + job.file_name);
    return true;
  }

  bool compress(const BackupJob &job) {

    log("Compressing " + job.file_name);
    return true;
  }

  bool encrypt(const BackupJob &job) {

    log("Encrypting " + job.file_name);
    return true;
  }

  bool upload(const BackupJob &job) {

    log("Uploading " + job.file_name);

    // Simulate occasional failure
    if (job.job_id % 2 == 1 && job.retry_count == 0)
      return false;

    return true;
  }
};

int main() {

  CloudBackupPipeline pipeline(3);

  pipeline.submit_job({1, "database.db"});
  pipeline.submit_job({2, "photos.zip"});
  pipeline.submit_job({3, "documents.tar"});
  pipeline.submit_job({4, "videos.mp4"});

  // Give worker_threads time to finish
  this_thread::sleep_for(chrono::seconds(2));

  return 0;
}
/*
Output:
Starting Backup : database.db
Reading database.db
Chunking database.db
Compressing database.db
Encrypting database.db
Uploading database.db

Starting Backup : photos.zip
Reading photos.zip
Chunking photos.zip
Compressing photos.zip
Encrypting photos.zip
Uploading photos.zip
Backup Completed

Starting Backup : documents.tar
Reading documents.tar
Chunking documents.tar
Compressing documents.tar

Starting Backup : videos.mp4
Backup Failed
Reading videos.mp4
Chunking videos.mp4
Compressing videos.mp4
Encrypting videos.mp4
Uploading videos.mp4
Backup Completed
Retrying database.db (Attempt_1)
Encrypting documents.tar

Starting Backup : database.db
Reading database.db
Chunking database.db
Compressing database.db
Encrypting database.db
Uploading database.db
Backup Completed
Uploading documents.tar
Backup Failed
Retrying documents.tar (Attempt_1)

Starting Backup : documents.tar
Reading documents.tar
Chunking documents.tar
Compressing documents.tar
Encrypting documents.tar
Uploading documents.tar
Backup Completed
*/
