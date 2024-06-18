#include <assert.h>
#include <functional>

using namespace std;

#include <gtest/gtest.h>

#include "word.hpp"
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
  const unsigned pc;
  const unsigned opnLoc;
  const unsigned acLoc;

  const W36 aBig;
  const W36 aNeg;
  const W36 bNeg;
  const W36 bNg1;
  const W36 bPos;


  void noCheck() { }


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


////////////////////////////////////////////////////////////////
TEST(BasicStructures, ExecutiveProcessTable) {
  ASSERT_EQ(sizeof(KMState::ExecutiveProcessTable), 512 * 8);
}

TEST(BasicStructures, UserProcessTable) {
  ASSERT_EQ(sizeof(KMState::UserProcessTable), 512 * 8);
}


////////////////////////////////////////////////////////////////
class InstructionADD: public KM10Test {
};


TEST_F(InstructionADD, CY1) {
  a = aBig;
  b = expectMem = aBig;
  result = expectAC = a.extend() + b.extend();
  test(VW36{W36(0270, acLoc, 0, 0, opnLoc)},
       &InstructionADD::check,
       &InstructionADD::checkFlagsC1);
};


TEST_F(InstructionADD, CY0) {
  a = bNeg;
  b = expectMem = bNeg;
  result = expectAC = a.extend() + b.extend();
  test(VW36{W36(0270, acLoc, 0, 0, opnLoc)},
       &InstructionADD::check,
       &InstructionADD::checkFlagsC0);
};

TEST_F(InstructionADD, NC) {
  a = aBig;
  b = expectMem = bPos;
  result = expectAC = a.extend() + b.extend();
  test(VW36{W36(0270, acLoc, 0, 0, opnLoc)},
       &InstructionADD::check,
       &InstructionADD::checkFlagsNC);
};

TEST_F(InstructionADD, I_NC) {
  a = aBig;
  b = bPos.rhu;
  result = expectAC = a.extend() + b.extend();
  test(VW36{W36(0271, acLoc, 0, 0, b.rhu)},
       &InstructionADD::checkI,
       &InstructionADD::checkFlagsNC);
};

TEST_F(InstructionADD, M_NC) {
  a = expectAC = aBig;
  b = bPos;
  result = expectMem = a.extend() + b.extend();
  test(VW36{W36(0272, acLoc, 0, 0, opnLoc)},
       &InstructionADD::check,
       &InstructionADD::checkFlagsNC);
};

TEST_F(InstructionADD, B_NC) {
  a = aBig;
  b = bPos;
  result = expectMem = expectAC = a.extend() + b.extend();
  test(VW36{W36(0273, acLoc, 0, 0, opnLoc)},
       &InstructionADD::check,
       &InstructionADD::checkFlagsNC);
};


////////////////////////////////////////////////////////////////
class InstructionSUB: public KM10Test {
};


TEST_F(InstructionSUB, CY1) {
  a = aBig;
  b = expectMem = bNg1;
  result = expectAC = a.extend() - b.extend();
  test(VW36{W36(0274, acLoc, 0, 0, opnLoc)},
       &InstructionADD::check,
       &InstructionADD::checkFlagsC1);
};

TEST_F(InstructionSUB, CY0) {
  a = bNg1;
  b = expectMem = bPos;
  result = expectAC = a.extend() - b.extend();
  test(VW36{W36(0274, acLoc, 0, 0, opnLoc)},
       &InstructionADD::check,
       &InstructionADD::checkFlagsC0);
};

TEST_F(InstructionSUB, NC) {
  a = aBig;
  b = expectMem = bPos;
  result = expectAC = a.extend() - b.extend();
  test(VW36{W36(0274, acLoc, 0, 0, opnLoc)},
       &InstructionADD::check,
       &InstructionADD::checkFlagsNC);
};

TEST_F(InstructionSUB, I_NC) {
  a = aBig;
  b = bPos.rhu;
  result = expectAC = a.extend() - b.extend();
  test(VW36{W36(0275, acLoc, 0, 0, b.rhu)},
       &InstructionADD::checkI,
       &InstructionADD::checkFlagsNC);
};

