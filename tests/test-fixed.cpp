#include <assert.h>
#include <functional>

using namespace std;

#include <gtest/gtest.h>

#include "word.hpp"
#include "km10.hpp"
#include "km10-test.hpp"


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
  result = expectAC = a.ext64() + b.ext64();
  test(VW36{W36(0270, acLoc, 0, 0, opnLoc)},
       &InstructionADD::check,
       &InstructionADD::checkFlagsC1);
};


TEST_F(InstructionADD, CY0) {
  a = bNeg;
  b = expectMem = bNeg;
  result = expectAC = a.ext64() + b.ext64();
  test(VW36{W36(0270, acLoc, 0, 0, opnLoc)},
       &InstructionADD::check,
       &InstructionADD::checkFlagsC0);
};

TEST_F(InstructionADD, NC) {
  a = aBig;
  b = expectMem = bPos;
  result = expectAC = a.ext64() + b.ext64();
  test(VW36{W36(0270, acLoc, 0, 0, opnLoc)},
       &InstructionADD::check,
       &InstructionADD::checkFlagsNC);
};

TEST_F(InstructionADD, I_NC) {
  a = aBig;
  b = bPos.rhu;
  result = expectAC = a.ext64() + b.ext64();
  test(VW36{W36(0271, acLoc, 0, 0, b.rhu)},
       &InstructionADD::checkI,
       &InstructionADD::checkFlagsNC);
};

TEST_F(InstructionADD, M_NC) {
  a = expectAC = aBig;
  b = bPos;
  result = expectMem = a.ext64() + b.ext64();
  test(VW36{W36(0272, acLoc, 0, 0, opnLoc)},
       &InstructionADD::check,
       &InstructionADD::checkFlagsNC);
};

TEST_F(InstructionADD, B_NC) {
  a = aBig;
  b = bPos;
  result = expectMem = expectAC = a.ext64() + b.ext64();
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
  result = expectAC = a.ext64() - b.ext64();
  test(VW36{W36(0274, acLoc, 0, 0, opnLoc)},
       &InstructionADD::check,
       &InstructionADD::checkFlagsC1);
};

TEST_F(InstructionSUB, CY0) {
  a = bNg1;
  b = expectMem = bPos;
  result = expectAC = a.ext64() - b.ext64();
  test(VW36{W36(0274, acLoc, 0, 0, opnLoc)},
       &InstructionADD::check,
       &InstructionADD::checkFlagsC0);
};

TEST_F(InstructionSUB, NC) {
  a = aBig;
  b = expectMem = bPos;
  result = expectAC = a.ext64() - b.ext64();
  test(VW36{W36(0274, acLoc, 0, 0, opnLoc)},
       &InstructionADD::check,
       &InstructionADD::checkFlagsNC);
};

TEST_F(InstructionSUB, I_NC) {
  a = aBig;
  b = bPos.rhu;
  result = expectAC = a.ext64() - b.ext64();
  test(VW36{W36(0275, acLoc, 0, 0, b.rhu)},
       &InstructionADD::checkI,
       &InstructionADD::checkFlagsNC);
};

TEST_F(InstructionSUB, M_NC) {
  a = expectAC = aBig;
  b = bPos;
  result = expectMem = a.ext64() - b.ext64();
  test(VW36{W36(0276, acLoc, 0, 0, opnLoc)},
       &InstructionADD::check,
       &InstructionADD::checkFlagsNC);
};

