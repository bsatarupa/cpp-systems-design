/*
Implement:
SELECT dept, COUNT(*), SUM(salary) FROM Employee GROUP BY dept;

Complexity-----
Build groups: O(N)
Memory : O(number of groups)

1. AVG?
Just store another field in:
struct AggregateState {
    int count = 0;
    int sum = 0;
};
avg = sum / count;

2. MIN?
Just store another field in:
struct AggregateState {
    int minSalary = INT_MAX;
};
Update state.minSalary = min(state.minSalary, e.salary);

3. MAX?
state.maxSalary = max(state.maxSalary, e.salary);

4. Multiple Aggregates?
struct AggregateState {
    int count;
    int sum;
    int min;
    int max;
};
One scan computes everything.

5. Composite GROUP BY GROUP BY dept, city?
Use pair<string,string> as the key with a custom hash.

6. Memory Overflow?
If there are millions of groups:
Hash Partition
↓
Partition0
Partition1
Partition2
↓
Aggregate each partition
↓
Merge
Exactly the same idea as Grace Hash Join.

7. Parallel Aggregation?
Thread1 unordered_map ->
Thread2 unordered_map ->
...
Merge Maps
Every distributed SQL engine does something similar.
*/
#include <iostream>
#include <unordered_map>
#include <vector>
using namespace std;

struct Employee {
  string name;
  string dept;
  int salary;
};

struct AggregateState {
  int count = 0;
  int sum = 0;
};

class AggregationOperator {
  unordered_map<string, AggregateState>
      groups; //<dept, <emp_count, salary_sum>>
public:
  void aggregate(const vector<Employee> &employees) {

    for (auto const &emp : employees) {

      auto &state = groups[emp.dept];

      state.count++;
      state.sum += emp.salary;
    }
  }

  void print() {
    for (const auto &[dept, state] : groups) {
      cout << "Dept:" << dept << ", Count:" << state.count
           << ", Sum:" << state.sum << endl;
    }
  }
};

int main() {

  vector<Employee> employees = {{"Alice", "HR", 5000},
                                {"Bob", "HR", 6000},
                                {"Charlie", "Engineering", 9000},
                                {"David", "Engineering", 8000},
                                {"Eva", "HR", 7000}};

  AggregationOperator op;
  op.aggregate(employees);

  op.print();

  return 0;
}
/*
Output:
Dept:Engineering, Count:2, Sum:17000
Dept:HR, Count:3, Sum:18000
*/
