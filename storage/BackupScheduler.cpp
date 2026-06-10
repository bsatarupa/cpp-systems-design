/*
Backup Scheduler : Design a backup service that schedules backup jobs based on
user-defined policies. It should support daily and weekly backups. Failed jobs
should be retryable.
Requirements:------------
Users create backup jobs. Each job has a backup policy.
Policies: Daily/Weekly/... Scheduler checks which jobs should run.
Failed jobs can be retried. Easy to add Monthly policy later.

Uses the Strategy Pattern:-----
               +----------------+
               | BackupScheduler|
               +----------------+
                       |
              scheduleAllJobs()
                       |
      -------------------------------
      |                             |
+-------------+             +---------------+
| BackupJob   |-----------> | BackupPolicy  | (Strategy)
+-------------+             +---------------+
      |                           |
      |                       shouldRun()
      |                       /        \
                      DailyPolicy   WeeklyPolicy
    run()
   retry()
*/
#include <iostream>
#include <vector>

using namespace std;

class BackupPolicy {
public:
  virtual bool shouldRun(int day) = 0;
  virtual ~BackupPolicy() = default;
};

class DailyPolicy : public BackupPolicy {
public:
  bool shouldRun(int day) override { return true; }
};

class WeeklyPolicy : public BackupPolicy {
public:
  bool shouldRun(int day) override { return day % 7 == 0; }
};

enum class Status { PENDING, SUCCESS, FAILED };

class BackupJob {

  int job_id;
  string job_name;
  BackupPolicy *policy;
  Status status;

public:
  BackupJob(int id, string name, BackupPolicy *pol)
      : job_id(id), job_name(name), policy(pol), status(Status::PENDING) {}

  Status getStatus() { return status; }

  void run(int day) {

    if (policy->shouldRun(day) == false)
      return;

    cout << "Running backup_job :" << job_name << endl;

    // placeholder to simulate backup success/failure
    if (job_id % 2 == 0) {
      status = Status::SUCCESS;
      cout << "Backup Success\n";
    } else {
      status = Status::FAILED;
      cout << "Backup Failed\n";
    }
  }

  void retry() {

    if (status == Status::FAILED) {

      cout << "Retrying backup_job :" << job_name << endl;
      status = Status::SUCCESS;
      cout << "Retry Success\n";
    }
  }
};

class BackupScheduler {

  vector<BackupJob *> jobs;

public:
  void addJob(BackupJob *job) { jobs.push_back(job); }

  void schedule(int day) {

    cout << "\n===== Day " << day << " =====\n";
    for (auto &job : jobs)
      job->run(day);
  }

  void retryFailedJobs() {

    cout << "\nRetrying failed Jobs\n";
    for (auto &job : jobs)
      job->retry();
  }
};

int main() {

  DailyPolicy daily;
  WeeklyPolicy weekly;

  BackupScheduler scheduler;

  scheduler.addJob(new BackupJob(1, "UserDocs", &daily));
  scheduler.addJob(new BackupJob(2, "Database", &weekly));
  scheduler.addJob(new BackupJob(3, "Photos", &daily));

  scheduler.schedule(1);
  scheduler.retryFailedJobs();

  scheduler.schedule(7);
  scheduler.retryFailedJobs();

  return 0;
}
/*
===== Day 1 =====
Running backup_job :UserDocs
Backup Failed
Running backup_job :Photos
Backup Failed

Retrying failed Jobs
Retrying backup_job :UserDocs
Retry Success
Retrying backup_job :Photos
Retry Success

===== Day 7 =====
Running backup_job :UserDocs
Backup Failed
Running backup_job :Database
Backup Success
Running backup_job :Photos
Backup Failed

Retrying failed Jobs
Retrying backup_job :UserDocs
Retry Success
Retrying backup_job :Photos
Retry Success

After finishing the basic implementation, we can extend it:
Monthly backups: Add a MonthlyPolicy implementing BackupPolicy; no scheduler
changes needed (Open/Closed Principle). Retry limits: Track retry count in
BackupJob and stop retrying after a configurable maximum. Exponential backoff:
Delay each retry progressively instead of retrying immediately. Priority
scheduling: Execute higher-priority backup jobs first. Parallel execution: Use a
thread pool to run independent backup jobs concurrently. Persistent job
metadata: Store job status and history so retries survive process restarts.
Notifications: Send alerts on repeated failures or successful completion.
Cron-style policies: Replace the simple day-based strategy with a more flexible
schedule expression.
*/
