/* Query Execution Engine = Operator Tree = Query Plan Tree :
Design a query execution framework consisting of operators like Scan, Filter,
Join, and Aggregate. Operators should process tuples incrementally.
New operators should be easy to add.

A query execution engine where each operator processes tuples and
passes them to its parent. Operators such as Scan, Filter, Join,
and Aggregate should be composable into an execution tree.

We will use Composite pattern. For simplicity, Join is implemented as a
concatenation (UNION ALL) to demonstrate operator composition. In a real
query engine, this would be replaced with a nested-loop join or hash join.
*/

#include <iostream>
#include <vector>
using namespace std;

class QueryOperator {
public:
  virtual vector<int> execute() = 0;
  virtual ~QueryOperator() = default;
};

class Scan : public QueryOperator {
  vector<int> data;

public:
  Scan(vector<int> data) : data(data) {}

  vector<int> execute() override { return data; }
};

class Filter : public QueryOperator {
  QueryOperator *child;

public:
  Filter(QueryOperator *child) : child(child) {}

  vector<int> execute() override {
    vector<int> output;

    for (int value : child->execute()) {

      // placeholder/stub filtering
      if (value % 2 == 0)
        output.push_back(value);
    }
    return output;
  }
};

class Join : public QueryOperator {

  QueryOperator *left, *right;

public:
  Join(QueryOperator *left, QueryOperator *right) : left(left), right(right) {}

  vector<int> execute() override {

    vector<int> result = left->execute();
    vector<int> right_data = right->execute();

    result.insert(result.end(), right_data.begin(), right_data.end());

    return result;
  }
};

class Aggregate : public QueryOperator {

  QueryOperator *child;

public:
  Aggregate(QueryOperator *child) : child(child) {}

  vector<int> execute() override {

    int sum = 0;
    for (int value : child->execute())
      sum += value;

    return {sum};
  }
};

int main() {

  Scan scan1({1, 2, 3, 4, 5});
  Scan scan2({10, 20});

  Filter filter(&scan1);
  Join join(&filter, &scan2);
  Aggregate aggregate(&join);
  cout << "Sum = " << aggregate.execute()[0] << endl;

  return 0;
}
/*
Output:
Sum = 36

Class Diagram:
                QueryOperator
                     ^
      --------------------------------
      |        |        |            |
    Scan    Filter     Join     Aggregate
                |      /   \
                |     /     \
             QueryOperator  QueryOperator
*/
