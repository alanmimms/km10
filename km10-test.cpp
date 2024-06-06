#include <assert.h>
using namespace std;

#include <gtest/gtest.h>

#include "kmstate.hpp"
#include "km10.hpp"

#include "logger.hpp"


Logger logger{};


TEST(BasicStructures, ExecutiveProcessTable) {
  ASSERT_EQ(sizeof(KMState::ExecutiveProcessTable), 512 * 8);
}

TEST(BasicStructures, UserProcessTable) {
  ASSERT_EQ(sizeof(KMState::UserProcessTable), 512 * 8);
}

TEST(ADDInstruction, ADDFlavor) {
  W36 a(0123456u, 654321u);
  W36 b(0654321u, 123456u);
  KMState state(256 * 1024);

  // ADD
  state.AC[5] = a;
  EXPECT_EQ(state.AC[5], a);

  state.memP[0100] = b;
  EXPECT_EQ(state.memP[0100], b);

  state.memP[01000] = W36(0270, 5, 0, 0, 0100);
  EXPECT_EQ(state.memP[01000], W36((0270u << 9) | (5 << 5), 0100));

  state.pc.u = 01000;
  state.flags.u = 0;
  state.maxInsns = 1;
  KM10 km10(state);
  km10.emulate();

  EXPECT_EQ(state.AC[5], W36(a.u + b.u));
  EXPECT_EQ(state.memP[0100], b);

  EXPECT_EQ(state.flags.ndv | state.flags.fuf, 0);
  EXPECT_EQ(state.flags.afi | state.flags.pub, 0);
  EXPECT_EQ(state.flags.uio | state.flags.usr, 0);
  EXPECT_EQ(state.flags.fpd | state.flags.fov, 0);

  EXPECT_EQ(state.flags.tr1, 0);
  EXPECT_EQ(state.flags.tr2, 0);
  EXPECT_EQ(state.flags.cy1, 0);
  EXPECT_EQ(state.flags.cy0, 0);
  EXPECT_EQ(state.flags.ov, 0);
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
