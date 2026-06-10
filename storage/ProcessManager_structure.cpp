#include <cstdint>
#include <iostream>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>
#include <vector>

using namespace std;

enum class ProcessState { READY, RUNNING, BLOCKED, TERMINATED };

struct CPUcontext {

  uint64_t pc;            // program counter
  uint64_t sp;            // stack pointer
  uint64_t registers[32]; // general-purpose registers

  CPUcontext() : pc(0), sp(0) {
    for (int i = 0; i < 32; i++)
      registers[i] = 0;
  }
};

struct Process {

  int pid;
  int ppid;

  ProcessState state;

  uint64_t cpu_time;     // ms
  uint64_t memory_usage; // bytes

  CPUcontext context;

  Process *parent;
  vector<Process *> children; // or, unordered_set<Process *> children; can be
                              // used for quick search

  Process(int pid, int ppid)
      : pid(pid), ppid(ppid), state(ProcessState::READY), cpu_time(0),
        memory_usage(0), parent(nullptr) {}
};

class ProcessManager {

  unordered_map<int, Process *> process_table; //<pid, reference to PCB>
  mutable shared_mutex rwlock;

public:
  ~ProcessManager() {
    for (auto &[pid, process] : process_table)
      delete process;
  }

  void addProcess(int pid, int ppid = -1) {

    unique_lock<shared_mutex> writeLock(rwlock);

    Process *proc = new Process(pid, ppid);

    if (process_table.count(ppid)) { // parent already exists in process_table

      proc->parent = process_table[ppid];
      // VVIMP:establish parent-child relationship with new Process
      process_table[ppid]->children.push_back(proc);
    }

    process_table[pid] = proc;
  }

  Process *getProcess(int pid) {

    shared_lock<shared_mutex> readlock(rwlock);

    if (!process_table.count(pid))
      return nullptr;

    return process_table[pid];
  }

  vector<Process *> getChildren(int pid) {

    shared_lock<shared_mutex> readlock(rwlock);

    if (!process_table.count(pid))
      return {};

    return process_table[pid]->children;
  }

  void updateState(int pid, ProcessState state) {

    unique_lock<shared_mutex> writelock(rwlock);

    if (!process_table.count(pid))
      return;

    process_table[pid]->state = state;
  }

  void updateCpuTime(int pid, uint64_t cpu_time) {

    unique_lock<shared_mutex> writelock(rwlock);

    if (!process_table.count(pid))
      return;

    process_table[pid]->cpu_time = cpu_time;
  }

  void updateMemoryUsage(int pid, uint64_t memory_usage) {

    unique_lock<shared_mutex> writelock(rwlock);

    if (!process_table.count(pid))
      return;

    process_table[pid]->memory_usage = memory_usage;
  }

  void setState(int pid, ProcessState state) {

    unique_lock<shared_mutex> writelock(rwlock);

    if (!process_table.count(pid))
      return;

    process_table[pid]->state = state;
  }

  void removeProcess(int pid) {

    unique_lock<shared_mutex> writelock(rwlock);

    if (!process_table.count(pid))
      return;

    Process *proc = process_table[pid];

    // remove process from its parent's children vector
    if (proc->parent) {
      auto &siblings = proc->parent->children;

      /*
      for (auto it = siblings.begin(); it != siblings.end(); it++) {
        if (*it == proc) {
          siblings.erase(it);
          break;
        }
      }
      */
      // remove all occurence of proc from siblings vector
      siblings.erase(remove(siblings.begin(), siblings.end(), proc),
                     siblings.end());
    }

    process_table.erase(pid); // remove dangling reference
    delete proc;
  }

  // Recursive methods
  void killProcess(int pid) {
    Process *p = getProcess(pid);
    if (!p)
      return;

    // kill descendants first, postorder traversal DFS
    vector<Process *> children =
        p->children; //***VVIMP: should extract children outside for loop
    for (auto child : children)
      killProcess(child->pid);

    // Now update p's state and erase p from process_table and reclaim space
    p->state = ProcessState::TERMINATED;

    // remove p from its parent's children vector
    if (p->parent) {
      auto &siblings = p->parent->children;

      /*
      for (auto it = siblings.begin(); it != siblings.end(); ++it) {
        if (*it == p) {
          siblings.erase(it);
          break;
        }
      }
      */
      // remove all occurence of p from siblings vector
      siblings.erase(remove(siblings.begin(), siblings.end(), p),
                     siblings.end());
    }

    process_table.erase(pid);
    delete p;
  }

  void printTree(Process *root, int depth = 0) {

    if (!root)
      return;

    for (int i = 0; i < depth; i++)
      cout << " ";

    if (depth > 0)
      cout << "|- ";

    cout << "PID = " << root->pid << endl;
    for (auto &child : root->children)
      printTree(child, depth + 1);
  }
};