TEST_F(InstructionSUB, M_NC) {
  a = expectAC = aBig;
  b = bPos;
  result = expectMem = a.extend() - b.extend();
  test(VW36{W36(0276, acLoc, 0, 0, opnLoc)},
       &InstructionADD::check,
       &InstructionADD::checkFlagsNC);
};

TEST_F(InstructionSUB, B_NC) {
  a = aBig;
  b = bPos;
  result = expectAC = expectMem = a.extend() - b.extend();
  test(VW36{W36(0277, acLoc, 0, 0, opnLoc)},
       &InstructionADD::check,
       &InstructionADD::checkFlagsNC);
};


////////////////////////////////////////////////////////////////
class InstructionMUL: public KM10Test {
public:
  using CallbackFn72 = void (InstructionMUL::*)(W72 result72);

  using ResultF = function<W72(W36,W36)>;

  static inline ResultF defaultResultF = [](W36 aA, W36 aB) -> W72 const {
    return W72(((int128_t) aA.extend() * (int128_t) aB.extend()));
  };

  void test(VW36 insns, CallbackFn72 checker, CallbackFn flagChecker,
	    ResultF getResultF = defaultResultF)
  {
    W72 result72{getResultF(a, b)};
    KM10Test::test(insns, &KM10Test::noCheck, flagChecker);
    invoke(checker, this, result72);
  }

  void checkI72(W72 result72) {
    EXPECT_EQ(state.AC[acLoc+0], W36(result72.hi));
    EXPECT_EQ(state.AC[acLoc+1], W36(result72.lo));
  }

  void checkM72(W72 result72) {
    EXPECT_EQ(state.memP[opnLoc], W36(result72.hi));
  }

  void checkB72(W72 result72) {
    EXPECT_EQ(state.AC[acLoc+0], W36(result72.hi));
    EXPECT_EQ(state.memP[opnLoc], W36(result72.hi));
  }

  void check72(W72 result72) {
    checkI72(result72);
    EXPECT_EQ(state.memP[opnLoc], expectMem);
  }
};


TEST_F(InstructionMUL, NCpp) {
  a = aBig;
  b = expectMem = bPos;
  test(VW36{W36(0224, acLoc, 0, 0, opnLoc)},
       &InstructionMUL::check72,
       &KM10Test::checkFlagsNC);
};

TEST_F(InstructionMUL, NCnn) {
  a = -aBig;
  b = expectMem = -bPos;
  test(VW36{W36(0224, acLoc, 0, 0, opnLoc)},
       &InstructionMUL::check72,
       &InstructionMUL::checkFlagsNC);
};


TEST_F(InstructionMUL, NCnp) {
  a = -aBig;
  b = expectMem = bPos;
  test(VW36{W36(0224, acLoc, 0, 0, opnLoc)},
       &InstructionMUL::check72,
       &InstructionMUL::checkFlagsNC);
};


TEST_F(InstructionMUL, NCpn) {
  a = aBig;
  b = expectMem = -bPos;
  test(VW36{W36(0224, acLoc, 0, 0, opnLoc)},
       &InstructionMUL::check72,
       &InstructionMUL::checkFlagsNC);
}


TEST_F(InstructionMUL, TR1) {
  a = b = expectMem = -1ll << 35;
  test(VW36{W36(0224, acLoc, 0, 0, opnLoc)},
       &InstructionMUL::check72,
       &InstructionMUL::checkFlagsT1,
       [](W36 aA, W36 aB) -> W72 const {return W72{1ull << 34, 0};});
};

TEST_F(InstructionMUL, I_NC) {
  a = aBig;
  b = expectMem = W36(0, bPos.rhu);
  test(VW36{W36(0225, acLoc, 0, 0, b.rhu)},
       &InstructionMUL::checkI72,
       &InstructionMUL::checkFlagsNC);
};

TEST_F(InstructionMUL, M_NC) {
  a = expectAC = aBig;
  b = bPos;
  test(VW36{W36(0226, acLoc, 0, 0, opnLoc)},
       &InstructionMUL::checkM72,
       &InstructionMUL::checkFlagsNC);
}

