/*
Implement Lock-Free Stack:
CAS
acquire/release
ABA awareness
*/
#include <atomic>
#include <iostream>
#include <thread>
#include <vector>

using namespace std;

template <typename T>

class LockFreeStack {
  struct Node {
    T val;
    Node *next;

    explicit Node(const T &v) : val(v), next(nullptr) {}
  };

  atomic<Node *> head{nullptr};

public:
  void push(const T &v) {
    Node *new_node = new Node(v);
    new_node->next = head.load(memory_order_relaxed);

    // update head to new_node
    while (!head.compare_exchange_weak(
        new_node->next, new_node, memory_order_release, memory_order_relaxed)) {
    }
  }

  bool pop(T &result) {
    Node *old_head = head.load(memory_order_acquire);

    while (old_head) {
      Node *next_node = old_head->next;

      if (head.compare_exchange_weak(old_head, next_node, memory_order_acquire,
                                     memory_order_relaxed)) {
        result = old_head->val;
        // Do not delete old_head, memory reclamation is a separate problem
        return true;
      }
    }
    return false;
  }
};

int main() {
  LockFreeStack<int> stack;

  constexpr int NUM_THREADS = 4, ITEMS_PER_THREAD = 1000;

  vector<thread> producers, consumers;

  for (int i = 0; i < NUM_THREADS; i++) {
    producers.emplace_back([&stack, i]() {
      for (int j = 0; j < ITEMS_PER_THREAD; j++)
        stack.push(i * ITEMS_PER_THREAD + j);
    });
  }

  for (auto &t : producers)
    t.join();

  atomic<int> popped_count{0}; // popped_elements_count

  for (int i = 0; i < NUM_THREADS; i++) {
    consumers.emplace_back([&stack, &popped_count]() {
      int val;
      while (stack.pop(val)) {
        popped_count++;
      }
    });
  }

  for (auto &t : consumers)
    t.join();

  cout << "Popped : " << popped_count.load() << '\n';
  return 0;
}
/*
Output:
Popped : 4000

Q1. What is CAS doing?
Suppose:
head
 ↓
 A → B → C

Thread wants to pop A. It reads:
old_head = A;
next = A->next;  // B

Then:
CAS(head, A, B) means:
if (head == A)
    head = B;
else
    fail
If another thread changed the stack first, CAS fails and old_head
is updated with the current head. The loop retries.

Q2. Why acquire on pop?
head.load(std::memory_order_acquire);
ensures that once we observe a node through the atomic head,
we can safely observe the data initialized/published before it.

Q3. Why release on push?
head.compare_exchange_weak(
    ...,
    std::memory_order_release,
    ...
); publishes the newly initialized node.

Before CAS:
new_node->value = value;
new_node->next = old_head;

Then release CAS makes those writes visible to a thread that
subsequently acquires the node through head.

Memory-Order Rule to remember:
PUSH: CAS success → release
POP: load/CAS success → acquire

Q4. ABA problem:
Initial:
head
 ↓
 A → B

Thread 1 Reads:
old_head = A
next = B
Then it gets paused.

Thread 2 Pops A:
head → B
Then something happens such that A becomes head again:

head
 ↓
 A → C

Now Thread 1 wakes up. It still has:
old_head = A
next = B
and executes:
CAS(head, A, B)

CAS sees: head == A, so it succeeds.
But that's wrong.The pointer changed.
CAS only sees: A → A
It does not know that the pointer went through other states. That is ABA.

Q5. Why delete makes this even harder?
We might be tempted to write: delete old_head; after a successful pop.

But, Another thread might still be accessing that node.
We now have a memory reclamation problem.
The real lock-free stack problem is therefore: CAS + memory ordering + ABA +
safe memory reclamation

Q6. How do real implementations handle ABA?
Common approaches:
1. Tagged/versioned pointers
2. Hazard pointers
3. Epoch-based reclamation
4. RCU-like techniques

Summary:
Node*
   ↓
std::atomic<Node*> head
   ↓
push → CAS loop
   ↓
pop → CAS loop
   ↓
release on publication
   ↓
acquire on consumption
   ↓
ABA problem
   ↓
memory reclamation problem
   ↓
hazard pointers / tagged pointers / epochs
*/
