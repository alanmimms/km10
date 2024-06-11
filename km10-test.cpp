#include <assert.h>
#include <functional>

using namespace std;

#include <gtest/gtest.h>

#include "kmstate.hpp"
#include "km10.hpp"

#include "logger.hpp"


Logger logger{};


using VW36 = vector<W36>;


class KM10Test: public testing::Test {
public:

  KM10Test()
    : pc(01000),
      opnLoc(02000),
      acLoc(5)
  { }


  using CallbackFn = void (KM10Test::*)(KMState& state);


  W36 a;
  W36 b;
  W36 result;
  W72 result72;

  W36 expectAC;
  W36 expectAC2;
  W36 expectMem;

  string cx;
  const unsigned pc;
  const unsigned opnLoc;
  const unsigned acLoc;


  void checkUnmodifiedFlags(KMState& state) {
    EXPECT_EQ(state.flags.ndv | state.flags.fuf, 0) << cx;
    EXPECT_EQ(state.flags.afi | state.flags.pub, 0) << cx;
    EXPECT_EQ(state.flags.uio | state.flags.usr, 0) << cx;
    EXPECT_EQ(state.flags.fpd | state.flags.fov, 0) << cx;
    EXPECT_EQ(state.flags.tr2, 0) << cx;
  }

  void checkFlagsC0(KMState& state) {
    EXPECT_EQ(state.flags.tr1, 1) << cx;
    EXPECT_EQ(state.flags.cy1, 0) << cx;
    EXPECT_EQ(state.flags.cy0, 1) << cx;
    EXPECT_EQ(state.flags.ov, 1)  << cx;
  }

  void checkFlagsC1(KMState& state) {
    EXPECT_EQ(state.flags.tr1, 1) << cx;
    EXPECT_EQ(state.flags.cy1, 1) << cx;
    EXPECT_EQ(state.flags.cy0, 0) << cx;
    EXPECT_EQ(state.flags.ov, 1) << cx;
  }

  void checkFlagsNC(KMState& state) {
    EXPECT_EQ(state.flags.tr1, 0) << cx;
    EXPECT_EQ(state.flags.cy1, 0) << cx;
    EXPECT_EQ(state.flags.cy0, 0) << cx;
    EXPECT_EQ(state.flags.ov, 0) << cx;
  }


  void check(KMState& state) {
    EXPECT_EQ(state.AC[acLoc], expectAC) << cx;
    EXPECT_EQ(state.memP[opnLoc], expectMem) << cx;
  }


  void checkI(KMState& state) {
    EXPECT_EQ(state.AC[acLoc], expectAC) << cx;
  }


  void check72(KMState& state) {
    EXPECT_EQ(state.AC[acLoc], expectAC) << cx;
    EXPECT_EQ(state.AC[acLoc+1], expectAC2) << cx;
    EXPECT_EQ(state.memP[opnLoc], expectMem) << cx;
  }


  void checkI72(KMState& state) {
    EXPECT_EQ(state.AC[acLoc], expectAC) << cx;
    EXPECT_EQ(state.AC[acLoc+1], expectAC2) << cx;
  }


  void test(VW36 insns, CallbackFn checker, CallbackFn flagChecker) {
    KMState state(512 * 1024);
    state.pc.u = pc;
    state.maxInsns = 1;
    state.running = true;

    state.AC[acLoc] = a;
    state.memP[opnLoc] = b;
    unsigned dest = pc;
    for (auto insn: insns) state.memP[dest++] = insn;

    KM10{state}.emulate();
    invoke(checker, this, state);
    invoke(flagChecker, this, state);
    checkUnmodifiedFlags(state);
  }
};


class InstructionTest: public KM10Test { };



TEST(BasicStructures, ExecutiveProcessTable) {
  ASSERT_EQ(sizeof(KMState::ExecutiveProcessTable), 512 * 8);
}

TEST(BasicStructures, UserProcessTable) {
  ASSERT_EQ(sizeof(KMState::UserProcessTable), 512 * 8);
}


TEST_F(InstructionTest, ADD) {
  const W36 aBig(0377777u, 0654321u);
  const W36 aNeg(0765432u, 0555555u);
  const W36 bNeg(0400000u, 0123456u);
  const W36 bPos(0000000u, 0123456u);

  cx = "ADD CY1";
  a = aBig;
  b = expectMem = aBig;
  result = expectAC = a.extend() + b.extend();
  test(VW36{W36(0270, acLoc, 0, 0, opnLoc)},
       &InstructionTest::check,
       &InstructionTest::checkFlagsC1);

  cx = "ADD CY0";
  a = bNeg;
  b = expectMem = bNeg;
  result = expectAC = a.extend() + b.extend();
  test(VW36{W36(0270, acLoc, 0, 0, opnLoc)},
       &InstructionTest::check,
       &InstructionTest::checkFlagsC0);

  cx = "ADD NC";
  a = aBig;
  b = expectMem = bPos;
  result = expectAC = a.extend() + b.extend();
  test(VW36{W36(0270, acLoc, 0, 0, opnLoc)},
       &InstructionTest::check,
       &InstructionTest::checkFlagsNC);

  cx = "ADDI";
  a = aBig;
  b = bPos.rhu;
  result = expectAC = a.extend() + b.extend();
  test(VW36{W36(0271, acLoc, 0, 0, b.rhu)},
       &InstructionTest::checkI,
       &InstructionTest::checkFlagsNC);

  cx = "ADDM";
  a = expectAC = aBig;
  b = bPos;
  result = expectMem = a.extend() + b.extend();
  test(VW36{W36(0272, acLoc, 0, 0, opnLoc)},
       &InstructionTest::check,
       &InstructionTest::checkFlagsNC);

  cx = "ADDB";
  a = aBig;
  b = bPos;
  result = expectMem = expectAC = a.extend() + b.extend();
  test(VW36{W36(0273, acLoc, 0, 0, opnLoc)},
       &InstructionTest::check,
       &InstructionTest::checkFlagsNC);
}


