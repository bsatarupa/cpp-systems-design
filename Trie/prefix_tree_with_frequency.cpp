/*
Implement a Trie with:

countWordsEqualTo(word) — count exact occurrences.
countWordsStartingWith(prefix) — count words with the prefix.

erase(word) — remove one occurrence of a word.
insert(word) — insert a word.
Key requirement: duplicate words are allowed.

Input:
["Trie", "insert", "insert", "countWordsEqualTo",
 "countWordsStartingWith", "erase",
 "countWordsEqualTo", "countWordsStartingWith",
 "erase", "countWordsStartingWith"]

[[], ["apple"], ["apple"], ["apple"], ["app"],
 ["apple"], ["apple"], ["app"], ["apple"], ["app"]]

Output:
[null, null, null, 2, 2, null, 1, 1, null, 0]
*/
#include <iostream>
#include <string>

using namespace std;

#define ALPHABET_SIZE 26

class TrieNode {
public:
  int wordCount, prefixCount;
  TrieNode *children[ALPHABET_SIZE];

  TrieNode() : wordCount(0), prefixCount(0) {
    for (int i = 0; i < ALPHABET_SIZE; i++)
      children[i] = nullptr;
  }
};

class Trie {
  TrieNode *root;

public:
  Trie() { root = new TrieNode(); }

  TrieNode *findNode(const string &word) {
    TrieNode *p = root;

    for (char ch : word) {
      int idx = ch - 'a';

      if (!p->children[idx])
        return nullptr;

      p = p->children[idx];
    }
    return p;
  }

  int countWordsEqualTo(string word) {
    TrieNode *node = findNode(word);
    return node ? node->wordCount : 0;
  }

  int countWordsStartingWith(string prefix) {
    TrieNode *node = findNode(prefix);
    return node ? node->prefixCount : 0;
  }

  void insert(string word) {
    TrieNode *p = root;

    for (char ch : word) {
      int idx = ch - 'a';

      if (!p->children[idx])
        p->children[idx] = new TrieNode();

      p = p->children[idx];

      p->prefixCount++; // words going through current node
    }
    p->wordCount++; // words ending at current node
  }

  void erase(string word) {
    TrieNode *p = root;

    for (char ch : word) {
      int idx = ch - 'a';
      p = p->children[idx];

      p->prefixCount--;
    }
    p->wordCount--;
  }
};

int main() {
  Trie trie;

  trie.insert("apple");
  trie.insert("apple");

  cout << trie.countWordsEqualTo("apple") << endl;    // 2
  cout << trie.countWordsStartingWith("app") << endl; // 2

  trie.erase("apple");

  cout << trie.countWordsEqualTo("apple") << endl;    // 1
  cout << trie.countWordsStartingWith("app") << endl; // 1

  trie.erase("apple");

  cout << trie.countWordsEqualTo("apple") << endl;    // 0
  cout << trie.countWordsStartingWith("app") << endl; // 0

  return 0;
}
/*
2
2
1
1
0
0
*/
