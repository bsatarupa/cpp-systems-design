/*
Use multiple CPU cores to reduce sorting latency.
why maxDepth matters? It provides Bounded parallelism.
Without it, we encounter : scheduler overhead, context switching, memory
overhead worse performance while merging halves.

Parallel recursion is used when independent subproblems can run simultaneously
on multiple CPU cores to improve performance. Example: Sort left half  ||  Sort
right half

Sequential recursion is used when subproblems are too small
/thread overhead becomes expensive /maximum parallelism limit reached.

Strategy:
Top levels  -> parallel recursion
Deep levels -> sequential recursion
*/
#include <cmath>
#include <iostream>
#include <thread>
#include <vector>

using namespace std;

class ParallelMergeSort {

  void merge(vector<int> &arr, int left, int mid, int right) {

    vector<int> temp;
    int i = left, j = mid + 1;

    while (i <= mid && j <= right) {

      if (arr[i] <= arr[j])
        temp.push_back(arr[i++]);
      else
        temp.push_back(arr[j++]);
    }

    while (i <= mid)
      temp.push_back(arr[i++]);

    while (j <= right)
      temp.push_back(arr[j++]);

    for (int k = 0; k < temp.size(); k++) //***write back to original array
      arr[left + k] = temp[k];
  }

  void merge_sort(vector<int> &arr, int left, int right, int curr_depth,
                  int max_depth) {

    if (left >= right)
      return;

    int mid = left + (right - left) / 2;

    if (curr_depth < max_depth) { // parallel recursion execution

      thread left_thread(&ParallelMergeSort::merge_sort, this, ref(arr), left,
                         mid, curr_depth + 1, max_depth);
      thread right_thread(&ParallelMergeSort::merge_sort, this, ref(arr),
                          mid + 1, right, curr_depth + 1, max_depth);

      left_thread.join();
      right_thread.join();
    } else { // sequential recursion execution

      merge_sort(arr, left, mid, curr_depth + 1, max_depth);
      merge_sort(arr, mid + 1, right, curr_depth + 1, max_depth);
    }

    // should be added for both Parallel and Sequential recursion, because every
    // recursion level must MERGE.
    merge(arr, left, mid, right);
  }

public:
  void sort(vector<int> &arr) {

    // max parallel depth based on #CPU cores
    int max_depth = log2(thread::hardware_concurrency());
    merge_sort(arr, 0, arr.size() - 1, 0, max_depth);
  }
};

int main() {

  vector<int> arr = {9, 4, 7, 1, 3, 8, 2, 6, 5};

  ParallelMergeSort sorter;
  sorter.sort(arr);

  for (int &x : arr)
    cout << x << " ";

  cout << endl;
  return 0;
}
// 1 2 3 4 5 6 7 8 9
