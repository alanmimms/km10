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


union W72 {
  int128_t s: 72;

  uint128_t u: 72;

  struct ATTRPACKED {
    uint128_t lo: 35;
    unsigned loSign: 1;
    uint128_t hi: 35;
    unsigned hiSign: 1;
  };

  W72() {u = 0;}
};

//static W72 x, y;


union W140 {

  struct ATTRPACKED {
    uint128_t u3: 35;
    uint128_t u2: 35;
    uint128_t u1: 35;
    uint128_t u0: 35;
  };

  struct ATTRPACKED {
    uint128_t lo70: 70;
    uint128_t hi70: 70;
  };

  struct ATTRPACKED {
    uint128_t lo35: 35;
    uint128_t mid70: 70;
    uint128_t hi35: 35;
  };

  W140() : lo70(0), hi70(0) {}
};


struct W256 {
  uint128_t lo;
  uint128_t hi;

  W256() : lo(0), hi(0) {}
  W256(uint128_t vLo, uint128_t vHi=0) : lo(vLo), hi(vHi) {}

  // NOTE this takes arguments in MSW .. LSW order.
  W256(uint64_t v3, uint64_t v2, uint64_t v1, uint64_t v0)
    : lo(((uint128_t) v1 << 64) | v0),
      hi(((uint128_t) v3 << 64) | v2)
  {}


  W256 operator+(const W256 &a) const {
    W256 r;
    r.lo = lo + a.lo;
    r.hi = hi + a.hi + (r.lo < lo); // Carry
    return r;
  }


  W256 operator+=(const W256 &a) const {
    return W256(*this + a);
  }


  string toOct128(uint128_t t) const {
    string result;

    while (t > 0) {
      result.insert(result.begin(), '0' + (t & 7));
      t >>= 3;
    }

    if (result.length() == 0) result = "0";
    return result;
  }


  W256 &operator<<=(const int n) {

    if (n >= 128) {
      hi = lo << (n - 128);
      lo = 0;
    } else if (n != 0) {
      hi = (hi << n) | (lo >> (128 - n));
      lo <<= n;
    }

    return *this;
  }


  W256 &operator>>=(const int n) {

    if (n >= 128) {
      lo = hi >> (n - 128);
      hi = 0;
    } else if (n != 0) {
      lo = (lo >> n) | (hi << (128 - n));
      hi >>= n;
      cout << "operator>>=" << n << " lo=" << oct << toOct128(lo) << " hi=" << oct << toOct128(hi) << endl;
    }

    return *this;
  }


  uint128_t operator>>(const int n) const {
    W256 result = *this;
    result >>= n;
    return result;
  }


  uint128_t operator<<(const int n) const {
    W256 result = *this;
    result <<= n;
    return result;
  }


  bool operator==(const W256 &a) {
    return lo == a.lo && hi == a.hi;
  }


  bool operator!=(const W256 &a) {
    return lo != a.lo || hi != a.hi;
  }


  operator uint128_t() const { return lo; }

  W140 to140() const {
    W140 r;
    r.lo70 = lo;
    r.hi70 = *this >> 70;
    return r;
  }


  // Format in octal (yech).
  string toOct() const {
    if (lo == 0 && hi == 0) return "0";

    string s{};
    s.reserve(128);
    W256 value = *this;

    do {
      s += '0' + (value.lo & 7);
      value >>= 3;
    } while (value.lo != 0 || value.hi != 0);

    return string(s.rbegin(), s.rend());
  }
};


// Format a 128-bit unsigned in any base <= 10.
static string fmt128(uint128_t v128, int base=8) {
  if (v128 == 0) return "0";

  string s{};
  s.reserve(64);

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
	 << "  t=" << fmt128(t) << endl
	 << "  x=" << fmt128(x) << endl
	 << "  y=" << fmt128(y) << endl;

    x = (x << 1) | (y >> (nBits-1));	       // Shift x||y left
    y <<= 1;				       // one bit.

    if ((x | t) >= z) {
      cerr << "subtract z=" << fmt128(z) << " from x" << endl;
      x -= z;
      ++y;
    }

    cerr << endl;
  }

  return make_tuple(y, x);
}



