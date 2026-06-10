/*
Buffer pools(Page cache) reduce expensive disk IO by caching frequently accessed
pages in memory and evicting least recently used pages when memory is full.
Storage systems read/write data in fixed-size blocks called Pages.Typical
size:4/8/16KB. A page may contain: DB rows/ index nodes/ filesystem blocks.
Application
   ↓
Buffer Pool (RAM cache)
   ↓
Disk (only if needed)
*/
#include <iostream>
#include <list>
#include <string>
#include <unordered_map>

using namespace std;

class BufferPool {

  struct Page {

    int page_id;
    string data;
    bool dirty = false; // for each page
  };

  int Tcapacity;
  list<Page> cache; // dll with MRU page at the front
  unordered_map<int, list<Page>::iterator> mm;

  void flush_page(Page &page) {

    cout << "Flushing dirty page : " << page.page_id << endl;

    //***mark the page clean
    page.dirty = false;
  }

  void evict_page() {

    auto victim = cache.back();

    //***if victim is dirty, flush it before eviction
    if (victim.dirty == true)
      flush_page(victim);

    cout << "Evicting page : " << victim.page_id << endl;
    mm.erase(victim.page_id);
    cache.pop_back();
  }

public:
  BufferPool(int capacity) : Tcapacity(capacity) {}

  // Read page
  string getPage(int page_id) {

    auto found = mm.find(page_id);
    if (found == mm.end()) {
      cout << "Page miss : " << page_id << endl;
      return "PAGE_NOT_FOUND!";
    }

    // move page to front
    cache.splice(cache.begin(), cache, found->second);
    return found->second->data;
  }

  // Insert/Update page
  void put(int page_id, const string &value) {

    auto found = mm.find(page_id);

    if (found != mm.end()) { // page found in buffer pool

      found->second->data = value;
      found->second->dirty = true;

      // move to cache front
      cache.splice(cache.begin(), cache, found->second);
      cout << "Updated page : " << page_id << endl;

      return;
    } else {
      if (cache.size() >= Tcapacity) {

        evict_page();
      }

      cache.push_front({page_id, value, true}); // new page is always dirty
      mm[page_id] = cache.begin();
      cout << "Inserted page : " << page_id << endl;
    }
  }

  void print_state() {

    cout << "----Buffer Pool / Page cache content----" << endl;
    for (auto &page : cache)
      cout << "[Page ID : " << page.page_id
           << " => Page Content : " << page.data
           << "; Page Dirty: " << page.dirty << "]" << endl;
  }
};

int main() {

  BufferPool bp(3);

  bp.put(1, "Page1_data");
  bp.put(2, "Page2_data");
  bp.put(3, "Page3_data");

  bp.getPage(1);

  bp.put(4, "Page4_data");

  bp.print_state();

  return 0;
}
/*
Inserted page : 1
Inserted page : 2
Inserted page : 3
Flushing dirty page : 2     //since Page1 got accessed recently, when page cache
becomes full, Page2 is evicted. Evicting page : 2 Inserted page : 4
----Buffer Pool / Page cache content----
[Page ID : 4 => Page Content : Page4_data; Page Dirty: 1]
[Page ID : 1 => Page Content : Page1_data; Page Dirty: 1]
[Page ID : 3 => Page Content : Page3_data; Page Dirty: 1]
*/
