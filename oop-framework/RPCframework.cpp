/* RPC Framework:
Design a lightweight RPC framework where clients invoke methods exposed by
services. Requests and responses should be serialized and routed correctly. New
services should be registerable dynamically.

Requirements:
Clients invoke remote methods.
Requests are serialized.
Requests are routed to the correct service.
Responses are serialized back.
Services can be registered dynamically.
Easy to add new services.

Uses Registry Pattern.

Client
   |
RPCFramework
   |
ServiceRegistry
   |
Service
*/

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

struct Request {
  string serviceName;
  string methodName;
  vector<string> params; // method arguments
};

class Service {
public:
  virtual string invoke(const Request &req) = 0;
  virtual ~Service() = default;
};

class CalculatorService : public Service {
public:
  string invoke(const Request &req) override {

    int a = stoi(req.params[0]);
    int b = stoi(req.params[1]);

    if (req.methodName == "add")
      return to_string(a + b);

    if (req.methodName == "multiply")
      return to_string(a * b);

    return "Unknown Method!";
  }
};

class UserService : public Service {
public:
  string invoke(const Request &req) override {
    if (req.methodName == "getUser")
      return "xyz";
    return "";
  }
};

class RPCFramework {

  unordered_map<string, Service *> services;

public:
  void registerService(const string &service_name, Service *service) {
    services[service_name] = service;
  }

  string invoke(Request &req) {

    if (services.count(req.serviceName) == 0)
      return "Service Not Found!";

    cout << "Serializing Service Request\n";
    Service *curr_service = services[req.serviceName];
    string result = curr_service->invoke(req);

    cout << "Serializing Service Response\n";
    return result;
  }
};

int main() {

  RPCFramework rpc;
  rpc.registerService("Calculator", new CalculatorService());

  Request req = {"Calculator", "add", {"10", "20"}};
  // reuests and responses should be serialized
  cout << rpc.invoke(req) << '\n';

  return 0;
}
/*
Output:
Serializing Service Request
Serializing Service Response
30

The requirements were:
✅ invoke methods
✅ serialize request/response (simulated with comments/prints)
✅ route to correct service (unordered_map)
✅ dynamic registration (registerService())

For simplicity, I've simulated serialization. In production, I'd replace it with
Protocol Buffers or JSON over TCP/HTTP. The registry can also be extended to
support multiple instances of a service and load balancing.

Future extension:
Network transport: Replace the direct invoke() call with TCP/HTTP/gRPC
communication. Serialization: Use Protocol Buffers, FlatBuffers, or JSON instead
of the simple |-delimited format. Service discovery: Support distributed
registration using ZooKeeper, etcd, or Consul. Thread pool: Process multiple RPC
requests concurrently. Timeouts & retries: Add client-side timeouts and retry
policies. Authentication: Validate client identity before dispatching requests.
Load balancing: Route requests to one of multiple instances of the same service.
*/
