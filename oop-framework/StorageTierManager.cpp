/*
Design a Storage Tier Manager that stores files across multiple storage tiers
such as SSD, HDD, and Cloud. Different storage tiers should provide a common
interface for storing and reading data. New storage tiers should be easy to add
without modifying existing code.

Requirements:
- Store a file.
- Read a file.
- Support multiple storage tiers.
- Easy to add new storage tiers.

Uses Strategy Pattern.

                 StorageTier
                      ^
        ----------------------------
        |            |             |
       SSD          HDD         Cloud

              StorageManager
*/
#include <iostream>
#include <string>
using namespace std;

class StorageTier {
public:
  virtual void write(const string &file) = 0;
  virtual void read(const string &file) = 0;
  virtual ~StorageTier() = default;
};

class SSD : public StorageTier {
public:
  void write(const string &file) override {
    cout << "Writing " << file << " to SSD\n";
  }
  void read(const string &file) override {
    cout << "Reading " << file << " from SSD\n";
  }
};

class HDD : public StorageTier {
public:
  void write(const string &file) override {
    cout << "Writing " << file << " to HDD\n";
  }
  void read(const string &file) override {
    cout << "Reading " << file << " from HDD\n";
  }
};

class Cloud : public StorageTier {
public:
  void write(const string &file) override {
    cout << "Writing " << file << " to Cloud\n";
  }
  void read(const string &file) override {
    cout << "Reading " << file << " from Cloud\n";
  }
};

class StorageManager {

  StorageTier *tier;

public:
  StorageManager(StorageTier *tier) : tier(tier) {}

  void store(const string &file) { tier->write(file); }
  void retrieve(const string &file) { tier->read(file); }
};

int main() {

  SSD ssd;
  HDD hdd;
  Cloud cloud;

  cout << "-------- SSD --------\n";
  StorageManager manager1(&ssd);
  manager1.store("photo.jpg");
  manager1.retrieve("photo.jpg");

  cout << "--------HDD------------\n";
  StorageManager manager2(&hdd);
  manager2.store("movie.mp4");
  manager2.retrieve("movie.mp4");

  cout << "-------- Cloud --------\n";
  StorageManager manager3(&cloud);
  manager3.store("backup.zip");
  manager3.retrieve("backup.zip");

  return 0;
}
/*
Output:
-------- SSD --------
Writing photo.jpg to SSD
Reading photo.jpg from SSD
--------HDD------------
Writing movie.mp4 to HDD
Reading movie.mp4 from HDD
-------- Cloud --------
Writing backup.zip to Cloud
Reading backup.zip from Cloud
*/
