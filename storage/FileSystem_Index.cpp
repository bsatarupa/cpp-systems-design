#include <cstddef>
#include <iostream>
#include <mutex>
#include <shared_mutex>
#include <sstream>
#include <unordered_map>
#include <vector>

using namespace std;

struct Metadata {

  long inode_number;
  size_t file_size;
};

class FileNode { // can be File or directory, similar to Trie implementation

public:
  bool is_file;
  Metadata metadata;

  unordered_map<string, FileNode *> children;
  // map of <children name, children node> under each parent FileNode.
  // Each node corresponds to one path component(FileNode *) instead of 1
  // character.

  mutable shared_mutex rwlock; // each node acquiring a mutex

  FileNode(bool is_file = false, long inode_number = 0, size_t file_size = 0) {

    this->is_file = is_file;
    metadata.inode_number = inode_number;
    metadata.file_size = file_size;
  }
};

class FileSystemIndex {

  FileNode *root;
  long next_inode = 1;

  vector<string> tokenize(const string &path) {

    vector<string> tokens;
    string token;
    stringstream ss(path);

    while (getline(ss, token, '/')) {

      if (!token.empty())
        tokens.push_back(token);
    }
    return tokens;
  }

  // recursively delete all nodes in subtree rooted at node_to_delete
  void free_subtree(FileNode *node_to_delete) {

    if (!node_to_delete)
      return;

    for (auto &[name, node] : node_to_delete->children)
      free_subtree(node);

    delete node_to_delete; // recursive deletion DFS
  }

public:
  FileSystemIndex() { root = new FileNode(); }

  ~FileSystemIndex() { free_subtree(root); }

  // mkdir a/b/c
  void mkdir(const string &path) {

    FileNode *curr = root;

    vector<string> tokens = tokenize(path);

    for (auto &dir : tokens) {

      unique_lock<shared_mutex> writeLock(curr->rwlock); // per-node mutex

      if (curr->children.count(dir) == 0)
        curr->children[dir] = new FileNode();

      curr = curr->children[dir];
    }
  }

  // create file /a/b/file.txt
  void createFile(const string &path, size_t file_size) {

    FileNode *curr = root;

    vector<string> tokens = tokenize(path);

    for (int i = 0; i < tokens.size(); i++) {

      unique_lock<shared_mutex> writeLock(curr->rwlock);

      if (curr->children.count(tokens[i]) == 0) {

        bool is_file = (i == tokens.size() - 1);

        curr->children[tokens[i]] = new FileNode(
            is_file, is_file ? next_inode++ : 0, is_file ? file_size : 0);
      }

      curr = curr->children[tokens[i]];
    }
  }

  bool exists(const string &path) {

    FileNode *curr = root;

    vector<string> tokens = tokenize(path);

    for (auto &name : tokens) {

      shared_lock<shared_mutex> readLock(curr->rwlock); // per-node mutex

      if (curr->children.count(name) == 0)
        return false;

      curr = curr->children[name];
    }
    return true;
  }

  Metadata getMetadata(const string &path) {

    FileNode *curr = root;

    vector<string> tokens = tokenize(path);

    for (auto &name : tokens) {

      shared_lock<shared_mutex> readLock(curr->rwlock); // per-node mutex

      if (curr->children.count(name) == 0)
        return {-1, 0}; // metadata for non-existing file

      curr = curr->children[name];
    }

    // now we have reached end of path
    return curr->metadata;
  }

  vector<string> ls(const string &path) {

    FileNode *curr = root;

    vector<string> tokens = tokenize(path);

    for (auto &dir : tokens) {

      shared_lock<shared_mutex> readLock(curr->rwlock); // per-node mutex

      if (curr->children.count(dir) == 0)
        return vector<string>{}; // empty string vector

      curr = curr->children[dir];
    }

    // now we have reached end of path
    if (curr->is_file)
      return {tokens.back()}; // filename itself, since it has no children

    vector<string> result;

    shared_lock<shared_mutex> readLock(curr->rwlock);

    for (auto &[name, node] : curr->children)
      result.push_back(name);

    return result;
  }

  void deletePath(const string &path) {

    FileNode *curr = root;

    vector<string> tokens = tokenize(path);
    if (tokens.empty())
      return;

    for (int i = 0; i < tokens.size() - 1; i++) {

      unique_lock<shared_mutex> writeLock(curr->rwlock);

      if (curr->children.count(tokens[i]) == 0)
        return;

      curr = curr->children[tokens[i]];
    }

    // now we have reached end of path
    unique_lock<shared_mutex> writeLock(curr->rwlock);

    // curr->children.erase(tokens.back());
    /*Ignore below code if production-level recursive subtree
     deletion(post-order traversal) not required and instead, uncomment above
     line*/

    auto it =
        curr->children.find(tokens.back()); // let's start from right-most token
    if (it == curr->children.end())
      return;

    FileNode *node_to_delete = it->second;
    curr->children.erase(it); // delete from children map <name->node mapping>
    // This avoids dangling pointers inside the map.

    // recursively delete entire subtree
    free_subtree(node_to_delete);
  }
};

int main() {

  FileSystemIndex fs;
  fs.mkdir("/a/b");

  fs.createFile("/a/b/file1.txt", 1024);
  fs.createFile("/a/b/file2.txt", 2048);

  cout << fs.exists("/a/b/file1.txt") << endl;

  auto files = fs.ls("/a/b");
  for (auto &x : files)
    cout << x << " ";
  cout << endl;

  Metadata meta = fs.getMetadata("/a/b/file1.txt");

  cout << "\nMetadata for file1.txt\n";
  cout << "inode = " << meta.inode_number << endl;
  cout << "size  = " << meta.file_size << endl;

  fs.deletePath("/a/b/file2.txt");

  cout << fs.exists("/a/b/file2.txt") << endl;

  return 0;
}

/* Multi-threaded hierarchical File System Index:
Implemented a hierarchical file system metadata index as a Trie over path
components. Each node stores file metadata (inode number and file size) and
maintains child pointers <child_name, child_node> mapping in a hashmap. Supports
namespace operations such as create, lookup, list, metadata retrieval, and
deletion with O(depth) traversal. Thread safety is added using reader-writer
locks, and subtree deletion is implemented using recursive DFS.

***Q.For simplicity I use per-node locking.
Production systems would use lock coupling.

shared_lock<shared_mutex> lock(curr->rwlock);
curr = curr->children[name];

This releases parent lock before child lock is acquired.
Strictly speaking, another thread could delete the child.
Production systems use hand-over-hand locking:

lock(parent)
lookup child
lock(child)
unlock(parent)
move to child

Components:
FileNode
 ├── bool isFile
 ├── unordered_map<string, FileNode*> entries
 └── shared_mutex rwlock

FileSystemIndex
 ├── root
 ├── tokenize()
 ├── mkdir()
 ├── createFile()
 ├── exists()
 ├── ls()
 ├── deletePath()
 └── freeSubtree()
Key Ideas
Trie over path components, not characters.
unordered_map stores immediate directory entries.
shared_mutex enables multiple readers and a single writer.
Recursive post-order DFS for subtree deletion.
Path traversal complexity: O(depth).

Output:
1
file2.txt file1.txt

Metadata for file1.txt
inode = 1
size  = 1024
0
*/
