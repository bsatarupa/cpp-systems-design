#include <iostream>
#include <map>

using namespace std;

class DynamicMemoryAllocator {

  map<int, int> available, allocated; //<start_address, block_size>

public:
  DynamicMemoryAllocator(int TotalMemory) {
    available[0] = TotalMemory; // initialize available: {0, TotalMemory}
  }

  int custom_malloc(int requested_size) { // returns ALLOCATED start_address.
                                          // malloc(requested_size)

    map<int, int>::iterator it = available.begin();
    for (; it != available.end(); it++) {

      int block_start = it->first;
      int block_size = it->second;

      if (requested_size <= block_size) {

        available.erase(it);
        allocated[block_start] = requested_size;

        if (requested_size < block_size)
          available[block_start + requested_size] =
              block_size - requested_size; //<remaining_start, remaining_size>

        return block_start;
      }
    }
    return -1; // no suitable block found to occupy
  }

  void custom_free(int address) { // free(address)

    auto found = allocated.find(address);
    if (found == allocated.end()) {

      cout << "Invalid FREE address!" << endl;
      return;
    }

    int block_start = address;
    int block_size = found->second;

    allocated.erase(found);
    available[block_start] = block_size;

    // Merge adjacent Free blocks as well, starting from current location on
    // available map
    auto curr = available.find(block_start);

    // 1. Merge with next block
    auto next_iter = next(curr);
    if (next_iter != available.end() &&
        curr->first + curr->second == next_iter->first) {

      curr->second += next_iter->second;
      available.erase(next_iter);
    }

    // 2. Merge with previous block
    if (curr != available.begin()) {

      auto prev_iter = prev(curr);
      if (prev_iter->first + prev_iter->second == curr->first) {

        prev_iter->second += curr->second;
        available.erase(curr);
      }
    }
  }
};

int main() {

  DynamicMemoryAllocator allocator(1000);

  int p1 = allocator.custom_malloc(100);
  int p2 = allocator.custom_malloc(200);
  int p3 = allocator.custom_malloc(150);

  cout << "---Allocated---" << endl;

  cout << "Starting Location for Block_1 : " << p1 << endl;
  cout << "Starting Location for Block_2 : " << p2 << endl;
  cout << "Starting Location for Block_3 : " << p3 << endl;

  allocator.custom_free(p2);
  allocator.custom_free(p1);
  allocator.custom_free(p3);

  return 0;
}
/*
---Allocated---
Starting Location for Block_1 : 0
Starting Location for Block_2 : 100
Starting Location for Block_3 : 300
*/
