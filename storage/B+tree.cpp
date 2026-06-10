#include <cstddef>
#include <iostream>
#include <vector>

using namespace std;

class BplusTree {

  class Node {
  public:
    bool is_leaf;
    Node *next; // leaf linkage

    vector<int> keys;
    vector<Node *> children;

    Node(bool leaf) : is_leaf(leaf), next(nullptr) {}
  };

  Node *root;

  int MAX_KEYS_PER_NODE;

public:
  BplusTree(int max_keys) : MAX_KEYS_PER_NODE(max_keys) {

    root = new Node(true); // create the first node from constructor
  }

  Node *findLeaf(int key) {

    Node *curr = root;

    while (curr->is_leaf == false) {

      int i;
      for (i = 0; i < curr->keys.size() && curr->keys[i] <= key; i++)
        ;

      // now curr->keys[i] > key
      curr = curr->children[i];
    }

    return curr;
  }

  bool searchKey(int key) {

    Node *leaf = findLeaf(key);

    for (int &k : leaf->keys)
      if (k == key)
        return true;

    return false;
  }

  void insertKey(int key) {

    Node *leaf = findLeaf(key);

    // insert key at proper position in leaf level
    auto it = leaf->keys.begin();
    for (; it != leaf->keys.end() && (*it) < key; it++)
      ;

    leaf->keys.insert(it, key);

    // split leaf if it overflows
    if (leaf->keys.size() > MAX_KEYS_PER_NODE)
      splitLeaf(leaf);
  }

  void splitLeaf(Node *leaf) {

    int mid = leaf->keys.size() / 2;

    // create new leaf and move half keys to it
    Node *newLeaf = new Node(true);

    newLeaf->keys.assign(leaf->keys.begin() + mid, leaf->keys.end());
    leaf->keys.erase(leaf->keys.begin() + mid, leaf->keys.end());

    newLeaf->next = leaf->next; // leaf linkage
    leaf->next = newLeaf;

    // root split case
    if (root == leaf) {

      Node *newRoot = new Node(false);
      newRoot->keys.push_back(
          newLeaf->keys[0]); // get an internal node with key same as first key
                             // of newLeaf

      newRoot->children.push_back(leaf); // children of newRoot
      newRoot->children.push_back(newLeaf);

      root = newRoot; // change root assignment
    }
  }

  void printLeaves() {

    Node *curr = root;

    // go to the leftmost leaf and print the linked list
    while (curr->is_leaf == false)
      curr = curr->children[0];

    while (curr) {

      cout << "[";
      for (int &k : curr->keys)
        cout << k << ",";
      cout << "] -> ";

      curr = curr->next;
    }
    cout << endl;
  }
};

int main() {

  BplusTree tree(3);
  vector<int> keys = {10, 20, 5, 6, 12, 30, 7, 17};

  for (int &k : keys)
    tree.insertKey(k);

  tree.printLeaves();

  cout << "Search 12 : " << tree.searchKey(12) << endl;
  cout << "Search 50 : " << tree.searchKey(50) << endl;

  return 0;
}
/*
[5,6,7,] -> [10,12,17,] -> [20,30,] ->
Search 12 : 1
Search 50 : 0
*/
