#include <iostream>
#include <map>
#include <string>

using namespace std;

class ConsistentHashing {

  static const int VIRTUAL_NODE = 3; // to avoid hotspots, poor load balancing
  map<size_t, string> ring; //<node_position_on_hash_ring, node>, sorted based
                            // on clockwise node position

public:
  void addNode(const string &node) {

    // for each node, add #VIRTUAL_NODE to the hash ring
    for (int i = 0; i < VIRTUAL_NODE; i++) {

      string vnode = node + "#" + to_string(i);

      size_t hash_vnode = hash<string>{}(vnode);

      ring[hash_vnode] = node;
    }
  }

  string getNode(const string &user) { // for given user, get clockwise
                                       // immediately next node on hash ring

    size_t hash_vnode = hash<string>{}(user);

    auto it = ring.lower_bound(hash_vnode);

    if (it == ring.end()) //***wrap around hash ring
      it = ring.begin();

    return it->second;
  }

  void printRing() {
    cout << "-------HASH RING-------" << endl;
    for (auto &[hash, node] : ring)
      cout << hash << " -> " << node << endl;
  }
};

int main() {

  ConsistentHashing ch;
  ch.addNode("NodeA");
  ch.addNode("NodeB");
  ch.addNode("NodeC");

  ch.printRing();

  cout << "-------LOOKUPS-------" << endl;
  cout << "User1 -> " << ch.getNode("User1") << endl;
  cout << "User2 -> " << ch.getNode("User2") << endl;
  cout << "User3 -> " << ch.getNode("User3") << endl;

  return 0;
}

/*
-------HASH RING-------
702484405489787454 -> NodeC
1414690432622519879 -> NodeA
7741461504655952506 -> NodeB
9517888151422487358 -> NodeA
13826579966888993881 -> NodeB
15143588748618626778 -> NodeC
16474198694792144539 -> NodeB
17199145800986232035 -> NodeA
18051985579642990337 -> NodeC
-------LOOKUPS-------
User1 -> NodeC
User2 -> NodeA
User3 -> NodeA
*/
