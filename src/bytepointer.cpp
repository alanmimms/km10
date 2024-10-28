#include "bytepointer.hpp"

  // The magic BytePointer factory that creates the right
  // BytePointerXXX instance based on type of BytePointer data it
  // finds at `ea`..
BytePointer *BytePointer::makeFrom(W36 bpa, KMState &state) {
  W36 w1(state.memGetN(bpa));

  if (w1.u >> 30 > 36) {		// Must be a one word 30-bit global BP
    return new BytePointerG1(w1);
  } else if (w1.u & W36::bit(12)) {	// Must be a two word global BP
    return new BytePointerG2(W72(w1, state.memGetN(bpa+1)));
  } else {				// Must be a one word local BP
    return new BytePointerL1(w1);
  }
}
