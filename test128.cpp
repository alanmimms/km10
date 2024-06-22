#include <stdio.h>
#include <string>
#include <cstdint>
#include <sstream>
#include <iostream>
#include <tuple>

using namespace std;

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


// Format a 128-bit number as whatever base <= 10.
static string fmt128(uint128_t v128, int base=10) {
  if (v128 == 0) return "0";

  string s{};
  s.reserve(40);

  do {
    s += '0' + (v128 % base);
    v128 /= base;
  } while (v128 != 0);

  return string(s.rbegin(), s.rend());
}


// Unsigned Divide x||y by z. See Hacker's Delight Second Edition,
// Chapter 9, Figure 9-2.
static tuple<uint128_t,uint128_t> divlllu(uint128_t x, uint128_t y, uint128_t z) {
  static const int nBits = 128;
  static const uint128_t allOnes128 = ((uint128_t) ~0ull << 64) | ~0ull;

  for (int i=1; i <= nBits; ++i) {
    uint128_t t = (x >> (nBits - 1)) ? allOnes128 : 0;
    cerr << "iteration " << i << endl
	 << "  t=" << fmt128(t, 8) << endl
	 << "  x=" << fmt128(x, 8) << endl
	 << "  y=" << fmt128(y, 8) << endl;

    x = (x << 1) | (y >> (nBits-1));	       // Shift x||y left
    y <<= 1;				       // one bit.

    if ((x | t) >= z) {
      cerr << "subtract z=" << fmt128(z, 8) << " from x" << endl;
      x -= z;
      ++y;
    }

    cerr << endl;
  }

  return make_tuple(y, x);
}



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

  uint128_t denHi{
    ((uint128_t) 0123456'123456ull << 44) |
    ((uint128_t) 0123456'123456ull >> 20)};
  uint128_t denLo{
    ((uint128_t) 0123456'123456ull << 108) |
    ((uint128_t) 0123456'123456ull << 72) |
    ((uint128_t) 0123456'654321ull << 36) |
    ((uint128_t) 0123456'654321ull <<  0)};
  uint128_t div{0100};

  cerr << endl << "[all octal]" << endl
       << "  denHi=" << fmt128(denHi, 8) << endl
       << "  denLo=" << fmt128(denLo, 8) << endl
       << "  div=" << fmt128(div, 8) << endl << endl;

  auto const [quo, rem] = divlllu(denHi, denLo, div);

  cerr << (denHi == 0 ? "" : fmt128(denHi, 8)) << ",," << fmt128(denLo, 8)
       << " / "
       << fmt128(div, 8)
       << endl
       << "  quo="
       << fmt128(quo)
       << endl
       << "  rem="
       << fmt128(rem)
       << endl;

  return 0;
}