TEST_F(InstructionMUL, B_NC) {
  a = aBig;
  b = bPos;
  test(VW36{W36(0227, acLoc, 0, 0, opnLoc)},
       &InstructionMUL::checkB72,
       &InstructionMUL::checkFlagsNC);
}

////////////////////////////////////////////////////////////////
class InstructionDIV: public KM10Test {
public:
  InstructionDIV()
    : KM10Test(),
      hugePos(W36{037777u,0777777u}, W36{0,3}),
      smPos(W36{0,5})
  {}

  W72 a;
  const W72 hugePos;
  const W72 smPos;

  using CallbackFn72 = void (InstructionDIV::*)(W72 result72);

  using ResultF = function<W72(W72,W36)>;

  static inline ResultF defaultResultF = [](W72 s1, W36 s2) {
    uint128_t den70 = ((uint128_t) s1.hi35 << 35) | s1.lo35;
    auto dor = s2.mag;
    auto signBit = s1.s < 0 ? 1ull << 35 : 0ull;
    uint64_t quo = den70 / dor;
    uint64_t rem = den70 % dor;
    const uint64_t mask35 = W36::rMask(35);
    W72 ret{(quo & mask35) | signBit, (rem & mask35) | signBit};
    return ret;
  };

  virtual void setupMachine() override {
    state.AC[acLoc+0] = a.hi;
    state.AC[acLoc+1] = a.lo;
    state.memP[opnLoc] = b;
  }

  virtual void test(VW36 insns,
		    CallbackFn72 checker,
		    CallbackFn flagChecker,
		    ResultF getResultF = defaultResultF)
  {
    W72 result72{getResultF(a, b)};
    KM10Test::test(insns, &KM10Test::noCheck, flagChecker);
    invoke(checker, this, result72);
  }


  virtual void checkUnmodifiedFlags() override {
    EXPECT_EQ(state.flags.tr2 | state.flags.fuf, 0);
    EXPECT_EQ(state.flags.afi | state.flags.pub, 0);
    EXPECT_EQ(state.flags.uio | state.flags.usr, 0);
    EXPECT_EQ(state.flags.fpd | state.flags.fov, 0);
    EXPECT_EQ(state.flags.cy0 | state.flags.cy1, 0);
  }

  virtual void checkFlagsNC() override {
    EXPECT_EQ(state.flags.tr1, 0);
    EXPECT_EQ(state.flags.ndv, 0);
    EXPECT_EQ(state.flags.ov, 0);
  }

  virtual void checkFlagsT1() override {
    EXPECT_EQ(state.flags.tr1, 1);
    EXPECT_EQ(state.flags.ndv, 1);
    EXPECT_EQ(state.flags.ov, 1);
  }


  virtual void checkI72(W72 result72) {
    EXPECT_EQ(state.AC[acLoc+0], W36(result72.hi));
    EXPECT_EQ(state.AC[acLoc+1], W36(result72.lo));
  }

  virtual void checkM72(W72 result72) {
    EXPECT_EQ(state.memP[opnLoc], W36(result72.hi));
  }

  virtual void checkB72(W72 result72) {
    EXPECT_EQ(state.AC[acLoc+0], W36(result72.hi));
    EXPECT_EQ(state.memP[opnLoc], W36(result72.hi));
  }

  virtual void check72(W72 result72) {
    checkI72(result72);
    EXPECT_EQ(state.memP[opnLoc], expectMem);
  }

  virtual void check72unchanged(W72 result72) {
    EXPECT_EQ(state.memP[opnLoc], expectMem);
  }
};


TEST_F(InstructionDIV, NDV) {
  a = W72{W36{0222222, 0222222}, W36{0222222, 0222222}};
  b = expectMem = W36{0, 2};
  test(VW36{W36(0234, acLoc, 0, 0, opnLoc)},
       &InstructionDIV::check72unchanged,
       &KM10Test::checkFlagsT1);
};

TEST_F(InstructionDIV, NCpp) {
  a = W72{W36{0, 0654321}, W36{0, 7}};
  b = expectMem = W36{0303030, 0};
  test(VW36{W36(0234, acLoc, 0, 0, opnLoc)},
       &InstructionDIV::check72,
       &KM10Test::checkFlagsNC);
};

