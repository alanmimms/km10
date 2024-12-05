// These are tests that verify proper PC changes for instructions of
// various classes, traps, interrupts, XCT and XCT chains, and HALT.
#include <assert.h>
#include <functional>

using namespace std;

#include <gtest/gtest.h>

#include "word.hpp"
#include "km10.hpp"


////////////////////////////////////////////////////////////////
struct PCTest: testing::Test {
  PCTest()
    : km10{1024*1024}
  { }

  KM10 km10;

  unsigned pcSB;
  unsigned flagsSB;

  enum Opcodes {
    JRST = 0254,
  };
};


TEST_F(PCTest, JRST) {
  km10.pc = 01000;
  km10.flags = 0;

  pcSB = 02000;
  flagsSB = km10.flags;
  km10.memP[km10.pc.rhu] = W36{JRST, 0, 0, 0, pcSB};	// JRST
  km10.memP[pcSB] = W36{JRST, 4, 0, 0, pcSB};		// HALT (should not execute)

  km10.nSteps = 1;
  km10.emulate();

  EXPECT_EQ(km10.pc.rhu, pcSB);
  EXPECT_EQ(km10.flags, flagsSB);
};

