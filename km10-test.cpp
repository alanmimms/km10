#include <assert.h>
using namespace std;

#include <gtest/gtest.h>

#include "kmstate.hpp"
#include "km10.hpp"

#include "logger.hpp"


Logger logger{};


class KM10Test: public testing::Test {
protected:

  KM10Test()
    : state(256 * 1024)
  { }

  KMState state;

  
};


class InstructionTest: public KM10Test { };



TEST(BasicStructures, ExecutiveProcessTable) {
  ASSERT_EQ(sizeof(KMState::ExecutiveProcessTable), 512 * 8);
}

TEST(BasicStructures, UserProcessTable) {
  ASSERT_EQ(sizeof(KMState::UserProcessTable), 512 * 8);
}


TEST_F(InstructionTest, ADD) {
  const unsigned pc = 01000;
  const unsigned opLoc = 02000;
  const unsigned acLoc = 5;

  auto runTest = [&](W36 insn, W36 a, W36 b) {
    state.AC[acLoc] = a;
    state.memP[opLoc] = b;
    state.memP[pc] = insn;

    state.pc.u = pc;
    state.maxInsns = 1;
    KM10 km10(state);
    km10.emulate();
  };

  auto checkUnmodifiedFlags = [&] {
    EXPECT_EQ(state.flags.ndv | state.flags.fuf, 0);
    EXPECT_EQ(state.flags.afi | state.flags.pub, 0);
    EXPECT_EQ(state.flags.uio | state.flags.usr, 0);
    EXPECT_EQ(state.flags.fpd | state.flags.fov, 0);
  };

  auto checkFlagsC0 = [&](string &cx) {
    checkUnmodifiedFlags();
    EXPECT_EQ(state.flags.tr1, 1) << cx;
    EXPECT_EQ(state.flags.tr2, 0) << cx;
    EXPECT_EQ(state.flags.cy1, 0) << cx;
    EXPECT_EQ(state.flags.cy0, 1) << cx;
    EXPECT_EQ(state.flags.ov, 1)  << cx;
  };

  auto checkFlagsC1 = [&] {
    checkUnmodifiedFlags();
    EXPECT_EQ(state.flags.tr1, 1);
    EXPECT_EQ(state.flags.tr2, 0);
    EXPECT_EQ(state.flags.cy1, 1);
    EXPECT_EQ(state.flags.cy0, 0);
    EXPECT_EQ(state.flags.ov, 1);
  };

  auto checkFlagsNC = [&] {
    checkUnmodifiedFlags();
    EXPECT_EQ(state.flags.tr1, 0);
    EXPECT_EQ(state.flags.tr2, 0);
    EXPECT_EQ(state.flags.cy1, 0);
    EXPECT_EQ(state.flags.cy0, 0);
    EXPECT_EQ(state.flags.ov, 0);
  };

  const W36 aBig(0123456u, 0654321u);
  const W36 aNeg(0765432u, 0555555u);
  const W36 bNeg(0654321u, 0123456u);
  const W36 bPos(0000000u, 0123456u);
  W36 sum;

  // ADD with CY1
  sum = W36(aBig.u + bNeg.u);
  runTest(W36(0270, acLoc, 0, 0, opLoc), aBig, bNeg);
  EXPECT_EQ(state.AC[acLoc].u, sum);
  EXPECT_EQ(state.memP[opLoc], bNeg);
  checkFlagsC1();

  // ADD with CY0
  sum = W36(aNeg.u + bNeg.u);
  runTest(W36(0270, acLoc, 0, 0, opLoc), aNeg, bNeg);
  EXPECT_EQ(state.AC[acLoc], sum);
  EXPECT_EQ(state.memP[opLoc], bNeg);
  checkFlagsC0();

  // ADD with no carry
  sum = W36(aBig.u + bPos.u);
  runTest(W36(0270, acLoc, 0, 0, opLoc), aBig, bPos);
  EXPECT_EQ(state.AC[acLoc], sum);
  EXPECT_EQ(state.memP[opLoc], bPos);
  checkFlagsC0();

  // ADDI
  sum = W36(aBig.u + bPos.rhu);
  runTest(W36(0271, acLoc, 0, 0, bPos.rhu), aBig, bPos.rhu);
  EXPECT_EQ(state.AC[acLoc], sum);
  EXPECT_EQ(state.memP[opLoc], bPos.rhu);
  checkFlagsNC();

  // ADDM
  sum = W36(aBig.u + bPos.u);
  runTest(W36(0272, acLoc, 0, 0, opLoc), aBig, bPos);
  EXPECT_EQ(state.AC[acLoc], aBig);
  EXPECT_EQ(state.memP[opLoc], sum);
  checkFlagsNC();

  // ADDB
  runTest(W36(0273, acLoc, 0, 0, opLoc), aBig, bPos);
  EXPECT_EQ(state.AC[acLoc], sum);
  EXPECT_EQ(state.memP[opLoc], sum);
  checkFlagsNC();
}



int main(int argc, char *argv[]) {
  testing::InitGoogleTest(&argc, argv);

  KMState state(4 * 1024 * 1024);

  state.maxInsns = 0;
  KM10 km10(state);

  state.running = true;
  //  km10.emulate();

  return RUN_ALL_TESTS();
}


void Logger::nyi(KMState &state) {
  s << " [not yet implemented]";
  cerr << "Not yet implemented at " << state.pc.fmtVMA() << endl;
  ADD_FAILURE();
}


void Logger::nsd(KMState &state) {
  s << " [no such device]";
  cerr << "No such device at " << state.pc.fmtVMA() << endl;
  ADD_FAILURE();
}
