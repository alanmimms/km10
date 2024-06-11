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
      acLoc(5),
      aBig(0377777u, 0654321u),
      aNeg(0765432u, 0555555u),
      bNeg(0400000u, 0123456u),
      bNg1(0400000u, 0111111u),
      bPos(0000000u, 0123456u)
  { }


  using CallbackFn = void (KM10Test::*)();


  W36 a;
  W36 b;
  W36 result;

  W36 expectAC;
  W36 expectMem;

  KMState state;
  string cx;
  const unsigned pc;
  const unsigned opnLoc;
  const unsigned acLoc;

  const W36 aBig;
  const W36 aNeg;
  const W36 bNeg;
  const W36 bNg1;
  const W36 bPos;


  void noCheck() {
  }


  void checkUnmodifiedFlags() {
    EXPECT_EQ(state.flags.ndv | state.flags.fuf, 0) << cx;
    EXPECT_EQ(state.flags.afi | state.flags.pub, 0) << cx;
    EXPECT_EQ(state.flags.uio | state.flags.usr, 0) << cx;
    EXPECT_EQ(state.flags.fpd | state.flags.fov, 0) << cx;
    EXPECT_EQ(state.flags.tr2, 0) << cx;
  }

  void checkFlagsC0() {
    EXPECT_EQ(state.flags.tr1, 1) << cx;
    EXPECT_EQ(state.flags.cy1, 0) << cx;
    EXPECT_EQ(state.flags.cy0, 1) << cx;
    EXPECT_EQ(state.flags.ov, 1)  << cx;
  }

  void checkFlagsC1() {
    EXPECT_EQ(state.flags.tr1, 1) << cx;
    EXPECT_EQ(state.flags.cy1, 1) << cx;
    EXPECT_EQ(state.flags.cy0, 0) << cx;
    EXPECT_EQ(state.flags.ov, 1) << cx;
  }

  void checkFlagsT1() {
    EXPECT_EQ(state.flags.tr1, 1) << cx;
    EXPECT_EQ(state.flags.cy1, 0) << cx;
    EXPECT_EQ(state.flags.cy0, 0) << cx;
    EXPECT_EQ(state.flags.ov, 1) << cx;
  }

  void checkFlagsNC() {
    EXPECT_EQ(state.flags.tr1, 0) << cx;
    EXPECT_EQ(state.flags.cy1, 0) << cx;
    EXPECT_EQ(state.flags.cy0, 0) << cx;
    EXPECT_EQ(state.flags.ov, 0) << cx;
  }


  void check() {
    EXPECT_EQ(state.AC[acLoc], expectAC) << cx;
    EXPECT_EQ(state.memP[opnLoc], expectMem) << cx;
  }


  void checkI() {
    EXPECT_EQ(state.AC[acLoc], expectAC) << cx;
  }


  void test(VW36 insns, CallbackFn checker, CallbackFn flagChecker) {
    state.pc.u = pc;
    state.maxInsns = 1;
    state.running = true;

    state.AC[acLoc] = a;
    state.memP[opnLoc] = b;
    unsigned dest = pc;
    for (auto insn: insns) state.memP[dest++] = insn;

    KM10{state}.emulate();
    invoke(checker, this);
    invoke(flagChecker, this);
    checkUnmodifiedFlags();
  }
};


TEST(BasicStructures, ExecutiveProcessTable) {
  ASSERT_EQ(sizeof(KMState::ExecutiveProcessTable), 512 * 8);
}

TEST(BasicStructures, UserProcessTable) {
  ASSERT_EQ(sizeof(KMState::UserProcessTable), 512 * 8);
}


class InstructionADD: public KM10Test {
};


TEST_F(InstructionADD, CY1) {
  cx = "ADD CY1";
  a = aBig;
  b = expectMem = aBig;
  result = expectAC = a.extend() + b.extend();
  test(VW36{W36(0270, acLoc, 0, 0, opnLoc)},
       &InstructionADD::check,
       &InstructionADD::checkFlagsC1);
};


