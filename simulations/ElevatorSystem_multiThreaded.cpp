#include <chrono>
#include <condition_variable>
#include <iostream>
#include <thread>

using namespace std;

class ElevatorSystem {

  struct ElevatorRequest {
    int person_id;
    int source_floor, destination_floor;
  };

  // request queue
  queue<ElevatorRequest> requests;

  // current elevator state
  int current_floor;

  mutex mtx;
  condition_variable cv;
  bool stop;
  thread elevatorThread;

public:
  ElevatorSystem() : current_floor(0), stop(false) {
    elevatorThread = thread(&ElevatorSystem::run,
                            this); // always call run() from constructor
  }

  void run() {

    while (true) {

      ElevatorRequest req;

      { // critical section
        unique_lock<mutex> lock(mtx);
        cv.wait(lock, [&]() { return stop || !requests.empty(); });

        // graceful shutDown
        if (stop && requests.empty())
          return;

        req = requests.front();
        requests.pop();
      }

      // Move to source floor to pick up person_id
      moveToFloor(req.source_floor);
      cout << "Picked Person_" << req.person_id << " from floor_"
           << req.source_floor << endl;

      // Take the person to destination floor
      moveToFloor(req.destination_floor);
      cout << "Dropped Person_" << req.person_id << " at floor_"
           << req.destination_floor << endl;
    }
  }

  void moveToFloor(int targetFloor) {

    while (current_floor != targetFloor) {

      if (targetFloor < current_floor)
        current_floor--;
      else
        current_floor++;

      cout << "Elevator is at floor_" << current_floor << endl;

      // simulation, time taken by lift to move across floors
      this_thread::sleep_for(chrono::milliseconds(500));
    }
  }

  ~ElevatorSystem() { shutDown(); }

  void shutDown() {

    { // critical section
      lock_guard<mutex> lock(mtx);
      stop = true;
    }

    cv.notify_all();

    if (elevatorThread.joinable())
      elevatorThread.join();
  }

  void requestLift(int person_id, int source_floor, int destination_floor) {

    { // critical section
      lock_guard<mutex> lock(mtx);
      requests.push({person_id, source_floor, destination_floor});

      cout << "Person_" << person_id << " requested Lift from floor_"
           << source_floor << " to floor_" << destination_floor << endl;
    }
    cv.notify_one();
  }

  void passenger(int person_id, int source_floor, int destination_floor) {
    requestLift(person_id, source_floor, destination_floor);
  }
};

int main() {

  ElevatorSystem elevator;

  // passenger threads
  vector<thread> passenger_threads;

  passenger_threads.emplace_back(&ElevatorSystem::passenger, ref(elevator), 1,
                                 0, 5);
  passenger_threads.emplace_back(&ElevatorSystem::passenger, ref(elevator), 2,
                                 3, 1);
  passenger_threads.emplace_back(&ElevatorSystem::passenger, ref(elevator), 3,
                                 2, 8);

  for (auto &t : passenger_threads)
    t.join();

  // allow elevator to finish
  this_thread::sleep_for(chrono::seconds(15));

  return 0;
}
/*
Picked Person_1 from floor_0
Elevator is at floor_1
Elevator is at floor_2
Elevator is at floor_3
Elevator is at floor_4
Elevator is at floor_5
Dropped Person_1 at floor_5
Elevator is at floor_4
Elevator is at floor_3
Picked Person_2 from floor_3
Elevator is at floor_2
Elevator is at floor_1
Dropped Person_2 at floor_1
Elevator is at floor_2
Picked Person_3 from floor_2
Elevator is at floor_3
Elevator is at floor_4
Elevator is at floor_5
Elevator is at floor_6
Elevator is at floor_7
Elevator is at floor_8
Dropped Person_3 at floor_8
*/
