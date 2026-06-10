/*
 Design a UI framework consisting of containers and widgets like buttons, text
boxes, and images. Containers can contain other components recursively.
Rendering a parent should render all its children.
              Component
             /         \
          Leaf       Container
         /    \        /     \
      Text   Button Fragment Activity

I modeled the UI hierarchy using the Composite pattern. Container represents
composite nodes and manages child components, while Text and Button are leaves.
Fragment is a composite and can recursively contain any component. Activity is
the root container and is implemented as a Singleton to enforce exactly one
activity instance. Common rendering logic is pushed up into Component and
Container to maximize code reuse and minimize duplication.
|-A1
  |-T1
  |-B1
  |-F1
    |-T2
    |-B2
    |-F2
      |-T3
      |-F3
*/

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace std;

class Component {
  string id;

public:
  Component(string id) : id(id) {}

  virtual void show(int indent = 0) {
    cout << string(indent, ' ') << "|-" << id << '\n';
  }

  virtual void add(Component *child) {
    throw runtime_error("cannot add Child to a leaf");
  }

  virtual ~Component() = default;
};

class Leaf : public Component {
public:
  Leaf(string id) : Component(id) {}
};

class Text : public Leaf {
public:
  Text(string id) : Leaf(id) {}
};

class Button : public Leaf {
public:
  Button(string id) : Leaf(id) {}
};

class Container : public Component {

  vector<Component *> children;

public:
  Container(string id) : Component(id) {}

  void add(Component *child) override { children.push_back(child); }

  void show(int indent = 0) override {

    Component::show(indent); // superclass VVIMP

    for (auto child : children)
      child->show(indent + 2);
  }
};

class Fragment : public Container {
public:
  Fragment(string id) : Container(id) {}
};

class Activity : public Container {
private:
  Activity(string id)
      : Container(id) {} // private constructor for singleton object
public:
  static Activity &getInstance(string id) { // object scope is program lifetime
    static Activity activity(id);
    return activity;
  }
};

int main() {

  Activity &activity = Activity::getInstance("A1"); // Singleton object creation
  Text text1("T1");
  Button button1("B1");
  Fragment fragment1("F1");

  Text text2("T2");
  Button button2("B2");
  Fragment fragment2("F2");

  Text text3("T3");
  Fragment fragment3("F3");

  activity.add(&text1);
  activity.add(&button1);

  fragment1.add(&text2);
  fragment1.add(&button2);

  fragment2.add(&text3);
  fragment2.add(&fragment3);

  fragment1.add(&fragment2);
  activity.add(&fragment1);

  activity.show();

  return 0;
}
