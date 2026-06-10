/*
"Implement tail -100 on a 500GB file"
For very large files, Unix tail does not read the whole file.

Backward-Scan: Seek to end.
Read blocks backwards.
Count newlines.
After N+1 newlines,
seek to that position and print.

We read the file backwards in blocks while reading from disk to 4KB buffer.
while data is already in buffer. perform char-by-char memory/buffer READ.
TC : O(bytes scanned from file end)
SC : O(BLOCK_SIZE)

Q: Why backward scan?
Need only last N lines.

Q: Why block reads?
Avoid one read() per character. Reduce syscall overhead.

Q: If file has fewer than N lines?
pos reaches 0. Print entire file.

$ clang++ my_tail.cpp && ./a.out 5 input.txt
10.0.0.1
10.0.0.3
10.0.0.2
10.0.0.9
10.0.0.9
*/
#include <algorithm>
#include <cstdio>
#include <fcntl.h>
#include <iostream>
#include <unistd.h>
using namespace std;

constexpr int BLOCK_SIZE = 4096;

void my_tail_command(const char *filename, int line_number) {

  int fd = open(filename, O_RDONLY);
  if (fd < 0) {

    perror("open");
    return;
  }

  off_t file_size = lseek(fd, 0, SEEK_END);
  off_t pos = file_size;

  int line_count = 0;
  char buffer[BLOCK_SIZE];

  while (line_count <= line_number && pos > 0) {

    ssize_t bytes_to_read = min<off_t>(BLOCK_SIZE, pos);
    pos -= bytes_to_read;

    lseek(fd, pos, SEEK_SET); // starting from file beginning, jump to pos bytes
    read(fd, buffer, bytes_to_read);

    for (int i = bytes_to_read - 1; i >= 0;
         i--) { // start reading from end of buffer

      if (buffer[i] == '\n') {
        line_count++;
        if (line_count > line_number) {

          pos += (i + 1); // starting position for printing
          goto done;
        }
      }
    }
  }
  pos = 0; // in case we don't have enough lines
done:
  lseek(fd, pos, SEEK_SET);

  ssize_t n;
  while ((n = read(fd, buffer, sizeof(buffer))) > 0)
    cout.write(buffer, n);

  close(fd);
}

int main(int argc, char *argv[]) {

  if (argc != 3) {

    cerr << "usage: tail <N> <file> \n";
    return 1;
  }

  my_tail_command(argv[2], abs(stoi(argv[1])));

  return 0;
}
