/*
Build a simplified Search Engine Index(in-memory inverted index)
that supports concurrent indexing and keyword search.
Requirements:
Add (index) documents.
Build an inverted index:
word → list of document IDs.
Search for a word and return matching documents.
Support multiple indexing threads.
Support concurrent search while indexing.
              Documents
                   │
          Producer Queue
                   │
        +----------+----------+
        |                     |
   Index Worker 1       Index Worker 2
        |                     |
        +----------+----------+
                   │
            Inverted Index
          word → set<docId>
                   │
             Search Threads
Complexity:
Index document : O(number of words)
Search : O(1) average + output size
Space : O(total indexed words)
*/
#include <cctype>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace std;

class SearchEngine {
private:
  mutex mtx, cout_mtx;
  condition_variable cv;

  bool done = false;

  // mapping term -> document ids
  unordered_map<string, unordered_set<int>> inverted_index;

  // documents waiting to be indexed; <document_id, text_string> pairs
  queue<pair<int, string>> document_queue;

  void worker(int id) {

    while (true) {

      pair<int, string> doc_text;

      {
        unique_lock<mutex> lock(mtx);
        cv.wait(lock, [&]() { return done || !document_queue.empty(); });

        // graceful shutdown
        if (done && document_queue.empty())
          return;

        doc_text = document_queue.front();
        document_queue.pop();
      }

      {
        lock_guard<mutex> lock(cout_mtx);
        cout << "Worker_" << id << " Indexing document : " << doc_text.first
             << endl;
      }

      vector<string> words = tokenize(doc_text.second);
      {
        // acquire mutex before inserting term to global inverted_index
        lock_guard<mutex> lock(mtx);
        for (auto &term : words)
          inverted_index[term].insert(doc_text.first);
      }

      {
        lock_guard<mutex> lock(cout_mtx);
        cout << "Worker_" << id << " Finished document : " << doc_text.first
             << endl;
      }
    }
  }

  vector<string> tokenize(const string &doc_text) {

    vector<string> words;

    string word;
    for (char c : doc_text) {
      if (isalnum(c))
        word += c;
      else if (!word.empty()) { // end of word reached
        words.push_back(word);
        word.clear(); // reset for next word
      }
    }

    // check for the last word
    if (!word.empty())
      words.push_back(word);

    return words;
  }

public:
  void build_index(int numThreads) {

    vector<thread> threads;

    for (int i = 1; i <= numThreads; i++)
      threads.emplace_back(&SearchEngine::worker, this, i);

    for (auto &t : threads)
      t.join();
  }

  void stop() {

    {
      lock_guard<mutex> lock(mtx);
      done = true;
    }
    cv.notify_all();
  }

  vector<int> serach(const string &term) {

    lock_guard<mutex> lock(mtx);

    // return list of document_ids containing term
    auto it = inverted_index.find(term);
    if (it == inverted_index.end())
      return {};

    vector<int> result(it->second.begin(), it->second.end());
    sort(result.begin(), result.end());
    return result;
  }

  // add a document into document_queue for indexing
  void add_document(int document_id, const string &doc_text) {
    {
      lock_guard<mutex> lock(mtx);
      document_queue.push({document_id, doc_text});
    }
    cv.notify_one(); // notify worker thread for indexing
  }
};

int main() {

  SearchEngine engine;

  thread builder([&]() { engine.build_index(3); });

  engine.add_document(1, "apple banana orange");
  engine.add_document(2, "banana mango");
  engine.add_document(3, "apple mango banana");

  this_thread::sleep_for(chrono::seconds(1));

  engine.stop();
  builder.join();

  auto result = engine.serach("apple");
  cout << "\nDocuments containing 'apple': ";
  for (int id : result)
    cout << id << " ";
  cout << endl;

  result = engine.serach("banana");
  cout << "\nDocuments containing 'banana': ";
  for (int id : result)
    cout << id << " ";
  cout << endl;

  return 0;
}
/*
output:
Worker_1 Indexing document : 1
Worker_2 Indexing document : 2
Worker_1 Finished document : 1
Worker_3 Indexing document : 3
Worker_2 Finished document : 2
Worker_3 Finished document : 3

Documents containing 'apple': 1 3

Documents containing 'banana': 1 2 3
*/
