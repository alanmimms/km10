#include <stdio.h>
#include <stdint.h>


int main(int argc, char *argv[]) {
  unsigned __int128 z = 1234ull;

  printf("sizeof unsigned __int128=%lu\n", sizeof(z));
  return 0;
}