TEST_F(InstructionDIV, NCnn) {
  a = W72{W36{0400000, 0654321}, W36{0, 7}};
  b = expectMem = W36{0477777, 0303030};
  test(VW36{W36(0234, acLoc, 0, 0, opnLoc)},
       &InstructionDIV::check72,
       &KM10Test::checkFlagsNC);
};


TEST_F(InstructionDIV, NCnp) {
  a = W72{W36{0400000, 0654321}, W36{0, 7}};
  b = expectMem = W36{0303030, 0};
  test(VW36{W36(0234, acLoc, 0, 0, opnLoc)},
       &InstructionDIV::check72,
       &KM10Test::checkFlagsNC);
};


TEST_F(InstructionDIV, NCpn) {
  a = W72{W36{0, 0654321}, W36{0, 7}};
  b = expectMem = W36{0477777, 0303030};
  test(VW36{W36(0234, acLoc, 0, 0, opnLoc)},
       &InstructionDIV::check72,
       &KM10Test::checkFlagsNC);
}


TEST_F(InstructionDIV, I_NC) {
  a = W72{W36{0, 0111111}, W36{0, 7}};
  b = expectMem = W36{0, 0777777};
  test(VW36{W36(0235, acLoc, 0, 0, 0777777)},
       &InstructionDIV::checkI72,
       &KM10Test::checkFlagsNC);
};

TEST_F(InstructionDIV, M_NC) {
  a = W72{W36{0, 0654321}, W36{0, 7}};
  b = W36{0303030, 0};
  test(VW36{W36(0236, acLoc, 0, 0, opnLoc)},
       &InstructionDIV::checkM72,
       &KM10Test::checkFlagsNC);
}

TEST_F(InstructionDIV, B_NC) {
  a = W72{W36{0, 0654321}, W36{0, 7}};
  b = W36{0303030, 0};
  test(VW36{W36(0237, acLoc, 0, 0, opnLoc)},
       &InstructionDIV::checkB72,
       &KM10Test::checkFlagsNC);
}


////////////////////////////////////////////////////////////////
class InstructionIDIV: public KM10Test {
public:
  InstructionIDIV()
    : KM10Test()
  {}

  using CallbackFn72 = void (InstructionIDIV::*)(W72 result72);

  using ResultF = function<W72(W36,W36)>;

  static inline ResultF defaultResultF = [](W36 s1, W36 s2) {

    if ((s1.u == W36::bit0 && s2.s == -1ll) || s2.u == 0ull) {
      return W72{s1, s2};
    } else {
      return W72{s1 / s2, s1 % s2};
    }
  };

  virtual void test(VW36 insns,
		    CallbackFn72 checker,
		    CallbackFn flagChecker,
		    ResultF getResultF = defaultResultF)
  {
    W72 result72{getResultF(a, b)};
    KM10Test::test(insns, &KM10Test::noCheck, flagChecker);
    invoke(checker, this, result72);
  }


  virtual void checkUnmodifiedFlags() override {
    EXPECT_EQ(state.flags.tr2 | state.flags.fuf, 0);
    EXPECT_EQ(state.flags.afi | state.flags.pub, 0);
    EXPECT_EQ(state.flags.uio | state.flags.usr, 0);
    EXPECT_EQ(state.flags.fpd | state.flags.fov, 0);
    EXPECT_EQ(state.flags.cy0 | state.flags.cy1, 0);
  }

  virtual void checkFlagsNC() override {
    EXPECT_EQ(state.flags.tr1, 0);
    EXPECT_EQ(state.flags.ndv, 0);
    EXPECT_EQ(state.flags.ov, 0);
  }

  virtual void checkFlagsT1() override {
    EXPECT_EQ(state.flags.tr1, 1);
    EXPECT_EQ(state.flags.ndv, 1);
    EXPECT_EQ(state.flags.ov, 1);
  }


  virtual void checkI72(W72 result72) {
    EXPECT_EQ(state.AC[acLoc+0], W36(result72.hi));
    EXPECT_EQ(state.AC[acLoc+1], W36(result72.lo));
  }

