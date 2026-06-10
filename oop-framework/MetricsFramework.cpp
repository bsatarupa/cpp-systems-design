/*
Design a metrics system supporting counters, gauges, and histograms.
Components should register metrics and periodically expose them.
New metric types should be easy to add.

              Metric
                 ^
      ------------------------
      |          |           |
   Counter     Gauge     Histogram

           MetricsSystem

Metric → abstraction
Counter/Gauge/Histogram → concrete metric types
MetricsSystem → registry and exposes metrics

Example of Strategy + Polymorphism
*/

/*
Output:


Q.How would you periodically expose metrics?
Start a background thread.
In production, I would start exposePeriodically() on a background thread
or schedule it with a timer. For interview, I am calling exposeMetrics()
directly from main() to demonstrate the design.

In Every 10 seconds:
Iterate through all registered metrics.
Serialize them.
Expose them over HTTP (e.g., Prometheus /metrics) or push them to a monitoring
backend.
*/
#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <unordered_map>
using namespace std;

class Metric {
  string metric_name;

public:
  Metric(const string &name) : metric_name(name) {}

  string getName() { return metric_name; }

  virtual void update(double value) = 0;
  virtual void print() = 0;
  virtual ~Metric() = default;
};

class Counter : public Metric {
  int count = 0;

public:
  Counter(const string &name) : Metric(name) {}

  void update(double value) override { count += value; }

  void print() override { cout << getName() << " = " << count << "\n"; }
};

class Gauge : public Metric {
  double value = 0;

public:
  Gauge(const string &name) : Metric(name) {}

  void update(double val) override { value = val; }

  void print() override { cout << getName() << " = " << value << "\n"; }
};

class Histogram : public Metric {
  int sample_count = 0;
  double sum = 0;

public:
  Histogram(const string &name) : Metric(name) {}

  void update(double value) override {
    sample_count++;
    sum += value;
  }

  void print() override {
    cout << getName() << " Avg=" << sum / sample_count << "\n";
  }
};

class MetricSystem {

  unordered_map<string, Metric *> metrics;

public:
  void registerMetric(Metric *metric) { metrics[metric->getName()] = metric; }

  void update(const string &metric_name, double value) {
    metrics[metric_name]->update(value);
  }

  void exposeMetrics() {
    cout << "\n----Metrics----\n";
    for (auto &[name, metric] : metrics)
      metric->print();
  }

  // Normally runs in a backgroud thread, here written as a stub / placeholder
  void exposePeriodically() {

    while (true) {
      exposeMetrics();
      this_thread::sleep_for(chrono::seconds(5));
    }
  }
};

int main() {

  MetricSystem system;

  // Database component
  Counter requests("Requests");

  // CPU monitor
  Gauge cpu("CPU");

  // RPC server
  Histogram latency("Latency");

  system.registerMetric(&requests);
  system.registerMetric(&cpu);
  system.registerMetric(&latency);

  system.update("Requests", 1);
  system.update("Requests", 1);

  system.update("CPU", 67.5);

  system.update("Latency", 20);
  system.update("Latency", 30);
  system.update("Latency", 40);

  system.exposeMetrics();

  // system.exposePeriodically();

  return 0;
}
/*
----Metrics----
CPU = 67.5
Latency Avg=30
Requests = 2
*/
