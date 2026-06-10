#include <iostream>
#include <mutex>
#include <shared_mutex>
#include <sstream>
#include <unordered_map>
#include <vector>

using namespace std;

class FileNode { // can be File or directory, similar to Trie implementation
public:
  bool is_file;
  string content;

  unordered_map<string, FileNode *> children;
  // map of <children name, children node> under each parent FileNode.
  // Each node corresponds to one path component(FileNode *) instead of 1
  // character.

  mutable shared_mutex rwlock; // each node acquiring a mutex

  FileNode(bool is_file = false) : is_file(is_file) {}
};

class FileSystemIndex {

  FileNode *root;

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
  void createFile(const string &path) {

    FileNode *curr = root;

    vector<string> tokens = tokenize(path);

    for (int i = 0; i < tokens.size(); i++) {

      unique_lock<shared_mutex> writeLock(curr->rwlock);

      if (curr->children.count(tokens[i]) == 0)
        curr->children[tokens[i]] =
            new FileNode(i == tokens.size() - 1); // send True only for leaf

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

  void write(const string &path, string data) {

    FileNode *curr = root;

    vector<string> tokens = tokenize(path);

    for (auto &name : tokens) {

      shared_lock<shared_mutex> readLock(curr->rwlock); // per-node mutex

      if (curr->children.count(name) == 0)
        return;

      curr = curr->children[name];
    }
    // now we have reached end of path
    unique_lock<shared_mutex> writeLock(curr->rwlock);

    if (!curr->is_file) // return if not a file
      return;

    curr->content += data;
  }

  string read(const string &path) {

    FileNode *curr = root;

    vector<string> tokens = tokenize(path);

    for (auto &name : tokens) {

      shared_lock<shared_mutex> readLock(curr->rwlock); // per-node mutex

      if (curr->children.count(name) == 0)
        return "";

      curr = curr->children[name];
    }
    // now we have reached end of path
    shared_lock<shared_mutex> readLock(curr->rwlock);

    if (!curr->is_file)
      return "";

    return curr->content;
  }
};

int main() {

  FileSystemIndex fs;
  fs.mkdir("/a/b");

  fs.createFile("/a/b/file1.txt");
  fs.createFile("/a/b/file2.txt");

  // Write contents
  fs.write("/a/b/file1.txt", "Hello ");
  fs.write("/a/b/file1.txt", "World!");

  fs.write("/a/b/file2.txt", "Good Morning");

  // Read contents
  cout << "file1.txt : " << fs.read("/a/b/file1.txt") << endl;

  cout << "file2.txt : " << fs.read("/a/b/file2.txt") << endl;

  cout << fs.exists("/a/b/file1.txt") << endl;

  auto files = fs.ls("/a/b");
  for (auto &x : files)
    cout << x << " ";
  cout << endl;

  fs.deletePath("/a/b/file2.txt");

  cout << fs.exists("/a/b/file2.txt") << endl;

  // Read file1 again
  cout << "\nfile1.txt : " << fs.read("/a/b/file1.txt") << endl;

  return 0;
}

/* Multi-threaded hierarchical In-memory File System:
Implemented a thread-safe hierarchical file system index using a Trie over path
components. Each FileNode represents either a file or directory and maintains an
unordered_map<string, FileNode*> mapping entry names to child nodes.
Reader-writer synchronization is achieved using shared_mutex, allowing
concurrent reads (exists(), ls()) via shared_lock and exclusive writes (mkdir(),
createFile(), deletePath()) via unique_lock. Directory deletion recursively
frees the entire subtree using post-order DFS. Supports efficient path traversal
with O(depth) lookup and creation operations.

Components
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
file1.txt : Hello World!
file2.txt : Good Morning
1
file2.txt file1.txt
0

file1.txt : Hello World!
*/
