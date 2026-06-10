/*
 Volcano (pull) Iterator Model = Every operator pulls one row from its child.
 TableScan : Reads rows from storage.
Filter : Keeps only rows satisfying a predicate.
Project : Selects (or outputs) the required columns.
HashJoin : Combines rows from two child iterators.
Aggregate : Groups rows and computes COUNT/SUM/etc.
Sort : Orders rows.
Limit : Stops after N rows.

Because every operator implements the same interface, they are composable.
open();
next();
close();
A database engine can build different execution trees simply by wiring
these operators together, without changing their implementations.
This is the essence of the Volcano iterator model used in many relational query
engines.

Pull based (Volcano) Iterator Model: Execution Tree
        Project
           |
        Filter
           |
       TableScan

Execution flow:---------
Project.next()
↓
Filter.next()
↓
Scan.next()
↓
returns row
↓
Filter checks predicate
↓
Project prints columns

Complexity:
TableScan = Filter = Project = O(N)
*/
#include <iostream>
#include <vector>
using namespace std;
// Equivalent SQL: SELECT id, name FROM Employee WHERE age >= 30;
struct Row {
  int row_id;
  string name;
  int age;
};

class Iterator {
public:
  virtual void open() = 0;
  virtual bool next(Row &row) = 0;
  virtual void close() = 0;
  ~Iterator() = default;
};

// TableScan simply reads rows from the table one by one.
// It knows nothing about filtering or projection.
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

// Filter doesn't have a table. It only has a child(=TableScan) iterator.
class Filter : public Iterator {

  Iterator *child;

public:
  Filter(Iterator *child) : child(child) {}

  void open() override { child->open(); }

  bool next(Row &row) override {

    while (child->next(row)) {
      // if any row has age >= 30, then only return T
      if (row.age >= 30)
        return true;
    }
    return false;
  }

  void close() override { child->close(); }
};

// Project doesn't have a table. It only has a child(=Filter) iterator.
class Project : public Iterator {

  Iterator *child;

public:
  Project(Iterator *child) : child(child) {}

  void open() override { child->open(); }

  bool next(Row &row) override {

    if (!child->next(row))
      return false;

    cout << row.row_id << " " << row.name << endl;
    return true;
  }

  void close() override { child->close(); }
};

int main() {

  vector<Row> table = {{1, "Alice", 25},
                       {2, "Bob", 35},
                       {3, "Charlie", 40},
                       {4, "David", 22},
                       {5, "Eva", 45}};

  TableScan scan(table);
  Filter filter(&scan);
  Project project(&filter);

  project.open();

  Row row;
  while (project.next(row))
    ;

  project.close();
  return 0;
}
/* Output:
2 Bob
3 Charlie
5 Eva

This code implements the Volcano Iterator Model:

            Project
               |
            Filter
               |
           TableScan

Each operator exposes:
open();
next();
close();
This is exactly how many query execution engines are structured.

1. Why Iterator?
Every operator has the same interface. Easy to compose.
open()
next()
close()

2. Why Volcano Model?
Each parent pulls tuples from its child, instead of pushing.
Project
↓
Filter
↓
Scan

3. Add Hash Join?
            Project
               |
            HashJoin
           /       \
       Scan       Scan

4. Add Aggregation?
           Aggregate
               |
            Filter
               |
             Scan

5. Add Sort?
          Project
             |
           Sort
             |
          Filter
             |
           Scan

6. Predicate Pushdown?
Move Filter as close to Scan as possible.
*/
