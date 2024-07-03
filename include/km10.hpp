// This is the KM10 CPU implementation.

// TODO:
//
// * Globally use U32,S32, U64,S64, U128,S128 typedefs instead of
//   verbose <cstdint> names.

#pragma once
#include <string>
#include <cstdint>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <ostream>
#include <fmt/core.h>
#include <limits>
#include <functional>
#include <assert.h>

using namespace std;
using namespace fmt;


#include "logger.hpp"
#include "word.hpp"
#include "kmstate.hpp"
#include "bytepointer.hpp"
#include "device.hpp"
#include "apr.hpp"
#include "pag.hpp"
#include "pi.hpp"
#include "dte20.hpp"


class KM10 {
public:
  KMState &state;

  APRDevice apr;
  PIDevice pi;
  PAGDevice pag;
  DTE20 dte;
  Device noDevice;


  // Constructors
  KM10(KMState &aState)
    : state(aState),
      apr(state),
      pi(state),
      pag(state),
      dte(040, state),
      noDevice(0777777ul, "?NoDevice?", aState)
  {}

  
  // I find these a lot less ugly in the emulator...
  using WFunc = function<W36()>;
  using DFunc = function<W72()>;
  using FuncW = function<void(W36)>;
  using WFuncW = function<W36(W36)>;
  using FuncWW = function<void(W36,W36)>;
  using FuncD = function<void(W72)>;
  using WFuncWW = function<W36(W36,W36)>;
  using DFuncWW = function<W72(W36,W36)>;
  using DFuncDW = function<W72(W72,W36)>;
  using BoolPredW = function<bool(W36)>;
  using BoolPredWW = function<bool(W36,W36)>;
  using VoidFunc = function<void()>;


  template <class S1, class S2, class M, class D>
  void doBinOp(S1 &doGetSrc1F, S2 &doGetSrc2F, M &doModifyF, D &doPutDstF) {
    auto result = doModifyF(doGetSrc1F(), doGetSrc2F());
    if (!state.flags.ndv) doPutDstF(result);
  }


  void logFlow(const char *msg) {
    if (logger.pc) logger.s << " [" << msg << "]";
  }