int main() {

  ProcessManager pm;

  pm.addProcess(1, 0);
  pm.addProcess(2, 1);
  pm.addProcess(3, 1);

  pm.addProcess(4, 2);
  pm.addProcess(5, 2);

  pm.updateState(2, ProcessState::RUNNING);
  pm.updateCpuTime(2, 500);
  pm.updateMemoryUsage(2, 1024);

  auto children = pm.getChildren(1);
  cout << "Children of Process 1 :\n";
  for (auto child : children)
    cout << child->pid << endl;

  Process *p = pm.getProcess(2);
  if (p)
    cout << "CPU time : " << p->cpu_time << endl;

  pm.removeProcess(3);

  Process *root = pm.getProcess(1);
  cout << "Process hierarchy:\n";
  pm.printTree(root);

  cout << "\nKilling PID 2\n";
  pm.killProcess(2);

  cout << "\nAfter kill:\n";
  root = pm.getProcess(1);
  pm.printTree(root);

  return 0;
}
/*
Output:
Children of Process 1 :
2
3
CPU time : 500
Process hierarchy:
PID = 1
 |- PID = 2
  |- PID = 4
  |- PID = 5

Killing PID 2

After kill:
PID = 1

Implemented a thread-safe process table using a hashmap keyed by PID. Each
process stores its parent pointer, list of children, CPU time, memory usage, and
current state. Reader-writer locks enable concurrent access. Parent-child
relationships are maintained using pointers, allowing O(1) process lookup and
efficient traversal of the process hierarchy.

Follow-up 1: Why store both ppid and parent pointer?

Without ppid, serialization becomes difficult.

Without parent, traversal requires a hashmap lookup.

Hence both are useful:

int ppid;          // lightweight identifier
Process* parent;   // direct navigation

Follow-up 2: What information is saved during context switching?

Extend PCB:

struct CPUContext {

    uint64_t pc;     // program counter
    uint64_t sp;     // stack pointer

    uint64_t registers[32];
};

Process:

struct Process {

    int pid;
    int ppid;

    ProcessState state;

    uint64_t cpuTime;
    uint64_t memoryUsage;

    CPUContext context;

    Process* parent;
    vector<Process*> children;
};

Scheduler saves:

PC
SP
Registers

and restores another process.

Follow-up 3: How to support multithreading?
struct Thread {

    int tid;
    ProcessState state;

    uint64_t cpuTime;
};

Process:

struct Process {

    int pid;

    vector<Thread*> threads;

    Process* parent;
    vector<Process*> children;
};

Hierarchy:

Process
    |
    +---Thread1
    +---Thread2
    +---Thread3

Follow-up 4: Zombie Processes

Add:

bool zombie;

or

enum class ProcessState {
    NEW,
    READY,
    RUNNING,
    WAITING,
    ZOMBIE,
    TERMINATED
};

Child terminates:

exit()

Parent hasn't called:

wait()

⇒ process becomes ZOMBIE.

Follow-up 5: How to implement kill(pid)?
void kill(int pid) {

    Process* p = processTable[pid];

    p->state = ProcessState::TERMINATED;

    for (auto child : p->children)
        kill(child->pid);
}

Recursive tree deletion.

Complexity:

O(number of descendants)

Follow-up 6: Concurrent access?

Protect process table:

shared_mutex mutex;

Reads:

shared_lock<shared_mutex> lock(mutex);

Writes:

unique_lock<shared_mutex> lock(mutex);

Follow-up 7: How does Linux actually do it?

Each process has a PCB (task_struct) containing:

PID
Parent pointer
Children list
Process state
CPU registers
Memory descriptors
Open file descriptors
Scheduling information
Signals
Credentials

The scheduler operates on these PCBs.

Interview answer (ideal)
struct Process {
    int pid;
    int ppid;

    ProcessState state;

    uint64_t cpuTime;
    uint64_t memoryUsage;

    CPUContext context;

    Process* parent;
    vector<Process*> children;
};

and maintain
unordered_map<int, Process*> processTable;
for O(1) PID lookup.

What additional information must be saved during a context switch?"
Program counter, stack pointer, and CPU registers, which together form the CPU
context stored inside the PCB.

1. Context switch

Add:
struct CPUContext {
    uint64_t pc;
    uint64_t sp;
    uint64_t registers[32];
};
Saved when scheduler switches processes.

2. Threads
struct Thread {
    int tid;
    ProcessState state;
    CPUContext context;
};
vector<Thread*> threads;
inside Process.

3. Open files
vector<int> openFDs;
or
vector<File*> openFiles;

4. Scheduling information
int priority;
uint64_t timeSlice;

5. Zombie processes
Add:
ProcessState::ZOMBIE
instead of immediately deleting.

6. Synchronization
Protect processTable with:
shared_mutex mutex;
for concurrent create/kill/lookups.

7. Smart pointers (production)
Replace raw pointers:
unordered_map<int, unique_ptr<Process>> processTable;
and
vector<Process*> children;
to avoid manual deletes.
*/
