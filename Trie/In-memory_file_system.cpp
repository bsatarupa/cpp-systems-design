/*
Design an in-memory file system supporting:
ls(path)
mkdir(path)
addContentToFile(filePath, content)
readContentFromFile(filePath)

FileSystem
     │
     ▼
   Tree
     │
     ├── name
     ├── isFile
     ├── content
     └── children

For children, we use map<string, TrieNode *> keeps ls() lexicographically
sorted.
*/
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

class TrieNode {
public:
  string name;
  bool isFile;
  string content;

  map<string, TrieNode *> children;

  TrieNode(string name = "", bool isFile = false)
      : name(name), isFile(isFile) {}
};

class FileSystem {
  TrieNode *root;

  vector<string> split_filepath(const string &path) {

    stringstream ss(path);
    string part;

    vector<string> parts;
    while (getline(ss, part, '/')) {
      if (!part.empty())
        parts.push_back(part);
    }

    return parts;
  }

  TrieNode *traverse_filepath(const string &path) {
    TrieNode *p = root;

    vector<string> parts = split_filepath(path);
    for (const string &part : parts) {

      if (p->children.count(part) == 0)
        return nullptr;

      p = p->children[part];
    }
    return p;
  }

public:
  FileSystem() { root = new TrieNode("/"); }

  vector<string> ls(string path) {
    TrieNode *node = traverse_filepath(path);

    if (node->isFile)
      return {node->name};

    vector<string> result;

    for (auto &[child_name, child_node] : node->children)
      result.push_back(child_name);

    return result;
  }

  string readContentFromFile(string path) {
    TrieNode *node = traverse_filepath(path);
    return node->content;
  }

  void mkdir(string path) {
    TrieNode *p = root;

    vector<string> parts = split_filepath(path);
    for (string &part : parts) {
      if (p->children.count(part) == 0)
        p->children[part] = new TrieNode();

      p = p->children[part];
    }
  }

  void addContentToFile(string path, string content) {
    TrieNode *p = root;

    vector<string> parts = split_filepath(path);
    for (int i = 0; i < parts.size(); i++) {
      const string &part = parts[i];

      if (!p->children.count(part))
        p->children[part] = new TrieNode(part, i == parts.size() - 1);

      p = p->children[part];
    }

    p->isFile = true;
    p->content += content;
  }
};

int main() {
  FileSystem fs;

  fs.mkdir("/a/b/c");

  fs.addContentToFile("/a/b/c/d", "hello");

  cout << fs.readContentFromFile("/a/b/c/d") << endl;

  for (const string &x : fs.ls("/a/b/c"))
    cout << x << " ";

  cout << endl;

  fs.addContentToFile("/a/b/c/d", " world");

  cout << fs.readContentFromFile("/a/b/c/d") << endl;

  return 0;
}
/*
hello
d
hello world
*/
