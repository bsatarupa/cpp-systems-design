#include <cstddef>
#include <cstdlib>
#include <iostream>

using namespace std;

class Node {
public:
  int val;
  Node *right, *down;

  Node(int v, Node *r, Node *d) : val(v), right(r), down(d) {}
};

class SkipList {
  Node *head;

public:
  SkipList() { head = new Node(0, NULL, NULL); }

  bool serach(int target) {

    Node *curr = head;

    while (curr) {

      while (curr && curr->right && curr->right->val < target)
        curr = curr->right;

      if (curr->right && curr->right->val == target) // value found
        return true;
      else // if (!curr->right || curr->right->val > target)
        curr = curr->down;
    }
    return false;
  }

  void insert(int target) {

    vector<Node *> insertPoints;
    // store 1 immediate predecessor node from each level while traversing from
    // top to bottom
    Node *curr = head;

    while (curr) {
      while (curr && curr->right && curr->right->val < target) {
        curr = curr->right;
      }

      insertPoints.push_back(curr);
      curr = curr->down;
    }

    // insert node from bottom to top at levels randomly selected
    bool insert_upwards = true;
    Node *downNode = NULL, *leftNode = NULL;

    while (insertPoints.size() > 0 && insert_upwards) {

      leftNode = insertPoints.back();
      insertPoints.pop_back();
      Node *temp = new Node(target, leftNode->right, downNode);
      leftNode->right = temp;

      // for next iteration
      downNode = temp;

      insert_upwards = (rand() & 1) == 0;
    }

    if (insert_upwards) { // for we need to add an extra level at top

      Node *temp = new Node(target, NULL, downNode);
      head = new Node(0, temp, head);
      // get a head node on top of previous head node, and left of target node
    }
  }

  bool erase(int target) {

    Node *curr = head;
    bool found = false;

    while (curr) {
      while (curr && curr->right && curr->right->val < target)
        curr = curr->right;

      if (curr && curr->right && curr->right->val == target) {
        found = true;

        Node *temp = curr->right;
        curr->right = curr->right->right;
        delete temp;

        curr =
            curr->down; //****to remove the same node from all levels downwards
      } else {          //! curr->right || curr->right->val > target
        curr = curr->down;
      }
    }
    return found;
  }
};

int main() {

  SkipList skpl;
  skpl.insert(10);
  skpl.insert(15);

  cout << "Serach 15 : " << skpl.serach(15) << endl;
  cout << "Serach 25 : " << skpl.serach(25) << endl;

  return 0;
}
