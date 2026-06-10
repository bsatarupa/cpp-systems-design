/*
Design a lightweight RPC Dispatcher. Services can register methods. Clients
invoke methods by name, and the dispatcher routes the request to the correct
service. Ignore networking and serialization.

          RPCDispatcher
          ------------------------------
          serviceRegistry
          ------------------------------
          registerService()
          dispatch()
                  |
      --------------------------
      |                        |
 UserService             OrderService
*/
#include <functional>
#include <iostream>
#include <unordered_map>
using namespace std;

class RPC_Dispatcher {

  unordered_map<string, function<void(void)>> registry;
  //<method name, function handler>
public:
  void register_service(const string &method, function<void(void)> handler) {
    registry[method] = handler;
  }

  void dispatch(const string &method) {

    auto it = registry.find(method);
    if (it == registry.end()) {
      cout << "Method not found : " << method << endl;
      return;
    }

    it->second(); // invoke function handler
  }
};

class UserService {
public:
  void login() { cout << "User Login\n"; }
  void logout() { cout << "User Logout\n"; }
};

class OrderService {
public:
  void place_order() { cout << "Order Placed\n"; }
};

int main() {

  RPC_Dispatcher dispatcher;
  UserService user;
  OrderService order;

  dispatcher.register_service("login", [&]() { user.login(); });
  dispatcher.register_service("logout", [&]() { user.logout(); });
  dispatcher.register_service("place_order", [&]() { order.place_order(); });

  dispatcher.dispatch("login");
  dispatcher.dispatch("place_order");
  dispatcher.dispatch("logout");
  dispatcher.dispatch("unknown");

  return 0;
}
/*
Output:---- TC: O(1)
User Login
Order Placed
User Logout
Method not found : unknown

Follow-up Q&A:
How to add Request Parameters?
Instead of function<void()>, use
function<string(const string&)>
or, function<Response(Request)>
Exactly how real RPC frameworks work.

2. Serialization: How are requests sent?
Client
↓
Serialize
↓
Network
↓
Deserialize
↓
Dispatcher
No need to implement.

3. Service Registry?
Instead of login, store:
UserService.login
Inventory.reserve
Payment.pay

4. Dynamic Registration?
Already supported. Just call registerService()

5. Thread Safety?
Replace unordered_map with shared_mutex/mutex
*/
