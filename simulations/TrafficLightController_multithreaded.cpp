/*
 Intersection:
 1. North-South road
 2. East-West road
Goal: Only 1 direction gets GREEN at a time.
*/
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

using namespace std;

enum Direction { NORTH_SOUTH, EAST_WEST };

class TrafficController {

  Direction currentGreenDirection;

  mutex mtx;
  condition_variable cv;

public:
  TrafficController() : currentGreenDirection(NORTH_SOUTH) {}

  void trafficLight() {

    // 1. switch direction
    while (true) {

      { // critical section
        lock_guard<mutex> lock(mtx);

        if (currentGreenDirection == NORTH_SOUTH) {
          currentGreenDirection = EAST_WEST;
          cout << "GREEN -> EAST_WEST" << endl;
        } else {
          cout << "GREEN -> NORTH_SOUTH" << endl;
          currentGreenDirection = NORTH_SOUTH;
        }
      }

      // 2. wake awaiting cars
      cv.notify_all();

      // 3. keep signal GREEN for 3 seconds
      this_thread::sleep_for(chrono::seconds(3));
    }
  }

  void car_at_crossing(Direction dir, int car_thread_id) {

    // wait until signal turns GREEN
    { // critical section
      unique_lock<mutex> lock(mtx);
      cv.wait(lock, [&]() { return dir == currentGreenDirection; });

      if (dir == NORTH_SOUTH)
        cout << "Car_" << car_thread_id << " crossing NORTH_SOUTH" << endl;
      else
        cout << "Car_" << car_thread_id << " crossing EAST_WEST" << endl;
    }
    // simulate crossing
    this_thread::sleep_for(chrono::milliseconds(500));
  }
};

int main() {

  TrafficController tc;

  // start traffic signal Controller
  thread signalThread(&TrafficController::trafficLight, &tc);

  // car threads
  vector<thread> car_threads;
  for (int car_id = 1; car_id <= 4; car_id++)
    car_threads.emplace_back(
        &TrafficController::car_at_crossing, ref(tc),
        static_cast<Direction>(car_id % 2),
        car_id); // IMP: static_cast remainder explicitly to Direction

  for (auto &t : car_threads)
    t.join();

  // detach Infinite traffic signal Controller
  signalThread.detach();

  return 0;
}
/*
GREEN -> EAST_WEST
Car_1 crossing EAST_WEST
Car_3 crossing EAST_WEST
GREEN -> NORTH_SOUTH
Car_2 crossing NORTH_SOUTH
Car_4 crossing NORTH_SOUTH
*/
