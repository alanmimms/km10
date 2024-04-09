#ifndef __KL10_H__
#define __KL10_H__ 1

#include <stdint.h>
#include <stdio.h>


// A 36-bit PDP10 word.
typedef uint64_t W36;

// Bitmask for 36 bits.
static const W36 ALL1s = 0777777777777ull;


static inline int ShiftForBit(int b) {
  return 35 - b;
}


// Return the bit mask for PDP-10 numbered bit n. This special-cases
// negative n to return zero.
static inline W36 MaskForBit(int n) {
  return (n < 0 || n > 35) ? 0ull : (1ull << 35) >> n;
}


// Extract from 36-bit word `w` the PDP-10 numbered bits from `s` to
// `e` and return the result. This also strips off bits to the left of
// the 36-bit real content of `w`.
static inline W36 Extract(W36 w, int s, int e) {
  unsigned toShift = 35 - e;
  if (s > e) return 0ull;
  return (w & ALL1s & (MaskForBit(s - 1) - 1ull)) >> toShift;
}


// Format `w` as standard 777777,,777777 format and return a pointer
// to the buffer it's formatted into (which is passed in from caller).
// This makes printf suck a little less. (Why doesn't printf
// formatting have an extension API for those who want to add more
// formatting types?)
static inline char *oct36(char *bufP, W36 w) {
  sprintf(bufP, "%06lo,,%06lo", w, w);
  return bufP;
}

#endif
