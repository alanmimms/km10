#include <assert.h>
#include <functional>

using namespace std;

#include <gtest/gtest.h>

#include "word.hpp"
#include "kmstate.hpp"
#include "km10.hpp"
#include "km10-test.hpp"


////////////////////////////////////////////////////////////////
class InstructionMOVEs: public KM10Test {
};


TEST_F(InstructionMOVEs, EXCH) {
  a = expectMem = aBig;
  b = expectAC = aNeg;
  test(VW36{W36(0250, acLoc, 0, 0, opnLoc)},
       &InstructionMOVEs::check,
       &InstructionMOVEs::checkAllFlagsUnmodified);
};


TEST_F(InstructionMOVEs, MOVE) {
  a = aBig;
  b = expectMem = aNeg;
  expectAC = expectMem;
  test(VW36{W36(0200, acLoc, 0, 0, opnLoc)},
       &InstructionMOVEs::check,
       &InstructionMOVEs::checkAllFlagsUnmodified);
};


TEST_F(InstructionMOVEs, MOVEI) {
  a = aBig;
  b = expectMem = aNeg;
  expectAC = W36{0, opnLoc};
  test(VW36{W36(0201, acLoc, 0, 0, opnLoc)},
       &InstructionMOVEs::check,
       &InstructionMOVEs::checkAllFlagsUnmodified);
};


TEST_F(InstructionMOVEs, MOVEM) {
  a = expectAC = aBig;
  b = aNeg;
  expectMem = expectAC;
  test(VW36{W36(0202, acLoc, 0, 0, opnLoc)},
       &InstructionMOVEs::check,
       &InstructionMOVEs::checkAllFlagsUnmodified);
};


TEST_F(InstructionMOVEs, MOVESnon0AC) {
  a = aBig;
  b = aNeg;
  expectMem = expectAC = b;
  test(VW36{W36(0203, acLoc, 0, 0, opnLoc)},
       &InstructionMOVEs::check,
       &InstructionMOVEs::checkAllFlagsUnmodified);
};
