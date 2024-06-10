#include <assert.h>
using namespace std;

#include <gtest/gtest.h>

#include "kmstate.hpp"
#include "km10.hpp"

#include "logger.hpp"


Logger logger{};


using T2 = W36::T2;
using T3 = W36::T3;


using VW36 = vector<W36>;


class KM10Test: public testing::Test {
protected:

  KM10Test()
    : pc(01000),
      opnLoc(02000),
      acLoc(5)
  { }


  W36 a;
  W36 b;
  W36 result;
  VW36 insns;

  W36 expectAC;
  W36 expectMem;

  string cx;
  const unsigned pc;
  const unsigned opnLoc;
  const unsigned acLoc;


  void checkUnmodifiedFlags(KMState &state) {
    EXPECT_EQ(state.flags.ndv | state.flags.fuf, 0) << cx;
    EXPECT_EQ(state.flags.afi | state.flags.pub, 0) << cx;
    EXPECT_EQ(state.flags.uio | state.flags.usr, 0) << cx;
    EXPECT_EQ(state.flags.fpd | state.flags.fov, 0) << cx;
    EXPECT_EQ(state.flags.tr2, 0) << cx;
  }


  void checkFlagsC0(KMState &state) {
    EXPECT_EQ(state.flags.tr1, 1) << cx;
    EXPECT_EQ(state.flags.cy1, 0) << cx;
    EXPECT_EQ(state.flags.cy0, 1) << cx;
    EXPECT_EQ(state.flags.ov, 1)  << cx;
  }

  void checkFlagsC1(KMState &state) {
    EXPECT_EQ(state.flags.tr1, 1) << cx;
    EXPECT_EQ(state.flags.cy1, 1) << cx;
    EXPECT_EQ(state.flags.cy0, 0) << cx;
    EXPECT_EQ(state.flags.ov, 1) << cx;
  }

  void checkFlagsNC(KMState &state) {
    EXPECT_EQ(state.flags.tr1, 0) << cx;
    EXPECT_EQ(state.flags.cy1, 0) << cx;
    EXPECT_EQ(state.flags.cy0, 0) << cx;
    EXPECT_EQ(state.flags.ov, 0) << cx;
  }


  typedef void (KM10Test::*CallbackFn)(KMState &state);


  void test(CallbackFn checker) {
    KMState state(512 * 1024);
    state.pc.u = pc;
    state.maxInsns = 1;
    state.running = true;

    state.AC[acLoc] = a;
    state.memP[opnLoc] = b;
    unsigned dest = pc;
    for (auto insn: insns) state.memP[dest++] = insn;

    KM10{state}.emulate();

    checkUnmodifiedFlags(state);
    invoke(checker, this, state);
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
  insns = VW36{W36(0270, acLoc, 0, 0, opnLoc)};
  test(&InstructionTest_ADD_Test::checkFlagsC1);

  cx = "ADD CY0";
  a = bNeg;
  b = expectMem = bNeg;
  result = expectAC = a.extend() + b.extend();
  insns = VW36{W36(0270, acLoc, 0, 0, opnLoc)};
  test(&InstructionTest_ADD_Test::checkFlagsC0);

  cx = "ADD NC";
  a = aBig;
  b = expectMem = bPos;
  result = expectAC = a.extend() + b.extend();
  insns = VW36{W36(0270, acLoc, 0, 0, opnLoc)};
  test(&InstructionTest_ADD_Test::checkFlagsNC);

  cx = "ADDI";
  a = aBig;
  b = expectMem = bPos.rhu;
  result = expectAC = a.extend() + b.extend();
  insns = VW36{W36(0271, acLoc, 0, 0, opnLoc)};
  test(&InstructionTest_ADD_Test::checkFlagsNC);

  cx = "ADDM";
  a = expectAC = aBig;
  b = bPos;
  result = expectMem = a.extend() + b.extend();
  insns = VW36{W36(0272, acLoc, 0, 0, opnLoc)};
  test(&InstructionTest_ADD_Test::checkFlagsNC);

  cx = "ADDB";
  a = aBig;
  b = bPos;
  result = expectMem = expectAC = a.extend() + b.extend();
  insns = VW36{W36(0273, acLoc, 0, 0, opnLoc)};
  test(&InstructionTest_ADD_Test::checkFlagsNC);
}


TEST_F(InstructionTest, SUB) {
  const W36 aBig(0377777u, 0654321u);
  const W36 aNeg(0765432u, 0555555u);
  const W36 bNeg(0400000u, 0111111u);
  const W36 bPos(0000000u, 0123456u);

  cx = "SUB CY1";
  a = expectAC = aBig;
  b = bNeg;
  result = expectMem = a.extend() - b.extend();
  insns = VW36{W36(0274, acLoc, 0, 0, opnLoc)};
  test(&InstructionTest_SUB_Test::checkFlagsC1);

  cx = "SUB CY0";
  a = expectAC = bNeg;
  b = bPos;
  result = expectMem = a.extend() - b.extend();
  insns = VW36{W36(0274, acLoc, 0, 0, opnLoc)};
  test(&InstructionTest_SUB_Test::checkFlagsC0);

  cx = "SUB NC";
  a = expectAC = aBig;
  b = bPos;
  result = expectMem = a.extend() - b.extend();
  insns = VW36{W36(0274, acLoc, 0, 0, opnLoc)};
  test(&InstructionTest_SUB_Test::checkFlagsNC);

  cx = "SUBI";
  a = aBig;
  b = expectMem = bPos.rhu;
  result = expectAC = a.extend() - b.extend();
  insns = VW36{W36(0275, acLoc, 0, 0, opnLoc)};
  test(&InstructionTest_SUB_Test::checkFlagsNC);

  cx = "SUBM";
  a = expectAC = aBig;
  b = bPos;
  result = expectMem = a.extend() - b.extend();
  insns = VW36{W36(0276, acLoc, 0, 0, opnLoc)};
  test(&InstructionTest_SUB_Test::checkFlagsNC);

  cx = "SUBB";
  a = aBig;
  b = bPos;
  result = expectAC = expectMem = a.extend() - b.extend();
  insns = VW36{W36(0277, acLoc, 0, 0, opnLoc)};
  test(&InstructionTest_SUB_Test::checkFlagsNC);
}


int main(int argc, char *argv[]) {
  testing::InitGoogleTest(&argc, argv);
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
