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
#include <set>

using namespace std;


#include "logging.hpp"
#include "kmstate.hpp"
#include "w36.hpp"
#include "bytepointer.hpp"
#include "device.hpp"
#include "dte20.hpp"


class KM10 {
public:
  KMState state;


  struct APRDevice: Device {

    // CONI APR interrupt flags
    union Flags {

      struct ATTRPACKED {
	unsigned sweepDone: 1;
	unsigned powerFailure: 1;
	unsigned addrParity: 1;
	unsigned cacheDirParity: 1;
	unsigned mbParity: 1;
	unsigned ioPageFail: 1;
	unsigned noMemory: 1;
	unsigned sbusError: 1;
      };

      unsigned u: 8;
    };
  
    // CONI APR status bits
    union State {

      struct ATTRPACKED {
	unsigned intLevel: 3;
	unsigned intRequest: 1;

	Flags active;
	unsigned: 4;

	unsigned sweepBusy: 1;
	unsigned: 5;

	Flags enabled;

	unsigned: 6;
      };

      uint64_t u: 36;
    } state;


    // CONO APR function bits
    union Functions {

      struct ATTRPACKED {
	unsigned intLevel: 3;
	unsigned: 1;

	Flags select;

	unsigned set: 1;
	unsigned clear: 1;
	unsigned disable: 1;
	unsigned enable: 1;

	unsigned clearIO: 1;
	unsigned: 1;
      };

      unsigned u: 18;

      Functions(unsigned v) :u(v) {};
    };

    struct Levels {
      unsigned sweepDone: 3;
      unsigned powerFailure: 3;
      unsigned addrParity: 3;
      unsigned cacheDirParity: 3;
      unsigned mbParity: 3;
      unsigned ioPageFail: 3;
      unsigned noMemory: 3;
      unsigned sbusError: 3;
    } levels;


    // APRID value see 1982_ProcRefMan.pdf p.244
    static inline const union {

      struct ATTRPACKED {
	unsigned serialNumber: 12;

	unsigned: 1;
	unsigned masterOscillator: 1;
	unsigned extendedKL10: 1;
	unsigned channelSupported: 1;
	unsigned cacheSupported: 1;
	unsigned AC50Hz: 1;

	unsigned microcodeVersion: 9;
	unsigned: 6;
	unsigned exoticMicrocode: 1;
	unsigned extendedAddressing: 1;
	unsigned tops20Paging: 1;
      };

      uint64_t u: 36;
    } apridValue{
      04321,	// Processor serial number (always octal?)
	1,	// Master oscillator
	1,	// Extended KL10
	1,	// Channel support
	0,	// No cache support
	0,	// 60Hz,
	0442,	// Microcode version number
	0,	// "Exotic" microcode
	1,	// Microcode handles extended addresses
	1,	// TOPS-20 paging
	};


    // Constructors
    APRDevice()
      : Device(0000, "APR")
    {
    }


    // I/O instruction handlers
    void clearIO() {
      state.u = 0;
    }
  } apr;

  
  struct PIDevice: Device {

    // PI CONO function bits
    union Functions {
      Functions(unsigned v) :u(v) {};

      struct ATTRPACKED {
	unsigned levels: 7;

	unsigned turnPIOn: 1;
	unsigned turnPIOff: 1;

	unsigned levelsOff: 1;
	unsigned levelsOn: 1;
	unsigned levelsInitiate: 1;
      
	unsigned clearPI: 1;
	unsigned dropRequests: 1;
	unsigned: 1;

	unsigned writeEvenParityDir: 1;
	unsigned writeEvenParityData: 1;
	unsigned writeEvenParityAddr: 1;
      };

      unsigned u: 18;
    };

    // PI state and CONI bits
    union State {

      struct ATTRPACKED {
	unsigned levelsEnabled: 7;
	unsigned piEnabled: 1;
	unsigned intInProgress: 7;
	unsigned writeEvenParityDir: 1;
	unsigned writeEvenParityData: 1;
	unsigned writeEvenParityAddr: 1;
	unsigned levelsRequested: 7;
	unsigned: 11;
      };

      uint64_t u: 36;

      State() :u(0) {};
    } state;


    // Constructors
    PIDevice():
      Device(0060, "PI")
    {
      state.u = 0;
    }


    // I/O instruction handlers
    void clearIO() {
      state.u = 0;
    }
  } pi;

  struct PAGDevice: Device {

    // CONO PAG state bits
    union State {

      struct ATTRPACKED {
	unsigned execBasePage: 13;
	unsigned enablePager: 1;
	unsigned tops2Paging: 1;
	unsigned: 1;
	unsigned cacheStrategyLoad: 1;
	unsigned cacheStrategyLook: 1;
      };

      unsigned u: 18;
    } state;


    // Constructors
    PAGDevice():
      Device(0120, "PAG")
    {
      state.u = 0;
    }


    // Accessors
    bool pagerEnabled() {
      return state.enablePager;
    }

    // I/O instruction handlers
    void clearIO() {
      state.u = 0;
    }
  } pag;



