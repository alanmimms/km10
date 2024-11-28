#pragma once

#include <cstdint>
#include <sstream>
#include <iomanip>
#include <ostream>
#include <iostream>
#include <vector>

#include "word.hpp"
#include "logger.hpp"

using namespace std;


class KM10;


// Abstract superclass with behaviors that work the same regardless of
// type of bytepointer, using methods provided by each type and a
// factory that creates instances based on the source data it finds.
struct BytePointer {
  static BytePointer *makeFrom(W36 bpa, KM10 &cpu);

  typedef tuple<unsigned, unsigned, unsigned> PSA;

  virtual PSA getPSA(KM10 &cpu) = 0;

  virtual bool isTwoWords() {
    return false;
  }

  unsigned getByte(KM10 &cpu);
  void putByte(W36 v, KM10 &cpu);
  virtual void inc(KM10 &cpu);
  virtual bool adjust(unsigned ac, KM10 &cpu);
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
  virtual PSA getPSA(KM10 &cpu);
  virtual void inc(KM10 &cpu);

  // Returns true if trap1, overflow, and no-divide flags should be set.

  // From KLX microcode 442:
  // (FYI: "OWL" == one word local, "OWG" == one word global,
  // "TWG" == two word global.)
  //
  // ADJBP is handled separately for OWGs and OWL/TWGs. We consider
  // the latter case first.
  //
  // Step 1: figure out the byte capacity of a word. This is broken
  // into the capacity to the left of the current byte (including the
  // byte itself) and the capacity to the right of the byte. If these
  // add up to zero, then the byte can't fit in a word, and we return
  // to the user with no divide set. If the byte size is zero, we
  // return with the pointer as is.
  //
  // For this version, we compute the capacities by using repeated
  // subtraction. Since the numbers involved are typically no greater
  // than five or six (and are never bigger than 36) this will be
  // faster than division.
  //
  // Step 2: generate a modified adjustment count and compute the
  // number of words to move and the relative byte position within the
  // word. All adjustments are done relative to the first byte in the
  // word, so that the resulting quotient is the actual number of
  // words to add to the base address. If the adjustment is negative,
  // however, we must back up the quotient by one and offset the
  // remainder by the capacity if it is non zero.
  //
  // In order to speed up the division, the absolute value of the
  // modified adjustment is broken into ranges of up to 63, 64 to
  // 2**18-1, and 2**18 or greater. This lets us use step counts of 7,
  // 19, and 36, respectively, saving a lot of time for the most
  // common cases.
  //
  // For this portion of the work, OWGs and OWLs are identical.
  //
  // Step 3: add the final quotient to the address, and offset the
  // byte into the word by the adjusted remainder. To do this, we must
  // finally differentiate an OWL from a TWG. (Recall that we saved
  // most of the original byte pointer (including bit 12) in T0 before
  // we did the division.) In any event, for an OWL we add the
  // quotient to the right half of the byte pointer; for a TWG we
  // fetch the second word and then add the quotient to bits 6-35 if
  // it's global, to bits 18-35 if it's local.
  // 
  // After this, we subtract the byte pointer S field from (36 - the
  // alignment information left in the P field) precisely remainder
  // times (recall that division copied SC, preloaded with 36, into FE
  // when it finished). That's about it.
  // 
  // OWGs split off their separate way.
  virtual bool adjust(unsigned ac, KM10 &cpu);
};


struct BytePointerG1: BytePointer {

  union {

    struct ATTRPACKED {
      unsigned a: 30;
      unsigned ps: 6;
    };

    uint64_t u: 36;
  };


  const static vector<tuple<unsigned, unsigned>> toPS;


  // Constructors
  BytePointerG1(W36 w = 0) : u(w.u) {}


  // Accessors
  virtual PSA getPSA(KM10 &cpu);
  virtual void inc(KM10 &cpu);
  virtual bool adjust(unsigned ac, KM10 &cpu);
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

  virtual PSA getPSA(KM10 &cpu);
  virtual bool isTwoWords() override;
  virtual void inc(KM10 &cpu);
  virtual bool adjust(unsigned ac, KM10 &cpu);
};