TEST_F(InstructionTest, SUB) {
  const W36 aBig(0377777u, 0654321u);
  const W36 aNeg(0765432u, 0555555u);
  const W36 bNeg(0400000u, 0111111u);
  const W36 bPos(0000000u, 0123456u);

  cx = "SUB CY1";
  a = aBig;
  b = expectMem = bNeg;
  result = expectAC = a.extend() - b.extend();
  test(VW36{W36(0274, acLoc, 0, 0, opnLoc)},
       &InstructionTest::check,
       &InstructionTest::checkFlagsC1);

  cx = "SUB CY0";
  a = bNeg;
  b = expectMem = bPos;
  result = expectAC = a.extend() - b.extend();
  test(VW36{W36(0274, acLoc, 0, 0, opnLoc)},
       &InstructionTest::check,
       &InstructionTest::checkFlagsC0);

  cx = "SUB NC";
  a = aBig;
  b = expectMem = bPos;
  result = expectAC = a.extend() - b.extend();
  test(VW36{W36(0274, acLoc, 0, 0, opnLoc)},
       &InstructionTest::check,
       &InstructionTest::checkFlagsNC);

  cx = "SUBI";
  a = aBig;
  b = bPos.rhu;
  result = expectAC = a.extend() - b.extend();
  test(VW36{W36(0275, acLoc, 0, 0, b.rhu)},
       &InstructionTest::checkI,
       &InstructionTest::checkFlagsNC);

  cx = "SUBM";
  a = expectAC = aBig;
  b = bPos;
  result = expectMem = a.extend() - b.extend();
  test(VW36{W36(0276, acLoc, 0, 0, opnLoc)},
       &InstructionTest::check,
       &InstructionTest::checkFlagsNC);

  cx = "SUBB";
  a = aBig;
  b = bPos;
  result = expectAC = expectMem = a.extend() - b.extend();
  test(VW36{W36(0277, acLoc, 0, 0, opnLoc)},
       &InstructionTest::check,
       &InstructionTest::checkFlagsNC);
}


TEST_F(InstructionTest, MUL) {
  const W36 aBig(0377777u, 0654321u);
  const W36 aNeg(0765432u, 0555555u);
  const W36 bNeg(0400000u, 0123456u);
  const W36 bPos(0000000u, 0123456u);

  cx = "MUL CY1";
  a = aBig;
  b = expectMem = aBig;
  result72 = (int128_t) a.extend() * (int128_t) b.extend();
  expectAC = result72.lo;
  expectAC2 = result72.hi;
  test(VW36{W36(0224, acLoc, 0, 0, opnLoc)},
       &InstructionTest::check72,
       &InstructionTest::checkFlagsC1);

#if 0
  cx = "MUL CY0";
  a = bNeg;
  b = expectMem = bNeg;
  result = expectAC = a.extend() * b.extend();
  insns = VW36{W36(0224, acLoc, 0, 0, opnLoc)};
  test(&InstructionTest::checkFlagsC0);

  cx = "MUL NC";
  a = aBig;
  b = expectMem = bPos;
  result = expectAC = a.extend() * b.extend();
  insns = VW36{W36(0224, acLoc, 0, 0, opnLoc)};
  test(&InstructionTest::checkFlagsNC);

  cx = "MULI";
  a = aBig;
  b = expectMem = bPos.rhu;
  result = expectAC = a.extend() * b.extend();
  insns = VW36{W36(0225, acLoc, 0, 0, opnLoc)};
  test(&InstructionTest::checkFlagsNC);

  cx = "MULM";
  a = expectAC = aBig;
  b = bPos;
  result = expectMem = a.extend() * b.extend();
  insns = VW36{W36(0226, acLoc, 0, 0, opnLoc)};
  test(&InstructionTest::checkFlagsNC);

  cx = "MULB";
  a = aBig;
  b = bPos;
  result = expectMem = expectAC = a.extend() * b.extend();
  insns = VW36{W36(0227, acLoc, 0, 0, opnLoc)};
  test(&InstructionTest::checkFlagsNC);
#endif
}


int main(int argc, char *argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}


void Logger::nyi(KMState& state) {
  s << " [not yet implemented]";
  cerr << "Not yet implemented at " << state.pc.fmtVMA() << endl;
  ADD_FAILURE();
}


void Logger::nsd(KMState& state) {
  s << " [no such device]";
  cerr << "No such device at " << state.pc.fmtVMA() << endl;
  ADD_FAILURE();
}
