/*
Implement an Index Scan operator that uses an existing B+Tree index to retrieve
rows by key. IndexScan
        --------------------
        B+Tree*
        searchKey
        --------------------
        open()
        next()
        close()

IndexScan
↓
B+Tree Search Index
(Instead of Sequential Search)
↓
Return matching rows
*/

#include <iostream>
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
  virtual ~Iterator() = default;
};

class B_plus_Tree { // Fake B+tree

  unordered_map<int, Row> index; //<row_id, Row> mapping
public:
  void insert(const Row &row) { index[row.row_id] = row; }

  Row *search(int key) {

    auto it = index.find(key);
    if (it == index.end())
      return nullptr;

    return &it->second;
  }
};

class IndexScan : public Iterator {

  B_plus_Tree *tree;

  int searchKey;
  bool returned = false;

public:
  IndexScan(B_plus_Tree *tree, int key) : tree(tree), searchKey(key) {}

  void open() override { returned = false; }

  bool next(Row &row) override {

    if (returned) // already found searchKey in previous iterations
      return false;

    Row *result = tree->search(searchKey);
    if (!result)
      return false;

    row = *result; // VVIMP

    returned = true;
    return true;
  }

  void close() override {}
};

int main() {

  B_plus_Tree tree;

  tree.insert({1, "Alice", 5000});
  tree.insert({2, "Bob", 3000});
  tree.insert({3, "Charlie", 9000});

  IndexScan scan(&tree, 2);

  scan.open();

  Row row;
  while (scan.next(row))
    cout << row.row_id << " " << row.name << " " << row.salary << endl;

  scan.close();

  return 0;
}
/* Output:
2 Bob 3000

What Each Class Does?
a. BPlusTree:
We're assuming the index already exists.
1
↓
Alice
2
↓
Bob
3
↓
Charlie
Provides search(key)

b. IndexScan:
Instead of Sequential Scan,
Row1
↓
Row2
↓
Row3

it does search(2),
↓
Bob
Only returns matching rows.
Complexity: B+tree: O(log N), unordered_map: O(1)

Follow-up Q&A:
1. Why Index Scan?
Table Scan: O(N)
Index Scan: O(log N)
Huge improvement for selective queries.

2. Why B+Tree instead of Hash?
Because B+Trees support:
Range scans, ORDER BY, Prefix search
Hash indexes don't.

3. Range Scan?
Instead of
search(20)

implement
rangeSearch(20,40)

Returns
20
21
22
...
40

4. Why Iterator?
Because now
Filter
↓
IndexScan

works exactly like
Filter
↓
TableScan
The parent operator doesn't care where rows come from.
*/