  virtual void checkM72(W72 result72) {
    EXPECT_EQ(state.memP[opnLoc], W36(result72.hi));
  }

  virtual void checkB72(W72 result72) {
    EXPECT_EQ(state.AC[acLoc+0], W36(result72.hi));
    EXPECT_EQ(state.memP[opnLoc], W36(result72.hi));
  }

  virtual void check72(W72 result72) {
    checkI72(result72);
    EXPECT_EQ(state.memP[opnLoc], expectMem);
  }

  virtual void check72unchanged(W72 result72) {
    EXPECT_EQ(state.memP[opnLoc], expectMem);
  }
};


TEST_F(InstructionIDIV, NDV0) {
  a = W36{1ull << (35 - 18), 0};
  b = expectMem = W36{0, 0};
  test(VW36{W36(0230, acLoc, 0, 0, opnLoc)},
       &InstructionIDIV::check72unchanged,
       &KM10Test::checkFlagsT1);
};

TEST_F(InstructionIDIV, NDVbig) {
  a = W36{0400000, 0};
  b = expectMem = W36{0777777, 0777777};
  test(VW36{W36(0230, acLoc, 0, 0, opnLoc)},
       &InstructionIDIV::check72unchanged,
       &KM10Test::checkFlagsT1);
};

TEST_F(InstructionIDIV, NCpp) {
  a = W36{0654321};
  b = expectMem = W36{3};
  test(VW36{W36(0230, acLoc, 0, 0, opnLoc)},
       &InstructionIDIV::check72,
       &KM10Test::checkFlagsNC);
};

TEST_F(InstructionIDIV, NCnn) {
  a = W36{(uint64_t) -0123456};
  b = expectMem = W36{(uint64_t) -3};
  test(VW36{W36(0230, acLoc, 0, 0, opnLoc)},
       &InstructionIDIV::check72,
       &KM10Test::checkFlagsNC);
};


TEST_F(InstructionIDIV, NCnp) {
  a = W36{(uint64_t) -0123456};
  b = expectMem = W36{3};
  test(VW36{W36(0230, acLoc, 0, 0, opnLoc)},
       &InstructionIDIV::check72,
       &KM10Test::checkFlagsNC);
};


TEST_F(InstructionIDIV, NCpn) {
  a = W36{0654321};
  b = expectMem = W36{(uint64_t) -3};
  test(VW36{W36(0230, acLoc, 0, 0, opnLoc)},
       &InstructionIDIV::check72,
       &KM10Test::checkFlagsNC);
}


TEST_F(InstructionIDIV, I_NC) {
  a = W36{0, 0111111};
  b = expectMem = W36{0, 3};
  test(VW36{W36(0231, acLoc, 0, 0, 3)},
       &InstructionIDIV::checkI72,
       &KM10Test::checkFlagsNC);
};

TEST_F(InstructionIDIV, M_NC) {
  a = W36{0, 0654321};
  b = W36{0303030, 0};
  test(VW36{W36(0232, acLoc, 0, 0, opnLoc)},
       &InstructionIDIV::checkM72,
       &KM10Test::checkFlagsNC);
}

TEST_F(InstructionIDIV, B_NC) {
  a = W36{0, 0654321};
b = W36{0, 3};
  test(VW36{W36(0233, acLoc, 0, 0, opnLoc)},
       &InstructionIDIV::checkB72,
       &KM10Test::checkFlagsNC);
}

////////////////////////////////////////////////////////////////
class InstructionDADD: public KM10Test {
public:
  InstructionDADD()
    : KM10Test()
  {}

  W72 a;
  W72 b;

  using CallbackFn72 = void (InstructionDADD::*)(W72 result72);

  using ResultF = function<W72(W72,W72)>;

  static inline ResultF defaultResultF = [](W72 a1, W72 a2) {
    int128_t s1 = a1.toS70();
    int128_t s2 = a2.toS70();
    int128_t sum128 = s1 + s2;
    auto [hi36, lo36] = W72::toDW(sum128);
    return W72{hi36, lo36};
  };

