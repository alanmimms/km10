#pragma once

#include <cstdint>
#include <sstream>
#include <iomanip>

#include "w36.hpp"
#include "logging.hpp"

using namespace std;


// This is the single word format.
struct BytePointer {

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
  BytePointer(W36 w = 0) : u(w) {}


  // Accessors
  operator uint64_t() {return u;}
};


struct BytePointerG1 {

  union {

    struct ATTRPACKED {
      unsigned a: 30;
      unsigned ps: 6;
    };

    uint64_t u: 36;
  };
  

  // Constructors
  BytePointerG1(W36 w = 0) : u(w) {}


  // Accessors
  operator uint64_t() {return u;}
};


struct BytePointerG2 {

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
  BytePointerG2(W72 w) : u(w) {}


  // Accessors
  operator uint64_t() {return u;}
};
  
