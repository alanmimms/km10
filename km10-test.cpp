#include <assert.h>
using namespace std;

#include <gtest/gtest.h>

#include "kmstate.hpp"
#include "km10.hpp"

#include "logger.hpp"


Logger logger{};


using T2 = W36::T2;


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
  const unsigned opnLoc = 02000;
  const unsigned acLoc = 5;
  string cx;

  auto checkUnmodifiedFlags = [&](KMState &state) {
    EXPECT_EQ(state.flags.ndv | state.flags.fuf, 0) << cx;
    EXPECT_EQ(state.flags.afi | state.flags.pub, 0) << cx;
    EXPECT_EQ(state.flags.uio | state.flags.usr, 0) << cx;
    EXPECT_EQ(state.flags.fpd | state.flags.fov, 0) << cx;
  };

  auto checkFlagsC0 = [&](KMState &state) {
    EXPECT_EQ(state.flags.tr1, 1) << cx;
    EXPECT_EQ(state.flags.tr2, 0) << cx;
    EXPECT_EQ(state.flags.cy1, 0) << cx;
    EXPECT_EQ(state.flags.cy0, 1) << cx;
    EXPECT_EQ(state.flags.ov, 1)  << cx;
  };

  auto checkFlagsC1 = [&](KMState &state) {
    EXPECT_EQ(state.flags.tr1, 1) << cx;
    EXPECT_EQ(state.flags.tr2, 0) << cx;
    EXPECT_EQ(state.flags.cy1, 1) << cx;
    EXPECT_EQ(state.flags.cy0, 0) << cx;
    EXPECT_EQ(state.flags.ov, 1) << cx;
  };

  auto checkFlagsNC = [&](KMState &state) {
    EXPECT_EQ(state.flags.tr1, 0) << cx;
    EXPECT_EQ(state.flags.tr2, 0) << cx;
    EXPECT_EQ(state.flags.cy1, 0) << cx;
    EXPECT_EQ(state.flags.cy0, 0) << cx;
    EXPECT_EQ(state.flags.ov, 0) << cx;
  };

  auto runTest = [&](W36 insn,
		     T2 inVec,
		     function<void(KMState&,W36,T2)> checker)
  {
    KMState state(512 * 1024);
    state.maxInsns = 0;
    KM10 km10(state);

    state.running = true;

    auto [a, b] = inVec;
    W36 sum(a.extend() + b.extend());
    state.AC[acLoc] = a;
    state.memP[opnLoc] = b;
    state.memP[pc] = insn;

    state.pc.u = pc;
    state.maxInsns = 1;
    km10.emulate();

    checkUnmodifiedFlags(state);
    checker(state, sum, inVec);

    return sum;
  };

  const W36 aBig(0377777u, 0654321u);
  const W36 aNeg(0765432u, 0555555u);
  const W36 bNeg(0400000u, 0123456u);
  const W36 bPos(0000000u, 0123456u);
  W36 sum;

  cx = "ADD CY1";
  runTest(W36(0270, acLoc, 0, 0, opnLoc),
	  T2{aBig, aBig},
	  [&](KMState &state, W36 sum, T2 inVec) {
	    EXPECT_EQ(state.AC[acLoc], sum);
	    EXPECT_EQ(state.memP[opnLoc], get<0>(inVec));
	    checkFlagsC1(state);
	  });

  cx = "ADD CY0";
  runTest(W36(0270, acLoc, 0, 0, opnLoc), 
	  T2{bNeg, bNeg},
	  [&](KMState &state, W36 sum, T2 inVec) {
	    EXPECT_EQ(state.AC[acLoc], sum);
	    EXPECT_EQ(state.memP[opnLoc], get<0>(inVec));
	    checkFlagsC0(state);
	  });

  cx = "ADD NC";
  runTest(W36(0270, acLoc, 0, 0, opnLoc),
	  T2{aBig, bPos},
	  [&](KMState &state, W36 sum, T2 inVec) {
	    EXPECT_EQ(state.AC[acLoc], sum);
	    EXPECT_EQ(state.memP[opnLoc], get<1>(inVec));
	    checkFlagsNC(state);
	  });

  cx = "ADDI";
  runTest(W36(0271, acLoc, 0, 0, bPos.rhu),
	  T2{aBig, bPos.rhu},
	  [&](KMState &state, W36 sum, T2 inVec) {
	    EXPECT_EQ(state.AC[acLoc], sum);
	    EXPECT_EQ(state.memP[opnLoc], W36(0, get<1>(inVec).rhu));
	    checkFlagsNC(state);
	  });

  cx = "ADDM";
  runTest(W36(0272, acLoc, 0, 0, opnLoc), 
	  T2{aBig, bPos},
	  [&](KMState &state, W36 sum, T2 inVec) {
	    EXPECT_EQ(state.AC[acLoc], get<0>(inVec));
	    EXPECT_EQ(state.memP[opnLoc], sum);
	    checkFlagsNC(state);
	  });

  cx = "ADDB";
  runTest(W36(0273, acLoc, 0, 0, opnLoc), T2{aBig, bPos},
	  [&](KMState &state, W36 sum, T2 inVec) {
	    EXPECT_EQ(state.AC[acLoc], sum);
	    EXPECT_EQ(state.memP[opnLoc], sum);
	  });
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
