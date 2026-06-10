/*
How would you read a 500GB file?
avoid character-by-character reads.
Use read() with a large buffer (64KB–1MB),reducing #expensive
system calls from O(N) to roughly O(N / BUFFER_SIZE).
process chunks sequentially, avoid character-by-character reads.

Suppose:

File = 1 GB
Buffer = 64 KB

Then:

read()
↓
Kernel copies 64KB
↓
User processes it

read()
↓
Next 64KB
↓
Process

...

Until EOF.
*/
#include <fcntl.h>
#include <iostream>
#include <unistd.h>
using namespace std;

constexpr size_t CHUNK_SIZE = 64 * 1024; // buffer size = 64 KB

int main(int argc, char *argv[]) {

  if (argc != 2) {

    cerr << "usage: ./a.out <input_file> \n";
    return -1;
  }

  int fd = open(argv[1], O_RDONLY);
  if (fd < 0) {

    perror("open");
    return -1;
  }

  char buffer[CHUNK_SIZE];

  ssize_t bytes_read;
  while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0) {

    // process_data(buffer, bytes_read);
    for (ssize_t i = 0; i < bytes_read; i++) {
      cout << buffer[i];
      // if (buffer[i] == '\n') line_count++;
    }
  }

  close(fd);

  return 0;
}
