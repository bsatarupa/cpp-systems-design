/*
Design a Compression Framework supporting multiple compression algorithms such
as LZ4, Snappy, and Zstd. Clients should be able to compress and decompress
data without depending on a specific algorithm. New compression algorithms
should be easy to add.

Requirements:
- Compress data.
- Decompress data.
- Support multiple compression algorithms.
- Easy to add new compression algorithms.

Uses Strategy Pattern.

               Compression
                     ^
        --------------------------
        |           |            |
       LZ4       Snappy        Zstd

           CompressionManager

Q. Why Strategy Pattern?
Different compression algorithms implement the same interface.
CompressionManager depends only on the abstraction (Compression), not concrete
algorithms. Adding a new algorithm (e.g., Brotli) only requires creating another
class derived from Compression; no existing code changes are needed.
*/

#include <iostream>
#include <string>
using namespace std;

class Compression {
public:
  virtual string compress(const string &data) = 0;
  virtual string decompress(const string &data) = 0;
  virtual ~Compression() = default;
};

class LZ4 : public Compression {
public:
  string compress(const string &data) override {
    cout << "Compressing using LZ4\n";
    return "LZ4(" + data + ")";
  }

  string decompress(const string &data) override {
    cout << "Decompressing using LZ4\n";
    return data;
  }
};

class Snappy : public Compression {
public:
  string compress(const string &data) override {
    cout << "Compressing using Snappy\n";
    return "Snappy(" + data + ")";
  }

  string decompress(const string &data) override {
    cout << "Decompressing using Snappy\n";
    return data;
  }
};

class Zstd : public Compression {
public:
  string compress(const string &data) override {
    cout << "Compressing using Zstd\n";
    return "Zstd(" + data + ")";
  }

  string decompress(const string &data) override {
    cout << "Decompressing using Zstd\n";
    return data;
  }
};

class CompressionManager {

  Compression *algorithm;

public:
  CompressionManager(Compression *algorithm) : algorithm(algorithm) {}

  string compress(const string &data) { return algorithm->compress(data); }

  string decompress(const string &data) { return algorithm->decompress(data); }
};

int main() {

  LZ4 lz4;
  CompressionManager manager(&lz4);
  string compressed = manager.compress("Hello World");
  cout << "Compressed data : " << compressed << endl;
  cout << "Original data after decompression : "
       << manager.decompress(compressed) << endl;

  cout << "\n-----------------------------\n";

  Snappy snappy;
  CompressionManager manager2(&snappy);
  compressed = manager2.compress("Database");
  cout << "Compressed Data : " << compressed << endl;
  cout << "Original Data after decompression : "
       << manager2.decompress(compressed) << endl;

  return 0;
}

/* Output:
decompressiong using LZ4
Compressed data : LZ4(Hello World)
Original data after decompression : Decompressing using LZ4
LZ4(Hello World)

-----------------------------
Compressing using Snappy
Compressed Data : Snappy(Database)
Original Data after decompression : Decompressing using Snappy
Snappy(Database)
*/