/*
  Divide 192 bit n2||n1||n0 by d, returning remainder in rem.
  performs : (n2||n1||0) = ((n2||n1||n0) / d)
  d : a 128bit unsigned integer
*/
static void udiv192by128(uint64_t &n2, uint64_t &n1, uint64_t &n0, uint128_t d, uint128_t &rem) {
  uint128_t partial, remainder;
  remainder = n2 % d;
  n2 = n2 / d;
  partial = (remainder << 64) | n1;
  n1 = partial / d;
  remainder = partial % d;
  partial = (remainder << 64) | n0;
  n0 = partial / d;
  rem = remainder;
}


static void testDiv140(uint128_t denHi, uint128_t denLo, uint128_t div) {
  cerr << endl << "[all octal]" << endl
       << "  denHi=" << fmt128(denHi) << endl
       << "  denLo=" << fmt128(denLo) << endl
       << "  div=" << fmt128(div) << endl << endl;

  //  auto const [quo, rem] = divlllu(denHi, denLo, div);
  uint128_t rem = 0;
  uint64_t n2 = denHi;
  uint64_t n1 = denLo >> 64;
  uint64_t n0 = denLo;

  udiv192by128(n2, n1, n0, div, rem);
  uint128_t quo = ((uint128_t) n1 << 64) | n0;

  cerr << (denHi == 0 ? "" : fmt128(denHi)) << ",," << fmt128(denLo)
       << " / "
       << fmt128(div)
       << endl
       << "  quo="
       << fmt128(quo)
       << endl
       << "  rem="
       << fmt128(rem)
       << endl;
}


int main(int argc, char *argv[]) {
  unsigned __int128 z = 1234ull;

  printf("sizeof unsigned __int128=%lu\n", sizeof(z));

#if 0
  __int128 n = -1ll << 35;
  __int128 prod = n * n;

  printf("(-1 << 35) * (-1 << 35): hi=%12Lo lo=%12Lo\n",
	 (uint64_t) (prod >> 36) & ((1ull << 36) - 1),
	 (uint64_t) prod & ((1ull << 36) - 1));
#endif

#if 0
  x.lo = 012345012345ull;
  x.hi = 0;

  printf("x.lo << 6 = %12lo\n", x.lo << 6);

  testDiv140(uint128_t{0123456<<2},
	     uint128_t{0123456},
	     uint128_t{01});

  testDiv140(uint128_t{0123456<<2},
	     uint128_t{0123456},
	     uint128_t{010});

  testDiv140(uint128_t{0123456<<2},
	     uint128_t{0123456},
	     uint128_t{0100});

  testDiv140(uint128_t{0123456<<2},
	     uint128_t{0123456},
	     uint128_t{01000});

  testDiv140(uint128_t{0123456<<2},
	     uint128_t{0123456},
	     uint128_t{010000});

  testDiv140(uint128_t{0123456<<2},
	     uint128_t{0123456},
	     uint128_t{0100000});

  testDiv140(uint128_t{
      ((uint128_t) 0123456'123456ull << 16) |
      ((uint128_t) 0123456'123456ull >> 20)},
    uint128_t{
      ((uint128_t) 0123456'123456ull << 108) |
      ((uint128_t) 0123456'123456ull << 72) |
      ((uint128_t) 0123456'654321ull << 36) |
      ((uint128_t) 0123456'654321ull <<  0)},
    uint128_t{0100});
#endif
  
  W72 x, y;
  W256 p256;

  x.lo = 0123456'123456ull;
  x.loSign = 0;
  x.hi = x.lo;
  x.hiSign = 0;

  y.lo = 0111111'111111ull;
  y.loSign = 0;
  y.hi = y.lo;
  y.hiSign = 0;

  p256  = W256(x.lo * y.lo) <<  0;
  p256 += W256(x.hi * y.lo) << 35;
  p256 += W256(x.lo * y.hi) << 35;
  p256 += W256(x.hi * y.hi) << 70;

  W140 prod = p256.to140();

  cout << "prod.lo70=" << fmt128(prod.lo70) << "  "
       << "prod.hi70=" << fmt128(prod.hi70) << endl;

  W256 test{
    01234'123456'654321'123456ull,
    01234'123456'654321'123456ull,
    01234'123456'654321'123456ull,
    01234'123456'654321'123456ull};
  cout << "test=" << test.toOct() << endl;

  cout << "test.lo=" << test.toOct128(test.lo) << " .hi=" << test.toOct128(test.hi) << endl;

  for (int n=0; n<=70; ++n) {
    W256 v = test >> n;
    cout << "test>>" << n << "=" << v.toOct() << endl;
  }

  return 0;
}
