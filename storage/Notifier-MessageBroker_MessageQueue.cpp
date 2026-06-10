/*
Design a message broker supporting producers and consumers. Producers publish
messages to topics, and consumers subscribe to receive them. Multiple consumers
may consume the same topic.


Notification System <-----this is different, solved separately

Notification
      |
Email
SMS
Push

No topics. No subscribers.

Message Broker <----- this is what we are building

Producer
     |
 publish()
     |
Broker
     |
Topic / eventType
     |
Subscribers / Consumers

This is Exactly Observer Pattern.

                MessageBroker
                      |
      topic -> vector<Consumer *>
                      |
      -------------------------------
      |             |              |
  Consumer1     Consumer2     Consumer3


  Producer
     |
 publish(topic,msg)
     |
MessageBroker
     |
notify all subscribers / Consumers
*/
#include <iostream>
#include <string>
#include <vector>

using namespace std;

class Subscriber { // pure abstract class
public:
  virtual void notify(const string &message) = 0;
  virtual ~Subscriber() = default;
};

class EmailSubscriber : public Subscriber {
  string email;

public:
  EmailSubscriber(string email) : email(email) {}

  void notify(const string &message) override {
    cout << "Email to " << email << " : " << message << endl;
  }
};

class SMSSubscriber : public Subscriber {
  string phone;

public:
  SMSSubscriber(string phone) : phone(phone) {}

  void notify(const string &message) override {
    cout << "SMS to " << phone << " : " << message << endl;
  }
};

class PushSubscriber : public Subscriber {

public:
  void notify(const string &message) override {
    cout << "Push notification : " << message << endl;
  }
};

class MessageBroker {

  unordered_map<string, vector<Subscriber *>> subscribers;
  //<eventType -> list of subscribers>
public:
  void subscribe(const string &topic,
                 Subscriber *subscriber) { // eventType = Topic
    subscribers[topic].push_back(subscriber);
  }

  void unsubscribe(const string &topic, Subscriber *subscriber) {

    auto &vec = subscribers[topic];

    /*
    for (auto it = vec.begin(); it != vec.end(); it++) {
        if (*it == subscriber) {
            vec.erase(it);
            break;
        }
    }
    */
    vec.erase(remove(vec.begin(), vec.end(), subscriber), vec.end());
    // removes all occurrences of subscriber from vec
  }

  void publish(const string &topic,
               const string &message) { // eventType = Topic

    if (!subscribers.count(topic))
      return;

    for (auto subscriber : subscribers[topic]) // consumer = subscriber
      subscriber->notify(message);
  }
};

class Publisher { // Publisher = Producer

  MessageBroker &broker;

public:
  Publisher(MessageBroker &broker) : broker(broker) {}

  void publish(const string &topic,
               const string &message) { // eventType = Topic

    broker.publish(topic, message);
  }
};

int main() {

  MessageBroker broker;

  EmailSubscriber email("abc@gmail.com");
  SMSSubscriber sms("9999999999");
  PushSubscriber push;

  broker.subscribe("Orders", &email);
  broker.subscribe("Orders", &sms);
  broker.subscribe("Orders", &push);

  Publisher publisher(broker);
  publisher.publish("Orders", "Order #101 created");
  cout << endl;

  broker.unsubscribe("Orders", &sms);

  publisher.publish("Orders", "Order #102 created");

  return 0;
}
/*
 Implemented a Notification Service using the Observer Pattern. The service
maintains a mapping from event types to subscribers and supports subscribe,
unsubscribe, and publish operations. Publishers are decoupled from consumers,
 allowing multiple channels such as Email, SMS, and Push notifications to listen
to the same event. The design is extensible and can be enhanced with
asynchronous delivery using a ThreadPool and BlockingQueue.

Output:
Email to abc@gmail.com : Order #101 created
SMS to 9999999999 : Order #101 created
Push notification : Order #101 created

Email to abc@gmail.com : Order #102 created
Push notification : Order #102 created

Components:
                 NotificationService
                           |
        -----------------------------------------
        |                   |                   |
    BackupSuccess      SnapshotDone       UploadFailure
        |
    ----------------------------
    |             |            |
 EmailSubscriber SMSSubscriber PushSubscriber

Q1. Why Observer Pattern?

Decouples producers from consumers.
Publisher
    ↓
NotificationService
    ↓
Email
SMS
Push

Adding:
class SlackSubscriber : public Subscriber {};
requires no changes elsewhere.

Q2. Async notifications?
publish()
     ↓
BlockingQueue
     ↓
ThreadPool
     ↓
worker threads
     ↓
notify()
Avoids blocking publishers.

Q3. Retry failed notifications?

Maintain:
struct NotificationTask {
    int retryCount;
};
Push back into queue on failure.

Q4. Guarantee delivery?
For Persistent notifications:
Use WAL, Queue, Kafka before publishing.

Q5.Faster unsubscribe?
unordered_map<string, unordered_set<Subscriber*>>

Then:
subscribe()
unsubscribe()
becomes O(1), as unordered_set uses hashmap under the hood,
unlike set that uses ordered Red-Black tree(balanced BST)

and
for(auto s : subscribers[event.type])
    s->notify(event);

still works.

Q6: Async notifications?

Current:

publish()
 ↓
Email
 ↓
SMS
 ↓
Push

Slow email blocks everyone.

Better:

publish()
    ↓
TaskQueue
    ↓
ThreadPool
threadPool.submit([subscriber,event]{
        subscriber->notify(event);
    });

Q7: Retry failed notifications?
try {
    subscriber->notify(event);
}
catch(...) {
    retryQueue.push(event);
}

Exponential backoff:
1s
2s
4s
8s

Q8: Want to add subscriber Priority?

Store:
struct SubscriberInfo {
    int priority;
    Subscriber* subscriber;
};
Higher priority gets notified first.

Q9: Multiple events?
Same subscriber with multiple events:
Email
    |
ORDER_CREATED
PAYMENT_SUCCESS
USER_REGISTERED

No changes needed.

Q10: Filtering?

Example:
Email receives only HIGH priority events.

Add:
bool canHandle(const Event&)
inside Subscriber.

Q11.What design pattern is this?
Observer Pattern. NotificationService is the Subject (Publisher), and
Email/SMS/Push are Observers (Subscribers). Multiple observers can subscribe to
the same event and are notified when that event is published.
*/
