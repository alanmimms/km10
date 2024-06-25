#pragma once

using namespace std;

#include <gtest/gtest.h>

#include "word.hpp"
#include "kmstate.hpp"
#include "km10.hpp"


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
  const unsigned pc;
  const unsigned opnLoc;
  const unsigned acLoc;

  const W36 aBig;
  const W36 aNeg;
  const W36 bNeg;
  const W36 bNg1;
  const W36 bPos;


  void noCheck() { }


  virtual void checkAllFlagsUnmodified() {
    EXPECT_EQ(state.flags.ndv | state.flags.fuf, 0);
    EXPECT_EQ(state.flags.afi | state.flags.pub, 0);
    EXPECT_EQ(state.flags.uio | state.flags.usr, 0);
    EXPECT_EQ(state.flags.fpd | state.flags.fov, 0);
    EXPECT_EQ(state.flags.tr2, 0);
    EXPECT_EQ(state.flags.tr1, 0);
    EXPECT_EQ(state.flags.cy1, 0);
    EXPECT_EQ(state.flags.cy0, 0);
    EXPECT_EQ(state.flags.ov, 0) ;
  }

  virtual void checkUnmodifiedFlags() {
    EXPECT_EQ(state.flags.ndv | state.flags.fuf, 0);
    EXPECT_EQ(state.flags.afi | state.flags.pub, 0);
    EXPECT_EQ(state.flags.uio | state.flags.usr, 0);
    EXPECT_EQ(state.flags.fpd | state.flags.fov, 0);
    EXPECT_EQ(state.flags.tr2, 0);
  }

  virtual void checkFlagsC0() {
    EXPECT_EQ(state.flags.tr1, 1);
    EXPECT_EQ(state.flags.cy1, 0);
    EXPECT_EQ(state.flags.cy0, 1);
    EXPECT_EQ(state.flags.ov, 1) ;
  }

  virtual void checkFlagsC1() {
    EXPECT_EQ(state.flags.tr1, 1);
    EXPECT_EQ(state.flags.cy1, 1);
    EXPECT_EQ(state.flags.cy0, 0);
    EXPECT_EQ(state.flags.ov, 1);
  }

  virtual void checkFlagsT1() {
    EXPECT_EQ(state.flags.tr1, 1);
    EXPECT_EQ(state.flags.cy1, 0);
    EXPECT_EQ(state.flags.cy0, 0);
    EXPECT_EQ(state.flags.ov, 1);
  }

  virtual void checkFlagsNC() {
    EXPECT_EQ(state.flags.tr1, 0);
    EXPECT_EQ(state.flags.cy1, 0);
    EXPECT_EQ(state.flags.cy0, 0);
    EXPECT_EQ(state.flags.ov, 0);
  }


  virtual void check() {
    EXPECT_EQ(state.AC[acLoc], expectAC);
    EXPECT_EQ(state.memP[opnLoc], expectMem);
  }


  virtual void checkI() {
    EXPECT_EQ(state.AC[acLoc], expectAC);
  }


  virtual void setupMachine() {
    state.AC[acLoc+0] = a;
    state.memP[opnLoc] = b;
  }

  virtual void test(VW36 insns, CallbackFn checker, CallbackFn flagChecker) {
    state.pc.u = pc;
    state.maxInsns = 1;
    state.running = true;

    setupMachine();
    unsigned dest = pc;
    for (auto insn: insns) state.memP[dest++] = insn;

    KM10{state}.emulate();
    invoke(checker, this);
    invoke(flagChecker, this);
    checkUnmodifiedFlags();
  }
};
