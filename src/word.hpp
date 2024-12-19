// XXX TODO:
// * Consolidate extnOf, ext64, etc.

#pragma once

#include <cstdint>
#include <sstream>
#include <iomanip>
#include <array>

using namespace std;


typedef unsigned __int128 uint128_t;
typedef signed __int128 int128_t;


class Debugger;


#define ATTRPACKED    __attribute__((packed))

struct W36 {

  enum IOOp {
    BLKI,
    DATAI,
    BLKO,
    DATAO,
    CONO,
    CONI,
    CONSZ,
    CONSO
  };


  enum IntFunction {
    zeroIF,
    standardIF,
    vectorIF,
    incIF,
    examineIF,
    depositIF,
    byteIF,
  };


  enum AddrSpace {
    execPT,
    execVA,
    physical = 4,
  };


  union {
    int64_t s: 36;

    uint64_t u: 36;

    struct ATTRPACKED {
      signed rhs: 18;
      signed lhs: 18;
    };

    struct ATTRPACKED {
      unsigned rhu: 18;
      unsigned lhu: 18;
    };

    struct ATTRPACKED {
      uint64_t mag: 35;
      unsigned sign: 1;
    };

    // This is done this way instead of doing union/struct for the
    // various views of the upper halfword because I could NOT get g++
    // to correctly bit-align the op field.
    struct ATTRPACKED {
      unsigned y: 18;
      unsigned x: 4;
      unsigned i: 1;
    };

    struct ATTRPACKED {
      unsigned: 23;
      unsigned ac: 4;
      unsigned op: 9;
    };
	
    struct ATTRPACKED {
      unsigned: 23;
      IOOp ioOp: 3;
      unsigned ioDev: 7;
      unsigned ioSeven: 3;
    };

    struct ATTRPACKED {
      unsigned: 23;
      unsigned ioAll: 13;
    };

    struct ATTRPACKED {
      unsigned vma: 23;
      unsigned pcFlags: 13;
    };

    struct ATTRPACKED {
      unsigned intAddr: 23;
      unsigned mustBeZero: 2;
      unsigned device: 4;
      unsigned q: 1;
      IntFunction intFunction: 3;
      AddrSpace addrSpace: 3;
    };
  };


  // Constants
  static inline const unsigned halfOnes = 0777'777u;
  static inline const uint64_t all1s = 0777777'777777ull;
  static inline const uint64_t bit0 = 1ull << 35;
  static inline const uint64_t magMask = bit0 - 1;

  // Constructors/factories
  W36(int64_t w = 0) : s(w) {}
  W36(unsigned lh, unsigned rh) : rhu(rh), lhu(lh) {}

  // "Assembler"
  W36(int aOp, int aAC, int aI, int aX, int aY) {
    op = aOp;
    ac = aAC;
    i = aI;
    x = aX;
    y = aY;
  }

  // Build up from 35-bit magnitude and a sign bit.
  static inline W36 fromMag(uint64_t aMag, int aSign = 0) {
    W36 w(aMag & magMask);
    w.sign = aSign;
    return w;
  }

  // Extract a bitfield using the awful PDP10 bit numbering.
  inline uint64_t extract(int startBit, int endBit) {
    const int width = endBit - startBit + 1;
    const uint64_t mask = (1ull << width) - 1;
    return (u >> (35 - endBit)) & mask;
  }


  // Return mask for PDP10 bit number `n`.
  constexpr static uint64_t bit(unsigned n) {return 1ull << (35 - n);}

  // Return rightmost `s` bit mask.
  constexpr static uint64_t rMask(unsigned s) {return (1ull << s) - 1;}

  // Return `s` bit mask at location `p` (like a byte pointer).
  constexpr static uint64_t bMask(unsigned p, unsigned s) {return rMask(s) << p;}


  // Accessors
  operator uint64_t() const {return u;}
  int64_t ext64() const {return s < 0 ? (int64_t) s | ~0ll << 36 : s;}
  bool operator==(const W36 &other) const {return u == other.u;}


  // For googletest stringification
  friend void PrintTo(const W36& w, std::ostream* os) {
    *os << w.fmt36();
  }


  void putLH(unsigned aLH) {lhu = aLH;}
  void putRH(unsigned aRH) {rhu = aRH;}

