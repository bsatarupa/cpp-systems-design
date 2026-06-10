/*
Given a dictionary of root words and a sentence, replace each word with the
shortest dictionary root that is its prefix.

Example:
dictionary = ["cat", "bat", "rat"]
sentence = "the cattle was rattled by the battery"

Output:
"the cat was rat by the bat"

The key idea is: stop at the first isEnd node.
*/
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

#define ALPHABET_SIZE 26

class TrieNode {
public:
  bool isLeaf;
  TrieNode *children[ALPHABET_SIZE];

  TrieNode() : isLeaf(false) {
    for (int i = 0; i < ALPHABET_SIZE; i++)
      children[i] = nullptr;
  }
};

class Trie {
  TrieNode *root;

public:
  Trie() { root = new TrieNode(); }

  string findRoot(const string &word) {

    TrieNode *p = root;
    string result;

    for (char ch : word) {
      int idx = ch - 'a';

      if (!p->children[idx]) // no shorter prefix found in this path, return
                             // original word
        break;

      p = p->children[idx];

      result += ch;
      if (p->isLeaf)
        return result; // shorter prefix found, return prefix
    }
    return word; // no shorter prefix found in ALL paths, return original word
  }

  void insert(string word) {
    TrieNode *p = root;

    for (char ch : word) {
      int idx = ch - 'a';

      if (!p->children[idx])
        p->children[idx] = new TrieNode();

      p = p->children[idx];
    }
    p->isLeaf = true;
  }

  string replaceWords(vector<string> &dictionary, string sentence) {

    for (const string &word : dictionary)
      insert(word);

    stringstream ss(sentence);
    string word;
    string result;

    while (ss >> word) {

      if (!result.empty())
        result += " ";

      result += findRoot(word);
    }
    return result;
  }
};

int main() {

  vector<string> dictionary = {"cat", "bat", "rat"};
  string sentence = "the cattle was rattled by the battery";

  Trie trie;
  cout << trie.replaceWords(dictionary, sentence) << endl;

  return 0;
}
/*
the cat was rat by the bat
*/
