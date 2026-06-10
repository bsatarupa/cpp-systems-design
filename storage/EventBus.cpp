/*
Design an event bus supporting publishers and subscribers.
Subscribers should receive events they are interested in. New event types should
be easy to introduce. Identical to notifier-MessageBroker prblem.

  Publisher
      |
 publish()
      |
  EventBus
      |
-----------------------------
|            |              |
Inventory  Analytics   Notification

Implemented an in-memory Event Bus using the Publish-Subscribe (Observer)
pattern. Publishers are decoupled from subscribers through an EventBus, allowing
multiple subscribers to listen for the same event while making it easy to
introduce new event types without modifying existing code.
*/

#include <algorithm>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

//---------------- Subscriber ----------------

class Subscriber {
public:
  virtual void onEvent(const string &eventType, const string &message) = 0;

  virtual ~Subscriber() = default;
};

//---------------- Concrete Subscribers ----------------

class InventoryService : public Subscriber {

public:
  void onEvent(const string &eventType, const string &message) override {

    cout << "[Inventory] " << eventType << " -> " << message << endl;
  }
};

class AnalyticsService : public Subscriber {

public:
  void onEvent(const string &eventType, const string &message) override {

    cout << "[Analytics] " << eventType << " -> " << message << endl;
  }
};

class NotificationService : public Subscriber {

public:
  void onEvent(const string &eventType, const string &message) override {

    cout << "[Notification] " << eventType << " -> " << message << endl;
  }
};

//---------------- EventBus ----------------

class EventBus {

  unordered_map<string, vector<Subscriber *>> topicSubscribers;

public:
  void subscribe(const string &eventType, Subscriber *subscriber) {

    topicSubscribers[eventType].push_back(subscriber);
  }

  void unsubscribe(const string &eventType, Subscriber *subscriber) {

    auto &vec = topicSubscribers[eventType];

    vec.erase(remove(vec.begin(), vec.end(), subscriber), vec.end());
  }

  void publish(const string &eventType, const string &message) {

    if (!topicSubscribers.count(eventType))
      return;

    for (auto subscriber : topicSubscribers[eventType])
      subscriber->onEvent(eventType, message);
  }
};

//---------------- Publisher ----------------

class Publisher {

  EventBus &bus;

public:
  Publisher(EventBus &b) : bus(b) {}

  void publish(const string &eventType, const string &message) {

    bus.publish(eventType, message);
  }
};

//---------------- Main ----------------

int main() {

  EventBus bus;

  InventoryService inventory;
  AnalyticsService analytics;
  NotificationService notification;

  bus.subscribe("OrderCreated", &inventory);
  bus.subscribe("OrderCreated", &analytics);
  bus.subscribe("OrderCreated", &notification);

  bus.subscribe("PaymentSuccess", &analytics);
  bus.subscribe("PaymentSuccess", &notification);

  Publisher publisher(bus);

  cout << "----- Order Created -----\n";

  publisher.publish("OrderCreated", "Order #101 created");

  cout << "\n----- Payment Success -----\n";

  publisher.publish("PaymentSuccess", "Payment received for Order #101");

  cout << "\n----- Unsubscribe Analytics -----\n";

  bus.unsubscribe("OrderCreated", &analytics);

  publisher.publish("OrderCreated", "Order #102 created");

  return 0;
}
