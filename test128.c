#include <stdio.h>
#include <stdint.h>


int main(int argc, char *argv[]) {
  unsigned __int128 z = 1234ull;

  printf("sizeof unsigned __int128=%lu\n", sizeof(z));

  __int128 n = -1ll << 35;
  __int128 prod = n * n;

  printf("(-1 << 35) * (-1 << 35): hi=%12Lo lo=%12Lo\n",
	 (uint64_t) (prod >> 36) & ((1ull << 36) - 1),
	 (uint64_t) prod & ((1ull << 36) - 1));
  return 0;
}
