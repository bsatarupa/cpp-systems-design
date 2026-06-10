/*
Design a query optimizer that improves database execution plans before
execution. Optimization rules such as Predicate Pushdown and Projection Pruning
should be easy to add. The optimizer should apply multiple optimization rules to
a query plan without modifying existing code.

Requirements:
- Accept a query execution plan.
- Apply one or more optimization rules.
- Support multiple optimization rules.
- New optimization rules should be easy to add.
- Optimize the plan before execution.

Uses Strategy Pattern (Rule-Based Optimization).
1st part is same as Query Plan Tree. 2nd part(QueryOptimizer) is extra.

                    PlanNode
                      ^
    ----------------------------------
    |        |         |            |
  Scan    Filter      Join      Aggregate

                        QueryOptimizer
                            |
             --------------------------------------------
             |                   |                      |
      PredicatePushdown   ProjectionPruning     JoinReordering
*/
#include <iostream>
#include <vector>

using namespace std;
// Part: 1 [Query Plan Tree]
class PlanNode {

public:
  virtual void execute() = 0;

  virtual ~PlanNode() = default;
};

class Scan : public PlanNode {

public:
  void execute() override { cout << "Scan\n"; }
};

class Filter : public PlanNode {

  PlanNode *child;

public:
  Filter(PlanNode *child) : child(child) {}

  void execute() override {

    child->execute();

    cout << "Filter\n";
  }
};

class Join : public PlanNode {

  PlanNode *left;
  PlanNode *right;

public:
  Join(PlanNode *left, PlanNode *right) : left(left), right(right) {}

  void execute() override {

    left->execute();
    right->execute();

    cout << "Join\n";
  }
};

class Aggregate : public PlanNode {

  PlanNode *child;

public:
  Aggregate(PlanNode *child) : child(child) {}

  void execute() override {

    child->execute();

    cout << "Aggregate\n";
  }
};
// Plan2: QueryOptimizer [Optimization Rules : Predicate Pushdown, Projection
// Pruning, Join Reordering etc.]
class OptimizerRule {
public:
  virtual void apply() = 0;
  virtual ~OptimizerRule() = default;
};

class PredicatePushdown : public OptimizerRule {
public:
  void apply() override { cout << "Applying Predicate Pushdown\n" << endl; }
};

class ProjectionPruning : public OptimizerRule {
public:
  void apply() override { cout << "Applying Projection Pruning\n" << endl; }
};

class QueryOptimizer {

  vector<OptimizerRule *> rules;

public:
  void addRule(OptimizerRule *rule) { rules.push_back(rule); }

  void optimize() {
    cout << "\n----Optimizing Query Plan----\n";
    for (OptimizerRule *r : rules)
      r->apply();
  }
};

int main() {

  Scan scan1;
  Scan scan2;

  Filter filter(&scan1);
  Join join(&filter, &scan2);
  Aggregate aggregate(&join);
  cout << "----- Original Query Plan -----\n";
  aggregate.execute();

  PredicatePushdown predicatePushdown;
  ProjectionPruning projectionPruning;

  QueryOptimizer optimizer;
  optimizer.addRule(&predicatePushdown);
  optimizer.addRule(&projectionPruning);
  optimizer.optimize();

  return 0;
}
/* Output:
----- Original Query Plan -----
Scan
Filter
Scan
Join
Aggregate

----Optimizing Query Plan----
Applying Predicate Pushdown

Applying Projection Pruning

*/
