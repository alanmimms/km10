#include "km10.hpp"
#include "bytepointer.hpp"


using PSA = BytePointer::PSA;


// The magic BytePointer factory that creates the right
// BytePointerXXX instance based on type of BytePointer data it
// finds at `ea`..
BytePointer *BytePointer::makeFrom(W36 bpa, KM10 &km10) {
  W36 w1(km10.memGetN(bpa));

  if (w1.u >> 30 > 36) {		// Must be a one word 30-bit global BP
    return new BytePointerG1(w1);
  } else if (w1.u & W36::bit(12)) {	// Must be a two word global BP
    return new BytePointerG2(W72(w1, km10.memGetN(bpa+1)));
  } else {				// Must be a one word local BP
    return new BytePointerL1(w1);
  }
}


unsigned BytePointer::getByte(KM10 &km10) {
  auto [p, s, a] = getPSA(km10);
  return (km10.memGetN(a) >> p) & W36::rMask(s);
}


void BytePointer::putByte(W36 v, KM10 &km10) {
  auto [p, s, a] = getPSA(km10);
  W36 w(km10.memGetN(a));
  km10.memPutN((w.u & ~W36::bMask(p, s)) | ((v & W36::rMask(p)) << p), a);
}


void BytePointer::inc(KM10 &km10) {
}


bool BytePointer::adjust(unsigned ac, KM10 &km10) {
  return false;
}


////////////////////////////////////////////////////////////////
PSA BytePointerL1::getPSA(KM10 &km10) {
  unsigned rp = p;
  unsigned rs = s;
  return PSA(rp, rs, km10.getEA(i, x, y));
}


void BytePointerL1::inc(KM10 &km10) {

  if (s > p) {		// Point to left-aligned byte in next word
    p = 36 - s;
    ++y;
    y &= W36::halfOnes;
  } else {			// Move further down into the same word
    p -= s;
  }
}

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

bool BytePointerL1::adjust(unsigned ac, KM10 &km10) {

  if (s == 0) {
    km10.acPutN(u, ac);
    return false;
  } else if (s > 36 - p) {
    return true;
  }
    
  // Step 1: figure out the byte capacity of a word. This is broken
  // into the capacity to the left of the current byte (including the
  // byte itself) and the capacity to the right of the byte. If these
  // add up to zero, then the byte can't fit in a word, and we return
  // to the user with no divide set.
  int delta = km10.acGetN(ac);
  int bpwL = 0;
  int bpwR = 0;

  // Count bytes to left of and including current byte.
  for (int ip=p; ip > 0; ip -= s) ++bpwL;

  // Count bytes to right of current byte.
  for (int ip=p; ip < 36; ip += s) ++bpwR;

  int bpw = bpwL + bpwR;

  // No bytes fit in a word, so return error.
  if (bpw == 0) return true;

  // Compute number of words forward or backward from current
  // implied by the byte count delta. This is done by counting from
  // the leftmost byte in the current word as KL microcode does it.
  int deltaW = (delta - bpwL) / bpw;

  y += deltaW;

  // Compute the number of bytes from the leftmost in the new word
  // to advance or retreat by.
  int deltaB = delta % bpw;

  p += s * deltaB;

  // Step 2: generate a modified adjustment count and compute the
  // number of words to move and the relative byte position within
  // the word. All adjustments are done relative to the first byte
  // in the word, so that the resulting quotient is the actual
  // number of words to add to the base address. If the adjustment
  // is negative, however, we must back up the quotient by one and
  // offset the remainder by the capacity if it is non zero.

  km10.acPutN(u, ac);
  return false;
}


////////////////////////////////////////////////////////////////
const vector<tuple<unsigned, unsigned>> BytePointerG1::toPS{
  {36, 6}, {30, 6}, {24, 6}, {28, 6}, {12, 6}, { 6, 6}, { 0, 6},
    {36, 8}, {28, 8}, {20, 8}, {12, 8}, { 4, 8},
    {36, 7}, {29, 7}, {22, 7}, {15, 7}, { 8, 7}, { 1, 7},
    {36, 9}, {27, 9}, {18, 9}, { 9, 9}, { 0, 9},
    {36, 18}, {18, 18}, {0, 18}
};


PSA BytePointerG1::getPSA(KM10 &km10) {
  auto [p, s] = toPS[(u >> 30) - 37];
  return PSA(p, s, km10.getEA(0, 0, a));
}

void BytePointerG1::inc(KM10 &km10) {
}

bool BytePointerG1::adjust(unsigned ac, KM10 &km10) {
  return false;
}


////////////////////////////////////////////////////////////////
PSA BytePointerG2::getPSA(KM10 &km10) {
  unsigned rp = p;
  unsigned rs = s;
  return PSA{rp, rs, km10.getEA(i, x, y)};
}


bool BytePointerG2::isTwoWords() {
  return true;
}


void inc(KM10 &km10) {
}

bool BytePointerG2::adjust(unsigned ac, KM10 &km10) {
  return false;
}
