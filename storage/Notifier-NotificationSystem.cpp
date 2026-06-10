/*
 Design a notification system that can send alerts through Email, SMS, and Push
notifications. A notification may need to be sent through multiple channels
simultaneously. New notification types should be extensible.

Notification System = Strategy + Composite design <---  This one
Message Broker = Observer Pattern (publisher/subscriber)

               NotificationChannel
                     (abstract)
                    send(msg)
                  /      |      \
              Email     SMS     Push

                    NotificationService
                          |
                vector<NotificationChannel *>
*/
#include <iostream>
#include <string>
#include <thread>
#include <vector>

using namespace std;

class NotificationChannel { // pure abstract class
public:
  virtual void notify(const string &message) = 0;
  virtual ~NotificationChannel() = default;
};

class Email : public NotificationChannel {
  string email;

public:
  Email(string email) : email(email) {}

  void notify(const string &message) override {
    cout << "Email to " << email << " : " << message << endl;
  }
};

class SMS : public NotificationChannel {
  string phone;

public:
  SMS(string phone) : phone(phone) {}

  void notify(const string &message) override {
    cout << "SMS to " << phone << " : " << message << endl;
  }
};

class Push : public NotificationChannel {

public:
  void notify(const string &message) override {
    cout << "Push notification : " << message << endl;
  }
};

class NotificationService {

  vector<NotificationChannel *> channels;

public:
  void addChannel(NotificationChannel *channel) { channels.push_back(channel); }

  void sendNotification(const string &message) {

    // Simultaneous notify: launch 1 thread per notification channel
    vector<thread> threads;

    for (auto channel : channels) {

      threads.emplace_back(
          [&, channel, message]() { channel->notify(message); });
    }

    for (auto &t : threads)
      t.join();
  }
};

int main() {

  NotificationService service;

  Email email("abc@gmail.com");
  SMS sms("9999999999");
  Push push;

  service.addChannel(&email);
  service.addChannel(&sms);
  service.addChannel(&push);

  service.sendNotification("Backup Compeleted");

  return 0;
}
/*
 Output:
Email to Push notification : Backup Compeleted
abc@gmail.com : Backup Compeleted
SMS to 9999999999 : Backup Compeleted

Q1. Why Strategy Pattern?
Strategy Pattern → Every channel has its own sending algorithm.
Open-Closed Principle → Add new channels without modifying existing classes.
It also behaves like a Composite because the service delegates to multiple
strategies.

Each notification channel has a different implementation of send().
By introducing a NotificationChannel interface, I can add new channels like
Slack, Teams, or WhatsApp without modifying the existing code, following the
Open/Closed Principle.

2. Why not use if-else?

Instead of:

if(type=="Email")
...
else if(type=="SMS")
...

use polymorphism.

Advantages:

Open for extension
Cleaner code
Less coupling
3. How would you add Slack?

Simply create:

class Slack : public NotificationChannel {

public:
    void send(const string& msg) override {
        cout << "Slack : " << msg << endl;
    }
};

No other code changes.

4. What if Email fails?

Current code ignores failures.

Production approach:

try {
    channel->send(message);
}
catch(...) {
    log();
    retry();
}

or

Retry Queue
5. How would you retry failed notifications?

Maintain

Retry Queue

Worker:

Retry after

1 sec

2 sec

4 sec

8 sec

Exponential backoff.

6. Should Email failure stop SMS?

No.

Each channel should be independent.

Email ❌

SMS ✔

Push ✔

Failures are isolated.

7. Why multiple threads?

Email may take

400 ms

SMS

150 ms

Push

80 ms

Sequential

630 ms

Parallel

≈400 ms
8. Would you create one thread per notification?

No.

Instead:

NotificationService

↓

Thread Pool

↓

Worker Threads

Submit

threadPool.submit(...)

Creating thousands of threads is expensive.

9. What if thousands of notifications arrive?

Introduce

Producer

↓

Blocking Queue

↓

Thread Pool

↓

Workers

Workers continuously consume.

10. How would you rate-limit SMS?

Maintain

Token Bucket

or

Leaky Bucket

Reject or delay excess requests.

11. How would you prioritize notifications?

Priority Queue

HIGH

MEDIUM

LOW

Workers always pick highest priority first.

12. How would you schedule notifications?

Instead of

sendNow()

store

executeAt

Use the delayed scheduler you already implemented.

13. How would you support "send after 5 minutes"?

Exactly your scheduler.

schedule(
    []{
        email.send(...);
    },
    5min
);
14. How would you send notifications asynchronously?

Instead of

send()

enqueue

Kafka

RabbitMQ

Blocking Queue

Workers consume later.

15. How would you guarantee delivery?

Maintain

Pending

↓

Sent

↓

Acknowledged

Retry until ACK.

16. What if Push is down?

Circuit Breaker

Failure

↓

Open Circuit

↓

Skip Push

↓

Retry after timeout
17. How would you batch notifications?

Instead of

100 Emails

Combine

Digest Email

every

5 min
18. Thread safety?

Current implementation isn't thread-safe if multiple threads call:

addChannel()

Need

mutex mtx;

Protect

channels
19. How would you avoid duplicate notifications?

Maintain

Notification ID

Use

unordered_set

Ignore duplicates.

20. How would you persist notifications?

Store

Notification Table

id

status

channel

message

retryCount
21. What design patterns are used?

Current

Strategy

Possible extensions

Factory

Create channels.

Decorator

Add logging.

Observer

Notify clients after delivery.

22. Complexity

Current

Add Channel

O(1)

Send

O(number of channels)
23. Can notifications be sent in parallel?

Yes.

for(...)
    threadPool.submit(...)
24. Why not Observer?

Observer is

Publisher

↓

Subscribers

Question asks

Notification

↓

Email

SMS

Push

No subscription.

25. If the interviewer says "Enterprise Design?"

Mention:

REST API

↓

Notification Service

↓

Kafka

↓

Worker Pool

↓

Email

SMS

Push

*/
