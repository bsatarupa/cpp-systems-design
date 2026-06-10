#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_set>

using namespace std;

class WebCrawler {

  struct UrlTask {
    string url;
    int depth;
  };

  int max_depth;

  queue<UrlTask> url_queue;
  unordered_set<string> visited;

  vector<thread> workers;

  mutex mtx;

  // separate mutex for printing
  mutex printMutex;
  condition_variable cv;

  bool stop;

public:
  WebCrawler(int num_threads, int depth) : stop(false), max_depth(depth) {

    for (int thread_id = 1; thread_id <= num_threads; thread_id++)
      workers.emplace_back(&WebCrawler::worker, this, thread_id);
  }

  ~WebCrawler() { shutdown(); }

  void addUrls(const string &url, int depth) {

    unique_lock<mutex> lock(mtx);

    if (visited.count(url)) // avoid duplicate crawling
      return;

    url_queue.push({url, depth});
    visited.insert(url);

    cv.notify_one();
  }

  void worker(int worker_id) {

    while (true) {

      UrlTask task;
      { // critical section

        unique_lock<mutex> lock(mtx);

        cv.wait(lock, [&]() { return stop == true || !url_queue.empty(); });

        // graceful shutdown
        if (stop && url_queue.empty())
          return;

        task = url_queue.front();
        url_queue.pop();
      }
      //***Recursive crawling should be out of lock and critical section
      crawl(task, worker_id);
    }
  }

  void crawl(const UrlTask &task, int worker_id) {

    {
      lock_guard<mutex> lock(printMutex);
      cout << "[Worker Thread_" << worker_id << "] crawling URL : " << task.url
           << " | Depth : " << task.depth << endl;
    }

    // simulate network delay
    this_thread::sleep_for(chrono::milliseconds(1000));

    // stop recursion at max depth
    if (task.depth >= max_depth)
      return;

    // simulate extracting child links
    vector<string> discovered_urls = {task.url + "/about",
                                      task.url + "/contact"};

    for (auto &next_url : discovered_urls)
      addUrls(next_url, task.depth + 1);
  }

  void shutdown() {

    { // critical section
      unique_lock<mutex> lock(mtx);
      stop = true;
    }

    cv.notify_all();

    for (auto &t : workers) {
      if (t.joinable())
        t.join();
    }
  }
};

int main() {

  WebCrawler crawler(4, 2);

  crawler.addUrls("https://example.com", 0);

  this_thread::sleep_for(chrono::seconds(8));

  crawler.shutdown();
  return 0;
}
/*
[Worker Thread_1] crawling URL : https://example.com | Depth : 0
[Worker Thread_1] crawling URL : https://example.com/about | Depth : 1
[Worker Thread_2] crawling URL : https://example.com/contact | Depth : 1
[Worker Thread_1] crawling URL : https://example.com/about/contact | Depth : 2
[Worker Thread_2] crawling URL : https://example.com/about/about | Depth : 2
[Worker Thread_3] crawling URL : https://example.com/contact/about | Depth : 2
[Worker Thread_4] crawling URL : https://example.com/contact/contact | Depth : 2
*/
