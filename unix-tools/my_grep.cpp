#include <fstream>
#include <iostream>
#include <string>
using namespace std;

int grep_file_pattern(const string &filename, const string &pattern) {

  ifstream file(filename);
  if (!file) {
    cerr << "Failed to open file : " << filename << '\n';
    return -1;
  }

  string line;
  while (getline(file, line)) {

    if (line.find(pattern) != string::npos) {
      cout << line << '\n';
    }
  }
  return 0;
}

int main(int argc, char *argv[]) {

  if (argc != 3) {
    cerr << "usage : grep <pattern> <filename> \n";
    // endl explicitly flushes after adding newline, so keep the newline only
    return 1;
  }

  return grep_file_pattern(argv[2], argv[1]);
}

/*
Q. How it works?
Open file
↓
Read one line at a time
↓
Check if pattern exists
↓
Print matching line

Works Even for: 100 GB / 500 GB / 1 TB. memory usage: O(max_line_length)
because: getline(file, line) - reads one line at a time. No need to load the
entire file.
*/
