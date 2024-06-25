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


////////////////////////////////////////////////////////////////
class InstructionMOVN: public KM10Test {
public:
  virtual void checkFlagsC01() {
    EXPECT_EQ(state.flags.tr1, 0);
    EXPECT_EQ(state.flags.cy1, 1);
    EXPECT_EQ(state.flags.cy0, 1);
    EXPECT_EQ(state.flags.ov, 0);
  }
};


TEST_F(InstructionMOVN, CY1) {
  a = expectAC = aBig;
  b = expectMem = W36::bit0;
  result = b;
  test(VW36{W36(0210, acLoc, 0, 0, opnLoc)},
       &InstructionMOVN::check,
       &InstructionMOVN::checkFlagsC1);
};

TEST_F(InstructionMOVN, CY01) {
  a = expectAC = aBig;
  b = expectMem = 0;
  result = b;
  test(VW36{W36(0210, acLoc, 0, 0, opnLoc)},
       &InstructionMOVN::check,
       (CallbackFn) &InstructionMOVN::checkFlagsC01);
};

TEST_F(InstructionMOVN, NCp) {
  a = aBig;
  b = expectMem = bPos;
  result = expectAC = -b.s;
  test(VW36{W36(0210, acLoc, 0, 0, opnLoc)},
       &InstructionMOVN::check,
       &InstructionMOVN::checkFlagsNC);
};

TEST_F(InstructionMOVN, NCn) {
  a = aBig;
  b = expectMem = bNeg;
  result = expectAC = -b.s;
  test(VW36{W36(0210, acLoc, 0, 0, opnLoc)},
       &InstructionMOVN::check,
       &InstructionMOVN::checkFlagsNC);
};

TEST_F(InstructionMOVN, MOVNI) {
  a = aBig;
  b = expectMem = bPos;
  result = expectAC = -(int64_t) opnLoc;
  test(VW36{W36(0211, acLoc, 0, 0, opnLoc)},
       &InstructionMOVN::check,
       &InstructionMOVN::checkFlagsNC);
};

TEST_F(InstructionMOVN, MOVNM) {
  a = expectAC = aBig;
  b = bPos;
  result = expectMem = -(int64_t) a;
  test(VW36{W36(0212, acLoc, 0, 0, opnLoc)},
       &InstructionMOVN::check,
       &InstructionMOVN::checkFlagsNC);
};

TEST_F(InstructionMOVN, MOVNS) {
  a = aBig;
  b = bPos;
  result = expectAC = expectMem = -(int64_t) a;
  test(VW36{W36(0213, acLoc, 0, 0, opnLoc)},
       &InstructionMOVN::check,
       &InstructionMOVN::checkFlagsNC);
};


////////////////////////////////////////////////////////////////
class InstructionMOVM: public KM10Test {
};


TEST_F(InstructionMOVM, CY1) {
  a = expectAC = aBig;
  b = expectMem = W36::bit0;
  result = b;
  test(VW36{W36(0214, acLoc, 0, 0, opnLoc)},
       &InstructionMOVM::check,
       &InstructionMOVM::checkFlagsC1);
};

TEST_F(InstructionMOVM, NCp) {
  a = aBig;
  b = expectMem = bPos;
  result = expectAC = b;
  test(VW36{W36(0214, acLoc, 0, 0, opnLoc)},
       &InstructionMOVM::check,
       &InstructionMOVM::checkFlagsNC);
};

TEST_F(InstructionMOVM, NCn) {
  a = aBig;
  b = expectMem = bNeg;
  result = expectAC = -b.s;
  test(VW36{W36(0214, acLoc, 0, 0, opnLoc)},
       &InstructionMOVM::check,
       &InstructionMOVM::checkFlagsNC);
};

TEST_F(InstructionMOVM, MOVMI) {
  a = aBig;
  b = expectMem = bNeg;
  result = expectAC = (int64_t) opnLoc;
  test(VW36{W36(0215, acLoc, 0, 0, opnLoc)},
       &InstructionMOVM::check,
       &InstructionMOVM::checkFlagsNC);
};

TEST_F(InstructionMOVM, MOVMM) {
  a = expectAC = aNeg;
  b = bNeg;
  result = expectMem = -(int64_t) a;
  test(VW36{W36(0216, acLoc, 0, 0, opnLoc)},
       &InstructionMOVM::check,
       &InstructionMOVM::checkFlagsNC);
};

TEST_F(InstructionMOVM, MOVMS) {
  a = aNeg;
  b = bPos;
  result = expectAC = expectMem = -(int64_t) a;
  test(VW36{W36(0217, acLoc, 0, 0, opnLoc)},
       &InstructionMOVM::check,
       &InstructionMOVM::checkFlagsNC);
};