  unsigned getLH() const {return lhu;}
  unsigned getRH() const {return rhu;}

  int64_t getLHextend() const {return (int64_t) lhs;}
  int64_t getRHextend() const {return (int64_t) rhs;}

  bool isSection0() const {return (vma >> 18) == 0u;}


  // Typedefs
  using tDoubleWord = tuple<W36,W36>;
  using tQuadWord = tuple<W36,W36,W36,W36>;


  // Formatters
  string fmt18() const;
  string fmtVMA() const;
  string fmt36() const;
  string sixbit() const;
  string ascii() const;

  // Disassembly of instruction words
  string disasm(Debugger *debuggerP);
};


// This is a 72-bit word whose internal representation is to keep all
// 72 bits. Using "halves()" it can be converted to the 1+35+1+35 bit
// representation used by PDP10 to represent signed values where the
// "1" bits here are both copies of the sign bit from bit #0.
struct W72 {

  union {
    int128_t s: 72;

    uint128_t u: 72;

    struct ATTRPACKED {
      uint64_t lo: 36;
      uint64_t hi: 36;
    };

    struct ATTRPACKED {
      int64_t sLo: 36;
      int64_t sHi: 36;
    };

    struct ATTRPACKED {
      uint64_t lo35: 35;
      unsigned loSign: 1;
      uint64_t hi35: 35;
      unsigned hiSign: 1;
    };
  };

  using tDoubleWord = W36::tDoubleWord;

  W72(uint128_t v = 0) : u(v) {}
  W72(W36 aHi, W36 aLo) : lo(aLo.u), hi(aHi.u) {}
  W72(uint64_t mag0, uint64_t mag1, int isNeg) : lo35(mag1), loSign(isNeg), hi35(mag0), hiSign(isNeg) {}

  // Factory to take a 70-bit unsigned magnitude and a sign and make a
  // doubleword.
  static inline W72 fromMag(uint128_t mag, int isNeg = 0) {
    return W72{(uint64_t) mag, (uint64_t) mag >> 35, !!isNeg};
  }

  operator uint128_t() {return u;}
  operator int128_t() {return s;}

  static inline const uint128_t bit0 = ((uint128_t) 1) << 71;
  static inline const int128_t sBit1 = ((int128_t) 1) << 70;
  static inline const uint128_t bit36 = ((uint128_t) 1) << 35;
  static inline const uint128_t all1s = (bit0 << 1) - 1;


  // Return mask for PDP10 bit number `n`.
  constexpr static uint128_t bit(unsigned n) {return ((uint128_t) 1) << (71 - n);}

  // Return rightmost `s` bit mask.
  constexpr static uint128_t rMask(unsigned s) {return (((uint128_t) 1) << (s + 1)) - 1;}

  // Grab the 70-bit signed number from the double word represention,
  // cutting out the low word's sign bit.
  int128_t toS70() const {return ((int128_t) sHi << 35) | lo35;}

  // Grab the 70-bit unsigned magnitude from the double word
  // represention, cutting out the low word's sign bit.
  uint128_t toU70() const {return ((uint128_t) hi35 << 35) | lo35;}

  // Convert a 72-bit internal representation to two 35-bit magnitudes
  // with duplicated sign bits.
  static inline auto toDW(int128_t v72) {
    const int isNeg = v72 < 0;
    return tDoubleWord(W36::fromMag(v72 >> 36, isNeg), W36::fromMag(v72, isNeg));
  }

    bool isMaxNeg() {
      return lo == 0400000'000000ull && hi == 0400000'000000ull;
    }


  // String formatting
  string fmt72() const {
    ostringstream ss;
    ss << W36(hi).fmt36() << ",,," << W36(lo).fmt36();
    return ss.str();
  }

  // Format a 128-bit number as whatever base <= 10.
  static inline string fmt128(int128_t v128, int base=10) {
    if (v128 == 0) return "0";

    string s{};
    s.reserve(40);

    if (v128 < 0) v128 = -v128;

    do {
      s += '0' + (v128 % base);
      v128 /= base;
    } while (v128 != 0);

    if (v128 < 0) {
      s += "-";
      v128 = -v128;
    }

    return string(s.rbegin(), s.rend());
  }
};


