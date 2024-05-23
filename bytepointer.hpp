#pragma once

#include <cstdint>
#include <sstream>
#include <iomanip>

#include "w36.hpp"
#include "logging.hpp"
#include "memory.hpp"

using namespace std;


class BytePointerL1;
class BytePointerG1;
class BytePointerG2;


// Abstract superclass with behaviors that work the same regardless of
// type of bytepointer, using methods provided by each type and a
// factory that creates instances based on the source data it finds.
struct BytePointer {
  static BytePointer makeFrom(W36 ea, Memory &memory);

  typedef tuple<unsigned, unsigned, unsigned> PSA;

  virtual PSA getPSA(Memory &memory) {
    return PSA();
  }


  unsigned getByte(Memory &memory) {
    auto [p, s, a] = getPSA(memory);
    W36 w(memory.memGetN(a));
    return (w.u >> p) & W36::mask(s);
  }
};


// This is the single word local format.
struct BytePointerL1: BytePointer {

  union {

    struct ATTRPACKED {
      unsigned y: 18;
      unsigned x: 4;
      unsigned i: 1;
      unsigned: 1;		// Must be zero
      unsigned s: 6;
      unsigned p: 6;
    };

    uint64_t u: 36;
  };


  // Constructors
  BytePointerL1(W36 w = 0) : u(w.u) {}


  // Accessors
  virtual PSA getPSA(Memory &memory) {
    unsigned rp = p;
    unsigned rs = s;
    return PSA(rp, rs, memory.getEA(i, x, y));
  }
};


struct BytePointerG1: BytePointer {

  union {

    struct ATTRPACKED {
      unsigned a: 30;
      unsigned ps: 6;
    };

    uint64_t u: 36;
  };


  inline const static vector<tuple<unsigned, unsigned>> toPS{
    {36, 6}, {30, 6}, {24, 6}, {28, 6}, {12, 6}, { 6, 6}, { 0, 6},
    {36, 8}, {28, 8}, {20, 8}, {12, 8}, { 4, 8},
    {36, 7}, {29, 7}, {22, 7}, {15, 7}, { 8, 7}, { 1, 7},
    {36, 9}, {27, 9}, {18, 9}, { 9, 9}, { 0, 9},
    {36, 18}, {18, 18}, {0, 18}
  };


  // Constructors
  BytePointerG1(W36 w = 0) : u(w.u) {}


  // Accessors
  virtual PSA getPSA(Memory &memory) {
    auto [p, s] = toPS[(u >> 30) - 37];
    return PSA(p, s, memory.getEA(0, 0, a));
  }
};


struct BytePointerG2: BytePointer {

  union {

    struct ATTRPACKED {
      unsigned user: 18;
      unsigned: 5;		// Must be zero
      unsigned one: 1;		// Must be 1
      unsigned s: 6;
      unsigned p: 6;
      unsigned y: 30;
      unsigned x: 4;
      unsigned i: 1;
      unsigned zero: 1;
    };

    uint128_t u: 72;
  };
  

  // Constructors
  BytePointerG2(W72 w) : u(w.u) {}


  // Accessors
  operator uint64_t() {return u;}

  virtual PSA getPSA(Memory &memory) {
    unsigned rp = p;
    unsigned rs = s;
    return PSA{rp, rs, memory.getEA(i, x, y)};
  }
};


  // The magic BytePointer factory that creates the right
  // BytePointerXXX instance based on type of BytePointer data it
  // finds at `ea`..
BytePointer BytePointer::makeFrom(W36 ea, Memory &memory) {
  W36 w1(ea);

  if (w1.u >> 30 > 36) {		// Must be a one word 30-bit global BP
    return BytePointerG1(w1);
  } else if (w1.u & W36::bit(12)) {	// Must be a two word global BP
    return BytePointerG2(W72(w1, memory.memGetN(ea+1)));
  } else {				// Must be a one word local BP
    return BytePointerL1(w1);
  }
}

