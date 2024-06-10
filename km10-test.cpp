#include <assert.h>
using namespace std;

#include <gtest/gtest.h>

#include "kmstate.hpp"
#include "km10.hpp"

#include "logger.hpp"


Logger logger{};


using T2 = W36::T2;
using T3 = W36::T3;


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


static void checkUnmodifiedFlags(KMState &state, string cx) {
  EXPECT_EQ(state.flags.ndv | state.flags.fuf, 0) << cx;
  EXPECT_EQ(state.flags.afi | state.flags.pub, 0) << cx;
  EXPECT_EQ(state.flags.uio | state.flags.usr, 0) << cx;
  EXPECT_EQ(state.flags.fpd | state.flags.fov, 0) << cx;
  EXPECT_EQ(state.flags.tr2, 0) << cx;
}


static void checkFlagsC0(KMState &state, string cx) {
  EXPECT_EQ(state.flags.tr1, 1) << cx;
  EXPECT_EQ(state.flags.cy1, 0) << cx;
  EXPECT_EQ(state.flags.cy0, 1) << cx;
  EXPECT_EQ(state.flags.ov, 1)  << cx;
}

static void checkFlagsC1(KMState &state, string cx) {
  EXPECT_EQ(state.flags.tr1, 1) << cx;
  EXPECT_EQ(state.flags.cy1, 1) << cx;
  EXPECT_EQ(state.flags.cy0, 0) << cx;
  EXPECT_EQ(state.flags.ov, 1) << cx;
}

static void checkFlagsNC(KMState &state, string cx) {
  EXPECT_EQ(state.flags.tr1, 0) << cx;
  EXPECT_EQ(state.flags.cy1, 0) << cx;
  EXPECT_EQ(state.flags.cy0, 0) << cx;
  EXPECT_EQ(state.flags.ov, 0) << cx;
}


TEST_F(InstructionTest, ADD) {
  const unsigned pc = 01000;
  const unsigned opnLoc = 02000;
  const unsigned acLoc = 5;
  string cx;

  auto addTest = [&](W36 insn,
		     T3 values,	// {a,b,result}
		     function<void(KMState&,T3,string)> checker)
  {
    KMState state(512 * 1024);

    state.AC[acLoc] = get<0>(values);
    state.memP[opnLoc] = get<1>(values);
    state.memP[pc] = insn;

    state.pc.u = pc;
    state.maxInsns = 1;
    state.running = true;
    KM10{state}.emulate();

    checkUnmodifiedFlags(state, cx);
    checker(state, values, cx);
  };

  const W36 aBig(0377777u, 0654321u);
  const W36 aNeg(0765432u, 0555555u);
  const W36 bNeg(0400000u, 0123456u);
  const W36 bPos(0000000u, 0123456u);

  cx = "ADD CY1";
  addTest(W36(0270, acLoc, 0, 0, opnLoc),
	  T3{aBig, aBig, aBig.extend() + aBig.extend()},
	  [&](KMState &state, T3 values, string cx) {
	    EXPECT_EQ(state.AC[acLoc], get<2>(values)) << cx;
	    EXPECT_EQ(state.memP[opnLoc], get<0>(values)) << cx;
	    checkFlagsC1(state, cx);
	  });

  cx = "ADD CY0";
  addTest(W36(0270, acLoc, 0, 0, opnLoc), 
	  T3{bNeg, bNeg, bNeg.extend() + bNeg.extend()},
	  [&](KMState &state, T3 values, string cx) {
	    EXPECT_EQ(state.AC[acLoc], get<2>(values)) << cx;
	    EXPECT_EQ(state.memP[opnLoc], get<0>(values)) << cx;
	    checkFlagsC0(state, cx);
	  });

  cx = "ADD NC";
  addTest(W36(0270, acLoc, 0, 0, opnLoc),
	  T3{aBig, bPos, aBig.extend() + bPos.extend()},
	  [&](KMState &state, T3 values, string cx) {
	    EXPECT_EQ(state.AC[acLoc], get<2>(values)) << cx;
	    EXPECT_EQ(state.memP[opnLoc], get<1>(values)) << cx;
	    checkFlagsNC(state, cx);
	  });

  cx = "ADDI";
  addTest(W36(0271, acLoc, 0, 0, bPos.rhu),
	  T3{aBig, bPos.rhu, aBig.extend() + W36(bPos.rhu).extend()},
	  [&](KMState &state, T3 values, string cx) {
	    EXPECT_EQ(state.AC[acLoc], get<2>(values)) << cx;
	    EXPECT_EQ(state.memP[opnLoc], W36(0, get<1>(values).rhu)) << cx;
	    checkFlagsNC(state, cx);
	  });

  cx = "ADDM";
  addTest(W36(0272, acLoc, 0, 0, opnLoc), 
	  T3{aBig, bPos, aBig.extend() + bPos.extend()},
	  [&](KMState &state, T3 values, string cx) {
	    EXPECT_EQ(state.AC[acLoc], get<0>(values)) << cx;
	    EXPECT_EQ(state.memP[opnLoc], get<2>(values)) << cx;
	    checkFlagsNC(state, cx);
	  });

  cx = "ADDB";
  addTest(W36(0273, acLoc, 0, 0, opnLoc),
	  T3{aBig, bPos, aBig.extend() + bPos.extend()},
	  [&](KMState &state, T3 values, string cx) {
	    EXPECT_EQ(state.AC[acLoc], get<2>(values)) << cx;
	    EXPECT_EQ(state.memP[opnLoc], get<2>(values)) << cx;
	    checkFlagsNC(state, cx);
	  });
}