// This is a 140-bit word whose internal representation is four copies
// of the sign bit interspersed with four 35-bit magnitude words.
struct W144 {

  union {

    struct ATTRPACKED {
      uint128_t w3: 36;
      uint128_t w2: 36;
      uint128_t w1: 36;
      uint128_t w0: 36;
    };

    struct ATTRPACKED {
      uint128_t mag3: 35;
      unsigned sign3: 1;
      uint128_t mag2: 35;
      unsigned sign2: 1;
      uint128_t mag1: 35;
      unsigned sign1: 1;
      uint128_t mag0: 35;
      unsigned sign0: 1;
    };
  };

  // Set if negative magnitude value.
  unsigned sign: 1;

  using tQuadWord = W36::tQuadWord;

  // Build one up from four W36s. This only uses the sign from the
  // highest order word.
  W144(W36 a0, W36 a1, W36 a2, W36 a3) {
    mag0 = a0.mag;
    sign0 = a0.sign;
    mag1 = a1.mag;
    sign1 = a0.sign;
    mag2 = a2.mag;
    sign2 = a0.sign;
    mag3 = a3.mag;
    sign3 = a0.sign;
  }


  static inline W144 fromMag(uint128_t aMag0, uint128_t aMag1, int aNeg) {
    aNeg = !!aNeg;

    return W144{
      W36::fromMag((uint64_t) ((aMag1 >> 105) | aMag0), aNeg),
      W36::fromMag((uint64_t) (aMag1 >> 70), aNeg),
      W36::fromMag((uint64_t) (aMag1 >> 35), aNeg),
      W36::fromMag((uint64_t) aMag1, aNeg)};
  }

  // Make a 140-bit product from two 70-bit unsigned magnitudes and a
  // sign (0=>positive, 1=>negative). See _Hacker's Delight_ Second
  // Edition, Chapter 8.
  static inline W144 product(uint128_t a, uint128_t b, int aNeg) {
    array<uint32_t,3>u{(uint32_t) a, (uint32_t) (a >> 32), (uint32_t) (a >> 64)};
    array<uint32_t,3>v{(uint32_t) b, (uint32_t) (b >> 32), (uint32_t) (b >> 64)};
    array<uint32_t,6>w{};

    for (unsigned j=0; j < v.size(); ++j) {
      // The carry into the current column from the previous one.
      uint32_t carry = 0;

      // Process columns, accumulating partial product and providing
      // carry out to the next one.
      for (unsigned i=0; i < u.size(); ++i) {
	// The explicitly wider accumulator into which the partial
	// product is accumulated. This will be truncated into our
	// current w[i+j]. The high order bits saved as the carry into
	// the next column to the left.
	uint64_t accum = (uint64_t) u[i] * (uint64_t) v[j] + w[i+j] + carry;
	w[i+j] = (uint32_t) accum;
	carry = (uint32_t) (accum >> 32);
      }

      w[j+u.size()] = carry;
    }

    // Since we know we only have 70-bit multiplicands, the result
    // is at most 140 bits and will therefore be found in w[0..4].
    return W144::fromMag(
      (uint128_t) w[2] >> 6 | (uint128_t) w[3] << 24 | (((uint128_t) w[4] << 56) & 077),
      (uint128_t) w[0] << 0 | (uint128_t) w[1] << 32 | (((uint128_t) w[2] << 64) & 077),
      aNeg);
  }


  uint128_t lowerU70() const {
    return ((uint128_t) mag2 << 35) | (uint128_t) mag3; 
  }

  uint128_t upperU70() const {
    return ((uint128_t) mag0 << 35) | (uint128_t) mag1;
  }


  // Compare this 140-bit magnitude against the specified 70-bit
  // magnitude return true if this >= a70.
  bool operator >= (const uint128_t a70) const {return upperU70() != 0 || lowerU70() >= a70;}


  // Accessors/converters
  tQuadWord toQuadWord() const {
    auto const signBit = sign ? W36::bit0 : 0ull;

    return tQuadWord{
      mag0 | signBit,
      mag1 | signBit,
      mag2 | signBit,
      mag3 | signBit
    };
  }
};
