/*
 Design a buffer pool that caches disk pages in memory. When memory is full,
 pages should be evicted according to a replacement policy. Disk accesses should
be minimized.

Buffer pools(Page cache) reduce expensive disk IO by caching frequently accessed
pages in memory and evicting least recently used pages when memory is full.
Storage systems read/write data in fixed-size blocks called Pages.Typical
size:4/8/16KB. A page may contain: DB rows/ index nodes/ filesystem blocks.

Implemented an LRU-based buffer pool. Pages are stored in a doubly linked list
and indexed by a hash map for O(1) access. Dirty pages are flushed before
eviction.
***Each page maintains a pin count so pages currently in use are not chosen as
victims. This minimizes disk accesses by keeping frequently used pages in
memory. Pages are pinned when fetched and explicitly unpinned when the
caller is done using them, and eviction skips pinned pages.

Application
     |
BufferPool (RAM)
--------------------------------
HashMap<PageID -> Iterator>
Doubly Linked List (MRU -> LRU)
Dirty bit
Pin count
--------------------------------
     |
  Disk (SSD)
*/
#include <chrono>
#include <iostream>
#include <list>
#include <string>
#include <thread>
#include <unordered_map>

using namespace std;

class BufferPool {

public:
  struct Page {

    int page_id;
    string data;

    bool dirty = false; // whether page is modified since last disk flush
    int pin_count = 0;  // whether page is currently used/referred
  };

private:
  int Tcapacity;
  list<Page> cache; // dll with MRU page at the front
  unordered_map<int, list<Page>::iterator> mm;

  void flush_page(Page &page) {

    cout << "Flushing dirty page : " << page.page_id << endl;

    //***mark the page clean
    page.dirty = false;
  }

  void evict_page() {

    // search from LRU towards MRU
    auto victim = prev(cache.end());

    while (victim != cache.begin() && victim->pin_count > 0)
      victim++;

    if (victim->pin_count > 0) {
      cout << "No evictable page found!\n";
      return;
    }

    // evictable page found, pin_count == 0
    if (victim->dirty)
      flush_page(*victim);

    cout << "Evicting page : " << victim->page_id << endl;
    mm.erase(victim->page_id);
    cache.erase(victim);
  }

public:
  BufferPool(int capacity) : Tcapacity(capacity) {}

  // Read page
  Page *fetchPage(int page_id) {

    auto found = mm.find(page_id);
    if (found == mm.end()) {
      cout << "Page miss : " << page_id << endl;
      return nullptr;
    }

    // page found, increase pin_count
    found->second->pin_count++;

    // move page to cache front
    cache.splice(cache.begin(), cache, found->second);
    return &(*found->second);
  }

  // release page
  void unpinPage(int page_id) {

    auto found = mm.find(page_id);
    if (found == mm.end())
      return;

    if (found->second->pin_count > 0)
      found->second->pin_count--;
  }

  // Insert/Update page
  void put(int page_id, const string &value) {

    auto found = mm.find(page_id);

    if (found != mm.end()) { // page found in buffer pool

      found->second->data = value;
      found->second->dirty = true;

      // move page to cache front
      cache.splice(cache.begin(), cache, found->second);
      cout << "Updated page : " << page_id << endl;

      return;
    }

    // page not found in buffer pool
    if (cache.size() >= Tcapacity) {
      evict_page();
    }

    cache.push_front({page_id, value, true, 0});
    // new page is always dirty, but pin_count = 0

    mm[page_id] = cache.begin();
    cout << "Inserted page : " << page_id << endl;
  }

  void print_state() {

    cout << "----Buffer Pool / Page cache content----" << endl;
    for (auto &page : cache)
      cout << "[Page ID : " << page.page_id
           << " => Page Content : " << page.data
           << "; Page Dirty: " << page.dirty
           << " ; Pin Count: " << page.pin_count << "]" << endl;
  }
};

int main() {

  BufferPool bp(3);

  bp.put(1, "Page1_data");
  bp.put(2, "Page2_data");
  bp.put(3, "Page3_data");

  // fetch page1
  BufferPool::Page *page = bp.fetchPage(1);
  if (page != nullptr) {
    cout << "Reading : " << page->data << endl;

    // simulate page use
    this_thread::sleep_for(chrono::seconds(2));

    // Pages are pinned when fetched and explicitly unpinned when
    // the caller is done using them, and eviction skips pinned pages.
    bp.unpinPage(1);
  }

  bp.put(4, "Page4_data");

  bp.print_state();

  return 0;
}
/*
 Inserted page : 1
Inserted page : 2
Inserted page : 3
Reading : Page1_data
Flushing dirty page : 2
Evicting page : 2
Inserted page : 4
----Buffer Pool / Page cache content----
[Page ID : 4 => Page Content : Page4_data; Page Dirty: 1 ; Pin Count: 0]
[Page ID : 1 => Page Content : Page1_data; Page Dirty: 1 ; Pin Count: 0]
[Page ID : 3 => Page Content : Page3_data; Page Dirty: 1 ; Pin Count: 0]
*/
