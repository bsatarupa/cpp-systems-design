#include <iostream>
#include <thread>

int counter = 0;

void worker() {
    for (int i = 0; i < 100000; i++) {
        counter++;
    }
}

int main() {
    std::thread t1(worker);
    std::thread t2(worker);

    t1.join();
    t2.join();

    std::cout << counter << "\n"; 
    //Without lock, causing Race Condition
    //Returns random value everytime, as 2 threads are having INTERLEAVED read-modify-write operations.
}
