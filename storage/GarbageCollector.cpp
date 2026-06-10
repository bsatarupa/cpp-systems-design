#include <iostream>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>

using namespace std;

class Object {
public:
  int object_id;
  int ref_count;
  bool deleted;

  Object(int object_id = 0)
      : object_id(object_id), ref_count(1), deleted(false) {}
};

class GCManager {

  unordered_map<int, Object *>
      object_table; // <object_id -> ObjectMetadata mapping>
  mutable shared_mutex rwlock;

public:
  ~GCManager() {
    for (auto &[id, obj] : object_table)
      delete obj;
  }

  void addObject(int object_id) {
    unique_lock<shared_mutex> writeLock(rwlock);
    if (object_table.count(object_id))
      return;

    object_table[object_id] = new Object(object_id);
  }

  void incrementRefCount(int object_id) {
    unique_lock<shared_mutex> writeLock(rwlock);
    if (!object_table.count(object_id))
      return;

    object_table[object_id]->ref_count++;
  }

  void decrementRefCount(int object_id) {
    unique_lock<shared_mutex> writeLock(rwlock);
    if (!object_table.count(object_id))
      return;

    Object *obj = object_table[object_id];
    obj->ref_count--;
    if (obj->ref_count == 0)
      obj->deleted = true;
  }

  void runGC() {

    unique_lock<shared_mutex> writeLock(rwlock);

    for (auto it = object_table.begin(); it != object_table.end();) {

      Object *obj = it->second;
      if (obj->deleted) {
        cout << "reclaiming object space for : " << obj->object_id << endl;

        delete obj; // removing dangling object first
        it = object_table.erase(it);
        //**VVIMP: assignment is a MUST to avoid seg fault, now returns next
        // iterator
      } else {
        it++;
      }
    }
  }

  void print() {

    shared_lock<shared_mutex> lock(rwlock);

    cout << "\nObject Table\n";

    for (auto &[id, obj] : object_table) {

      cout << "Object " << obj->object_id << " RefCount=" << obj->ref_count
           << " Deleted=" << obj->deleted << endl;
    }
  }
};

int main() {

  GCManager gc;

  gc.addObject(1);
  gc.addObject(2);
  gc.addObject(3);

  gc.incrementRefCount(1);
  gc.incrementRefCount(1);

  gc.print();

  gc.decrementRefCount(1);
  gc.decrementRefCount(1);
  gc.decrementRefCount(1);

  gc.decrementRefCount(3);

  cout << "\nRunning GC\n";

  gc.runGC();

  gc.print();

  return 0;
}
/*
Object Table
Object 3 RefCount=1 Deleted=0
Object 2 RefCount=1 Deleted=0
Object 1 RefCount=3 Deleted=0

Running GC
reclaiming object space for : 3
reclaiming object space for : 1

Object Table
Object 2 RefCount=1 Deleted=0

Components
Object
 ├── object_id
 ├── ref_count
 └── deleted

GCManager
 ├── addObject()
 ├── incrementRefCount()
 ├── decrementRefCount()
 ├── runGC()
 └── print()
Follow-up answer

This implementation uses reference counting with lazy reclamation.
Objects whose reference count reaches zero are marked deleted, and a background
GC thread can periodically invoke runGC() to reclaim them. A limitation of
reference counting is cyclic references, which can be handled using
mark-and-sweep.

Q1. Why lazy deletion?

Avoid expensive cleanup on the user path.
decrementRefCount()
      ↓
mark deleted
      ↓
background GC thread

Q2. Limitation of reference counting?
Cycles.

A → B
↑   ↓
└───┘
Both have refCount=1 and never get reclaimed.

Q3. How to solve cycles?
Mark-and-sweep:
Roots
 ↓
Mark reachable
 ↓
Sweep unreachable

Q4. Thread safety?
Use shared_mutex.

Q5. Can runGC() be asynchronous?
Yes.
Background thread
    |
periodically
    |
runGC()
*/
