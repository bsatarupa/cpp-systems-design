/* Cloud Tiering / Lifecycle Policy Manager:
Design a policy engine that automatically moves older backup snapshots across
different storage tiers (Hot Flash → Cold Disk → Archive Cloud).
Core Requirements:
Define pluggable policies (e.g., "If snapshot age > 30 days, move to AWS S3
Glacier"). Handle failed transfers with an exponential backoff retry mechanism.
Support multi-cloud targets (AWS, Azure, GCP).

                     Snapshot
                         |
                 PolicyManager
                  /          \
                 /            \
      LifecyclePolicy     CloudProvider
             ^                  ^
             |          -------------------
         AgePolicy      AWS  Azure  GCP
                 \
                  \
               RetryPolicy
*/

#include <chrono>
#include <iostream>
#include <string>
#include <thread>

using namespace std;

class Snapshot {

  string id;
  int age; // in days
  string currentTier;

public:
  Snapshot(string id, int age, string tier)
      : id(id), age(age), currentTier(tier) {}

  string getId() { return id; }
  int getAge() { return age; }
  string getTier() { return currentTier; }

  void setTier(string tier) { currentTier = tier; }
};

class LifecyclePolicy {
public:
  virtual bool shouldMove(Snapshot &snapshot) = 0;
  virtual ~LifecyclePolicy() = default;
};

class AgePolicy : public LifecyclePolicy {
  int thresholdDays;

public:
  AgePolicy(int threshold) : thresholdDays(threshold) {}

  bool shouldMove(Snapshot &snapshot) override {
    return snapshot.getAge() > thresholdDays;
  }
};

class CloudProvider {
public:
  virtual bool move(Snapshot &snapshot) = 0;
  virtual string name() = 0;
  virtual ~CloudProvider() = default;
};

class AWS : public CloudProvider {
public:
  bool move(Snapshot &snapshot) override {
    cout << "Moving Snapshot " << snapshot.getId() << " to AWS Glacier\n";

    snapshot.setTier("AWS Glacier");
    return true;
  }

  string name() override { return "AWS"; }
};

class Azure : public CloudProvider {
public:
  bool move(Snapshot &snapshot) override {
    cout << "Moving Snapshot " << snapshot.getId() << " to Azure Archive\n";

    snapshot.setTier("Azure Archive");
    return true;
  }

  string name() override { return "Azure"; }
};

class GCP : public CloudProvider {
public:
  bool move(Snapshot &snapshot) override {

    cout << "Moving Snapshot " << snapshot.getId() << " to GCP Archive\n";

    snapshot.setTier("GCP Archive");
    return true;
  }

  string name() override { return "GCP"; }
};

class RetryPolicy {
  int maxRetries;

public:
  RetryPolicy(int retries = 3) : maxRetries(retries) {}

  bool execute(CloudProvider *provider, Snapshot &snapshot) {

    int delay = 1;

    for (int attempt = 1; attempt <= maxRetries; attempt++) {

      cout << "Attempt " << attempt << "...\n";

      if (provider->move(snapshot)) {
        cout << "Transfer Successful\n";
        return true;
      }

      cout << "Transfer Failed. Retrying after " << delay << " seconds\n";
      this_thread::sleep_for(chrono::seconds(delay));
      delay *= 2; // Exponential Backoff
    }
    return false;
  }
};

class PolicyManager {

  LifecyclePolicy *policy;
  CloudProvider *provider;
  RetryPolicy retryPolicy;

public:
  PolicyManager(LifecyclePolicy *policy, CloudProvider *provider)
      : policy(policy), provider(provider) {}

  void evaluate(Snapshot &snapshot) {

    cout << "\nEvaluating Snapshot : " << snapshot.getId() << endl;

    if (!policy->shouldMove(snapshot)) {
      cout << "Policy not satisfied\n";
      return;
    }

    cout << "Policy satisfied\n";
    retryPolicy.execute(provider, snapshot);

    cout << "Current Tier : " << snapshot.getTier() << endl;
  }
};

int main() {

  Snapshot snapshot1("Snapshot_101", 45, "Hot SSD");
  Snapshot snapshot2("Snapshot_102", 10, "Hot SSD");

  AgePolicy agePolicy(30);

  AWS aws;
  PolicyManager manager(&agePolicy, &aws);

  manager.evaluate(snapshot1);
  manager.evaluate(snapshot2);

  return 0;
}
/* Output:
Evaluating Snapshot : Snapshot_101
Policy satisfied
Attempt 1...
Moving Snapshot Snapshot_101 to AWS Glacier
Transfer Successful
Current Tier : AWS Glacier

Evaluating Snapshot : Snapshot_102
Policy not satisfied

Q.How would you support multiple policies?
Instead of LifecyclePolicy *policy;
use vector<LifecyclePolicy *> policies;

Then,
for (auto policy : policies)
    if (policy->shouldMove(snapshot))
        ...

Q. How would you support multiple cloud providers?
You already have
CloudProvider
     ^
------------------------
AWS
Azure
GCP
Adding OCI is simply: class OCI : public CloudProvider { ... };

Q. How would you make retries configurable?
Instead of RetryPolicy retryPolicy;
inject it: RetryPolicy *retryPolicy;

Then you could have:
RetryPolicy
     ^
-------------------------
ExponentialBackoff
FixedInterval
NoRetry
*/