  virtual void setupMachine() override {
    state.AC[acLoc+0] = a.hi;
    state.AC[acLoc+1] = a.lo;
    state.memP[opnLoc+0] = b.hi;
    state.memP[opnLoc+1] = b.lo;
  }

  virtual void test(VW36 insns,
		    CallbackFn72 checker,
		    CallbackFn flagChecker,
		    ResultF getResultF = defaultResultF)
  {
    W72 result72{getResultF(a, b)};
    KM10Test::test(insns, &KM10Test::noCheck, flagChecker);
    invoke(checker, this, result72);
  }


  virtual void checkUnmodifiedFlags() override {
    EXPECT_EQ(state.flags.tr2 | state.flags.fuf, 0);
    EXPECT_EQ(state.flags.afi | state.flags.pub, 0);
    EXPECT_EQ(state.flags.uio | state.flags.usr, 0);
    EXPECT_EQ(state.flags.fpd | state.flags.fov, 0);
  }

  virtual void checkFlagsNC() override {
    EXPECT_EQ(state.flags.tr1, 0);
    EXPECT_EQ(state.flags.ov, 0);
    EXPECT_EQ(state.flags.cy0 | state.flags.cy1, 0);
  }

  virtual void checkFlagsC0() override {
    EXPECT_EQ(state.flags.tr1, 1);
    EXPECT_EQ(state.flags.ov, 1);
    EXPECT_EQ(state.flags.cy0, 1);
  }

  virtual void checkFlagsC1() override {
    EXPECT_EQ(state.flags.tr1, 1);
    EXPECT_EQ(state.flags.ov, 1);
    EXPECT_EQ(state.flags.cy1, 1);
  }


  virtual void check72(W72 result72) {
    EXPECT_EQ(state.AC[acLoc+0], W36(result72.hi));
    EXPECT_EQ(state.AC[acLoc+1], W36(result72.lo));
  }
};


TEST_F(InstructionDADD, CY1) {
  a = W72{((uint128_t) 1 << 71) - 1};
  b = W72{((uint128_t) 1 << 71) - 1};
  test(VW36{W36(0114, acLoc, 0, 0, opnLoc)},
       &InstructionDADD::check72,
       &KM10Test::checkFlagsC1);
};

TEST_F(InstructionDADD, CY0) {
  a = W72{W72::bit0};
  b = W72{W72::bit0};
  test(VW36{W36(0114, acLoc, 0, 0, opnLoc)},
       &InstructionDADD::check72,
       &KM10Test::checkFlagsC0);
};

TEST_F(InstructionDADD, NCpp) {
  a = W72{1, 0123456654321ull};
  b = W72{7, 0654321654321ull};
  test(VW36{W36(0114, acLoc, 0, 0, opnLoc)},
       &InstructionDADD::check72,
       &KM10Test::checkFlagsNC);
};

TEST_F(InstructionDADD, NCnn) {
  a = W72{0700000, 0123456654321ull};
  b = W72{0700000, 0123456123456ull};
  test(VW36{W36(0114, acLoc, 0, 0, opnLoc)},
       &InstructionDADD::check72,
       &KM10Test::checkFlagsNC);
};


TEST_F(InstructionDADD, NCnp) {
  a = W72{0700000, 0123456654321ull};
  b = W72{0123456123456ull, 0654321654321ull};
  test(VW36{W36(0114, acLoc, 0, 0, opnLoc)},
       &InstructionDADD::check72,
       &KM10Test::checkFlagsNC);
};


TEST_F(InstructionDADD, NCpn) {
  a = W72{0123456654321ull, 0654321654321ull};
  b = W72{0700000, 0123456123456ull};
  test(VW36{W36(0114, acLoc, 0, 0, opnLoc)},
       &InstructionDADD::check72,
       &KM10Test::checkFlagsNC);
}


////////////////////////////////////////////////////////////////
class InstructionDSUB: public KM10Test {
public:
  InstructionDSUB()
    : KM10Test()
  {}

  W72 a;
  W72 b;

  using CallbackFn72 = void (InstructionDSUB::*)(W72 result72);

  using ResultF = function<W72(W72,W72)>;

