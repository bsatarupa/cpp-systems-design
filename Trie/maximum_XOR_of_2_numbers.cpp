/*
Given an integer array nums[], find the maximum value of nums[i] XOR nums[j] for
two different elements. Example: nums = [3, 10, 5, 25, 2, 8] Output = 28
Because: 5 XOR 25 = 28

Strategy: Bitwise Trie
Store every number bit-by-bit, from the most significant bit to the least
significant bit.

For each number, while searching for its best partner:
current bit = 0 → prefer 1
current bit = 1 → prefer 0
At every bit, try to take the opposite bit because that makes the XOR bit 1.

Why?
0 XOR 1 = 1
1 XOR 0 = 1
We want as many 1 bits as possible, starting from the most significant bit.
*/

#include <iostream>
#include <iterator>
#include <vector>

using namespace std;

class TrieNode {
public:
  TrieNode *children[2];

  TrieNode() { children[0] = children[1] = nullptr; }
};

class maximumXOR {
  TrieNode *root;

public:
  maximumXOR() { root = new TrieNode(); }

  void insert(int num) {
    TrieNode *p = root;

    for (int bit = 31; bit >= 0; bit--) {
      int b = (num >> bit) & 1;

      if (!p->children[b])
        p->children[b] = new TrieNode();

      p = p->children[b];
    }
  }

  int getMaxXOR(int num) {
    TrieNode *p = root;

    int ans = 0;

    for (int bit = 31; bit >= 0; bit--) {
      int b = (num >> bit) & 1;

      int opposite = b ^ 1;

      if (p->children[opposite]) {
        ans |= (1 << bit);
        p = p->children[opposite];
      } else {
        p = p->children[b];
      }
    }
    return ans;
  }

  int findMaximumXOR(vector<int> &nums) {
    for (int num : nums)
      insert(num);

    int ans = 0;
    for (int num : nums)
      ans = max(ans, getMaxXOR(num));

    return ans;
  }
};

int main() {
  maximumXOR _xor;
  vector<int> nums = {3, 10, 5, 25, 2, 8};

  cout << _xor.findMaximumXOR(nums) << endl;
  return 0;
}
