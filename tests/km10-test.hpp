#pragma once
#include <vector>

using namespace std;

#include <gtest/gtest.h>

#include "word.hpp"
#include "km10.hpp"


using VW36 = vector<W36>;


struct KM10Test: testing::Test {
public:

  KM10Test()
    : km10{1024*1024},
      pc(01000),
      opnLoc(02000),
      acLoc(5),
      aBig(0377777u, 0654321u),
      aNeg(0765432u, 0555555u),
      bNeg(0400000u, 0123456u),
      bNg1(0400000u, 0111111u),
      bPos(0000000u, 0123456u)
  { }


  using CallbackFn = void (KM10Test::*)();

  KM10 km10;
  W36 a;
  W36 b;
  W36 result;

  W36 expectAC;
  W36 expectMem;

  unsigned pc;
  unsigned opnLoc;
  unsigned acLoc;

  const W36 aBig;
  const W36 aNeg;
  const W36 bNeg;
  const W36 bNg1;
  const W36 bPos;


  void noCheck() { }


  virtual void checkAllFlagsUnmodified() {
    EXPECT_EQ(km10.flags.ndv | km10.flags.fuf, 0);
    EXPECT_EQ(km10.flags.afi | km10.flags.pub, 0);
    EXPECT_EQ(km10.flags.uio | km10.flags.usr, 0);
    EXPECT_EQ(km10.flags.fpd | km10.flags.fov, 0);
    EXPECT_EQ(km10.flags.tr2, 0);
    EXPECT_EQ(km10.flags.tr1, 0);
    EXPECT_EQ(km10.flags.cy1, 0);
    EXPECT_EQ(km10.flags.cy0, 0);
    EXPECT_EQ(km10.flags.ov, 0) ;
  }

  virtual void checkUnmodifiedFlags() {
    EXPECT_EQ(km10.flags.ndv | km10.flags.fuf, 0);
    EXPECT_EQ(km10.flags.afi | km10.flags.pub, 0);
    EXPECT_EQ(km10.flags.uio | km10.flags.usr, 0);
    EXPECT_EQ(km10.flags.fpd | km10.flags.fov, 0);
    EXPECT_EQ(km10.flags.tr2, 0);
  }

  virtual void checkFlagsC0() {
    EXPECT_EQ(km10.flags.tr1, 1);
    EXPECT_EQ(km10.flags.cy1, 0);
    EXPECT_EQ(km10.flags.cy0, 1);
    EXPECT_EQ(km10.flags.ov, 1) ;
  }

  virtual void checkFlagsC1() {
    EXPECT_EQ(km10.flags.tr1, 1);
    EXPECT_EQ(km10.flags.cy1, 1);
    EXPECT_EQ(km10.flags.cy0, 0);
    EXPECT_EQ(km10.flags.ov, 1);
  }

  virtual void checkFlagsT1() {
    EXPECT_EQ(km10.flags.tr1, 1);
    EXPECT_EQ(km10.flags.cy1, 0);
    EXPECT_EQ(km10.flags.cy0, 0);
    EXPECT_EQ(km10.flags.ov, 1);
  }

  virtual void checkFlagsNC() {
    EXPECT_EQ(km10.flags.tr1, 0);
    EXPECT_EQ(km10.flags.cy1, 0);
    EXPECT_EQ(km10.flags.cy0, 0);
    EXPECT_EQ(km10.flags.ov, 0);
  }


  virtual void check() {
    EXPECT_EQ(km10.AC[acLoc], expectAC);
    EXPECT_EQ(km10.memP[opnLoc], expectMem);
  }


  virtual void checkI() {
    EXPECT_EQ(km10.AC[acLoc], expectAC);
  }


  virtual void setupMachine() {
    km10.AC[acLoc+0] = a;
    km10.memP[opnLoc] = b;
  }

  virtual void test(VW36 insns, CallbackFn checker, CallbackFn flagChecker) {
    km10.pc.u = pc;
    km10.maxInsns = 1;
    km10.running = true;

    setupMachine();
    unsigned dest = pc;
    for (auto insn: insns) km10.memP[dest++] = insn;

    km10.emulate();
    invoke(checker, this);
    invoke(flagChecker, this);
    checkUnmodifiedFlags();
  }
};
