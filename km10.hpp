// This is the KM10 CPU implementation.
#pragma once
#include <string>
#include <cstdint>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <ostream>
#include <limits>
#include <functional>
#include <assert.h>

using namespace std;


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


  // Constructors
  KM10(KMState &aState)
    : state(aState),
      apr(state),
      pi(state),
      pag(state),
      dte(040, state)
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
      if (logger.mem) logger.s << "; acRH" << oct << iw.ac << ": " << value.fmt36();
      return value;
    };

    WFunc acGetLH = [&]() -> W36 {
      W36 value{0, acGet().lhu};
      if (logger.mem) logger.s << "; acLH" << oct << iw.ac << ": " << value.fmt36();
      return value;
    };

    FuncW acPut = [&](W36 value) -> void {
      state.acPutN(value, iw.ac);
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

    WFuncW swap = [&](W36 src) -> W36 {return W36(src.rhu, src.lhu);};

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

    WFuncW noModification = [&](W36 s) -> auto const {return s;};
    WFuncW dirMask   = [&](W36 s) -> auto const {return s.u & memGet().rhu;};
    WFuncW zeroMaskR = [&](W36 s) -> auto const {return s.u & ~(uint64_t) ea.rhu;};
    WFuncW zeroMaskL = [&](W36 s) -> auto const {return s.u & ~((uint64_t) ea.rhu << 18);};
    WFuncW onesMaskR = [&](W36 s) -> auto const {return s.u | ea.rhu;};
    WFuncW onesMaskL = [&](W36 s) -> auto const {return s.u | ((uint64_t) ea.rhu << 18);};
    WFuncW compMaskR = [&](W36 s) -> auto const {return s.u ^ ea.rhu;};
    WFuncW compMaskL = [&](W36 s) -> auto const {return s.u ^ ((uint64_t) ea.rhu << 18);};

    WFuncW zeroWord = [&](W36 s) -> auto const {return 0;};
    WFuncW onesWord = [&](W36 s) -> auto const {return W36::all1s;};
    WFuncW compWord = [&](W36 s) -> auto const {return ~s.u;};

    FuncW noStore = [](W36 toSrc) -> void {};

    auto doTXXXX = [&](WFunc &doGetF,
		       WFuncW &doModifyF,
		       BoolPredW &condF,
		       FuncW &doStoreF) -> void
    {
      W36 eaw = doGetF() & ea;

      if (condF(eaw)) {
	logFlow("skip");
	++nextPC.rhu;
      }
      
      doStoreF(doModifyF(eaw));
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
      return (v & 04000000) ? W36::halfOnes : 0;
    };


    // doCopyF functions
    WFuncWW copyHRR = [&](W36 src, W36 dst) -> auto const {return W36(dst.lhu, src.rhu);};
    WFuncWW copyHRL = [&](W36 src, W36 dst) -> auto const {return W36(src.rhu, dst.rhu);};
    WFuncWW copyHLL = [&](W36 src, W36 dst) -> auto const {return W36(src.lhu, dst.rhu);};
    WFuncWW copyHLR = [&](W36 src, W36 dst) -> auto const {return W36(dst.rhu, src.lhu);};

    // doModifyF functions
    WFuncW zeroR = [&](W36 v) -> auto const {return W36(v.lhu, 0);};
    WFuncW onesR = [&](W36 v) -> auto const {return W36(v.lhu, W36::halfOnes);};
    WFuncW extnR = [&](W36 v) -> auto const {return W36(v.lhu, extnOf(v.lhu));};
    WFuncW zeroL = [&](W36 v) -> auto const {return W36(0, v.rhu);};
    WFuncW onesL = [&](W36 v) -> auto const {return W36(W36::halfOnes, v.rhu);};
    WFuncW extnL = [&](W36 v) -> auto const {return W36(extnOf(v.rhu), v.rhu);};

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
      auto sum = s1.extend() + s2.extend();

      if (sum < -W36::signedBit0) {
	state.flags.tr1 = state.flags.ov = state.flags.cy0 = 1;
      } else if ((uint64_t) sum >= W36::bit0) {
	state.flags.tr1 = state.flags.ov = state.flags.cy1 = 1;
      }

      return sum;
    };
    
    WFuncWW subWord = [&](W36 s1, W36 s2) -> auto const {
      auto diff = s1.extend() - s2.extend();

      if (diff < -W36::signedBit0) {
	state.flags.tr1 = state.flags.ov = state.flags.cy0 = 1;
      } else if ((uint64_t) diff >= W36::bit0) {
	state.flags.tr1 = state.flags.ov = state.flags.cy1 = 1;
      }

      return diff;
    };
    
    DFuncWW mulWord = [&](W36 s1, W36 s2) -> auto const {
      W72 prod{(int128_t) s1.extend() * s2.extend()};

      if (s1.u == W36::bit0 && s2.u == W36::bit0) {
	state.flags.tr1 = state.flags.ov = 1;
	return W72{1ull << 34, 0};
      }

      return prod;
    };
    
    WFuncWW imulWord = [&](W36 s1, W36 s2) -> auto const {
      W72 prod{(int128_t) s1.extend() * s2.extend()};

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
	return W72{s1 / s2, s1 % s2};
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
	uint64_t quo = den70 / dor;
	uint64_t rem = den70 % dor;
	const uint64_t mask35 = W36::rMask(35);
	W72 ret{(quo & mask35) | signBit, (rem & mask35) | signBit};
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
      doPutDstF(doModifyF(doGetSrcF()));
    };


    auto doSETXX = [&](WFunc &doGetSrcF,
		       WFuncW &doModifyF,
		       FuncW &doPutDstF) -> void
    {
      doPutDstF(doModifyF(doGetSrcF()));
    };


    // Binary comparison predicates
    BoolPredWW isLT    = [&](W36 v1, W36 v2) -> bool const {return v1.extend() <  v2.extend();};
    BoolPredWW isLE    = [&](W36 v1, W36 v2) -> bool const {return v1.extend() <= v2.extend();};
    BoolPredWW isGT    = [&](W36 v1, W36 v2) -> bool const {return v1.extend() >  v2.extend();};
    BoolPredWW isGE    = [&](W36 v1, W36 v2) -> bool const {return v1.extend() >= v2.extend();};
    BoolPredWW isNE    = [&](W36 v1, W36 v2) -> bool const {return v1.extend() != v2.extend();};
    BoolPredWW isEQ    = [&](W36 v1, W36 v2) -> bool const {return v1.extend() == v2.extend();};
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
	} else if (v.extend() == -1) {
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
	auto a2 = W72{state.acGetN(iw.ac+0),    state.acGetN(iw.ac+1)};

	auto s1 = a1.toS70();
	auto s2 = a2.toS70();
	auto u1 = a1.toU70();
	auto u2 = a2.toU70();
	auto isNeg1 = s1 < 0;
	auto isNeg2 = s2 < 0;
	auto sum = W72(s1 + s2);

	if (sum.s >= W72::sBit1) {
	  state.flags.cy1 = state.flags.tr1 = state.flags.ov = 1;
	} else if (sum.s < -W72::sBit1) {
	  state.flags.cy0 = state.flags.tr1 = state.flags.ov = 1;
	} else if ((s1 < 0 && s2 < 0) ||
		   (isNeg1 != isNeg2 &&
		    (u1 == u2 || ((!isNeg1 && u1 > u2) || (!isNeg2 && u2 > u1)))))
	  {
	    state.flags.cy0 = state.flags.cy1 = state.flags.tr1 = state.flags.ov = 1;
	  }

	state.acPutN(iw.ac+0, sum.hi);
	state.acPutN(iw.ac+1, sum.lo);
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
	doMOVXX(memGet, noModification, acPut);
	break;

      case 0201:		// MOVEI
	doMOVXX(immediate, noModification, acPut);
	break;

      case 0202:		// MOVEM
	doMOVXX(acGet, noModification, memPut);
	break;

      case 0203:		// MOVES
	doMOVXX(acGet, noModification, selfPut);
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
	doMOVXX(acGet, swap, selfPut);
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
	doMOVXX(acGet, negate, selfPut);
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
	doMOVXX(acGet, magnitude, selfPut);
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
	auto aSigned{a.extend()};

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
	if ((a.extend() > 0 && lostBits.u != 0) || (a.extend() < 0 && lostBits.u == 0))
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

	if (tmp.extend() != 0) {
	  unsigned count = 0;

	  while (tmp.extend() >= 0) {
	    ++count;
	    tmp.u <<= 1;
	  }

	  tmp.u = count;
	}

	state.acPutN(tmp, iw.ac+1);
	break;

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

	if (tmp.extend() >= 0) {
	  logFlow("jump");
	  nextPC = ea;
	}

	break;

      case 0253:		// AOBJN
	tmp = acGet();
	tmp = W36(tmp.lhu + 1, tmp.rhu + 1);
	acPut(tmp);

	if (tmp.extend() < 0) {
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
	  KMState::ProgramFlags newFlags{ea.pcFlags};

	  // Disallow clearing of USR flag from user mode.
	  newFlags.usr = state.flags.usr;

	  // Disallow clearing of PUB unless JRSTF is running in
	  // executive mode and switching to user mode.
	  if (newFlags.pub || state.flags.usr || !newFlags.usr) newFlags.pub = 1;

	  state.flags = newFlags.u;
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

      case 0255:		// JFCL
	if ((iw.ac & 8) && state.flags.ov)  {state.flags.ov = 0; nextPC = ea;}
	if ((iw.ac & 4) && state.flags.cy0) {state.flags.cy0 = 0; nextPC = ea;}
	if ((iw.ac & 2) && state.flags.cy1) {state.flags.cy1 = 0; nextPC = ea;}
	if ((iw.ac & 1) && state.flags.fov) {state.flags.fov = 0; nextPC = ea;}
	break;

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
	nextPC.u = ea.u + 1;	// XXX Wrap?
	acPut(state.pc.isSection0() ? state.flagsWord(nextPC.rhu) : W36(nextPC.vma));
	state.flags.fpd = state.flags.afi = state.flags.tr2 = state.flags.tr1 = 0;
	break;

      case 0266:		// JSA
	memPut(acGet());
	nextPC = ea.u + 1;
	acPut(W36(ea.lhu, nextPC.rhu));
	break;

      case 0267:		// JRA
	acPut(state.memGetN(acGet()));
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
	doCAXXX(acGet, immediate, never2);
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
	doCAXXX(acGet, memGet, never2);
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
	doAOSXX(acGet, 1, acPut, never, jumpAction);
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
	doAOSXX(memGet, 1, memPut, never, skipAction);
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
	doAOSXX(acGet, -1, acPut, never, jumpAction);
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
	doAOSXX(memGet, -1, memPut, never, skipAction);
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
	doSETXX(acGet, noModification, memPut);
	break;

      case 0415:		// SETMI
	doSETXX(immediate, noModification, memPut);
	break;

      case 0416:		// SETMM
	doSETXX(acGet, noModification, memPut);
	break;

      case 0417:		// SETMB
	doSETXX(acGet, noModification, bothPut);
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
	doSETXX(acGet, noModification, acPut);
	break;

      case 0425:		// SETAI
	doSETXX(immediate, noModification, acPut);
	break;

      case 0426:		// SETAM
	doSETXX(acGet, noModification, memPut);
	break;

      case 0427:		// SETAB
	doSETXX(acGet, noModification, bothPut);
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
	doBinOp(acGet, memGet, andCBWord, acPut);
	break;

      case 0441:		// ANDCBMI
	doBinOp(acGet, immediate, andCBWord, acPut);
	break;

      case 0442:		// ANDCBMM
	doBinOp(acGet, memGet, andCBWord, memPut);
	break;

      case 0443:		// ANDCBMB
	doBinOp(acGet, memGet, andCBWord, bothPut);
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
	doSETXX(immediate, compWord, acPut);
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
	doSETXX(acGet, compWord, memPut);
	break;

      case 0461:		// SETCMI
	doSETXX(immediate, compWord, memPut);
	break;

      case 0462:		// SETCMM
	doSETXX(acGet, compWord, memPut);
	break;

      case 0463:		// SETCMB
	doSETXX(acGet, compWord, bothPut);
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
	doBinOp(acGet, memGet, iorCBWord, acPut);
	break;

      case 0471:		// ORCBI
	doBinOp(acGet, immediate, iorCBWord, acPut);
	break;

      case 0472:		// ORCBM
	doBinOp(acGet, memGet, iorCBWord, memPut);
	break;

      case 0473:		// ORCBB
	doBinOp(acGet, memGet, iorCBWord, bothPut);
	break;

      case 0474:		// SETO
	doSETXX(memGet, onesWord, acPut);
	break;

      case 0475:		// SETOI
	doSETXX(immediate, onesWord, acPut);
	break;

      case 0476:		// SETOM
	doSETXX(memGet, onesWord, memPut);
	break;

      case 0477:		// SETOB
	doSETXX(memGet, onesWord, bothPut);
	break;

      case 0500:		// HLL
	doHXXXX(memGet, acGet, copyHLL, noModification, acPut);
	break;

      case 0501:		// HLLI/XHLLI
	doHXXXX(immediate, acGet, copyHLL, noModification, acPut);
	break;

      case 0502:		// HLLM
	doHXXXX(acGet, memGet, copyHLL, noModification, memPut);
	break;

      case 0503:		// HLLS
	doHXXXX(memGet, memGet, copyHLL, noModification, selfPut);
	break;

      case 0504:		// HRL
	doHXXXX(memGet, acGet, copyHRL, noModification, acPut);
	break;

      case 0505:		// HRLI
	doHXXXX(immediate, acGet, copyHRL, noModification, acPut);
	break;

      case 0506:		// HRLM
	doHXXXX(acGet, memGet, copyHRL, noModification, memPut);
	break;

      case 0507:		// HRLS
	doHXXXX(memGet, memGet, copyHRL, noModification, selfPut);
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
	doHXXXX(memGet, acGet, copyHLL, extnR, acPut);
	break;

      case 0531:		// HLLEI
	doHXXXX(immediate, acGet, copyHLL, extnR, acPut);
	break;

      case 0532:		// HLLEM
	doHXXXX(acGet, memGet, copyHLL, extnR, memPut);
	break;

      case 0533:		// HLLES
	doHXXXX(memGet, memGet, copyHLL, extnR, selfPut);
	break;

      case 0534:		// HRLE
	doHXXXX(memGet, acGet, copyHRL, extnR, acPut);
	break;

      case 0535:		// HRLEI
	doHXXXX(immediate, acGet, copyHRL, extnR, acPut);
	break;

      case 0536:		// HRLEM
	doHXXXX(acGet, memGet, copyHRL, extnR, memPut);
	break;

      case 0537:		// HRLES
	doHXXXX(memGet, memGet, copyHRL, extnR, selfPut);
	break;

      case 0540:		// HRR
	doHXXXX(memGet, acGet, copyHRR, noModification, acPut);
	break;

      case 0541:		// HRRI
	doHXXXX(immediate, acGet, copyHRR, noModification, acPut);
	break;

      case 0542:		// HRRM
	doHXXXX(acGet, memGet, copyHRR, noModification, memPut);
	break;

      case 0543:		// HRRS
	doHXXXX(memGet, memGet, copyHRR, noModification, selfPut);
	break;

      case 0544:		// HLR
	doHXXXX(memGet, acGet, copyHLR, noModification, acPut);
	break;

      case 0545:		// HLRI
	doHXXXX(immediate, acGet, copyHLR, noModification, acPut);
	break;

      case 0546:		// HLRM
	doHXXXX(acGet, memGet, copyHLR, noModification, memPut);
	break;

      case 0547:		// HLRS
	doHXXXX(memGet, memGet, copyHLR, noModification, selfPut);
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
	doHXXXX(memGet, acGet, copyHRR, extnL, acPut);
	break;

      case 0571:		// HRREI
	doHXXXX(immediate, acGet, copyHRR, extnL, acPut);
	break;

      case 0572:		// HRREM
	doHXXXX(acGet, memGet, copyHRR, extnL, memPut);
	break;

      case 0573:		// HRRES
	doHXXXX(memGet, memGet, copyHRR, extnL, selfPut);
	break;

      case 0574:		// HLRE
	doHXXXX(memGet, acGet, copyHLR, extnL, acPut);
	break;

      case 0575:		// HLREI
	doHXXXX(immediate, acGet, copyHLR, extnL, acPut);
	break;

      case 0576:		// HLREM
	doHXXXX(acGet, memGet, copyHLR, extnL, memPut);
	break;

      case 0577:		// HLRES
	doHXXXX(memGet, memGet, copyHLR, extnL, selfPut);
	break;

      case 0600:		// TRN
	doTXXXX(acGetRH, noModification, never, noStore);
	break;

      case 0601:		// TLN
	doTXXXX(acGetLH, noModification, never, noStore);
	break;

      case 0602:		// TRNE
	doTXXXX(acGetRH, noModification, isEQ0, noStore);
	break;

      case 0603:		// TLNE
	doTXXXX(acGetLH, noModification, isEQ0, noStore);
	break;

      case 0604:		// TRNA
	doTXXXX(acGetRH, noModification, always, noStore);
	break;

      case 0605:		// TLNA
	doTXXXX(acGetLH, noModification, always, noStore);
	break;

      case 0606:		// TRNN
	doTXXXX(acGetRH, noModification, isNE0, noStore);
	break;

      case 0607:		// TLNN
	doTXXXX(acGetLH, noModification, isNE0, noStore);
	break;

      case 0620:		// TRZ
	doTXXXX(acGetRH, zeroMaskR, never, acPut);
	break;

      case 0621:		// TLZ
	doTXXXX(acGetLH, zeroMaskL, never, acPut);
	break;

      case 0622:		// TRZE
	doTXXXX(acGetRH, zeroMaskR, isEQ0, acPut);
	break;

      case 0623:		// TLZE
	doTXXXX(acGetLH, zeroMaskL, isEQ0, acPut);
	break;

      case 0624:		// TRZA
	doTXXXX(acGetRH, zeroMaskR, always, acPut);
	break;

      case 0625:		// TLZA
	doTXXXX(acGetLH, zeroMaskL, always, acPut);
	break;

      case 0626:		// TRZN
	doTXXXX(acGetRH, zeroMaskR, isNE0, acPut);
	break;

      case 0627:		// TLZN
	doTXXXX(acGetLH, zeroMaskL, isNE0, acPut);
	break;

      case 0640:		// TRC
	doTXXXX(acGetRH, compMaskR, never, acPut);
	break;

      case 0641:		// TLC
	doTXXXX(acGetLH, compMaskL, never, acPut);
	break;

      case 0642:		// TRCE
	doTXXXX(acGetRH, compMaskR, isEQ0, acPut);
	break;

      case 0643:		// TLCE
	doTXXXX(acGetLH, compMaskL, isEQ0, acPut);
	break;

      case 0644:		// TRCA
	doTXXXX(acGetRH, compMaskR, always, acPut);
	break;

      case 0645:		// TLCA
	doTXXXX(acGetLH, compMaskL, always, acPut);
	break;

      case 0646:		// TRCN
	doTXXXX(acGetRH, compMaskR, isNE0, acPut);
	break;

      case 0647:		// TLCN
	doTXXXX(acGetLH, compMaskL, isNE0, acPut);
	break;

      case 0660:		// TRO
	doTXXXX(acGetRH, onesMaskR, never, acPut);
	break;

      case 0661:		// TLO
	doTXXXX(acGetLH, onesMaskL, never, acPut);
	break;

      case 0662:		// TROE
	doTXXXX(acGetRH, onesMaskR, isEQ0, acPut);
	break;

      case 0663:		// TLOE
	doTXXXX(acGetLH, onesMaskL, isEQ0, acPut);
	break;

      case 0664:		// TROA
	doTXXXX(acGetRH, onesMaskR, always, acPut);
	break;

      case 0665:		// TLOA
	doTXXXX(acGetLH, onesMaskL, always, acPut);
	break;

      case 0666:		// TRON
	doTXXXX(acGetRH, onesMaskR, isNE0, acPut);
	break;

      case 0667:		// TLON
	doTXXXX(acGetLH, onesMaskL, isNE0, acPut);
	break;

      case 0610:		// TDN
	doTXXXX(memGet, noModification, never, noStore);
	break;

      case 0611:		// TSN
	doTXXXX(memGetSwapped, noModification, never, noStore);
	break;

      case 0612:		// TDNE
	doTXXXX(memGet, noModification, isEQ0, noStore);
	break;

      case 0613:		// TSNE
	doTXXXX(memGetSwapped, noModification, isEQ0, noStore);
	break;

      case 0614:		// TDNA
	doTXXXX(memGet, noModification, always, noStore);
	break;

      case 0615:		// TSNA
	doTXXXX(memGetSwapped, noModification, always, noStore);
	break;

      case 0616:		// TDNN
	doTXXXX(memGet, noModification, isNE0, noStore);
	break;

      case 0617:		// TSNN
	doTXXXX(memGetSwapped, noModification, isNE0, noStore);
	break;

      case 0630:		// TDZ
	doTXXXX(memGet, dirMask, never, acPut);
	break;

      case 0631:		// TSZ
	doTXXXX(memGetSwapped, dirMask, never, acPut);
	break;

      case 0632:		// TDZE
	doTXXXX(memGet, dirMask, isEQ0, acPut);
	break;

      case 0633:		// TSZE
	doTXXXX(memGetSwapped, dirMask, isEQ0, acPut);
	break;

      case 0634:		// TDZA
	doTXXXX(memGet, dirMask, always, acPut);
	break;

      case 0635:		// TSZA
	doTXXXX(memGetSwapped, dirMask, always, acPut);
	break;

      case 0636:		// TDZN
	doTXXXX(memGet, dirMask, isNE0, acPut);
	break;

      case 0637:		// TSZN
	doTXXXX(memGetSwapped, dirMask, isNE0, acPut);
	break;

      default:

	if (iw.ioSeven == 7) {	// Only handle I/O instructions this way
	  if (logger.io) logger.s << "; ioDev=" << oct << iw.ioDev << " ioOp=" << oct << iw.ioOp;
	  Device::handleIO(iw, ea, state);
	} else {
	  logger.nyi(state);
	}

	break;
      }

      state.pc = nextPC;

      if ((state.flags.tr1 || state.flags.tr2) && pag.pagerEnabled()) {
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
