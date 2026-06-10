/* Query Execution Framework = Query Plan Tree
Design a Query Plan Tree representing database execution plans.
Each node performs an operation and produces tuples for its parent.
Plans should be composable and executable recursively.

          Aggregate
              |
            Join
          /      \
     Filter      Scan2
        |
      Scan1

Query Execution Engine:-----
Focuses on executing operators
execute() is the primary API
Runtime execution

Query Plan tree:-----
Focuses on representing the execution plan
Tree composition is the primary API
Execution Plan representation
*/
#include <iostream>

using namespace std;

class PlanNode {
public:
  virtual vector<int> execute() = 0;
  virtual ~PlanNode() = default;
};

class Scan : public PlanNode {
  vector<int> data;

public:
  Scan(vector<int> data) : data(data) {}

  vector<int> execute() override { return data; }
};

class Filter : public PlanNode {
  PlanNode *child;

public:
  Filter(PlanNode *child) : child(child) {}

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

class Join : public PlanNode {

  PlanNode *left, *right;

public:
  Join(PlanNode *left, PlanNode *right) : left(left), right(right) {}

  vector<int> execute() override {

    vector<int> result = left->execute();
    vector<int> right_data = right->execute();

    result.insert(result.end(), right_data.begin(), right_data.end());

    return result;
  }
};

class Aggregate : public PlanNode {

  PlanNode *child;

public:
  Aggregate(PlanNode *child) : child(child) {}

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

                PlanNode
                     ^
      --------------------------------
      |        |        |            |
    Scan    Filter     Join     Aggregate

Each node is a PlanNode.
*/
