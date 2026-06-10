/*
 $ clang++ create_large_log_file.cpp -o huge && ./huge
 Sample output:
1 bcdefghijklmnopqrstu
2 cdefghijklmnopqrstuv
3 defghijklmnopqrstuvw
4 efghijklmnopqrstuvwx
5 fghijklmnopqrstuvwxy
6 ghijklmnopqrstuvwxyz
7 hijklmnopqrstuvwxyza
...
*/
#include <cstdio>
using namespace std;

int main() {

  FILE *fp = fopen("huge.txt", "w");

  for (long i = 0; i < 10000000; i++) {

    fprintf(fp, "%ld ", i);

    for (int j = 0; j < 20; j++) {
      fputc('a' + (i + j) % 26, fp);
    }

    fputc('\n', fp);
  }

  fclose(fp);
  return 0;
}
