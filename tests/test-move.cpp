#include <assert.h>
#include <functional>

using namespace std;

#include <gtest/gtest.h>

#include "word.hpp"
#include "kmstate.hpp"
#include "km10.hpp"
#include "km10-test.hpp"


////////////////////////////////////////////////////////////////
class InstructionMOVE: public KM10Test {
};


TEST_F(InstructionMOVE, EXCH) {
  a = expectMem = aBig;
  b = expectAC = aNeg;
  test(VW36{W36(0250, acLoc, 0, 0, opnLoc)},
       &InstructionMOVE::check,
       &InstructionMOVE::checkAllFlagsUnmodified);
};


TEST_F(InstructionMOVE, MOVE) {
  a = aBig;
  b = expectMem = aNeg;
  expectAC = expectMem;
  test(VW36{W36(0200, acLoc, 0, 0, opnLoc)},
       &InstructionMOVE::check,
       &InstructionMOVE::checkAllFlagsUnmodified);
};


TEST_F(InstructionMOVE, MOVEI) {
  a = aBig;
  b = expectMem = aNeg;
  expectAC = W36{0, opnLoc};
  test(VW36{W36(0201, acLoc, 0, 0, opnLoc)},
       &InstructionMOVE::check,
       &InstructionMOVE::checkAllFlagsUnmodified);
};


TEST_F(InstructionMOVE, MOVEM) {
  a = expectAC = aBig;
  b = aNeg;
  expectMem = expectAC;
  test(VW36{W36(0202, acLoc, 0, 0, opnLoc)},
       &InstructionMOVE::check,
       &InstructionMOVE::checkAllFlagsUnmodified);
};


TEST_F(InstructionMOVE, MOVESnon0AC) {
  a = aBig;
  b = aNeg;
  expectMem = expectAC = b;
  test(VW36{W36(0203, acLoc, 0, 0, opnLoc)},
       &InstructionMOVE::check,
       &InstructionMOVE::checkAllFlagsUnmodified);
};


TEST_F(InstructionMOVE, MOVES0AC) {
  acLoc = 0;
  a = expectAC = aBig;
  b = aNeg;
  expectMem = b;
  test(VW36{W36(0203, acLoc, 0, 0, opnLoc)},
       &InstructionMOVE::check,
       &InstructionMOVE::checkAllFlagsUnmodified);
};


////////////////////////////////////////////////////////////////
class InstructionMOVS: public KM10Test {
};


TEST_F(InstructionMOVS, MOVS) {
  a = aBig;
  b = expectMem = aNeg;
  expectAC = W36{b.rhu, b.lhu};
  test(VW36{W36(0204, acLoc, 0, 0, opnLoc)},
       &InstructionMOVS::check,
       &InstructionMOVS::checkAllFlagsUnmodified);
};


TEST_F(InstructionMOVS, MOVSI) {
  a = aBig;
  b = expectMem = aNeg;
  expectAC = W36{opnLoc, 0};
  test(VW36{W36(0205, acLoc, 0, 0, opnLoc)},
       &InstructionMOVS::check,
       &InstructionMOVS::checkAllFlagsUnmodified);
};


TEST_F(InstructionMOVS, MOVSM) {
  a = expectAC = aBig;
  b = aNeg;
  expectMem = W36{a.rhu, a.lhu};
  test(VW36{W36(0206, acLoc, 0, 0, opnLoc)},
       &InstructionMOVS::check,
       &InstructionMOVS::checkAllFlagsUnmodified);
};


TEST_F(InstructionMOVS, MOVSSnon0AC) {
  a = aBig;
  b = aNeg;
  expectMem = expectAC = W36{a.rhu, a.lhu};
  test(VW36{W36(0207, acLoc, 0, 0, opnLoc)},
       &InstructionMOVS::check,
       &InstructionMOVS::checkAllFlagsUnmodified);
};


TEST_F(InstructionMOVS, MOVSS0AC) {
  acLoc = 0;
  a = expectAC = aBig;
  b = aNeg;
  expectMem = W36{a.rhu, a.lhu};
  test(VW36{W36(0207, acLoc, 0, 0, opnLoc)},
       &InstructionMOVS::check,
       &InstructionMOVS::checkAllFlagsUnmodified);
};
