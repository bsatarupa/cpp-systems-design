/*
Autocomplete Implementation:--------
Trie node: stores only Top-K sentence IDs for that prefix.
Separate store: ID → sentence + frequency/score.
Insert: traverse the sentence's path and update each node's Top-K list.
Query: traverse to the prefix node and return its Top-K IDs.
Ranking: higher frequency first; lexicographically smaller on ties.
Main benefit: avoids storing all matching sentences at every prefix.

Core idea to remember:

Prefix → Top-K IDs → separate sentence store
*/

#include <algorithm>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

class TrieNode {
public:
  unordered_map<char, TrieNode *> children;
  vector<int> topK; // topK sentence IDs
};

class AutocompleteSystem {

  static const int K = 3;

  struct Sentence {
    string text;
    int frequency;
  };
  vector<Sentence> sentences;

  TrieNode *root;

  bool better(int a, int b) {

    if (sentences[a].frequency != sentences[b].frequency)
      return sentences[a].frequency > sentences[b].frequency;

    else
      return sentences[a].text < sentences[b].text;
  }

  void update(TrieNode *node, int id) {
    node->topK.push_back(id);

    sort(node->topK.begin(), node->topK.end(),
         [&](int a, int b) { return better(a, b); });

    if (node->topK.size() > K)
      node->topK.pop_back();
  }

  void insert(int id) {
    TrieNode *p = root;

    for (char ch : sentences[id].text) {
      if (p->children.count(ch) == 0)
        p->children[ch] = new TrieNode();

      p = p->children[ch];

      update(p, id);
    }
  }

public:
  AutocompleteSystem(vector<string> &s, vector<int> &f) {

    root = new TrieNode();

    for (int i = 0; i < s.size(); i++)
      sentences.push_back({s[i], f[i]});

    for (int i = 0; i < sentences.size(); i++)
      insert(i);
  }

  vector<string> input(string prefix) {
    TrieNode *p = root;

    for (char ch : prefix) {
      if (!p->children.count(ch))
        return {};

      p = p->children[ch];
    }

    vector<string> result;

    for (int id : p->topK)
      result.push_back(sentences[id].text);

    return result;
  }
};

int main() {
  vector<string> sentences = {"i love you", "island", "ironman",
                              "i love leetcode"};

  vector<int> times = {5, 3, 2, 2};

  AutocompleteSystem system(sentences, times);

  vector<string> result = system.input("i");

  for (const string &s : result)
    cout << s << endl;

  return 0;
}
/*
i love you
island
i love leetcode
*/