TEST_F(InstructionSUB, B_NC) {
  a = aBig;
  b = bPos;
  result = expectAC = expectMem = a.ext64() - b.ext64();
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
    int128_t prod = (int128_t) aA.ext64() * (int128_t) aB.ext64();
    return W72::fromMag(prod < 0 ? -prod : prod, prod < 0);
  };

  virtual void test(VW36 insns, CallbackFn72 checker, CallbackFn flagChecker,
		    ResultF getResultF = defaultResultF)
  {
    W72 result72{getResultF(a, b)};
    KM10Test::test(insns, &KM10Test::noCheck, flagChecker);
    invoke(checker, this, result72);
  }

  void checkI72(W72 result72) {
    EXPECT_EQ(km10.AC[acLoc+0], W36(result72.hi));
    EXPECT_EQ(km10.AC[acLoc+1], W36(result72.lo));
  }

  void checkM72(W72 result72) {
    EXPECT_EQ(km10.memP[opnLoc], W36(result72.hi));
  }

  void checkB72(W72 result72) {
    EXPECT_EQ(km10.AC[acLoc+0], W36(result72.hi));
    EXPECT_EQ(km10.memP[opnLoc], W36(result72.hi));
  }

  void check72(W72 result72) {
    checkI72(result72);
    EXPECT_EQ(km10.memP[opnLoc], expectMem);
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
    : KM10Test()
  {}

  W72 a;

  using CallbackFn72 = void (InstructionDIV::*)(W72 result72);

  using ResultF = function<W72(W72,W36)>;

  static inline ResultF defaultResultF = [](const W72 s1, const W36 s2) {
    const uint128_t den70 = ((uint128_t) s1.hi35 << 35) | s1.lo35;
    const auto dor = s2.mag;
    const int isNeg = s1.s < 0;
    const uint64_t quo = den70 / dor;
    const uint64_t rem = den70 % dor;
    W72 ret{W36::fromMag(quo, isNeg), W36::fromMag(rem, isNeg)};
    return ret;
  };

  virtual void setupMachine() override {
    km10.AC[acLoc+0] = a.hi;
    km10.AC[acLoc+1] = a.lo;
    km10.memP[opnLoc] = b;
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
    EXPECT_EQ(km10.flags.tr2 | km10.flags.fuf, 0);
    EXPECT_EQ(km10.flags.afi | km10.flags.pub, 0);
    EXPECT_EQ(km10.flags.uio | km10.flags.usr, 0);
    EXPECT_EQ(km10.flags.fpd | km10.flags.fov, 0);
    EXPECT_EQ(km10.flags.cy0 | km10.flags.cy1, 0);
  }

  virtual void checkFlagsNC() override {
    EXPECT_EQ(km10.flags.tr1, 0);
    EXPECT_EQ(km10.flags.ndv, 0);
    EXPECT_EQ(km10.flags.ov, 0);
  }

  virtual void checkFlagsT1() override {
    EXPECT_EQ(km10.flags.tr1, 1);
    EXPECT_EQ(km10.flags.ndv, 1);
    EXPECT_EQ(km10.flags.ov, 1);
  }


  virtual void checkI72(W72 result72) {
    EXPECT_EQ(km10.AC[acLoc+0], W36(result72.hi));
    EXPECT_EQ(km10.AC[acLoc+1], W36(result72.lo));
  }

  virtual void checkM72(W72 result72) {
    EXPECT_EQ(km10.memP[opnLoc], W36(result72.hi));
  }

  virtual void checkB72(W72 result72) {
    EXPECT_EQ(km10.AC[acLoc+0], W36(result72.hi));
    EXPECT_EQ(km10.memP[opnLoc], W36(result72.hi));
  }

  virtual void check72(W72 result72) {
    checkI72(result72);
    EXPECT_EQ(km10.memP[opnLoc], expectMem);
  }

  virtual void check72unchanged(W72 result72) {
    EXPECT_EQ(km10.memP[opnLoc], expectMem);
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

    if (s2.u == 0ull || (s1.u == W36::bit0 && s2.s == -1ll)) {
      return W72{s1, s2};
    } else {
      int64_t quo = s1.s / s2.s;
      int64_t rem = abs(s1.s % s2.s);
      if (quo < 0) rem = -rem;
      return W72{W36{quo}, W36{abs(rem)}};
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
    EXPECT_EQ(km10.flags.tr2 | km10.flags.fuf, 0);
    EXPECT_EQ(km10.flags.afi | km10.flags.pub, 0);
    EXPECT_EQ(km10.flags.uio | km10.flags.usr, 0);
    EXPECT_EQ(km10.flags.fpd | km10.flags.fov, 0);
    EXPECT_EQ(km10.flags.cy0 | km10.flags.cy1, 0);
  }

  virtual void checkFlagsNC() override {
    EXPECT_EQ(km10.flags.tr1, 0);
    EXPECT_EQ(km10.flags.ndv, 0);
    EXPECT_EQ(km10.flags.ov, 0);
  }

  virtual void checkFlagsT1() override {
    EXPECT_EQ(km10.flags.tr1, 1);
    EXPECT_EQ(km10.flags.ndv, 1);
    EXPECT_EQ(km10.flags.ov, 1);
  }


  virtual void checkI72(W72 result72) {
    EXPECT_EQ(km10.AC[acLoc+0], W36(result72.hi));
    EXPECT_EQ(km10.AC[acLoc+1], W36(result72.lo));
  }

  virtual void checkM72(W72 result72) {
    EXPECT_EQ(km10.memP[opnLoc], W36(result72.hi));
  }

  virtual void checkB72(W72 result72) {
    EXPECT_EQ(km10.AC[acLoc+0], W36(result72.hi));
    EXPECT_EQ(km10.memP[opnLoc], W36(result72.hi));
  }

  virtual void check72(W72 result72) {
    checkI72(result72);
    EXPECT_EQ(km10.memP[opnLoc], expectMem);
  }

  virtual void check72unchanged(W72 result72) {
    EXPECT_EQ(km10.memP[opnLoc], expectMem);
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
  b = expectMem = W36{11};
  test(VW36{W36(0230, acLoc, 0, 0, opnLoc)},
       &InstructionIDIV::check72,
       &KM10Test::checkFlagsNC);
};

TEST_F(InstructionIDIV, NCnn) {
  a = W36{-0123456ll};
  b = expectMem = W36{-11ll};
  test(VW36{W36(0230, acLoc, 0, 0, opnLoc)},
       &InstructionIDIV::check72,
       &KM10Test::checkFlagsNC);
};


TEST_F(InstructionIDIV, NCnp) {
  a = W36{-0123456ll};
  b = expectMem = W36{11};
  test(VW36{W36(0230, acLoc, 0, 0, opnLoc)},
       &InstructionIDIV::check72,
       &KM10Test::checkFlagsNC);
};


TEST_F(InstructionIDIV, NCpn) {
  a = W36{0654321};
  b = expectMem = W36{-11ll};
  test(VW36{W36(0230, acLoc, 0, 0, opnLoc)},
       &InstructionIDIV::check72,
       &KM10Test::checkFlagsNC);
}


TEST_F(InstructionIDIV, I_NC) {
  a = W36{0u, 0111111u};
  b = expectMem = W36{0, 3};
  test(VW36{W36(0231, acLoc, 0, 0, 3)},
       &InstructionIDIV::checkI72,
       &KM10Test::checkFlagsNC);
};

TEST_F(InstructionIDIV, M_NC) {
  a = W36{0u, 0654321u};
  b = W36{0303030u, 0u};
  test(VW36{W36(0232, acLoc, 0, 0, opnLoc)},
       &InstructionIDIV::checkM72,
       &KM10Test::checkFlagsNC);
}

TEST_F(InstructionIDIV, B_NC) {
  a = W36{0u, 0654321u};
  b = W36{0u, 11u};
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
    km10.AC[acLoc+0] = a.hi;
    km10.AC[acLoc+1] = a.lo;
    km10.memP[opnLoc+0] = b.hi;
    km10.memP[opnLoc+1] = b.lo;
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
    EXPECT_EQ(km10.flags.tr2 | km10.flags.fuf, 0);
    EXPECT_EQ(km10.flags.afi | km10.flags.pub, 0);
    EXPECT_EQ(km10.flags.uio | km10.flags.usr, 0);
    EXPECT_EQ(km10.flags.fpd | km10.flags.fov, 0);
  }

  virtual void checkFlagsNC() override {
    EXPECT_EQ(km10.flags.tr1, 0);
    EXPECT_EQ(km10.flags.ov, 0);
    EXPECT_EQ(km10.flags.cy0, 0);
    EXPECT_EQ(km10.flags.cy1, 0);
  }

  virtual void checkFlagsC0() override {
    EXPECT_EQ(km10.flags.tr1, 1);
    EXPECT_EQ(km10.flags.ov, 1);
    EXPECT_EQ(km10.flags.cy0, 1);
  }

  virtual void checkFlagsC1() override {
    EXPECT_EQ(km10.flags.tr1, 1);
    EXPECT_EQ(km10.flags.ov, 1);
    EXPECT_EQ(km10.flags.cy1, 1);
  }


  virtual void check72(W72 result72) {
    EXPECT_EQ(km10.AC[acLoc+0], W36(result72.hi));
    EXPECT_EQ(km10.AC[acLoc+1], W36(result72.lo));
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
  a = W72{W36{1}, W36{0123456u, 0654321u}};
  b = W72{W36{7}, W36{0654321u, 0654321u}};
  test(VW36{W36(0114, acLoc, 0, 0, opnLoc)},
       &InstructionDADD::check72,
       &KM10Test::checkFlagsNC);
};

TEST_F(InstructionDADD, NCnn) {
  a = W72{W36{0700000u}, W36{0123456u, 0654321u}};
  b = W72{W36{0700000u}, W36{012345u, 06123456u}};
  test(VW36{W36(0114, acLoc, 0, 0, opnLoc)},
       &InstructionDADD::check72,
       &KM10Test::checkFlagsNC);
};


TEST_F(InstructionDADD, NCnp) {
  a = W72{W36{0700000u}, W36{0123456u, 0654321u}};
  b = W72{W36{0123456u, 0123456u}, W36{0654321u, 0654321u}};
  test(VW36{W36(0114, acLoc, 0, 0, opnLoc)},
       &InstructionDADD::check72,
       &KM10Test::checkFlagsNC);
};


TEST_F(InstructionDADD, NCpn) {
  a = W72{W36{0123456u, 0654321u}, W36{0654321u, 0654321u}};
  b = W72{W36{0700000u}, W36{012345u, 06123456u}};
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
    km10.memP[opnLoc+0] = a.hi;
    km10.memP[opnLoc+1] = a.lo;
    km10.AC[acLoc+0] = b.hi;
    km10.AC[acLoc+1] = b.lo;
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
    EXPECT_EQ(km10.flags.tr2 | km10.flags.fuf, 0);
    EXPECT_EQ(km10.flags.afi | km10.flags.pub, 0);
    EXPECT_EQ(km10.flags.uio | km10.flags.usr, 0);
    EXPECT_EQ(km10.flags.fpd | km10.flags.fov, 0);
  }

  virtual void checkFlagsNC() override {
    EXPECT_EQ(km10.flags.tr1, 0);
    EXPECT_EQ(km10.flags.ov, 0);
    EXPECT_EQ(km10.flags.cy0, 0);
    EXPECT_EQ(km10.flags.cy1, 0);
  }

  virtual void checkFlagsC0() override {
    EXPECT_EQ(km10.flags.tr1, 1);
    EXPECT_EQ(km10.flags.ov, 1);
    EXPECT_EQ(km10.flags.cy0, 1);
  }

  virtual void checkFlagsC1() override {
    EXPECT_EQ(km10.flags.tr1, 1);
    EXPECT_EQ(km10.flags.ov, 1);
    EXPECT_EQ(km10.flags.cy1, 1);
  }


  virtual void check72(W72 result72) {
    EXPECT_EQ(km10.AC[acLoc+0], W36(result72.hi));
    EXPECT_EQ(km10.AC[acLoc+1], W36(result72.lo));
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
  a = W72{W36{1}, W36{0123456u, 0654321u}};
  b = W72{W36{7}, W36{0654321u, 0654321u}};
  test(VW36{W36(0115, acLoc, 0, 0, opnLoc)},
       &InstructionDSUB::check72,
       &KM10Test::checkFlagsNC);
};

TEST_F(InstructionDSUB, NCnn) {
  a = W72{W36{0700000u}, W36{0123456u, 0654321u}};
  b = W72{W36{0700000u}, W36{0123456u, 0123456u}};
  test(VW36{W36(0115, acLoc, 0, 0, opnLoc)},
       &InstructionDSUB::check72,
       &KM10Test::checkFlagsNC);
};


TEST_F(InstructionDSUB, NCnp) {
  a = W72{W36{0700000u}, W36{0123456u, 0654321u}};
  b = W72{W36{0123456u, 0123456u}, W36{0654321u, 0654321u}};
  test(VW36{W36(0115, acLoc, 0, 0, opnLoc)},
       &InstructionDSUB::check72,
       &KM10Test::checkFlagsNC);
};


TEST_F(InstructionDSUB, NCpn) {
  a = W72{W36{0123456u, 0654321u}, W36{0654321u, 0654321u}};
  b = W72{W36{0700000u}, W36{0123456u, 0123456u}};
  test(VW36{W36(0115, acLoc, 0, 0, opnLoc)},
       &InstructionDSUB::check72,
       &KM10Test::checkFlagsNC);
}


////////////////////////////////////////////////////////////////
class InstructionDMUL: public KM10Test {
public:
  InstructionDMUL()
    : KM10Test()
  {}

  W72 a;
  W72 b;

  using CallbackFn140 = void (InstructionDMUL::*)(W144 result140);
  using CallbackFn = void (InstructionDMUL::*)();

  using ResultF = function<W144(W72,W72)>;

  static inline ResultF defaultResultF = [](W72 a, W72 b) {
    return W144::product(a.toU70(), b.toU70(), (a.s < 0) ^ (b.s < 0));
  };

  virtual void setupMachine() override {
    km10.AC[acLoc+0] = a.hi;
    km10.AC[acLoc+1] = a.lo;
    km10.memP[opnLoc+0] = b.hi;
    km10.memP[opnLoc+1] = b.lo;
  }

  virtual void test(VW36 insns,
		    CallbackFn140 checker,
		    CallbackFn flagChecker,
		    ResultF getResultF = defaultResultF)
  {
    W144 result140(getResultF(a, b));
    KM10Test::test(insns, &KM10Test::noCheck, (KM10Test::CallbackFn) flagChecker);
    invoke(checker, this, result140);
  }


  virtual void checkUnmodifiedFlags() override {
    EXPECT_EQ(km10.flags.tr2 | km10.flags.fuf, 0);
    EXPECT_EQ(km10.flags.afi | km10.flags.pub, 0);
    EXPECT_EQ(km10.flags.uio | km10.flags.usr, 0);
    EXPECT_EQ(km10.flags.fpd | km10.flags.fov, 0);
    EXPECT_EQ(km10.flags.cy0, 0);
    EXPECT_EQ(km10.flags.cy1, 0);
  }

  virtual void checkFlagsNC() override {
    EXPECT_EQ(km10.flags.tr1, 0);
    EXPECT_EQ(km10.flags.ov, 0);
  }

  virtual void checkFlagsT1() override {
    EXPECT_EQ(km10.flags.tr1, 1);
    EXPECT_EQ(km10.flags.ov, 1);
  }


  virtual void check140(W144 result140) {
    auto const [a0, a1, a2, a3] = result140.toQuadWord();
    EXPECT_EQ(km10.AC[acLoc+0], a0);
    EXPECT_EQ(km10.AC[acLoc+1], a1);
    EXPECT_EQ(km10.AC[acLoc+2], a2);
    EXPECT_EQ(km10.AC[acLoc+3], a3);
  }


  virtual void check140T1(W144 result140) {
    const W36 big1{0400000,0};
    EXPECT_EQ(km10.AC[acLoc+0], big1);
    EXPECT_EQ(km10.AC[acLoc+1], big1);
    EXPECT_EQ(km10.AC[acLoc+2], big1);
    EXPECT_EQ(km10.AC[acLoc+3], big1);
  }
};


TEST_F(InstructionDMUL, T1) {
  const W36 bit0{1ull << 35};
  a = W72{bit0, bit0};
  b = W72{bit0, bit0};
  test(VW36{W36(0116, acLoc, 0, 0, opnLoc)},
       &InstructionDMUL::check140T1,
       &InstructionDMUL::checkFlagsT1);
};

TEST_F(InstructionDMUL, NCpp) {
  a = W72{077777ul};
  b = W72{055555ul};
  test(VW36{W36(0116, acLoc, 0, 0, opnLoc)},
       &InstructionDMUL::check140,
       &InstructionDMUL::checkFlagsNC);
};

TEST_F(InstructionDMUL, NCnn) {
  a = W72{W36{0700000}, W36{0123456u, 0654321u}};
  b = W72{W36{0700000}, W36{0123456u, 0123456u}};
  test(VW36{W36(0116, acLoc, 0, 0, opnLoc)},
       &InstructionDMUL::check140,
       &InstructionDMUL::checkFlagsNC);
};


TEST_F(InstructionDMUL, NCnp) {
  a = W72{W36{0700000}, W36{0123456u, 0654321u}};
  b = W72{W36{0123456u, 0123456u}, W36{0654321u, 0654321u}};
  test(VW36{W36(0116, acLoc, 0, 0, opnLoc)},
       &InstructionDMUL::check140,
       &InstructionDMUL::checkFlagsNC);
};


TEST_F(InstructionDMUL, NCpn) {
  a = W72{W36{0123456u, 0654321u}, W36{0654321u, 0654321u}};
  b = W72{W36{0700000u}, W36{0123456u, 0123456u}};
  test(VW36{W36(0116, acLoc, 0, 0, opnLoc)},
       &InstructionDMUL::check140,
       &InstructionDMUL::checkFlagsNC);
}