  ////////////////////////////////////////////////////////////////////////////////
  // The instruction emulator. Call this to start, step, or continue
  // running.
  void emulate() {
    W36 iw{};
    W36 ea{};
    W36 nextPC = state.pc;
    W36 tmp;
    uint64_t nInsnsThisTime = 0;

    WFunc acGet = [&]() {
      return state.acGetN(iw.ac);
    };

    WFunc acGetRH = [&]() -> W36 {
      W36 value{0, acGet().rhu};
      if (logger.mem) logger.s << "; acRH" << oct << iw.ac << ": " << value.fmt18();
      return value;
    };

    // This retrieves LH into the RH of the return value, which is
    // required for things like TLNN to work properly since they use
    // the EA as a mask.
    WFunc acGetLH = [&]() -> W36 {
      W36 value{0, acGet().lhu};
      if (logger.mem) logger.s << "; acLH" << oct << iw.ac << ": " << value.fmt18();
      return value;
    };

    FuncW acPut = [&](W36 value) -> void {
      state.acPutN(value, iw.ac);
    };

    FuncW acPutRH = [&](W36 value) -> void {
      acPut(W36(acGet().lhu, value.rhu));
    };

    // This is used to store back in, e.g., TLZE. But the LH of AC is
    // in the RH of the word we're passed because of how the testing
    // logic of these instructions must work. So we put the RH of the
    // value into the LH of the AC, keeping the AC's RH intact.
    FuncW acPutLH = [&](W36 value) -> void {
      acPut(W36(value.rhu, acGet().rhu));
    };

    DFunc acGet2 = [&]() {
      W72 ret{state.acGetN(iw.ac+0), state.acGetN(iw.ac+1)};
      return ret;
    };

    FuncD acPut2 = [&](W72 v) -> void {
      state.acPutN(v.hi, iw.ac+0);
      state.acPutN(v.lo, iw.ac+1);
    };

    WFunc memGet = [&]() -> W36 {
      return state.memGetN(ea);
    };

    FuncW memPut = [&](W36 value) -> void {
      state.memPutN(value, ea);
    };

    FuncW selfPut = [&](W36 value) -> void {
      memPut(value);
      if (iw.ac != 0) acPut(value);
    };

    FuncW bothPut = [&](W36 value) -> void {
      acPut(value);
      memPut(value);
    };

    FuncD bothPut2 = [&](W72 v) -> void {
      state.acPutN(v.hi, iw.ac+0);
      state.acPutN(v.lo, iw.ac+1);
      memPut(v.hi);
    };

    WFuncW swap = [&](W36 src) -> W36 {return W36{src.rhu, src.lhu};};

    WFuncW negate = [&](W36 src) -> W36 {
      W36 v(-src.s);
      if (src.u == W36::bit0) state.flags.tr1 = state.flags.ov = state.flags.cy1 = 1;
      if (src.u == 0) state.flags.cy0 = state.flags.cy1 = 1;
      return v;
    };

    WFuncW magnitude = [&](W36 src) -> W36 {
      W36 v(src.s < 0 ? -src.s : src.s);
      if (src.u == W36::bit0) state.flags.tr1 = state.flags.ov = state.flags.cy1 = 1;
      return v;
    };

    WFunc memGetSwapped = [&]() {return swap(memGet());};

    FuncD memPutHi = [&](W72 v) {
      memPut(v.hi);
    };

    WFunc immediate = [&]() {return W36(state.pc.isSection0() ? 0 : ea.lhu, ea.rhu);};

    // Condition testing predicates
    BoolPredW isLT0  = [&](W36 v) -> bool const {return v.s  < 0;};
    BoolPredW isLE0  = [&](W36 v) -> bool const {return v.s <= 0;};
    BoolPredW isGT0  = [&](W36 v) -> bool const {return v.s  > 0;};
    BoolPredW isGE0  = [&](W36 v) -> bool const {return v.s >= 0;};
    BoolPredW isNE0  = [&](W36 v) -> bool const {return v.s != 0;};
    BoolPredW isEQ0  = [&](W36 v) -> bool const {return v.s == 0;};
    BoolPredW always = [&](W36 v) -> bool const {return  true;};
    BoolPredW never  = [&](W36 v) -> bool const {return false;};

    BoolPredWW isNE0T  = [&](W36 a, W36 b) -> bool const {return (a.u & b.u) != 0;};
    BoolPredWW isEQ0T  = [&](W36 a, W36 b) -> bool const {return (a.u & b.u) == 0;};
    BoolPredWW alwaysT = [&](W36 a, W36 b) -> bool const {return  true;};
    BoolPredWW neverT  = [&](W36 a, W36 b) -> bool const {return false;};

    auto doJUMP = [&](BoolPredW &condF) -> void {

      if (condF(acGet())) {
	logFlow("jump");
	nextPC.rhu = ea;
      }
    };

    auto doSKIP = [&](BoolPredW &condF) -> void {
      W36 eaw = memGet();

      if (condF(eaw)) {
	logFlow("skip");
	++nextPC.rhu;
      }
      
      if (iw.ac != 0) acPut(eaw);
    };

    WFunc getE = [&]() -> W36 {return ea;};

    WFuncW  noMod1 = [&](W36 a) -> auto const {return a;};
    WFuncWW noMod2 = [&](W36 a, W36 b) -> auto const {return a;};

    // There is no `zeroMaskL`, `compMaskR`, `onesMaskL` because,
    // e.g., TLZE operates on the LH of the AC while it's in the RH of
    // the value so the testing/masking work properly.
    WFuncWW zeroMaskR = [&](W36 a, W36 b) -> auto const {return a.u & ~(uint64_t) b.rhu;};
    WFuncWW zeroMask  = [&](W36 a, W36 b) -> auto const {return a.u & ~b.u;};

    WFuncWW onesMaskR = [&](W36 a, W36 b) -> auto const {return a.u | b.rhu;};
    WFuncWW onesMask  = [&](W36 a, W36 b) -> auto const {return a.u | b.u;};

    WFuncWW compMaskR = [&](W36 a, W36 b) -> auto const {return a.u ^ b.rhu;};
    WFuncWW compMask  = [&](W36 a, W36 b) -> auto const {return a.u ^ b.u;};

    WFuncW zeroWord = [&](W36 a) -> auto const {return 0;};
    WFuncW onesWord = [&](W36 a) -> auto const {return W36::all1s;};
    WFuncW compWord = [&](W36 a) -> auto const {return ~a.u;};

    FuncW noStore = [](W36 toSrc) -> void {};

    auto doTXXXX = [&](WFunc &doGet1F,
		       WFunc &doGet2F,
		       WFuncWW &doModifyF,
		       BoolPredWW &condF,
		       FuncW &doStoreF) -> void
    {
      W36 a1 = doGet1F();
      W36 a2 = doGet2F();

      if (condF(a1, a2)) {
	logFlow("skip");
	++nextPC.rhu;
      }
      
      doStoreF(doModifyF(a1, a2));
    };

    auto doPush = [&](W36 v, W36 acN) -> void {
      W36 ac = state.acGetN(acN);

      if (state.pc.isSection0() || ac.lhs < 0 || (ac.lhu & 0007777) == 0) {
	ac = W36(ac.lhu + 1, ac.rhu + 1);

	if (ac.lhu == 0)
	  state.flags.tr2 = 1;
	else			// Correct? Don't access memory for full stack?
	  state.memPutN(v, ac.rhu);
      } else {
	ac = ac + 1;
	state.memPutN(ac.vma, v);
      }

      state.acPutN(ac, acN);
    };

    auto doPop = [&](unsigned acN) -> W36 {
      W36 ac = state.acGetN(acN);
      W36 poppedWord;

      if (state.pc.isSection0() || ac.lhs < 0 || (ac.lhu & 0007777) == 0) {
	poppedWord = state.memGetN(ac.rhu);
	ac = W36(ac.lhu - 1, ac.rhu - 1);
	if (ac.lhs == -1) state.flags.tr2 = 1;
      } else {
	poppedWord = state.memGetN(ac.vma);
	ac = ac - 1;
      }

      state.acPutN(ac, acN);
      return poppedWord;
    };


    // For a given low halfword, this computes an upper halfword by
    // extending the low halfword's sign.
    auto extnOf = [&](const unsigned v) -> unsigned const {
      return (v & 0400'000) ? W36::halfOnes : 0u;
    };


    // doCopyF functions
    WFuncWW copyHRR = [&](W36 src, W36 dst) -> auto const {return W36(dst.lhu, src.rhu);};
    WFuncWW copyHRL = [&](W36 src, W36 dst) -> auto const {return W36(src.rhu, dst.rhu);};
    WFuncWW copyHLL = [&](W36 src, W36 dst) -> auto const {return W36(src.lhu, dst.rhu);};
    WFuncWW copyHLR = [&](W36 src, W36 dst) -> auto const {return W36(dst.lhu, src.lhu);};

    // doModifyF functions
    WFuncW zeroR = [&](W36 v) -> auto const {return W36(v.lhu, 0);};
    WFuncW onesR = [&](W36 v) -> auto const {return W36(v.lhu, W36::halfOnes);};
    WFuncW extnR = [&](W36 v) -> auto const {return W36(extnOf(v.rhu), v.rhu);};
    WFuncW zeroL = [&](W36 v) -> auto const {return W36(0, v.rhu);};
    WFuncW onesL = [&](W36 v) -> auto const {return W36(W36::halfOnes, v.rhu);};
    WFuncW extnL = [&](W36 v) -> auto const {return W36(v.lhu, extnOf(v.lhu));};

    // binary doModifyF functions
    WFuncWW andWord = [&](W36 s1, W36 s2) -> auto const {return s1.u & s2.u;};
    WFuncWW andCWord = [&](W36 s1, W36 s2) -> auto const {return s1.u & ~s2.u;};
    WFuncWW andCBWord = [&](W36 s1, W36 s2) -> auto const {return ~s1.u & ~s2.u;};
    WFuncWW iorWord = [&](W36 s1, W36 s2) -> auto const {return s1.u | s2.u;};
    WFuncWW iorCWord = [&](W36 s1, W36 s2) -> auto const {return s1.u | ~s2.u;};
    WFuncWW iorCBWord = [&](W36 s1, W36 s2) -> auto const {return ~s1.u | ~s2.u;};
    WFuncWW xorWord = [&](W36 s1, W36 s2) -> auto const {return s1.u ^ s2.u;};
    WFuncWW xorCWord = [&](W36 s1, W36 s2) -> auto const {return s1.u ^ ~s2.u;};
    WFuncWW xorCBWord = [&](W36 s1, W36 s2) -> auto const {return ~s1.u ^ ~s2.u;};
    WFuncWW eqvWord = [&](W36 s1, W36 s2) -> auto const {return ~(s1.u ^ s2.u);};
    WFuncWW eqvCWord = [&](W36 s1, W36 s2) -> auto const {return ~(s1.u ^ ~s2.u);};
    WFuncWW eqvCBWord = [&](W36 s1, W36 s2) -> auto const {return ~(~s1.u ^ ~s2.u);};

    WFuncWW addWord = [&](W36 s1, W36 s2) -> auto const {
      int64_t sum = s1.ext64() + s2.ext64();

      if (sum < -(int64_t) W36::bit0) {
	state.flags.tr1 = state.flags.ov = state.flags.cy0 = 1;
      } else if ((uint64_t) sum >= W36::bit0) {
	state.flags.tr1 = state.flags.ov = state.flags.cy1 = 1;
      } else {

	if (s1.s < 0 && s2.s < 0) {
	  state.flags.cy0 = state.flags.cy1 = 1;
	} else if ((s1.s < 0) != (s2.s < 0)) {
	  const uint64_t mag1 = abs(s1.s);
	  const uint64_t mag2 = abs(s2.s);

	  if ((s1.s >= 0 && mag1 >= mag2) ||
	      (s2.s >= 0 && mag2 >= mag1)) {
	    state.flags.cy0 = state.flags.cy1 = 1;
	  }
	}
      }

      return sum;
    };
    
    WFuncWW subWord = [&](W36 s1, W36 s2) -> auto const {
      int64_t diff = s1.ext64() - s2.ext64();

      if (diff < -(int64_t) W36::bit0) {
	state.flags.tr1 = state.flags.ov = state.flags.cy0 = 1;
      } else if ((uint64_t) diff >= W36::bit0) {
	state.flags.tr1 = state.flags.ov = state.flags.cy1 = 1;
      }

      return diff;
    };
    
    DFuncWW mulWord = [&](W36 s1, W36 s2) -> auto const {
      int128_t prod128 = (int128_t) s1.ext64() * s2.ext64();
      W72 prod = W72::fromMag((uint128_t) (prod128 < 0 ? -prod128 : prod128), prod128 < 0);

      if (s1.u == W36::bit0 && s2.u == W36::bit0) {
	state.flags.tr1 = state.flags.ov = 1;
	return W72{W36{1ull << 34}, W36{0}};
      }

      return prod;
    };
    
    WFuncWW imulWord = [&](W36 s1, W36 s2) -> auto const {
      int128_t prod128 = (int128_t) s1.ext64() * s2.ext64();
      W72 prod = W72::fromMag((uint128_t) (prod128 < 0 ? -prod128 : prod128), prod128 < 0);

      if (s1.u == W36::bit0 && s2.u == W36::bit0) {
	state.flags.tr1 = state.flags.ov = 1;
      }

      return W36((prod.s < 0 ? W36::bit0 : 0) | ((W36::all1s >> 1) & prod.u));
    };
    
    DFuncWW idivWord = [&](W36 s1, W36 s2) -> auto const {

      if ((s1.u == W36::bit0 && s2.s == -1ll) || s2.u == 0ull) {
	state.flags.ndv = state.flags.tr1 = state.flags.ov = 1;
	return W72{s1, s2};
      } else {
	int64_t quo = s1.s / s2.s;
	int64_t rem = abs(s1.s % s2.s);
	if (quo < 0) rem = -rem;
	return W72{W36{quo}, W36{abs(rem)}};
      }
    };
    
    DFuncDW divWord = [&](W72 s1, W36 s2) -> auto const {
      uint128_t den70 = ((uint128_t) s1.hi35 << 35) | s1.lo35;
      auto dor = s2.mag;
      auto signBit = s1.s < 0 ? 1ull << 35 : 0ull;

      if (s1.hi35 >= s2.mag || s2.u == 0) {
	state.flags.ndv = state.flags.tr1 = state.flags.ov = 1;
	return s1;
      } else {
	int64_t quo = den70 / dor;
	int64_t rem = den70 % dor;
	W72 ret{
	  W36{(int64_t) ((quo & W36::magMask) | signBit)},
	  W36{(int64_t) ((rem & W36::magMask) | signBit)}};
	return ret;
      }
    };
    
    auto doHXXXX = [&](WFunc &doGetSrcF,
		       WFunc &doGetDstF,
		       WFuncWW &doCopyF,
		       WFuncW &doModifyF,
		       FuncW &doPutDstF) -> void
    {
      doPutDstF(doModifyF(doCopyF(doGetSrcF(), doGetDstF())));
    };


    auto doMOVXX = [&](WFunc &doGetSrcF,
		       WFuncW &doModifyF,
		       FuncW &doPutDstF) -> void
    {
      auto preFlags = state.flags;
      W36 result = doModifyF(doGetSrcF());

      // Don't modify registers if we trapped in this instruction.
      if (state.flags.tr1 && !preFlags.tr1) return;

      doPutDstF(result);
    };


    auto doSETXX = [&](WFunc &doGetSrcF,
		       WFuncW &doModifyF,
		       FuncW &doPutDstF) -> void
    {
      doPutDstF(doModifyF(doGetSrcF()));
    };


    // Binary comparison predicates
    BoolPredWW isLT    = [&](W36 v1, W36 v2) -> bool const {return v1.ext64() <  v2.ext64();};
    BoolPredWW isLE    = [&](W36 v1, W36 v2) -> bool const {return v1.ext64() <= v2.ext64();};
    BoolPredWW isGT    = [&](W36 v1, W36 v2) -> bool const {return v1.ext64() >  v2.ext64();};
    BoolPredWW isGE    = [&](W36 v1, W36 v2) -> bool const {return v1.ext64() >= v2.ext64();};
    BoolPredWW isNE    = [&](W36 v1, W36 v2) -> bool const {return v1.ext64() != v2.ext64();};
    BoolPredWW isEQ    = [&](W36 v1, W36 v2) -> bool const {return v1.ext64() == v2.ext64();};
    BoolPredWW always2 = [&](W36 v1, W36 v2) -> bool const {return true;};
    BoolPredWW never2  = [&](W36 v1, W36 v2) -> bool const {return false;};

    auto doCAXXX = [&](WFunc &doGetSrc1F,
		       WFunc &doGetSrc2F,
		       BoolPredWW &condF) -> void
    {

      if (condF(doGetSrc1F(), doGetSrc2F())) {
	logFlow("skip");
	++nextPC.rhu;
      }
    };


    VoidFunc skipAction = [&] {++nextPC.u;};
    VoidFunc jumpAction = [&] {nextPC.u = ea;};

    auto doAOSXX = [&](WFunc &doGetF,
		       const signed delta,
		       FuncW &doPutF,
		       BoolPredW &condF,
		       VoidFunc &actionF) -> void
    {
      W36 v = doGetF();

      if (delta > 0) {		// Increment

	if (v.u == W36::all1s >> 1) {
	  state.flags.tr1 = state.flags.ov  = state.flags.cy1 = 1;
	} else if (v.ext64() == -1) {
	  state.flags.cy0 = state.flags.cy1 = 1;
	}
      } else {			// Decrement

	if (v.u == W36::bit0) {
	  state.flags.tr1 = state.flags.ov = state.flags.cy0 = 1;
	} else if (v.u != 0) {
	  state.flags.cy0 = state.flags.cy1 = 1;
	}
      }

      v.s += delta;

      if (iw.ac != 0) acPut(v);
      doPutF(v);

      if (condF(v)) actionF();
    };


    ////////////////////////////////////////////////////////////////
    // Connect our DTE20 (put console into raw mode)
    dte.connect();

    if ((state.flags.tr1 || state.flags.tr2) && pag.pagerEnabled()) {
      iw = state.flags.tr1 ? state.eptP->trap1Insn : state.eptP->stackOverflowInsn;
    } else {
      iw = state.memGetN(state.pc.vma);
    }

    // The instruction loop
    do {
      nextPC.lhu = state.pc.lhu;
      nextPC.rhu = state.pc.rhu + 1;

      // Set maxInsns to zero for infinite. This is NOT in the XCT
      // pathway so we can XCT an instruction while stepping.
      if ((state.maxInsns != 0 && state.nInsns >= state.maxInsns) ||
	  state.executeBPs.contains(state.pc.vma))
	{
	  state.running = false;
	  if (nInsnsThisTime != 0) break;
	}

    XCT_ENTRYPOINT:
      // When we XCT we have already set PC to point to the
      // instruction to be XCTed and nextPC is pointing after the XCT.

      ++state.nInsns;
      ++nInsnsThisTime;

      if (logger.loggingToFile && logger.pc) logger.s << state.pc.fmtVMA() << ": " << iw.dump();

      ea.u = state.getEA(iw.i, iw.x, iw.y);

      switch (iw.op) {

      case 0114: {		 // DADD
	auto a1 = W72{state.memGetN(ea.u+0), state.memGetN(ea.u+1)};
	auto a2 = W72{state.acGetN(iw.ac+0), state.acGetN(iw.ac+1)};

	int128_t s1 = a1.toS70();
	int128_t s2 = a2.toS70();
	uint128_t u1 = a1.toU70();
	uint128_t u2 = a2.toU70();
	auto isNeg1 = s1 < 0;
	auto isNeg2 = s2 < 0;
	int128_t sum128 = s1 + s2;

	if (sum128 >= W72::sBit1) {
	  state.flags.cy1 = state.flags.tr1 = state.flags.ov = 1;
	} else if (sum128 < -W72::sBit1) {
	  state.flags.cy0 = state.flags.tr1 = state.flags.ov = 1;
	} else if ((s1 < 0 && s2 < 0) ||
		   (isNeg1 != isNeg2 &&
		    (u1 == u2 || ((!isNeg1 && u1 > u2) || (!isNeg2 && u2 > u1)))))
	{
	  state.flags.cy0 = state.flags.cy1 = state.flags.tr1 = state.flags.ov = 1;
	}

	auto [hi36, lo36] = W72::toDW(sum128);
	state.acPutN(hi36, iw.ac+0);
	state.acPutN(lo36, iw.ac+1);
	break;
      }

      case 0115: {		 // DSUB
	auto a1 = W72{state.memGetN(ea.u+0), state.memGetN(ea.u+1)};
	auto a2 = W72{state.acGetN(iw.ac+0), state.acGetN(iw.ac+1)};

	int128_t s1 = a1.toS70();
	int128_t s2 = a2.toS70();
	uint128_t u1 = a1.toU70();
	uint128_t u2 = a2.toU70();
	auto isNeg1 = s1 < 0;
	auto isNeg2 = s2 < 0;
	int128_t diff128 = s1 - s2;

	if (diff128 >= W72::sBit1) {
	  state.flags.cy1 = state.flags.tr1 = state.flags.ov = 1;
	} else if (diff128 < -W72::sBit1) {
	  state.flags.cy0 = state.flags.tr1 = state.flags.ov = 1;
	} else if ((isNeg1 && isNeg2 && u2 >= u1) ||
		   (isNeg1 != isNeg2 && s2 < 0))
	{
	  state.flags.cy0 = state.flags.cy1 = state.flags.tr1 = state.flags.ov = 1;
	}

	auto [hi36, lo36] = W72::toDW(diff128);
	state.acPutN(hi36, iw.ac+0);
	state.acPutN(lo36, iw.ac+1);
	break;
      }

      case 0116: {		 // DMUL
	auto a = W72{state.memGetN(ea.u+0), state.memGetN(ea.u+1)};
	auto b = W72{state.acGetN(iw.ac+0), state.acGetN(iw.ac+1)};
	const uint128_t a70 = a.toU70();
	const uint128_t b70 = b.toU70();

	if (a.isMaxNeg() && b.isMaxNeg()) {
	  const W36 big1{0400000,0};
	  state.flags.tr1 = state.flags.ov = 1;
	  state.acPutN(big1, iw.ac+0);
	  state.acPutN(big1, iw.ac+1);
	  state.acPutN(big1, iw.ac+2);
	  state.acPutN(big1, iw.ac+3);
	  return;
	}

	W144 prod{W144::product(a70, b70, (a.s < 0) ^ (b.s < 0))};
	auto [r0, r1, r2, r3] = prod.toQuadWord();
	state.acPutN(r0, iw.ac+0);
	state.acPutN(r1, iw.ac+1);
	state.acPutN(r2, iw.ac+2);
	state.acPutN(r3, iw.ac+3);
	break;
      }

      case 0117: {		 // DDIV
	const W144 den{
	  state.acGetN(iw.ac+0),
	  state.acGetN(iw.ac+1),
	  state.acGetN(iw.ac+2),
	  state.acGetN(iw.ac+3)};
	const W72 div72{state.memGetN(ea.u+0), state.memGetN(ea.u+1)};
	auto const div = div72.toU70();

	if (den >= div) {
	  state.flags.tr1 = state.flags.ov = state.flags.ndv = 1;
	  return;
	}

	int denNeg = den.sign;
	int divNeg = div72.hiSign;

	/*
	  Divide 192 bit n2||n1||n0 by d, returning remainder in rem.
	  performs : (n2||n1||0) = ((n2||n1||n0) / d)
	  d : a 128bit unsigned integer
	*/
	const uint128_t lo70 = den.lowerU70();
	const uint128_t hi70 = den.upperU70();
	uint64_t n0 = lo70;
	uint64_t n1 = hi70 | (lo70 >> 64);
	uint64_t n2 = hi70 >> 6;

	uint128_t remainder = n2 % div;
	n2 = n2 / div;
	uint128_t partial = (remainder << 64) | n1;
	n1 = partial / div;
	remainder = partial % div;
	partial = (remainder << 64) | n0;
	n0 = partial / div;

	const auto quo72 = W72::fromMag(((uint128_t) n1 << 64) | n0, denNeg ^ divNeg);
	const auto rem72 = W72::fromMag(remainder, denNeg);

	state.acPutN(quo72.hi, iw.ac+0);
	state.acPutN(quo72.lo, iw.ac+1);
	state.acPutN(rem72.hi, iw.ac+2);
	state.acPutN(rem72.lo, iw.ac+3);
	break;
      }

      case 0133: {		// IBP/ADJBP
	BytePointer *bp = BytePointer::makeFrom(ea, state);

	if (iw.ac == 0) {	// IBP
	  bp->inc(state);
	} else {		// ADJBP
	  bp->adjust(iw.ac, state);
	}

	break;
      }

      case 0134: {		// ILBP
	BytePointer *bp = BytePointer::makeFrom(ea, state);
	bp->inc(state);
	acPut(bp->getByte(state));
	break;
      }

      case 0135: {		// LDB
	BytePointer *bp = BytePointer::makeFrom(ea, state);
	acPut(bp->getByte(state));
	break;
      }

      case 0136: {		// IDPB
	BytePointer *bp = BytePointer::makeFrom(ea, state);
	bp->inc(state);
	bp->putByte(acGet(), state);
	break;
      }

      case 0137: {		// DPB
	BytePointer *bp = BytePointer::makeFrom(ea, state);
	bp->putByte(acGet(), state);
	break;
      }

      case 0200:		// MOVE
	doMOVXX(memGet, noMod1, acPut);
	break;

      case 0201:		// MOVEI
	doMOVXX(immediate, noMod1, acPut);
	break;

      case 0202:		// MOVEM
	doMOVXX(acGet, noMod1, memPut);
	break;

      case 0203:		// MOVES
	doMOVXX(memGet, noMod1, selfPut);
	break;

      case 0204:		// MOVS
	doMOVXX(memGet, swap, acPut);
	break;

      case 0205:		// MOVSI
	doMOVXX(immediate, swap, acPut);
	break;

      case 0206:		// MOVSM
	doMOVXX(acGet, swap, memPut);
	break;

      case 0207:		// MOVSS
	doMOVXX(memGet, swap, selfPut);
	break;

      case 0210:		// MOVN
	doMOVXX(memGet, negate, acPut);
	break;

      case 0211:		// MOVNI
	doMOVXX(immediate, negate, acPut);
	break;

      case 0212:		// MOVNM
	doMOVXX(acGet, negate, memPut);
	break;

      case 0213:		// MOVNS
	doMOVXX(memGet, negate, selfPut);
	break;

      case 0214:		// MOVM
	doMOVXX(memGet, magnitude, acPut);
	break;

      case 0215:		// MOVMI
	doMOVXX(immediate, magnitude, acPut);
	break;

      case 0216:		// MOVMM
	doMOVXX(acGet, magnitude, memPut);
	break;

      case 0217:		// MOVMS
	doMOVXX(memGet, magnitude, selfPut);
	break;

      case 0220:		// IMUL
	doBinOp(acGet, memGet, imulWord, acPut);
	break;

      case 0221:		// IMULI
	doBinOp(acGet, immediate, imulWord, acPut);
	break;

      case 0222:		// IMULM
	doBinOp(acGet, memGet, imulWord, memPut);
	break;

      case 0223:		// IMULB
	doBinOp(acGet, memGet, imulWord, bothPut);
	break;

      case 0224:		// MUL
	doBinOp(acGet, memGet, mulWord, acPut2);
	break;

      case 0225:		// MULI
	doBinOp(acGet, immediate, mulWord, acPut2);
	break;

      case 0226:		// MULM
	doBinOp(acGet, memGet, mulWord, memPutHi);
	break;

      case 0227:		// MULB
	doBinOp(acGet, memGet, mulWord, bothPut2);
	break;

      case 0230:		// IDIV
	doBinOp(acGet, memGet, idivWord, acPut2);
	break;

      case 0231:		// IDIVI
	doBinOp(acGet, immediate, idivWord, acPut2);
	break;

      case 0232:		// IDIVM
	doBinOp(acGet, memGet, idivWord, memPutHi);
	break;

      case 0233:		// IDIVB
	doBinOp(acGet, memGet, idivWord, bothPut2);
	break;

      case 0234:		// DIV
	doBinOp(acGet2, memGet, divWord, acPut2);
	break;

      case 0235:		// DIVI
	doBinOp(acGet2, immediate, divWord, acPut2);
	break;

      case 0236:		// DIVM
	doBinOp(acGet2, memGet, divWord, memPutHi);
	break;

      case 0237:		// DIVB
	doBinOp(acGet2, memGet, divWord, bothPut2);
	break;

      case 0240: {		// ASH
	int n = ea.rhs % 36;
	W36 a(acGet());
	auto aSigned{a.ext64()};

	W36 lostBits;

	if (n > 0) {
	  lostBits.u = a.u & ((1ull << n) - 1);
	  a.s = aSigned >> n;
	} else if (n < 0) {
	  n = -n;
	  lostBits.u = a.u & (W36::all1s >> n);
	  a.s = aSigned << n;
	}

	// Set flags. XXX not sure if these should be set for negative
	// shift count. 1982_ProcRefMan.pdf p.97 is not clear.
	if ((a.ext64() > 0 && lostBits.u != 0) || (a.ext64() < 0 && lostBits.u == 0))
	  state.flags.tr1 = state.flags.ov = 1;

	// Restore sign bit from before shift.
	a.u = (aSigned & W36::bit0) | (a.u & ~W36::bit0);
	acPut(a);
	break;
      }

      case 0241: {		// ROT
	int n = ea.rhs % 36;
	W36 a(acGet());
	W36 prev(a);

	if (n > 0) {
	  a.u <<= n;
	  a.u |= prev >> (36 - n);
	} else if (n < 0) {
	  n = -n;
	  a.u >>= n;
	  a.u |= (prev << (36 - n)) & W36::all1s;
	}

	acPut(a);
	break;
      }

      case 0242: {		// LSH
	int n = ea.rhs % 36;
	W36 a(acGet());

	if (n > 0)
	  a.u <<= n;
	else if (n < 0)
	  a.u >>= -n;

	acPut(a);
	break;
      }

      case 0243: 		// JFFO
	tmp = acGet();

	if (tmp.ext64() != 0) {
	  unsigned count = 0;

	  while (tmp.ext64() >= 0) {
	    ++count;
	    tmp.u <<= 1;
	  }

	  tmp.u = count;
	}

	state.acPutN(tmp, iw.ac+1);
	break;

      case 0245: {		// ROTC
	int n = ea.rhs % 72;
	uint128_t a = ((uint128_t) state.acGetN(iw.ac+0) << 36) | state.acGetN(iw.ac+1);

	if (n > 0) {
	  uint128_t newLSBs = a >> (72-n);
	  a <<= n;
	  a |= newLSBs;
	} else if (n < 0) {
	  n = -n;
	  uint128_t newMSBs = a << (72-n);
	  a >>= n;
	  a |= newMSBs;
	}

	state.acPutN((a >> 36) & W36::all1s, iw.ac+0);
	state.acPutN(a & W36::all1s, iw.ac+1);
	break;
      }

      case 0246: {		// LSHC
	W72 a(acGet(), state.acGetN(iw.ac+1));

	if (ea.rhs > 0)
	  a.u <<= ea.rhs & 0377;
	else if (ea.rhs < 0)
	  a.u >>= -(ea.rhs & 0377);

	state.acPutN(a.hi, iw.ac+0);
	state.acPutN(a.lo, iw.ac+1);
	break;
      }

      case 0250:		// EXCH
	tmp = acGet();
	acPut(memGet());
	memPut(tmp);
	break;

      case 0251: {		// BLT
	W36 ac(acGet());
	bool mem = logger.mem;

	logger.mem = false;
	const string prefix{logger.endl + "                                                 ; "};

	do {
	  W36 srcA(ea.lhu, ac.lhu);
	  W36 dstA(ea.lhu, ac.rhu);

	  if (logger.mem) logger.s << prefix << "BLT src=" << srcA.vma << "  dst=" << dstA.vma;

	  // Note this isn't bug-for-bug compatible with KL10. See
	  // footnote [2] in 1982_ProcRefMan.pdf p.58. We do
	  // wraparound.
	  state.memPutN(state.memGetN(srcA), dstA);
	  ac = W36(ac.lhu + 1, ac.rhu + 1);

	  // Put it back for traps or page faults.
	  acPut(ac);
	} while (ac.rhu <= ea.rhu);

	if (logger.mem) logger.s << prefix << "BLT at end ac=" << ac.fmt36();
	logger.mem = mem;
	break;
      }

      case 0252:		// AOBJP
	tmp = acGet();
	tmp = W36(tmp.lhu + 1, tmp.rhu + 1);
	acPut(tmp);

	if (tmp.ext64() >= 0) {
	  logFlow("jump");
	  nextPC = ea;
	}

	break;

      case 0253:		// AOBJN
	tmp = acGet();
	tmp = W36(tmp.lhu + 1, tmp.rhu + 1);
	acPut(tmp);

	if (tmp.ext64() < 0) {
	  logFlow("jump");
	  nextPC = ea;
	}

	break;

      case 0254:		// JRST family

	switch (iw.ac) {
	case 000:		// JRST
	  nextPC.u = ea.u;
	  break;

	case 002: {		// JRSTF
	  KMState::ProgramFlags newFlags{(unsigned) ea.pcFlags};

	  // User mode cannot clear USR. User mode cannot set UIO.
	  if (state.flags.usr) {
	    newFlags.uio = 0;
	    newFlags.usr = 1;
	  }

	  // A program running in PUB mode cannot clear PUB mode.
	  if (state.flags.pub) newFlags.pub = 1;

	  state.flags.u = newFlags.u;
	  nextPC.u = ea.vma;
	  break;
	}

	case 004:		// HALT
	  cerr << "[HALT at " << state.pc.fmtVMA() << "]" << logger.endl;
	  state.running = false;
	  break;

	default:
	  logger.nyi(state);
	  state.running = false;
	  break;
	}

	break;

      case 0255: {		// JFCL
	unsigned wasFlags = state.flags.u;
	unsigned testFlags = (unsigned) iw.ac << 9; // Align with OV,CY0,CY1,FOV
	state.flags.u &= ~testFlags;
	if (wasFlags & testFlags) nextPC = ea;
	break;
      }

      case 0256:		// XCT/PXCT

	if (state.userMode() || iw.ac == 0) {
	  state.pc = ea;
	  iw = memGet();
	  if (logger.mem) logger.s << "; ";
	  goto XCT_ENTRYPOINT;
	} else {
	  logger.nyi(state);
	  state.running = false;
	  break;
	}

      case 0260:		// PUSHJ
	// Note this sets the flags that are cleared by PUSHJ before
	// doPush() since doPush() can set flags.tr2.
	state.flags.fpd = state.flags.afi = state.flags.tr1 = state.flags.tr2 = 0;
	doPush(state.pc.isSection0() ? state.flagsWord(nextPC.rhu) : W36(nextPC.vma), iw.ac);
	nextPC = ea;
	break;

      case 0261:		// PUSH
	doPush(memGet(), iw.ac);
	break;

      case 0262:		// POP
	memPut(doPop(iw.ac));
	break;

      case 0263:		// POPJ
	nextPC.rhu = doPop(iw.ac).rhu;
	break;

      case 0264:		// JSR
	memPut(state.pc.isSection0() ? state.flagsWord(nextPC.rhu) : W36(nextPC.vma));
	nextPC.u = ea.u + 1;	// XXX Wrap?
	state.flags.fpd = state.flags.afi = state.flags.tr2 = state.flags.tr1 = 0;
	break;

      case 0265:		// JSP
	acPut(state.pc.isSection0() ? state.flagsWord(nextPC.rhu) : W36(nextPC.vma));
	state.flags.fpd = state.flags.afi = state.flags.tr2 = state.flags.tr1 = 0;
	nextPC.u = ea.u;	// XXX Wrap?
	break;

      case 0266:		// JSA
	memPut(acGet());
	nextPC = ea.u + 1;
	acPut(W36(ea.rhu, state.pc.rhu + 1));
	break;

      case 0267:		// JRA
	acPut(state.memGetN(acGet().lhu));
	nextPC = ea;
	break;

      case 0270:		// ADD
	doBinOp(acGet, memGet, addWord, acPut);
	break;

      case 0271:		// ADDI
	doBinOp(acGet, immediate, addWord, acPut);
	break;

      case 0272:		// ADDM
	doBinOp(acGet, memGet, addWord, memPut);
	break;

      case 0273:		// ADDB
	doBinOp(acGet, memGet, addWord, bothPut);
	break;

      case 0274:		// SUB
	doBinOp(acGet, memGet, subWord, acPut);
	break;

      case 0275:		// SUBI
	doBinOp(acGet, immediate, subWord, acPut);
	break;

      case 0276:		// SUBM
	doBinOp(acGet, memGet, subWord, memPut);
	break;

      case 0277:		// SUBB
	doBinOp(acGet, memGet, subWord, bothPut);
	break;

      case 0300:		// CAI
	break;

      case 0301:		// CAIL
	doCAXXX(acGet, immediate, isLT);
	break;

      case 0302:		// CAIE
	doCAXXX(acGet, immediate, isEQ);
	break;

      case 0303:		// CAILE
	doCAXXX(acGet, immediate, isLE);
	break;

      case 0304:		// CAIA
	doCAXXX(acGet, immediate, always2);
	break;

      case 0305:		// CAIGE
	doCAXXX(acGet, immediate, isGE);
	break;

      case 0306:		// CAIN
	doCAXXX(acGet, immediate, isNE);
	break;

      case 0307:		// CAIG
	doCAXXX(acGet, immediate, isGT);
	break;

      case 0310:		// CAM
	break;

      case 0311:		// CAML
	doCAXXX(acGet, memGet, isLT);
	break;

      case 0312:		// CAME
	doCAXXX(acGet, memGet, isEQ);
	break;

      case 0313:		// CAMLE
	doCAXXX(acGet, memGet, isLE);
	break;

      case 0314:		// CAMA
	doCAXXX(acGet, memGet, always2);
	break;

      case 0315:		// CAMGE
	doCAXXX(acGet, memGet, isGE);
	break;

      case 0316:		// CAMN
	doCAXXX(acGet, memGet, isNE);
	break;

      case 0317:		// CAMG
	doCAXXX(acGet, memGet, isGT);
	break;

      case 0320:		// JUMP
	doJUMP(never);
	break;

      case 0321:		// JUMPL
	doJUMP(isLT0);
	break;

      case 0322:		// JUMPE
	doJUMP(isEQ0);
	break;

      case 0323:		// JUMPLE
	doJUMP(isLE0);
	break;

      case 0324:		// JUMPA
	doJUMP(always);
	break;

      case 0325:		// JUMPGE
	doJUMP(isGE0);
	break;

      case 0326:		// JUMPN
	doJUMP(isNE0);
	break;

      case 0327:		// JUMPG
	doJUMP(isGT0);
	break;

      case 0330:		// SKIP
	doSKIP(never);
	break;

      case 0331:		// SKIPL
	doSKIP(isLT0);
	break;

      case 0332:		// SKIPE
	doSKIP(isEQ0);
	break;

      case 0333:		// SKIPLE
	doSKIP(isLE0);
	break;

      case 0334:		// SKIPA
	doSKIP(always);
	break;

      case 0335:		// SKIPGE
	doSKIP(isGE0);
	break;

      case 0336:		// SKIPN
	doSKIP(isNE0);
	break;

      case 0337:		// SKIPGT
	doSKIP(isGT0);
	break;

      case 0340:		// AOJ
	doAOSXX(acGet, 1, acPut, never, jumpAction);
	break;

      case 0341:		// AOJL
	doAOSXX(acGet, 1, acPut, isLT0, jumpAction);
	break;

      case 0342:		// AOJE
	doAOSXX(acGet, 1, acPut, isEQ0, jumpAction);
	break;

      case 0343:		// AOJLE
	doAOSXX(acGet, 1, acPut, isLE0, jumpAction);
	break;

      case 0344:		// AOJA
	doAOSXX(acGet, 1, acPut, always, jumpAction);
	break;

      case 0345:		// AOJGE
	doAOSXX(acGet, 1, acPut, isGE0, jumpAction);
	break;

      case 0346:		// AOJN
	doAOSXX(acGet, 1, acPut, isNE0, jumpAction);
	break;

      case 0347:		// AOJG
	doAOSXX(acGet, 1, acPut, isGT0, jumpAction);
	break;

      case 0350:		// AOS
	doAOSXX(memGet, 1, memPut, never, skipAction);
	break;

      case 0351:		// AOSL
	doAOSXX(memGet, 1, memPut, isLT0, skipAction);
	break;

      case 0352:		// AOSE
	doAOSXX(memGet, 1, memPut, isEQ0, skipAction);
	break;

      case 0353:		// AOSLE
	doAOSXX(memGet, 1, memPut, isLE0, skipAction);
	break;

      case 0354:		// AOSA
	doAOSXX(memGet, 1, memPut, always, skipAction);
	break;

      case 0355:		// AOSGE
	doAOSXX(memGet, 1, memPut, isGE0, skipAction);
	break;

      case 0356:		// AOSN
	doAOSXX(memGet, 1, memPut, isNE0, skipAction);
	break;

      case 0357:		// AOSG
	doAOSXX(memGet, 1, memPut, isGT0, skipAction);
	break;

      case 0360:		// SOJ
	doAOSXX(acGet, -1, acPut, never, jumpAction);
	break;

      case 0361:		// SOJL
	doAOSXX(acGet, -1, acPut, isLT0, jumpAction);
	break;

      case 0362:		// SOJE
	doAOSXX(acGet, -1, acPut, isEQ0, jumpAction);
	break;

      case 0363:		// SOJLE
	doAOSXX(acGet, -1, acPut, isLE0, jumpAction);
	break;

      case 0364:		// SOJA
	doAOSXX(acGet, -1, acPut, always, jumpAction);
	break;

      case 0365:		// SOJGE
	doAOSXX(acGet, -1, acPut, isGE0, jumpAction);
	break;

      case 0366:		// SOJN
	doAOSXX(acGet, -1, acPut, isNE0, jumpAction);
	break;

      case 0367:		// SOJG
	doAOSXX(acGet, -1, acPut, isGT0, jumpAction);
	break;

      case 0370:		// SOS
	doAOSXX(memGet, -1, memPut, never, skipAction);
	break;

      case 0371:		// SOSL
	doAOSXX(memGet, -1, memPut, isLT0, skipAction);
	break;

      case 0372:		// SOSE
	doAOSXX(memGet, -1, memPut, isEQ0, skipAction);
	break;

      case 0373:		// SOSLE
	doAOSXX(memGet, -1, memPut, isLE0, skipAction);
	break;

      case 0374:		// SOSA
	doAOSXX(memGet, -1, memPut, always, skipAction);
	break;

      case 0375:		// SOSGE
	doAOSXX(memGet, -1, memPut, isGE0, skipAction);
	break;

      case 0376:		// SOSN
	doAOSXX(memGet, -1, memPut, isNE0, skipAction);
	break;

      case 0377:		// SOSG
	doAOSXX(memGet, -1, memPut, isGT0, skipAction);
	break;

      case 0400:		// SETZ
	doSETXX(memGet, zeroWord, acPut);
	break;

      case 0401:		// SETZI
	doSETXX(immediate, zeroWord, acPut);
	break;

      case 0402:		// SETZM
	doSETXX(memGet, zeroWord, memPut);
	break;

      case 0403:		// SETZB
	doSETXX(memGet, zeroWord, bothPut);
	break;

      case 0404:		// AND
	doBinOp(memGet, acGet, andWord, acPut);
	break;

      case 0405:		// ANDI
	doBinOp(immediate, acGet, andWord, acPut);
	break;

      case 0406:		// ANDM
	doBinOp(memGet, acGet, andWord, memPut);
	break;

      case 0407:		// ANDB
	doBinOp(memGet, acGet, andWord, bothPut);
	break;

      case 0410:		// ANDCA
	doBinOp(memGet, acGet, andCWord, acPut);
	break;

      case 0411:		// ANDCAI
	doBinOp(immediate, acGet, andCWord, acPut);
	break;

      case 0412:		// ANDCAM
	doBinOp(memGet, acGet, andCWord, memPut);
	break;

      case 0413:		// ANDCAB
	doBinOp(memGet, acGet, andCWord, bothPut);
	break;

      case 0414:		// SETM
	doSETXX(memGet, noMod1, acPut);
	break;

      case 0415:		// SETMI
	doSETXX(immediate, noMod1, acPut);
	break;

      case 0416:		// SETMM
	doSETXX(memGet, noMod1, memPut);
	break;

      case 0417:		// SETMB
	doSETXX(memGet, noMod1, bothPut);
	break;

      case 0420:		// ANDCM
	doBinOp(acGet, memGet, andCWord, acPut);
	break;

      case 0421:		// ANDCMI
	doBinOp(acGet, immediate, andCWord, acPut);
	break;

      case 0422:		// ANDCMM
	doBinOp(acGet, memGet, andCWord, memPut);
	break;

      case 0423:		// ANDCMB
	doBinOp(acGet, memGet, andCWord, bothPut);
	break;

      case 0424:		// SETA
	doSETXX(acGet, noMod1, acPut);
	break;

      case 0425:		// SETAI
	doSETXX(acGet, noMod1, acPut);
	break;

      case 0426:		// SETAM
	doSETXX(acGet, noMod1, memPut);
	break;

      case 0427:		// SETAB
	doSETXX(acGet, noMod1, bothPut);
	break;

      case 0430:		// XOR
	doBinOp(memGet, acGet, xorWord, acPut);
	break;

      case 0431:		// XORI
	doBinOp(immediate, acGet, xorWord, acPut);
	break;

      case 0432:		// XORM
	doBinOp(memGet, acGet, xorWord, memPut);
	break;

      case 0433:		// XORB
	doBinOp(memGet, acGet, xorWord, bothPut);
	break;

      case 0434:		// IOR
	doBinOp(memGet, acGet, iorWord, acPut);
	break;

      case 0435:		// IORI
	doBinOp(immediate, acGet, iorWord, acPut);
	break;

      case 0436:		// IORM
	doBinOp(memGet, acGet, iorWord, memPut);
	break;

      case 0437:		// IORB
	doBinOp(memGet, acGet, iorWord, bothPut);
	break;

      case 0440:		// ANDCBM
	doBinOp(memGet, acGet, andCBWord, acPut);
	break;

      case 0441:		// ANDCBMI
	doBinOp(immediate, acGet, andCBWord, acPut);
	break;

      case 0442:		// ANDCBMM
	doBinOp(memGet, acGet, andCBWord, memPut);
	break;

      case 0443:		// ANDCBMB
	doBinOp(memGet, acGet, andCBWord, bothPut);
	break;

      case 0444:		// EQV
	doBinOp(memGet, acGet, eqvWord, acPut);
	break;

      case 0445:		// EQVI
	doBinOp(immediate, acGet, eqvWord, acPut);
	break;

      case 0446:		// EQVM
	doBinOp(memGet, acGet, eqvWord, memPut);
	break;

      case 0447:		// EQVB
	doBinOp(memGet, acGet, eqvWord, bothPut);
	break;

      case 0450:		// SETCA
	doSETXX(acGet, compWord, acPut);
	break;

      case 0451:		// SETCAI
	doSETXX(acGet, compWord, acPut);
	break;

      case 0452:		// SETCAM
	doSETXX(acGet, compWord, memPut);
	break;

      case 0453:		// SETCAB
	doSETXX(acGet, compWord, bothPut);
	break;

      case 0454:		// ORCA
	doBinOp(memGet, acGet, iorCWord, acPut);
	break;

      case 0455:		// ORCAI
	doBinOp(immediate, acGet, iorCWord, acPut);
	break;

      case 0456:		// ORCAM
	doBinOp(memGet, acGet, iorCWord, memPut);
	break;

      case 0457:		// ORCAB
	doBinOp(memGet, acGet, iorCWord, bothPut);
	break;

      case 0460:		// SETCM
	doSETXX(memGet, compWord, acPut);
	break;

      case 0461:		// SETCMI
	doSETXX(immediate, compWord, acPut);
	break;

      case 0462:		// SETCMM
	doSETXX(memGet, compWord, memPut);
	break;

      case 0463:		// SETCMB
	doSETXX(memGet, compWord, bothPut);
	break;

      case 0464:		// ORCM
	doBinOp(acGet, memGet, iorCWord, acPut);
	break;

      case 0465:		// ORCMI
	doBinOp(acGet, immediate, iorCWord, acPut);
	break;

      case 0466:		// ORCMM
	doBinOp(acGet, memGet, iorCWord, memPut);
	break;

      case 0467:		// ORCMB
	doBinOp(acGet, memGet, iorCWord, bothPut);
	break;

      case 0470:		// ORCB
	doBinOp(memGet, acGet, iorCBWord, acPut);
	break;

      case 0471:		// ORCBI
	doBinOp(immediate, acGet, iorCBWord, acPut);
	break;

      case 0472:		// ORCBM
	doBinOp(memGet, acGet, iorCBWord, memPut);
	break;

      case 0473:		// ORCBB
	doBinOp(memGet, acGet, iorCBWord, bothPut);
	break;

      case 0474:		// SETO
	doSETXX(acGet, onesWord, acPut);
	break;

      case 0475:		// SETOI
	doSETXX(acGet, onesWord, acPut);
	break;

      case 0476:		// SETOM
	doSETXX(memGet, onesWord, memPut);
	break;

      case 0477:		// SETOB
	doSETXX(memGet, onesWord, bothPut);
	break;

      case 0500:		// HLL
	doHXXXX(memGet, acGet, copyHLL, noMod1, acPut);
	break;

      case 0501:		// HLLI/XHLLI
	doHXXXX(immediate, acGet, copyHLL, noMod1, acPut);
	break;

      case 0502:		// HLLM
	doHXXXX(acGet, memGet, copyHLL, noMod1, memPut);
	break;

      case 0503:		// HLLS
	doHXXXX(memGet, memGet, copyHLL, noMod1, selfPut);
	break;

      case 0504:		// HRL
	doHXXXX(memGet, acGet, copyHRL, noMod1, acPut);
	break;

      case 0505:		// HRLI
	doHXXXX(immediate, acGet, copyHRL, noMod1, acPut);
	break;

      case 0506:		// HRLM
	doHXXXX(acGet, memGet, copyHRL, noMod1, memPut);
	break;

      case 0507:		// HRLS
	doHXXXX(memGet, memGet, copyHRL, noMod1, selfPut);
	break;

      case 0510:		// HLLZ
	doHXXXX(memGet, acGet, copyHLL, zeroR, acPut);
	break;

      case 0511:		// HLLZI
	doHXXXX(immediate, acGet, copyHLL, zeroR, acPut);
	break;

      case 0512:		// HLLZM
	doHXXXX(acGet, memGet, copyHLL, zeroR, memPut);
	break;

      case 0513:		// HLLZS
	doHXXXX(memGet, memGet, copyHLL, zeroR, selfPut);
	break;

      case 0514:		// HRLZ
	doHXXXX(memGet, acGet, copyHRL, zeroR, acPut);
	break;

      case 0515:		// HRLZI
	doHXXXX(immediate, acGet, copyHRL, zeroR, acPut);
	break;

      case 0516:		// HRLZM
	doHXXXX(acGet, memGet, copyHRL, zeroR, memPut);
	break;

      case 0517:		// HRLZS
	doHXXXX(memGet, memGet, copyHRL, zeroR, selfPut);
	break;

      case 0520:		// HLLO
	doHXXXX(memGet, acGet, copyHLL, onesR, acPut);
	break;

      case 0521:		// HLLOI
	doHXXXX(immediate, acGet, copyHLL, onesR, acPut);
	break;

      case 0522:		// HLLOM
	doHXXXX(acGet, memGet, copyHLL, onesR, memPut);
	break;

      case 0523:		// HLLOS
	doHXXXX(memGet, memGet, copyHLL, onesR, selfPut);
	break;

      case 0524:		// HRLO
	doHXXXX(memGet, acGet, copyHRL, onesR, acPut);
	break;

      case 0525:		// HRLOI
	doHXXXX(immediate, acGet, copyHRL, onesR, acPut);
	break;

      case 0526:		// HRLOM
	doHXXXX(acGet, memGet, copyHRL, onesR, memPut);
	break;

      case 0527:		// HRLOS
	doHXXXX(memGet, memGet, copyHRL, onesR, selfPut);
	break;

      case 0530:		// HLLE
	doHXXXX(memGet, acGet, copyHLL, extnL, acPut);
	break;

      case 0531:		// HLLEI
	doHXXXX(immediate, acGet, copyHLL, extnL, acPut);
	break;

      case 0532:		// HLLEM
	doHXXXX(acGet, memGet, copyHLL, extnL, memPut);
	break;

      case 0533:		// HLLES
	doHXXXX(memGet, memGet, copyHLL, extnL, selfPut);
	break;

      case 0534:		// HRLE
	doHXXXX(memGet, acGet, copyHRL, extnL, acPut);
	break;

      case 0535:		// HRLEI
	doHXXXX(immediate, acGet, copyHRL, extnL, acPut);
	break;

      case 0536:		// HRLEM
	doHXXXX(acGet, memGet, copyHRL, extnL, memPut);
	break;

      case 0537:		// HRLES
	doHXXXX(memGet, memGet, copyHRL, extnL, selfPut);
	break;

      case 0540:		// HRR
	doHXXXX(memGet, acGet, copyHRR, noMod1, acPut);
	break;

      case 0541:		// HRRI
	doHXXXX(immediate, acGet, copyHRR, noMod1, acPut);
	break;

      case 0542:		// HRRM
	doHXXXX(acGet, memGet, copyHRR, noMod1, memPut);
	break;

      case 0543:		// HRRS
	doHXXXX(memGet, memGet, copyHRR, noMod1, selfPut);
	break;

      case 0544:		// HLR
	doHXXXX(memGet, acGet, copyHLR, noMod1, acPut);
	break;

      case 0545:		// HLRI
	doHXXXX(immediate, acGet, copyHLR, noMod1, acPut);
	break;

      case 0546:		// HLRM
	doHXXXX(acGet, memGet, copyHLR, noMod1, memPut);
	break;

      case 0547:		// HLRS
	doHXXXX(memGet, memGet, copyHLR, noMod1, selfPut);
	break;

      case 0550:		// HRRZ
	doHXXXX(memGet, acGet, copyHRR, zeroL, acPut);
	break;

      case 0551:		// HRRZI
	doHXXXX(immediate, acGet, copyHRR, zeroL, acPut);
	break;

      case 0552:		// HRRZM
	doHXXXX(acGet, memGet, copyHRR, zeroL, memPut);
	break;

      case 0553:		// HRRZS
	doHXXXX(memGet, memGet, copyHRR, zeroL, selfPut);
	break;

      case 0554:		// HLRZ
	doHXXXX(memGet, acGet, copyHLR, zeroL, acPut);
	break;

      case 0555:		// HLRZI
	doHXXXX(immediate, acGet, copyHLR, zeroL, acPut);
	break;

      case 0556:		// HLRZM
	doHXXXX(acGet, memGet, copyHLR, zeroL, memPut);
	break;

      case 0557:		// HLRZS
	doHXXXX(memGet, memGet, copyHLR, zeroL, selfPut);
	break;

      case 0560:		// HRRO
	doHXXXX(memGet, acGet, copyHRR, onesL, acPut);
	break;

      case 0561:		// HRROI
	doHXXXX(immediate, acGet, copyHRR, onesL, acPut);
	break;

      case 0562:		// HRROM
	doHXXXX(acGet, memGet, copyHRR, onesL, memPut);
	break;

      case 0563:		// HRROS
	doHXXXX(memGet, memGet, copyHRR, onesL, selfPut);
	break;

      case 0564:		// HLRO
	doHXXXX(memGet, acGet, copyHLR, onesL, acPut);
	break;

      case 0565:		// HLROI
	doHXXXX(immediate, acGet, copyHLR, onesL, acPut);
	break;

      case 0566:		// HLROM
	doHXXXX(acGet, memGet, copyHLR, onesL, memPut);
	break;

      case 0567:		// HLROS
	doHXXXX(memGet, memGet, copyHLR, onesL, selfPut);
	break;

      case 0570:		// HRRE
	doHXXXX(memGet, acGet, copyHRR, extnR, acPut);
	break;

      case 0571:		// HRREI
	doHXXXX(immediate, acGet, copyHRR, extnR, acPut);
	break;

      case 0572:		// HRREM
	doHXXXX(acGet, memGet, copyHRR, extnR, memPut);
	break;

      case 0573:		// HRRES
	doHXXXX(memGet, memGet, copyHRR, extnR, selfPut);
	break;

      case 0574:		// HLRE
	doHXXXX(memGet, acGet, copyHLR, extnR, acPut);
	break;

      case 0575:		// HLREI
	doHXXXX(immediate, acGet, copyHLR, extnR, acPut);
	break;

      case 0576:		// HLREM
	doHXXXX(acGet, memGet, copyHLR, extnR, memPut);
	break;

      case 0577:		// HLRES
	doHXXXX(memGet, memGet, copyHLR, extnR, selfPut);
	break;

      case 0600:		// TRN
	break;

      case 0601:		// TLN
	break;

      case 0602:		// TRNE
	doTXXXX(acGetRH, getE, noMod2, isEQ0T, noStore);
	break;

      case 0603:		// TLNE
	doTXXXX(acGetLH, getE, noMod2, isEQ0T, noStore);
	break;

      case 0604:		// TRNA
	++nextPC.u;
	break;

      case 0605:		// TLNA
	++nextPC.u;
	break;

      case 0606:		// TRNN
	doTXXXX(acGetRH, getE, noMod2, isNE0T, noStore);
	break;

      case 0607:		// TLNN
	doTXXXX(acGetLH, getE, noMod2, isNE0T, noStore);
	break;

      case 0620:		// TRZ
	doTXXXX(acGetRH, getE, zeroMaskR, neverT, acPutRH);
	break;

      case 0621:		// TLZ
	doTXXXX(acGetLH, getE, zeroMaskR, neverT, acPutLH);
	break;

      case 0622:		// TRZE
	doTXXXX(acGetRH, getE, zeroMaskR, isEQ0T, acPutRH);
	break;

      case 0623:		// TLZE
	doTXXXX(acGetLH, getE, zeroMaskR, isEQ0T, acPutLH);
	break;

      case 0624:		// TRZA
	doTXXXX(acGetRH, getE, zeroMaskR, alwaysT, acPutRH);
	break;

      case 0625:		// TLZA
	doTXXXX(acGetLH, getE, zeroMaskR, alwaysT, acPutLH);
	break;

      case 0626:		// TRZN
	doTXXXX(acGetRH, getE, zeroMaskR, isNE0T, acPutRH);
	break;

      case 0627:		// TLZN
	doTXXXX(acGetLH, getE, zeroMaskR, isNE0T, acPutLH);
	break;

      case 0640:		// TRC
	doTXXXX(acGetRH, getE, compMaskR, neverT, acPutRH);
	break;

      case 0641:		// TLC
	doTXXXX(acGetLH, getE, compMaskR, neverT, acPutLH);
	break;

      case 0642:		// TRCE
	doTXXXX(acGetRH, getE, compMaskR, isEQ0T, acPutRH);
	break;

      case 0643:		// TLCE
	doTXXXX(acGetLH, getE, compMaskR, isEQ0T, acPutLH);
	break;

      case 0644:		// TRCA
	doTXXXX(acGetRH, getE, compMaskR, alwaysT, acPutRH);
	break;

      case 0645:		// TLCA
	doTXXXX(acGetLH, getE, compMaskR, alwaysT, acPutLH);
	break;

      case 0646:		// TRCN
	doTXXXX(acGetRH, getE, compMaskR, isNE0T, acPutRH);
	break;

      case 0647:		// TLCN
	doTXXXX(acGetLH, getE, compMaskR, isNE0T, acPutLH);
	break;

      case 0660:		// TRO
	doTXXXX(acGetRH, getE, onesMaskR, neverT, acPutRH);
	break;

      case 0661:		// TLO
	doTXXXX(acGetLH, getE, onesMaskR, neverT, acPutLH);
	break;

      case 0662:		// TROE
	doTXXXX(acGetRH, getE, onesMaskR, isEQ0T, acPutRH);
	break;

      case 0663:		// TLOE
	doTXXXX(acGetLH, getE, onesMaskR, isEQ0T, acPutLH);
	break;

      case 0664:		// TROA
	doTXXXX(acGetRH, getE, onesMaskR, alwaysT, acPutRH);
	break;

      case 0665:		// TLOA
	doTXXXX(acGetLH, getE, onesMaskR, alwaysT, acPutLH);
	break;

      case 0666:		// TRON
	doTXXXX(acGetRH, getE, onesMaskR, isNE0T, acPutRH);
	break;

      case 0667:		// TLON
	doTXXXX(acGetLH, getE, onesMaskR, isNE0T, acPutLH);
	break;

      case 0610:		// TDN
	break;

      case 0611:		// TSN
	break;

      case 0612:		// TDNE
	doTXXXX(acGet, memGet, noMod2, isEQ0T, noStore);
	break;

      case 0613:		// TSNE
	doTXXXX(acGet, memGetSwapped, noMod2, isEQ0T, noStore);
	break;

      case 0614:		// TDNA
	++nextPC.u;
	break;

      case 0615:		// TSNA
	++nextPC.u;
	break;

      case 0616:		// TDNN
	doTXXXX(acGet, memGet, noMod2, isNE0T, noStore);
	break;

      case 0617:		// TSNN
	doTXXXX(acGet, memGetSwapped, noMod2, isNE0T, noStore);
	break;

      case 0630:		// TDZ
	doTXXXX(acGet, memGet, zeroMask, neverT, acPut);
	break;

      case 0631:		// TSZ
	doTXXXX(acGet, memGetSwapped, zeroMask, neverT, acPut);
	break;

      case 0632:		// TDZE
	doTXXXX(acGet, memGet, zeroMask, isEQ0T, acPut);
	break;

      case 0633:		// TSZE
	doTXXXX(acGet, memGetSwapped, zeroMask, isEQ0T, acPut);
	break;

      case 0634:		// TDZA
	doTXXXX(acGet, memGet, zeroMask, alwaysT, acPut);
	break;

      case 0635:		// TSZA
	doTXXXX(acGet, memGetSwapped, zeroMask, alwaysT, acPut);
	break;

      case 0636:		// TDZN
	doTXXXX(acGet, memGet, zeroMask, isNE0T, acPut);
	break;

      case 0637:		// TSZN
	doTXXXX(acGet, memGetSwapped, zeroMask, isNE0T, acPut);
	break;

      case 0650:		// TDC
	doTXXXX(acGet, memGet, compMask, neverT, acPut);
	break;

      case 0651:		// TSC
	doTXXXX(acGet, memGetSwapped, compMask, neverT, acPut);
	break;

      case 0652:		// TDCE
	doTXXXX(acGet, memGet, compMask, isEQ0T, acPut);
	break;

      case 0653:		// TSCE
	doTXXXX(acGet, memGetSwapped, compMask, isEQ0T, acPut);
	break;

      case 0654:		// TDCA
	doTXXXX(acGet, memGet, compMask, alwaysT, acPut);
	break;

      case 0655:		// TSCA
	doTXXXX(acGet, memGetSwapped, compMask, alwaysT, acPut);
	break;

      case 0656:		// TDCN
	doTXXXX(acGet, memGet, compMask, isNE0T, acPut);
	break;

      case 0657:		// TSZCN
	doTXXXX(acGet, memGetSwapped, compMask, isNE0T, acPut);
	break;

      case 0670:		// TDO
	doTXXXX(acGet, memGet, onesMask, neverT, acPut);
	break;

      case 0671:		// TSO
	doTXXXX(acGet, memGetSwapped, onesMask, neverT, acPut);
	break;

      case 0672:		// TDOE
	doTXXXX(acGet, memGet, onesMask, isEQ0T, acPut);
	break;

      case 0673:		// TSOE
	doTXXXX(acGet, memGetSwapped, onesMask, isEQ0T, acPut);
	break;

      case 0674:		// TDOA
	doTXXXX(acGet, memGet, onesMask, alwaysT, acPut);
	break;

      case 0675:		// TSOA
	doTXXXX(acGet, memGetSwapped, onesMask, alwaysT, acPut);
	break;

      case 0676:		// TDON
	doTXXXX(acGet, memGet, onesMask, isNE0T, acPut);
	break;

      case 0677:		// TSON
	doTXXXX(acGet, memGetSwapped, onesMask, isNE0T, acPut);
	break;

      default:

	if (iw.ioSeven == 7) {	// Only handle I/O instructions this way
	  if (logger.io) logger.s << "; ioDev=" << oct << iw.ioDev << " ioOp=" << oct << iw.ioOp;
	  Device::handleIO(iw, ea, state, nextPC);
	} else {
	  logger.nyi(state);
	  state.running = false;
	}

	break;
      }

      state.pc = nextPC;

      if ((state.flags.tr1 || state.flags.tr2) && pag.pagerEnabled()) {
	// XXX for now, stop on trap if paging is enabled.
	state.running = false;
	iw = state.flags.tr1 ? state.eptP->trap1Insn : state.eptP->stackOverflowInsn;
      } else {
	iw = state.memGetN(state.pc.vma);
      }

      if (logger.pc || logger.mem || logger.ac || logger.io || logger.dte)
	logger.s << logger.endl << flush;

    } while (state.running);

    // Restore console to normal
    dte.disconnect();
  }
};