TEST_F(InstructionTest, SUB) {
  const unsigned pc = 01000;
  const unsigned opnLoc = 02000;
  const unsigned acLoc = 5;
  string cx;

  auto subTest = [&](W36 insn,
		     T3 values,	// {a,b,result}
		     function<void(KMState&,T3,string)> checker)
  {
    KMState state(512 * 1024);

    state.AC[acLoc] = get<0>(values);
    state.memP[opnLoc] = get<1>(values);
    state.memP[pc] = insn;

    state.pc.u = pc;
    state.maxInsns = 1;
    state.running = true;
    KM10{state}.emulate();

    checkUnmodifiedFlags(state, cx);
    checker(state, values, cx);
  };

  const W36 aBig(0377777u, 0654321u);
  const W36 aNeg(0765432u, 0555555u);
  const W36 bNeg(0400000u, 0111111u);
  const W36 bPos(0000000u, 0123456u);

  cx = "SUB CY1";
  subTest(W36(0274, acLoc, 0, 0, opnLoc),
	  T3{aBig, bNeg, aBig.extend() - bNeg.extend()},
	  [&](KMState &state, T3 values, string cx) {
	    EXPECT_EQ(state.AC[acLoc], get<2>(values)) << cx;
	    EXPECT_EQ(state.memP[opnLoc], get<1>(values)) << cx;
	    checkFlagsC1(state, cx);
	  });

  cx = "SUB CY0";
  subTest(W36(0274, acLoc, 0, 0, opnLoc), 
	  T3{bNeg, bPos, bNeg.extend() - bPos.extend()},
	  [&](KMState &state, T3 values, string cx) {
	    EXPECT_EQ(state.AC[acLoc], get<2>(values)) << cx;
	    EXPECT_EQ(state.memP[opnLoc], get<1>(values)) << cx;
	    checkFlagsC0(state, cx);
	  });

  cx = "SUB NC";
  subTest(W36(0274, acLoc, 0, 0, opnLoc),
	  T3{aBig, bPos, aBig.extend() - bPos.extend()},
	  [&](KMState &state, T3 values, string cx) {
	    EXPECT_EQ(state.AC[acLoc], get<2>(values)) << cx;
	    EXPECT_EQ(state.memP[opnLoc], get<1>(values)) << cx;
	    checkFlagsNC(state, cx);
	  });

  cx = "SUBI";
  subTest(W36(0275, acLoc, 0, 0, bPos.rhu),
	  T3{aBig, bPos.rhu, aBig.extend() - W36(bPos.rhu).extend()},
	  [&](KMState &state, T3 values, string cx) {
	    EXPECT_EQ(state.AC[acLoc], get<2>(values)) << cx;
	    EXPECT_EQ(state.memP[opnLoc], W36(0, get<1>(values).rhu)) << cx;
	    checkFlagsNC(state, cx);
	  });

  cx = "SUBM";
  subTest(W36(0276, acLoc, 0, 0, opnLoc), 
	  T3{aBig, bPos, aBig.extend() - bPos.extend()},
	  [&](KMState &state, T3 values, string cx) {
	    EXPECT_EQ(state.AC[acLoc], get<0>(values)) << cx;
	    EXPECT_EQ(state.memP[opnLoc], get<2>(values)) << cx;
	    checkFlagsNC(state, cx);
	  });

  cx = "SUBB";
  subTest(W36(0277, acLoc, 0, 0, opnLoc),
	  T3{aBig, bPos, aBig.extend() - bPos.extend()},
	  [&](KMState &state, T3 values, string cx) {
	    EXPECT_EQ(state.AC[acLoc], get<2>(values)) << cx;
	    EXPECT_EQ(state.memP[opnLoc], get<2>(values)) << cx;
	    checkFlagsNC(state, cx);
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