  APRDevice *aprP;
  PIDevice *piP;
  PAGDevice *pagP;
  DTE20 *dteP;


  // Constructors
  KM10(KMState &aState, DTE20 *aDTE)
    : state(aState),
      dteP(aDTE)
  {}


  
  ////////////////////////////////////////////////////////////////////////////////
  // The instruction emulator. Call this to start or continue running.
  void emulate() {
    W36 iw{};
    W36 ea{};
    W36 nextPC = state.pc;
    W36 tmp;

    uint64_t nInsns = 0;

    function<W36()> acGet = [&]() -> W36 {
      return state.acGetN(iw.ac);
    };

    function<W36()> acGetRH = [&]() -> W36 {
      W36 value{0, acGet().rhu};
      if (logging.mem) logging.s << " ; acRH" << oct << iw.ac << ": " << value.fmt36();
      return value;
    };

    function<W36()> acGetLH = [&]() -> W36 {
      W36 value{0, acGet().lhu};
      if (logging.mem) logging.s << " ; acLH" << oct << iw.ac << ": " << value.fmt36();
      return value;
    };

    function<void(W36)> acPut = [&](W36 value) -> void {
      state.acPutN(value, iw.ac);
    };

    function<void(W36,W36)> acPut2 = [&](W36 hi, W36 lo) -> void {
      state.acPutN(hi, iw.ac);
      state.acPutN(lo, iw.ac+1);
    };

    function<W36()> memGet = [&]() -> W36 {
      return state.memGetN(ea);
    };

    function<void(W36)> memPut = [&](W36 value) -> void {
      state.memPutN(value, ea);
    };

    function<void(W36)> selfPut = [&](W36 value) -> void {
      memPut(value);
      if (iw.ac != 0) acPut(value);
    };

    function<void(W36)> bothPut = [&](W36 value) -> void {
      acPut(value);
      memPut(value);
    };

    function<void(W36,W36)> bothPut2 = [&](W36 hi, W36 lo) -> void {
      state.acPutN(hi, iw.ac);
      state.acPutN(lo, iw.ac+1);
      memPut(hi);
    };

    function<W36(W36)> swap = [&](W36 src) -> W36 {return W36(src.rhu, src.lhu);};

    function<W36(W36)> negate = [&](W36 src) -> W36 {
      W36 v(-src.s);
      if (src.u == W36::mostNegative) state.flags.tr1 = state.flags.ov = state.flags.cy1 = 1;
      if (src.u == 0) state.flags.cy0 = state.flags.cy1 = 1;
      return v;
    };

    function<W36(W36)> magnitude = [&](W36 src) -> W36 {
      W36 v(src.s < 0 ? -src.s : src.s);
      if (src.u == W36::mostNegative) state.flags.tr1 = state.flags.ov = state.flags.cy1 = 1;
      return v;
    };

    function<W36()> memGetSwapped = [&]() -> W36 {return swap(memGet());};

    function<void(W36,W36)> memPutHi = [&](W36 hi, W36 lo) -> void {memPut(hi);};

    function<W36()> immediate = [&]() -> W36 {return W36(state.pc.isSection0() ? 0 : ea.lhu, ea.rhu);};

    // Condition testing predicates
    function<bool(W36)> isLT0  = [&](W36 v) -> bool const {return v.s <  0;};
    function<bool(W36)> isLE0  = [&](W36 v) -> bool const {return v.s <= 0;};
    function<bool(W36)> isGT0  = [&](W36 v) -> bool const {return v.s >  0;};
    function<bool(W36)> isGE0  = [&](W36 v) -> bool const {return v.s >= 0;};
    function<bool(W36)> isNE0  = [&](W36 v) -> bool const {return v.s != 0;};
    function<bool(W36)> isEQ0  = [&](W36 v) -> bool const {return v.s == 0;};
    function<bool(W36)> always = [&](W36 v) -> bool const {return true;};
    function<bool(W36)> never  = [&](W36 v) -> bool const {return false;};

    auto doJUMP = [&](function<bool(W36)> &condF) -> void {

      if (condF(acGet())) {
	if (logging.mem) logging.s << " [jump]";
	nextPC.rhu = ea;
      }
    };

    auto doSKIP = [&](function<bool(W36)> &condF) -> void {
      W36 eaw = memGet();

      if (condF(eaw)) {
	if (logging.mem) logging.s << " [skip]";
	++nextPC.rhu;
      }
      
      if (iw.ac != 0) acPut(eaw);
    };

    function<W36(W36)> noModification = [&](W36 s) -> auto const {return s;};
    function<W36(W36)> dirMask   = [&](W36 s) -> auto const {return s.u & memGet().rhu;};
    function<W36(W36)> zeroMaskR = [&](W36 s) -> auto const {return s.u & ~(uint64_t) ea.rhu;};
    function<W36(W36)> zeroMaskL = [&](W36 s) -> auto const {return s.u & ~((uint64_t) ea.rhu << 18);};
    function<W36(W36)> onesMaskR = [&](W36 s) -> auto const {return s.u | ea.rhu;};
    function<W36(W36)> onesMaskL = [&](W36 s) -> auto const {return s.u | ((uint64_t) ea.rhu << 18);};
    function<W36(W36)> compMaskR = [&](W36 s) -> auto const {return s.u ^ ea.rhu;};
    function<W36(W36)> compMaskL = [&](W36 s) -> auto const {return s.u ^ ((uint64_t) ea.rhu << 18);};

    function<W36(W36)> zeroWord = [&](W36 s) -> auto const {return 0;};
    function<W36(W36)> onesWord = [&](W36 s) -> auto const {return W36::allOnes;};
    function<W36(W36)> compWord = [&](W36 s) -> auto const {return ~s.u;};

    function<void(W36)> noStore = [](W36 toSrc) -> void {};

    auto doTXXXX = [&](function<W36()> &doGetF,
		       function<W36(W36)> &doModifyF,
		       function<bool(W36)> &condF,
		       function<void(W36)> &doStoreF) -> void
    {
      W36 eaw = doGetF() & ea;

      if (condF(eaw)) {
	if (logging.mem) logging.s << " [skip]";
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


    auto extnOf = [&](const unsigned v) -> unsigned const {
      return (v & 04000000) ? W36::halfOnes : 0;
    };


    // doCopyF functions
    function<W36(W36,W36)> copyHRR = [&](W36 src, W36 dst) -> auto const {return W36(dst.lhu, src.rhu);};
    function<W36(W36,W36)> copyHRL = [&](W36 src, W36 dst) -> auto const {return W36(src.rhu, dst.rhu);};
    function<W36(W36,W36)> copyHLL = [&](W36 src, W36 dst) -> auto const {return W36(src.lhu, dst.rhu);};
    function<W36(W36,W36)> copyHLR = [&](W36 src, W36 dst) -> auto const {return W36(dst.rhu, src.lhu);};

    // doModifyF functions
    function<W36(W36)> zeroR = [&](W36 v) -> auto const {return W36(v.lhu, 0);};
    function<W36(W36)> onesR = [&](W36 v) -> auto const {return W36(v.lhu, W36::halfOnes);};
    function<W36(W36)> extnR = [&](W36 v) -> auto const {return W36(v.lhu, extnOf(v.lhu));};
    function<W36(W36)> zeroL = [&](W36 v) -> auto const {return W36(0, v.rhu);};
    function<W36(W36)> onesL = [&](W36 v) -> auto const {return W36(W36::halfOnes, v.rhu);};
    function<W36(W36)> extnL = [&](W36 v) -> auto const {return W36(extnOf(v.rhu), v.rhu);};

    // binary doModifyF functions
    function<W36(W36,W36)> andWord = [&](W36 s1, W36 s2) -> auto const {return s1.u & s2.u;};
    function<W36(W36,W36)> andCWord = [&](W36 s1, W36 s2) -> auto const {return s1.u & ~s2.u;};
    function<W36(W36,W36)> andCBWord = [&](W36 s1, W36 s2) -> auto const {return ~s1.u & ~s2.u;};
    function<W36(W36,W36)> iorWord = [&](W36 s1, W36 s2) -> auto const {return s1.u | s2.u;};
    function<W36(W36,W36)> iorCWord = [&](W36 s1, W36 s2) -> auto const {return s1.u | ~s2.u;};
    function<W36(W36,W36)> iorCBWord = [&](W36 s1, W36 s2) -> auto const {return ~s1.u | ~s2.u;};
    function<W36(W36,W36)> xorWord = [&](W36 s1, W36 s2) -> auto const {return s1.u ^ s2.u;};
    function<W36(W36,W36)> xorCWord = [&](W36 s1, W36 s2) -> auto const {return s1.u ^ ~s2.u;};
    function<W36(W36,W36)> xorCBWord = [&](W36 s1, W36 s2) -> auto const {return ~s1.u ^ ~s2.u;};
    function<W36(W36,W36)> eqvWord = [&](W36 s1, W36 s2) -> auto const {return ~(s1.u ^ s2.u);};
    function<W36(W36,W36)> eqvCWord = [&](W36 s1, W36 s2) -> auto const {return ~(s1.u ^ ~s2.u);};
    function<W36(W36,W36)> eqvCBWord = [&](W36 s1, W36 s2) -> auto const {return ~(~s1.u ^ ~s2.u);};

    function<W36(W36,W36)> addWord = [&](W36 s1, W36 s2) -> auto const {
      uint64_t sum = (uint64_t) s1.u + (uint64_t) s2.u;
      if (sum >= W36::mostNegative) state.flags.tr1 = state.flags.ov = state.flags.cy1 = 1;
      if ((int64_t) sum < -(int64_t) W36::mostNegative) state.flags.ov = state.flags.cy0 = 1;
      return sum;
    };
    
    function<W36(W36,W36)> subWord = [&](W36 s1, W36 s2) -> auto const {
      int64_t diff = (int64_t) s1.u - (int64_t) s2.u;
      if (diff >= (int64_t) W36::mostNegative) state.flags.tr1 = state.flags.ov = state.flags.cy1 = 1;
      if (diff < -(int64_t) W36::mostNegative) state.flags.ov = state.flags.cy0 = 1;
      return diff;
    };
    
    function<tuple<W36,W36>(W36,W36)> mulWord = [&](W36 s1, W36 s2) -> auto const {
      W72 prod = (int128_t) s1.s * (int128_t) s2.s;

      if (s1.s == W36::signedMostNegative && s2.s == W36::signedMostNegative) {
	state.flags.tr1 = state.flags.ov = state.flags.cy1 = 1;
      }

      return tuple<W36,W36>(W36(prod.hi), W36(prod.lo));
    };
    
    function<W36(W36,W36)> imulWord = [&](W36 s1, W36 s2) -> auto const {
      int128_t prod = (int128_t) s1.s * (int128_t) s2.s;

      if (s1.s == W36::signedMostNegative && s2.s == W36::signedMostNegative) {
	state.flags.tr1 = state.flags.ov = state.flags.cy1 = 1;
      }

      return W36((prod < 0 ? W36::mostNegative : 0) | ((W36::allOnes >> 1) & prod));
    };
    
    function<tuple<W36,W36>(W36,W36)> divWord = [&](W36 s1, W36 s2) -> auto const {
      int128_t d = (int128_t) s1.s / (int128_t) s2.s;
      int128_t r = (int128_t) s1.s % (int128_t) s2.s;

      if (s1.s == W36::signedMostNegative && s2.s == W36::signedMostNegative) {
	state.flags.tr1 = state.flags.ov = state.flags.cy1 = 1;
      }

      return tuple<W36,W36>(d, r);
    };
    
    function<tuple<W36,W36>(W36,W36)> idivWord = [&](W36 s1, W36 s2) -> auto const {
      int128_t d = (int128_t) s1.s * (int128_t) s2.s;
      int128_t r = (int128_t) s1.s % (int128_t) s2.s;

      if (s1.s == W36::signedMostNegative && s2.s == W36::signedMostNegative) {
	state.flags.tr1 = state.flags.ov = state.flags.cy1 = 1;
      }

      return tuple<W36,W36>(d, r);
    };
    
    auto doHXXXX = [&](function<W36()> &doGetSrcF,
		       function<W36()> &doGetDstF,
		       function<W36(W36,W36)> &doCopyF,
		       function<W36(W36)> &doModifyF,
		       function<void(W36)> &doPutDstF) -> void
    {
      doPutDstF(doModifyF(doCopyF(doGetSrcF(), doGetDstF())));
    };


    auto doMOVXX = [&](function<W36()> &doGetSrcF,
		       function<W36(W36)> &doModifyF,
		       function<void(W36)> &doPutDstF) -> void
    {
      doPutDstF(doModifyF(doGetSrcF()));
    };


    auto doSETXX = [&](function<W36()> &doGetSrcF,
		       function<W36(W36)> &doModifyF,
		       function<void(W36)> &doPutDstF) -> void
    {
      doPutDstF(doModifyF(doGetSrcF()));
    };


    auto doBinOpXX = [&](function<W36()> &doGetSrc1F,
			 function<W36()> &doGetSrc2F,
			 function<W36(W36,W36)> &doModifyF,
			 function<void(W36)> &doPutDstF) -> void
    {
      doPutDstF(doModifyF(doGetSrc1F(), doGetSrc2F()));
    };


    auto doBinOp2XX = [&](function<W36()> &doGetSrc1F,
			  function<W36()> &doGetSrc2F,
			  function<tuple<W36,W36>(W36,W36)> &doModifyF,
			  function<void(W36,W36)> &doPutDstF) -> void
    {
      auto [hi, lo] = doModifyF(doGetSrc1F(), doGetSrc2F());
      doPutDstF(hi, lo);
    };


    // Binary comparison predicates
    function<bool(W36,W36)> isLT    = [&](W36 v1, W36 v2) -> bool const {return v1.s <  v2.s;};
    function<bool(W36,W36)> isLE    = [&](W36 v1, W36 v2) -> bool const {return v1.s <= v2.s;};
    function<bool(W36,W36)> isGT    = [&](W36 v1, W36 v2) -> bool const {return v1.s >  v2.s;};
    function<bool(W36,W36)> isGE    = [&](W36 v1, W36 v2) -> bool const {return v1.s >= v2.s;};
    function<bool(W36,W36)> isNE    = [&](W36 v1, W36 v2) -> bool const {return v1.s != v2.s;};
    function<bool(W36,W36)> isEQ    = [&](W36 v1, W36 v2) -> bool const {return v1.s == v2.s;};
    function<bool(W36,W36)> always2 = [&](W36 v1, W36 v2) -> bool const {return true;};
    function<bool(W36,W36)> never2  = [&](W36 v1, W36 v2) -> bool const {return false;};

    auto doCAXXX = [&](function<W36()> &doGetSrc1F,
		       function<W36()> &doGetSrc2F,
		       function<bool(W36,W36)> &condF) -> void
    {

      if (condF(doGetSrc1F(), doGetSrc2F())) {
	if (logging.mem) logging.s << " [skip]";
	++nextPC.rhu;
      }
    };


    function<void()> skipAction = [&] {++nextPC.u;};
    function<void()> jumpAction = [&] {nextPC.u = ea;};

    auto doAOSOXX = [&](function<W36()> &doGetF,
			const signed delta,
			function<void(W36)> &doPutF,
			function<bool(W36)> &condF,
			function<void()> &actionF) -> void
    {
      W36 v = doGetF();

      if (delta > 0) {		// Increment

	if (v.u == W36::allOnes >> 1) {
	  state.flags.tr1 = state.flags.ov  = state.flags.cy1 = 1;
	} else if (v.s == -1) {
	  state.flags.cy0 = state.flags.cy1 = 1;
	}
      } else {			// Decrement

	if (v.u == W36::mostNegative) {
	  state.flags.tr1 = state.flags.ov = state.flags.cy0 = 1;
	} else if (v.u != 0) {
	  state.flags.cy0 = state.flags.cy1 = 1;
	}
      }

      v.s += delta;
      doPutF(v);

      if (condF(v)) actionF();
    };


    // Connect our DTE20 (put console into raw mode)
    dteP->connect();

    // The instruction loop
    do {

      if ((state.flags.tr1 || state.flags.tr2) && pag.pagerEnabled()) {
	iw = state.flags.tr1 ? state.eptP->trap1Insn : state.eptP->stackOverflowInsn;
      } else {
	iw = state.memGetN(state.pc.vma);
      }

      nextPC.lhu = state.pc.lhu;
      nextPC.rhu = state.pc.rhu + 1;

    XCT_ENTRYPOINT:
      // When we XCT we have already set PC to point to the
      // instruction to be XCTed and nextPC is pointing after the XCT.

      if (nInsns++ > logging.maxInsns) {
	cerr << "[" << dec << logging.maxInsns << " instructions executed at pc="
	     << state.pc.fmtVMA() << "]" << endl;
	state.running = false;
      }

      if (logging.pc) {
	logging.s << setfill('0')
		  << " " << setw(3) << iw.op
		  << " " << setw(2) << iw.ac
		  << " " << setw(1) << iw.i
		  << " " << setw(2) << iw.x
		  << " " << setw(6) << iw.y
		  << "  " << iw.disasm();
      }

      ea.u = state.getEA(iw.i, iw.x, iw.y);

      switch (iw.op) {

      case 0133: {		// IBP/ADJBP
	BytePointer *bp = BytePointer::makeFrom(memGet(), state);

	if (iw.ac == 0) {	// IBP
	  bp->inc(state);
	} else {		// ADJBP
	  bp->adjust(iw.ac, state);
	}

	break;
      }

      case 0135: {		// LDB
	BytePointer *bp = BytePointer::makeFrom(memGet(), state);
	acPut(bp->getByte(state));
	break;
      }

      case 0137: {		// DPB
	BytePointer *bp = BytePointer::makeFrom(memGet(), state);
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
	doBinOpXX(memGet, acGet, imulWord, acPut);
	break;

      case 0221:		// IMULI
	doBinOpXX(immediate, acGet, imulWord, acPut);
	break;

      case 0222:		// IMULM
	doBinOpXX(memGet, acGet, imulWord, memPut);
	break;

      case 0223:		// IMULB
	doBinOpXX(memGet, acGet, imulWord, bothPut);
	break;

      case 0224:		// MUL
	doBinOp2XX(memGet, acGet, mulWord, acPut2);
	break;

      case 0225:		// MULI
	doBinOp2XX(immediate, acGet, mulWord, acPut2);
	break;

      case 0226:		// MULM
	doBinOp2XX(memGet, acGet, mulWord, memPutHi);
	break;

      case 0227:		// MULB
	doBinOp2XX(memGet, acGet, mulWord, bothPut2);
	break;

      case 0230:		// IDIV
	doBinOp2XX(memGet, acGet, idivWord, acPut2);
	break;

      case 0231:		// IDIVI
	doBinOp2XX(immediate, acGet, idivWord, acPut2);
	break;

      case 0232:		// IDIVM
	doBinOp2XX(memGet, acGet, idivWord, memPutHi);
	break;

      case 0233:		// IDIVB
	doBinOp2XX(memGet, acGet, idivWord, bothPut2);
	break;

      case 0234:		// DIV
	doBinOp2XX(memGet, acGet, divWord, acPut2);
	break;

      case 0235:		// DIVI
	doBinOp2XX(immediate, acGet, divWord, acPut2);
	break;

      case 0236:		// DIVM
	doBinOp2XX(memGet, acGet, divWord, memPutHi);
	break;

      case 0237:		// DIVB
	doBinOp2XX(memGet, acGet, divWord, bothPut2);
	break;

      case 0242: {		// LSH
	W36 a(acGet());

	if (ea.rhs > 0)
	  a.u <<= ea.rhs;
	else if (ea.rhs < 0)
	  a.u >>= -ea.rhs;

	acPut(a);
	break;
      }

      case 0243: 		// JFFO
	tmp = acGet();

	if (tmp != 0) {
	  unsigned count = 0;

	  while (tmp.s >= 0) {
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
	bool mem = logging.mem;

	logging.mem = false;
	static const string prefix{"\n                                                 ; "};

	do {
	  W36 srcA(ea.lhu, ac.lhu);
	  W36 dstA(ea.lhu, ac.rhu);

	  logging.s << prefix << "BLT src=" << srcA.vma << "  dst=" << dstA.vma;

	  // Note this isn't bug-for-bug compatible with KL10. See
	  // footnote [2] in 1982_ProcRefMan.pdf p.58. We do
	  // wraparound.
	  state.memPutN(state.memGetN(srcA), dstA);
	  ac = W36(ac.lhu + 1, ac.rhu + 1);

	  // Put it back for traps or page faults.
	  acPut(ac);
	} while (ac.rhu <= ea.rhu);

	logging.s << prefix << "BLT at end ac=" << ac.fmt36();
	logging.mem = mem;
	break;
      }

      case 0252:		// AOBJP
	tmp = acGet();
	tmp = W36(tmp.lhu + 1, tmp.rhu + 1);
	acPut(tmp);

	if (tmp.s >= 0) {
	  if (logging.mem) logging.s << " [jump]";
	  nextPC = ea;
	}

	break;

      case 0253:		// AOBJN
	tmp = acGet();
	tmp = W36(tmp.lhu + 1, tmp.rhu + 1);
	acPut(tmp);

	if (tmp.s < 0) {
	  if (logging.mem) logging.s << " [jump]";
	  nextPC = ea;
	}

	break;

      case 0254:		// JRST family

	switch (iw.ac) {
	case 000:		// JRST
	  nextPC.u = ea.u;
	  break;

	case 004:		// HALT
	  cerr << "[HALT at " << state.pc.fmtVMA() << "]" << endl;
	  state.running = false;
	  break;

	default:
	  Logging::nyi();
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
	  if (logging.mem) logging.s << "; ";
	  goto XCT_ENTRYPOINT;
	} else {
	  Logging::nyi();
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
	doBinOpXX(memGet, acGet, addWord, acPut);
	break;

      case 0271:		// ADDI
	doBinOpXX(immediate, acGet, addWord, acPut);
	break;

      case 0272:		// ADDM
	doBinOpXX(memGet, acGet, addWord, memPut);
	break;

      case 0273:		// ADDB
	doBinOpXX(memGet, acGet, addWord, bothPut);
	break;

      case 0274:		// SUB
	doBinOpXX(memGet, acGet, subWord, acPut);
	break;

      case 0275:		// SUBI
	doBinOpXX(immediate, acGet, subWord, acPut);
	break;

      case 0276:		// SUBM
	doBinOpXX(memGet, acGet, subWord, memPut);
	break;

      case 0277:		// SUBB
	doBinOpXX(memGet, acGet, subWord, bothPut);
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
	doAOSOXX(acGet, 1, acPut, never, jumpAction);
	break;

      case 0341:		// AOJL
	doAOSOXX(acGet, 1, acPut, isLT0, jumpAction);
	break;

      case 0342:		// AOJE
	doAOSOXX(acGet, 1, acPut, isEQ0, jumpAction);
	break;

      case 0343:		// AOJLE
	doAOSOXX(acGet, 1, acPut, isLE0, jumpAction);
	break;

      case 0344:		// AOJA
	doAOSOXX(acGet, 1, acPut, always, jumpAction);
	break;

      case 0345:		// AOJGE
	doAOSOXX(acGet, 1, acPut, isGE0, jumpAction);
	break;

      case 0346:		// AOJN
	doAOSOXX(acGet, 1, acPut, never, jumpAction);
	break;

      case 0347:		// AOJG
	doAOSOXX(acGet, 1, acPut, isGT0, jumpAction);
	break;

      case 0350:		// AOS
	doAOSOXX(acGet, 1, acPut, never, skipAction);
	break;

      case 0351:		// AOSL
	doAOSOXX(acGet, 1, acPut, isLT0, skipAction);
	break;

      case 0352:		// AOSE
	doAOSOXX(acGet, 1, acPut, isEQ0, skipAction);
	break;

      case 0353:		// AOSLE
	doAOSOXX(acGet, 1, acPut, isLE0, skipAction);
	break;

      case 0354:		// AOSA
	doAOSOXX(acGet, 1, acPut, always, skipAction);
	break;

      case 0355:		// AOSGE
	doAOSOXX(acGet, 1, acPut, isGE0, skipAction);
	break;

      case 0356:		// AOSN
	doAOSOXX(acGet, 1, acPut, never, skipAction);
	break;

      case 0357:		// AOSG
	doAOSOXX(acGet, 1, acPut, isGT0, skipAction);
	break;

      case 0360:		// SOJ
	doAOSOXX(acGet, -1, acPut, never, jumpAction);
	break;

      case 0361:		// SOJL
	doAOSOXX(acGet, -1, acPut, isLT0, jumpAction);
	break;

      case 0362:		// SOJE
	doAOSOXX(acGet, -1, acPut, isEQ0, jumpAction);
	break;

      case 0363:		// SOJLE
	doAOSOXX(acGet, -1, acPut, isLE0, jumpAction);
	break;

      case 0364:		// SOJA
	doAOSOXX(acGet, -1, acPut, always, jumpAction);
	break;

      case 0365:		// SOJGE
	doAOSOXX(acGet, -1, acPut, isGE0, jumpAction);
	break;

      case 0366:		// SOJN
	doAOSOXX(acGet, -1, acPut, never, jumpAction);
	break;

      case 0367:		// SOJG
	doAOSOXX(acGet, -1, acPut, isGT0, jumpAction);
	break;

      case 0370:		// SOS
	doAOSOXX(acGet, -1, acPut, never, skipAction);
	break;

      case 0371:		// SOSL
	doAOSOXX(acGet, -1, acPut, isLT0, skipAction);
	break;

      case 0372:		// SOSE
	doAOSOXX(acGet, -1, acPut, isEQ0, skipAction);
	break;

      case 0373:		// SOSLE
	doAOSOXX(acGet, -1, acPut, isLE0, skipAction);
	break;

      case 0374:		// SOSA
	doAOSOXX(acGet, -1, acPut, always, skipAction);
	break;

      case 0375:		// SOSGE
	doAOSOXX(acGet, -1, acPut, isGE0, skipAction);
	break;

      case 0376:		// SOSN
	doAOSOXX(acGet, -1, acPut, never, skipAction);
	break;

      case 0377:		// SOSG
	doAOSOXX(acGet, -1, acPut, isGT0, skipAction);
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
	doBinOpXX(memGet, acGet, andWord, acPut);
	break;

      case 0405:		// ANDI
	doBinOpXX(immediate, acGet, andWord, acPut);
	break;

      case 0406:		// ANDM
	doBinOpXX(memGet, acGet, andWord, memPut);
	break;

      case 0407:		// ANDB
	doBinOpXX(memGet, acGet, andWord, bothPut);
	break;

      case 0410:		// ANDCA
	doBinOpXX(memGet, acGet, andCWord, acPut);
	break;

      case 0411:		// ANDCAI
	doBinOpXX(immediate, acGet, andCWord, acPut);
	break;

      case 0412:		// ANDCAM
	doBinOpXX(memGet, acGet, andCWord, memPut);
	break;

      case 0413:		// ANDCAB
	doBinOpXX(memGet, acGet, andCWord, bothPut);
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
	doBinOpXX(acGet, memGet, andCWord, acPut);
	break;

      case 0421:		// ANDCMI
	doBinOpXX(acGet, immediate, andCWord, acPut);
	break;

      case 0422:		// ANDCMM
	doBinOpXX(acGet, memGet, andCWord, memPut);
	break;

      case 0423:		// ANDCMB
	doBinOpXX(acGet, memGet, andCWord, bothPut);
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
	doBinOpXX(memGet, acGet, xorWord, acPut);
	break;

      case 0431:		// XORI
	doBinOpXX(immediate, acGet, xorWord, acPut);
	break;

      case 0432:		// XORM
	doBinOpXX(memGet, acGet, xorWord, memPut);
	break;

      case 0433:		// XORB
	doBinOpXX(memGet, acGet, xorWord, bothPut);
	break;

      case 0434:		// IOR
	doBinOpXX(memGet, acGet, iorWord, acPut);
	break;

      case 0435:		// IORI
	doBinOpXX(immediate, acGet, iorWord, acPut);
	break;

      case 0436:		// IORM
	doBinOpXX(memGet, acGet, iorWord, memPut);
	break;

      case 0437:		// IORB
	doBinOpXX(memGet, acGet, iorWord, bothPut);
	break;

      case 0440:		// ANDCBM
	doBinOpXX(acGet, memGet, andCBWord, acPut);
	break;

      case 0441:		// ANDCBMI
	doBinOpXX(acGet, immediate, andCBWord, acPut);
	break;

      case 0442:		// ANDCBMM
	doBinOpXX(acGet, memGet, andCBWord, memPut);
	break;

      case 0443:		// ANDCBMB
	doBinOpXX(acGet, memGet, andCBWord, bothPut);
	break;

      case 0444:		// EQV
	doBinOpXX(memGet, acGet, eqvWord, acPut);
	break;

      case 0445:		// EQVI
	doBinOpXX(immediate, acGet, eqvWord, acPut);
	break;

      case 0446:		// EQVM
	doBinOpXX(memGet, acGet, eqvWord, memPut);
	break;

      case 0447:		// EQVB
	doBinOpXX(memGet, acGet, eqvWord, bothPut);
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
	doBinOpXX(memGet, acGet, iorCWord, acPut);
	break;

      case 0455:		// ORCAI
	doBinOpXX(immediate, acGet, iorCWord, acPut);
	break;

      case 0456:		// ORCAM
	doBinOpXX(memGet, acGet, iorCWord, memPut);
	break;

      case 0457:		// ORCAB
	doBinOpXX(memGet, acGet, iorCWord, bothPut);
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
	doBinOpXX(acGet, memGet, iorCWord, acPut);
	break;

      case 0465:		// ORCMI
	doBinOpXX(acGet, immediate, iorCWord, acPut);
	break;

      case 0466:		// ORCMM
	doBinOpXX(acGet, memGet, iorCWord, memPut);
	break;

      case 0467:		// ORCMB
	doBinOpXX(acGet, memGet, iorCWord, bothPut);
	break;

      case 0470:		// ORCB
	doBinOpXX(acGet, memGet, iorCBWord, acPut);
	break;

      case 0471:		// ORCBI
	doBinOpXX(acGet, immediate, iorCBWord, acPut);
	break;

      case 0472:		// ORCBM
	doBinOpXX(acGet, memGet, iorCBWord, memPut);
	break;

      case 0473:		// ORCBB
	doBinOpXX(acGet, memGet, iorCBWord, bothPut);
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

	if (iw.ioSeven != 7) {	// Only handle I/O instructions this way
	  Logging::nyi();
	  continue;
	}

	logging.s << " ; ioDev=" << oct << iw.ioDev << " ioOp=" << oct << iw.ioOp;

	switch (iw.ioDev) {
	case 000:		// APR

	  switch (iw.ioOp) {
	  case W36::BLKI:	// APRID
	    memPut(apr.apridValue.u);
	    break;

	  case W36::BLKO:	// WRFIL
	    Logging::nyi();
	    break;

	  case W36::CONO: {	// WRAPR
	    APRDevice::Functions func(ea.u);

	    if (logging.mem) logging.s << " ; " << ea.fmt18();

	    if (func.clear) {
	      apr.state.active.u &= ~func.select.u;
	    } else if (func.set) {
	      apr.state.active.u |= func.select.u;

	      // This block argues the APR state needs to be
	      // metaprogrammed with C++17 parameter pack superpowers
	      // instead of this old fashioned mnaual method. This might
	      // be an interesting thing to do in the future. For now,
	      // it's done the 1940s hard way.
	      if (func.intLevel != 0) {
		if (func.select.sweepDone != 0)      apr.levels.sweepDone = func.intLevel;
		if (func.select.powerFailure != 0)   apr.levels.powerFailure = func.intLevel;
		if (func.select.addrParity != 0)     apr.levels.addrParity = func.intLevel;
		if (func.select.cacheDirParity != 0) apr.levels.cacheDirParity = func.intLevel;
		if (func.select.mbParity != 0)       apr.levels.mbParity = func.intLevel;
		if (func.select.ioPageFail != 0)     apr.levels.ioPageFail = func.intLevel;
		if (func.select.noMemory != 0)       apr.levels.noMemory = func.intLevel;
		if (func.select.sbusError != 0)      apr.levels.sbusError = func.intLevel;
	      }
	    } else if (func.enable) {
	      apr.state.enabled.u |= func.select.u;
	    } else if (func.disable) {
	      apr.state.enabled.u &= ~func.select.u;
	    }

	    if (func.clearIO) {
	      for (auto [ioDev, devP]: Device::devices) devP->clearIO();
	    }
	    break;
	  }

	  case W36::CONI:	// RDAPR
	    Logging::nyi();
	    break;

	  default:
	    Logging::nyi();
	    break;
	  }

	  break;

	case 001:		// PI

	  switch (iw.ioOp) {
	  case W36::CONO: {
	    PIDevice::Functions pif(ea);

	    if (logging.mem) logging.s << " ; " << oct << ea.fmt18();

	    if (pif.clearPI) {
	      logging.s << " ; CLEAR I/O SYSTEM";
	      Device::clearAll();
	    } else {
	      pi.state.writeEvenParityDir = pif.writeEvenParityDir;
	      pi.state.writeEvenParityData = pif.writeEvenParityData;
	      pi.state.writeEvenParityAddr = pif.writeEvenParityAddr;

	      if (pif.turnPIOn) {
		pi.state.piEnabled = 1;
	      } else if (pif.turnPIOff) {
		pi.state.piEnabled = 0;
	      } else if (pif.dropRequests != 0) {
		pi.state.levelsRequested &= ~pif.levels;
	      } else if (pif.levelsInitiate) {
		pi.state.levelsRequested |= pif.levels;
	      } else if (pif.levelsOff) {
		pi.state.levelsEnabled &= ~pif.levels;
	      } else if (pif.levelsOn) {
		pi.state.levelsEnabled |= pif.levels;
	      }
	    }
	    break;
	  }

	  case W36::CONI:
	    memPut(pi.state.u);
	    break;

	  default:
	    Logging::nyi();
	    break;
	  }

	  break;

	case 024:		// PAG
	  break;

	default:		// All other devices
	  Device::handleIO(iw, ea);
	  break;
	}

	break;
      }

      state.pc = nextPC;
      if (logging.pc || logging.mem) logging.s << endl;
    } while (state.running);

    // Restore console to normal
    dteP->disconnect();
  }
};
