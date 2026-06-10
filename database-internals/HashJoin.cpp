/*
 Design a query execution operator for an INNER JOIN. The operator should
 implement the build phase and probe phase of a hash join.
 Implement an efficient join between 2 datasets where 1 dataset can fit into
memory. SELECT * FROM A JOIN B ON A.id = B.id;

what is HashJoin?
SELECT * FROM Employee e JOIN Department d ON e.deptId = d.id;
Instead of Nested Loop O(N × M), Hash Join achieves O(N + M)

1. Why build on smaller table?
To minimize memory. Build : 100 MB, Probe : 10 GB. instead of Build : 10 GB

2. Duplicate Keys?
Use unordered_map<int, vector<Row>> instead of unordered_map<int, Row>

3. Memory Overflow?
If hash table doesn't fit RAM, Hash both tables into the same partitions.
Then join each partition independently. This is Grace Hash Join.

4. Grace Hash Join?
Employee
      \
       Partition
      /
Department
↓
P0
P1
P2
P3
↓
Hash Join each partition, Each partition fits in memory.
Complexity remains approximately O(N + M) with additional sequential disk I/O.
Grace Hash Join parallelizes by assigning different partitions to different
threads.

5. Parallel Hash Join?
Different partitions are independent.
Thread1 → P0
Thread2 → P1
Thread3 → P2
Thread4 → P3
*/
#include <iostream>
#include <vector>

using namespace std;

struct Employee {
  int emp_id;
  string name;
  int dept_id;
};

struct Department {
  int dept_id;
  string name;
};

class HashJoin {
  // If there exists Duplicate Keys(One-to-Many Join),
  // store a vector instead of a single value.
  unordered_map<int, vector<Department>> hashTable;

public:
  // 1. build hashtable using smaller table (Department)
  void buildHashtable(const vector<Department> &departments) {
    for (auto const &dept : departments)
      hashTable[dept.dept_id].push_back(dept);
  }

  // 2. Probe using bigger table (Employee)
  void probe(const vector<Employee> &employees) {
    for (auto const &emp : employees) {
      auto it = hashTable.find(emp.dept_id);

      if (it == hashTable.end())
        continue;

      for (const auto &dept : it->second)
        cout << emp.emp_id << " " << emp.name << " " << dept.name << endl;
      // emp_id, emp_name, dept_name
    }
  }

  void execute(const vector<Employee> &employees,
               const vector<Department> &departments) {
    buildHashtable(departments);
    probe(employees);
  }
};

int main() {
  vector<Department> departments = {
      {1, "HR"}, {2, "Sales"}, {3, "Engineering"}, {4, "Finance"}};

  vector<Employee> employees = {
      {101, "Alice", 2},   {102, "Bob", 1},
      {103, "Charlie", 3}, {104, "David", 2},
      {105, "Eva", 4},     {106, "Frank", 5} // No matching department
  };

  HashJoin join;
  join.execute(employees, departments);
}
/*
Output:
101 Alice Sales
102 Bob HR
103 Charlie Engineering
104 David Sales
105 Eva Finance

The class has only three responsibilities:
build(): Builds the hash table from the smaller relation.
probe(): Finds matching rows in the hash table.
execute(): Orchestrates build and probe.

Complexity
Build: O(M)
Probe: O(N)
Overall: O(M + N)
Space: O(M)
where M is the number of rows in the build relation.
*/