  static inline ResultF defaultResultF = [](W72 a1, W72 a2) {
    int128_t s1 = a1.toS70();
    int128_t s2 = a2.toS70();
    int128_t diff128 = s1 - s2;
    auto [hi36, lo36] = W72::toDW(diff128);
    return W72{hi36, lo36};
  };

  virtual void setupMachine() override {
    state.memP[opnLoc+0] = a.hi;
    state.memP[opnLoc+1] = a.lo;
    state.AC[acLoc+0] = b.hi;
    state.AC[acLoc+1] = b.lo;
  }

  virtual void test(VW36 insns,
		    CallbackFn72 checker,
		    CallbackFn flagChecker,
		    ResultF getResultF = defaultResultF)
  {
    W72 result72{getResultF(a, b)};
    KM10Test::test(insns, &KM10Test::noCheck, flagChecker);
    invoke(checker, this, result72);
  }


  virtual void checkUnmodifiedFlags() override {
    EXPECT_EQ(state.flags.tr2 | state.flags.fuf, 0);
    EXPECT_EQ(state.flags.afi | state.flags.pub, 0);
    EXPECT_EQ(state.flags.uio | state.flags.usr, 0);
    EXPECT_EQ(state.flags.fpd | state.flags.fov, 0);
  }

  virtual void checkFlagsNC() override {
    EXPECT_EQ(state.flags.tr1, 0);
    EXPECT_EQ(state.flags.ov, 0);
    EXPECT_EQ(state.flags.cy0 | state.flags.cy1, 0);
  }

  virtual void checkFlagsC0() override {
    EXPECT_EQ(state.flags.tr1, 1);
    EXPECT_EQ(state.flags.ov, 1);
    EXPECT_EQ(state.flags.cy0, 1);
  }

  virtual void checkFlagsC1() override {
    EXPECT_EQ(state.flags.tr1, 1);
    EXPECT_EQ(state.flags.ov, 1);
    EXPECT_EQ(state.flags.cy1, 1);
  }


  virtual void check72(W72 result72) {
    EXPECT_EQ(state.AC[acLoc+0], W36(result72.hi));
    EXPECT_EQ(state.AC[acLoc+1], W36(result72.lo));
  }
};


TEST_F(InstructionDSUB, CY1) {
  a = W72{(uint128_t) 1 << 70};
  b = W72{((uint128_t) 1 << 71) + 0123456};
  test(VW36{W36(0115, acLoc, 0, 0, opnLoc)},
       &InstructionDSUB::check72,
       &KM10Test::checkFlagsC1);
};

TEST_F(InstructionDSUB, CY0) {
  a = W72{W72::bit0};
  b = W72{W72::bit0};
  test(VW36{W36(0115, acLoc, 0, 0, opnLoc)},
       &InstructionDSUB::check72,
       &KM10Test::checkFlagsC0);
};

TEST_F(InstructionDSUB, NCpp) {
  a = W72{1, 0123456654321ull};
  b = W72{7, 0654321654321ull};
  test(VW36{W36(0115, acLoc, 0, 0, opnLoc)},
       &InstructionDSUB::check72,
       &KM10Test::checkFlagsNC);
};

TEST_F(InstructionDSUB, NCnn) {
  a = W72{0700000, 0123456654321ull};
  b = W72{0700000, 0123456123456ull};
  test(VW36{W36(0115, acLoc, 0, 0, opnLoc)},
       &InstructionDSUB::check72,
       &KM10Test::checkFlagsNC);
};


TEST_F(InstructionDSUB, NCnp) {
  a = W72{0700000, 0123456654321ull};
  b = W72{0123456123456ull, 0654321654321ull};
  test(VW36{W36(0115, acLoc, 0, 0, opnLoc)},
       &InstructionDSUB::check72,
       &KM10Test::checkFlagsNC);
};


TEST_F(InstructionDSUB, NCpn) {
  a = W72{0123456654321ull, 0654321654321ull};
  b = W72{0700000, 0123456123456ull};
  test(VW36{W36(0115, acLoc, 0, 0, opnLoc)},
       &InstructionDSUB::check72,
       &KM10Test::checkFlagsNC);
}


////////////////////////////////////////////////////////////////
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