TEST_F(InstructionADD, CY0) {
  cx = "ADD CY0";
  a = bNeg;
  b = expectMem = bNeg;
  result = expectAC = a.extend() + b.extend();
  test(VW36{W36(0270, acLoc, 0, 0, opnLoc)},
       &InstructionADD::check,
       &InstructionADD::checkFlagsC0);
};

TEST_F(InstructionADD, NC) {
  cx = "ADD NC";
  a = aBig;
  b = expectMem = bPos;
  result = expectAC = a.extend() + b.extend();
  test(VW36{W36(0270, acLoc, 0, 0, opnLoc)},
       &InstructionADD::check,
       &InstructionADD::checkFlagsNC);
};

TEST_F(InstructionADD, I) {
  cx = "ADDI";
  a = aBig;
  b = bPos.rhu;
  result = expectAC = a.extend() + b.extend();
  test(VW36{W36(0271, acLoc, 0, 0, b.rhu)},
       &InstructionADD::checkI,
       &InstructionADD::checkFlagsNC);
};

TEST_F(InstructionADD, M) {
  cx = "ADDM";
  a = expectAC = aBig;
  b = bPos;
  result = expectMem = a.extend() + b.extend();
  test(VW36{W36(0272, acLoc, 0, 0, opnLoc)},
       &InstructionADD::check,
       &InstructionADD::checkFlagsNC);
};

TEST_F(InstructionADD, B) {
  cx = "ADDB";
  a = aBig;
  b = bPos;
  result = expectMem = expectAC = a.extend() + b.extend();
  test(VW36{W36(0273, acLoc, 0, 0, opnLoc)},
       &InstructionADD::check,
       &InstructionADD::checkFlagsNC);
};


class InstructionSUB: public KM10Test {
};


TEST_F(InstructionSUB, CY1) {
  cx = "SUB CY1";
  a = aBig;
  b = expectMem = bNg1;
  result = expectAC = a.extend() - b.extend();
  test(VW36{W36(0274, acLoc, 0, 0, opnLoc)},
       &InstructionADD::check,
       &InstructionADD::checkFlagsC1);
};

TEST_F(InstructionSUB, CY0) {
  cx = "SUB CY0";
  a = bNg1;
  b = expectMem = bPos;
  result = expectAC = a.extend() - b.extend();
  test(VW36{W36(0274, acLoc, 0, 0, opnLoc)},
       &InstructionADD::check,
       &InstructionADD::checkFlagsC0);
};

TEST_F(InstructionSUB, NC) {
  cx = "SUB NC";
  a = aBig;
  b = expectMem = bPos;
  result = expectAC = a.extend() - b.extend();
  test(VW36{W36(0274, acLoc, 0, 0, opnLoc)},
       &InstructionADD::check,
       &InstructionADD::checkFlagsNC);
};

TEST_F(InstructionSUB, I) {
  cx = "SUBI";
  a = aBig;
  b = bPos.rhu;
  result = expectAC = a.extend() - b.extend();
  test(VW36{W36(0275, acLoc, 0, 0, b.rhu)},
       &InstructionADD::checkI,
       &InstructionADD::checkFlagsNC);
};

TEST_F(InstructionSUB, M) {
  cx = "SUBM";
  a = expectAC = aBig;
  b = bPos;
  result = expectMem = a.extend() - b.extend();
  test(VW36{W36(0276, acLoc, 0, 0, opnLoc)},
       &InstructionADD::check,
       &InstructionADD::checkFlagsNC);
};

TEST_F(InstructionSUB, B) {
  cx = "SUBB";
  a = aBig;
  b = bPos;
  result = expectAC = expectMem = a.extend() - b.extend();
  test(VW36{W36(0277, acLoc, 0, 0, opnLoc)},
       &InstructionADD::check,
       &InstructionADD::checkFlagsNC);
};


class InstructionMUL: public KM10Test {
public:
  W72 result72;
  W36 expectAC2;

