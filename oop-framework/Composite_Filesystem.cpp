/*
Design an in-memory file system supporting files and directories. Users should
be able to create, delete, and traverse directories and files.
The structure should resemble a hierarchical tree.

Implementation Summary:
Node is the base class containing common state and interfaces.
File is a leaf node that stores its size.
Directory is a composite node that maintains child nodes (File or Directory).
size() recursively computes the total size of the subtree.
ls() recursively traverses and prints the filesystem tree.
Common printing logic is reused in Node, while File and Directory extend it.

Pattern Used: Composite Pattern (File = Leaf, Directory = Composite).

|-/
  |-docs
    |-a.txt (100)
    |-b.txt (200)
  |-images
    |-pic.jpg (500)

Total Size = 800

Deleting b.txt
|-/
  |-docs
    |-a.txt (100)
  |-images
    |-pic.jpg (500)

Searching...
Found

Total Size = 600
 */

#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
using namespace std;

class Node {
  string name;

public:
  Node(string name) : name(name) {}

  virtual int size() const = 0;

  string getName() const { return name; }

  virtual void ls(int indent = 0) {
    cout << string(indent, ' ') << "|-" << name;
  }

  // default implementation for leaf
  virtual void add(Node *child) {
    throw runtime_error("can not add child to file!");
  }

  virtual void remove(const string &name) = 0;

  virtual Node *find(const string &name) {

    if (name == getName())
      return this;

    return nullptr;
  }

  virtual ~Node() = default;
};

class File : public Node {

  int file_size;

public:
  File(string name, int size) : Node(name), file_size(size) {}

  int size() const override { return file_size; }

  void remove(const string &name) override {}; // No-op

  void ls(int indent = 0) override {

    Node::ls(indent); // superclass function

    cout << " (" << file_size << ")\n";
  }
};

class Directory : public Node {

  vector<Node *> children;

public:
  Directory(string name) : Node(name) {}

  void add(Node *child) override { children.push_back(child); }

  int size() const override {
    int total = 0;
    for (auto child : children) {
      total += child->size();
    }
    return total;
  }

  void ls(int indent = 0) override {

    Node ::ls(indent);
    cout << '\n';

    for (auto child : children)
      child->ls(indent + 2);
  }

  Node *find(const string &name) override {

    if (getName() == name)
      return this;

    for (auto &child : children) {

      Node *result = child->find(name);
      if (result)
        return result;
    }
    return nullptr;
  }

  void remove(const string &name) override {

    // remove all occurences with the same name
    for (auto it = children.begin(); it != children.end();) {
      if ((*it)->getName() == name) {
        it = children.erase(it);
        // returns immediately next pointer, without assignment iterator becomes
        // invalid
      } else {
        // recurse (dig further) only if child is a directory, no-op for a file
        (*it)->remove(name);

        it++; // proceed to next iterator
      }
    }
  }
};

int main() {

  Directory root("/");

  Directory docs("docs");
  Directory images("images");

  File f1("a.txt", 100);
  File f2("b.txt", 200);
  File f3("pic.jpg", 500);

  docs.add(&f1);
  docs.add(&f2);

  images.add(&f3);

  root.add(&docs);
  root.add(&images);

  root.ls();

  cout << "\nTotal Size = " << root.size() << endl;

  cout << "\nDeleting b.txt\n";
  docs.remove("b.txt");

  root.ls();
  cout << "\nSearching...\n";
  if (root.find("pic.jpg"))
    cout << "Found\n";

  cout << "\nTotal Size = " << root.size() << endl;
  return 0;
}
/*
1. How do you support paths?

Instead of

find("a.txt")

support

/docs/a.txt

Split by '/' and traverse level by level.

2. Why Composite?

Both

File

and

Directory

are treated uniformly.

Example:

Node* node;
node->ls();

No need to know whether it's a file or directory.

3. How do you support cd?

Maintain

Directory* currentDirectory;

Commands

cd ..
cd docs

change this pointer.

4. How do you support pwd?

Maintain

Directory* parent;

inside every node.

Walk upwards.

5. Why store parent?

Without it,

pwd

becomes difficult.

6. How do you move files?
remove()

↓

add()
7. Copy?

Recursive DFS.

8. Delete directory?

Recursive DFS.

9. How do you compute size faster?

Current

size()

is

O(n)

Maintain cached size.

Whenever

add()

remove()

update parents.

Then

size()

becomes

O(1)
10. How do you search faster?

Current

DFS

Maintain

unordered_map<string, Node*>

Lookup

O(1)
11. Duplicate filenames?

Instead of

find("a.txt")

search by

full path
12. Thread safety?

Protect

children

with

shared_mutex

Readers

shared_lock

Writers

unique_lock
13. How do you support symbolic links?

Introduce

class Link : public Node

holding

Node* target;
14. How do you persist?

Serialize

Tree

to disk.

15. What pattern?

Composite Pattern.
*/
