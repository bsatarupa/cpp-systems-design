/*
Implement a Sort Operator for a query execution engine.
Example SQL: SELECT * FROM Employee ORDER BY salary;
Execution Tree:
        Sort
          |
      TableScan

The Sort operator:
Reads all rows from its child.
Stores them in a vector<Row>.
Calls std::sort().
Returns rows one at a time through next()

              Iterator
              open()
              next()
              close()
                   ^
                   |
        ------------------------
        |                      |
    TableScan             SortOperator
*/
#include <cstddef>
#include <iostream>
#include <vector>
using namespace std;

struct Row {
  int row_id;
  string name;
  int salary;
};

class Iterator {
public:
  virtual void open() = 0;
  virtual bool next(Row &row) = 0;
  virtual void close() = 0;
  ~Iterator() = default;
};

// TableScan simply reads rows from the table one by one.
// It knows nothing about filtering or sorting.
class TableScan : public Iterator {

  vector<Row> table;
  int index = 0;

public:
  TableScan(vector<Row> &rows) : table(rows) {}

  void open() override { index = 0; }

  // returns next row from table
  bool next(Row &row) override {

    if (index >= table.size())
      return false;

    row = table[index++];
    return true;
  }

  void close() override {}
};

// SortOperator doesn't have a table. It only has a child(=TableScan) iterator.
class SortOperator : public Iterator {

  Iterator *child;

  vector<Row> rows;
  size_t index = 0;

public:
  SortOperator(Iterator *child) : child(child) {}

  void open() override {

    index = 0;
    rows.clear();

    child->open();

    Row row;
    while (child->next(row))
      rows.push_back(row);

    sort(rows.begin(), rows.end(),
         [&](const Row &a, const Row &b) { return a.salary < b.salary; });
  }

  bool next(Row &row) override {

    if (index >= rows.size())
      return false;

    row = rows[index++];
    return true;
  }

  void close() override { child->close(); }
};

int main() {

  vector<Row> employees = {{1, "Alice", 5000},
                           {2, "Bob", 3000},
                           {3, "Charlie", 9000},
                           {4, "David", 4000},
                           {5, "Eva", 7000}};

  TableScan scan(employees);
  SortOperator sort(&scan);

  sort.open();

  Row row;
  while (sort.next(row)) {
    cout << row.row_id << " " << row.name << " " << row.salary << endl;
  }

  sort.close();

  return 0;
}
/* TC: O(N log N)
Output:
2 Bob 3000
4 David 4000
1 Alice 5000
5 Eva 7000
3 Charlie 9000

1. Sort by different columns?
Instead of hardcoding a.salary < b.salary, pass a comparator:
function<bool(const Row&, const Row&)> comparator;
This allows sorting by salary, name, id, etc.

2. What if the table doesn't fit in memory?
Use External Merge Sort.
Read chunk
↓
Sort chunk
↓
Write run to disk
↓
Merge runs

3. Stable Sort?
Use stable_sort(...) when equal keys must preserve input order.

What Each Class Does?
a. TableScan Reads rows sequentially.
Table
↓
Alice
↓
Bob
↓
Charlie
Returns one row every time next() is called.

b. SortOperator
Instead of returning rows immediately,
open()
↓
Read ALL rows
↓
vector<Row>
↓
std::sort()
↓
next()
↓
Return one sorted row each time.
*/