  using CallbackFn72 = void (InstructionMUL::*)();


  void test(VW36 insns, CallbackFn72 checker, CallbackFn flagChecker) {
    KM10Test::test(insns, &KM10Test::noCheck, flagChecker);
    invoke(checker, this);
  }


  void check72() {
    EXPECT_EQ(state.AC[acLoc+0], expectAC) << cx;
    EXPECT_EQ(state.AC[acLoc+1], expectAC2) << cx;
    EXPECT_EQ(state.memP[opnLoc], expectMem) << cx;
  }


  void checkI72() {
    EXPECT_EQ(state.AC[acLoc+0], expectAC) << cx;
    EXPECT_EQ(state.AC[acLoc+1], expectAC2) << cx;
  }
};


TEST_F(InstructionMUL, NCpp) {
  cx = "MUL NCpp";
  a = aBig;
  b = expectMem = bPos;
  result72 = (int128_t) a.extend() * b.extend();
  expectAC = result72.hi;
  expectAC2 = result72.lo;
  test(VW36{W36(0224, acLoc, 0, 0, opnLoc)},
       &InstructionMUL::check72,
       &KM10Test::checkFlagsNC);
};

TEST_F(InstructionMUL, NCnn) {
  cx = "MUL NCnn";
  a = -aBig;
  b = expectMem = -bPos;
  result72 = (int128_t) a.extend() * b.extend();
  expectAC = result72.hi;
  expectAC2 = result72.lo;
  test(VW36{W36(0224, acLoc, 0, 0, opnLoc)},
       &InstructionMUL::check72,
       &InstructionMUL::checkFlagsNC);
};


TEST_F(InstructionMUL, NCnp) {
  cx = "MUL NCnp";
  a = -aBig;
  b = expectMem = bPos;
  result72 = (int128_t) a.extend() * b.extend();
  expectAC = result72.hi;
  expectAC2 = result72.lo;
  test(VW36{W36(0224, acLoc, 0, 0, opnLoc)},
       &InstructionMUL::check72,
       &InstructionMUL::checkFlagsNC);
};


TEST_F(InstructionMUL, NCpn) {
  cx = "MUL NCpn";
  a = aBig;
  b = expectMem = -bPos;
  result72 = (int128_t) a.extend() * b.extend();
  expectAC = result72.hi;
  expectAC2 = result72.lo;
  test(VW36{W36(0224, acLoc, 0, 0, opnLoc)},
       &InstructionMUL::check72,
       &InstructionMUL::checkFlagsNC);
}


TEST_F(InstructionMUL, TR1) {
  cx = "MUL TR1";
  a = -1ll << 35;
  b = expectMem = a;
  result72 = W72{1ull << 34, 0};
  expectAC = result72.u >> 36;
  expectAC2 = 0;
  test(VW36{W36(0224, acLoc, 0, 0, opnLoc)},
       &InstructionMUL::check72,
       &InstructionMUL::checkFlagsT1);
};

TEST_F(InstructionMUL, NC) {
  cx = "MULI NC";
  a = aBig;
  b = expectMem = W36(0, bPos.rhu);
  result72 = (int128_t) a.extend() * b.extend();
  expectAC = result72.hi;
  expectAC2 = result72.lo;
  test(VW36{W36(0225, acLoc, 0, 0, opnLoc)},
       &InstructionMUL::checkI72,
       &InstructionMUL::checkFlagsNC);
};

#if 0
  cx = "MULM";
  a = expectAC = aBig;
  b = bPos;
  result = expectMem = a.extend() * b.extend();
  insns = VW36{W36(0226, acLoc, 0, 0, opnLoc)};
  test(&InstructionMUL::checkFlagsNC);

  cx = "MULB";
  a = aBig;
  b = bPos;
  result = expectMem = expectAC = a.extend() * b.extend();
  insns = VW36{W36(0227, acLoc, 0, 0, opnLoc)};
  test(&InstructionMUL::checkFlagsNC);
#endif


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
