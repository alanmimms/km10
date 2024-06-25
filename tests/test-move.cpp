#include <assert.h>
#include <functional>

using namespace std;

#include <gtest/gtest.h>

#include "word.hpp"
#include "kmstate.hpp"
#include "km10.hpp"
#include "km10-test.hpp"


////////////////////////////////////////////////////////////////
class InstructionEXCH: public KM10Test {
};


TEST_F(InstructionEXCH, Basic) {
  a = expectMem = aBig;
  b = expectAC = aNeg;
  test(VW36{W36(0250, acLoc, 0, 0, opnLoc)},
       &InstructionEXCH::check,
       &InstructionEXCH::checkAllFlagsUnmodified);
};
