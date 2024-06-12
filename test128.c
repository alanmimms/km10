#include <stdio.h>
#include <stdint.h>


typedef unsigned __int128 uint128_t;
typedef signed __int128 int128_t;

// This is used in km10.hpp as well
#define ATTRPACKED    __attribute__((packed))


static union {
  int128_t s: 72;

  uint128_t u: 72;

  struct ATTRPACKED {
    uint64_t lo: 36;
    uint64_t hi: 36;
  };
} x;



int main(int argc, char *argv[]) {
  unsigned __int128 z = 1234ull;

  printf("sizeof unsigned __int128=%lu\n", sizeof(z));

  __int128 n = -1ll << 35;
  __int128 prod = n * n;

  printf("(-1 << 35) * (-1 << 35): hi=%12Lo lo=%12Lo\n",
	 (uint64_t) (prod >> 36) & ((1ull << 36) - 1),
	 (uint64_t) prod & ((1ull << 36) - 1));

  x.lo = 012345012345ull;
  x.hi = 0;

  printf("x.lo << 6 = %12lo\n", x.lo << 6);
  return 0;
}
